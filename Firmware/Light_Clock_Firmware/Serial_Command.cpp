#include "Serial_Command.hpp"

#include <stdio.h>
#include <string.h>

namespace serial_command {
namespace {

bool hasPrefix(const char* line, size_t length, const char* prefix,
               size_t prefix_length) {
  return length >= prefix_length &&
         memcmp(line, prefix, prefix_length) == 0;
}

bool parseDigits(const char* line, size_t offset, size_t count,
                 uint16_t* value) {
  uint16_t parsed = 0;
  for (size_t index = 0; index < count; ++index) {
    const char digit = line[offset + index];
    if (digit < '0' || digit > '9') {
      return false;
    }
    parsed = static_cast<uint16_t>(parsed * 10 + digit - '0');
  }
  *value = parsed;
  return true;
}

bool parseUnsignedFields(const char* line, size_t length, size_t offset,
                         uint16_t* values, size_t fields) {
  for (size_t field = 0; field < fields; ++field) {
    const size_t start = offset;
    while (offset < length && line[offset] >= '0' && line[offset] <= '9')
      ++offset;
    const size_t count = offset - start;
    if (count == 0 || count > 3 ||
        !parseDigits(line, start, count, &values[field]) ||
        (field + 1 < fields &&
         (offset >= length || line[offset++] != ' ')) ||
        (field + 1 == fields && offset != length)) return false;
  }
  return true;
}

void setResponse(State* state, const char* response) {
  const int length = snprintf(state->response, sizeof(state->response), "%s\n",
                              response);
  if (length < 0 || static_cast<size_t>(length) >= sizeof(state->response)) {
    state->response[0] = '\0';
    state->response_length = 0;
    state->response_offset = 0;
    return;
  }
  state->response_length = static_cast<uint16_t>(length);
  state->response_offset = 0;
}

void setTimeResponse(State* state, const char* prefix,
                     const clock_command::DateTime& value) {
  const int length =
      snprintf(state->response, sizeof(state->response),
               "%s SET_TIME %04u-%02u-%02u %02u:%02u:%02u\n", prefix,
               static_cast<unsigned int>(value.year),
               static_cast<unsigned int>(value.month),
               static_cast<unsigned int>(value.day),
               static_cast<unsigned int>(value.hour),
               static_cast<unsigned int>(value.minute),
               static_cast<unsigned int>(value.second));
  if (length < 0 || static_cast<size_t>(length) >= sizeof(state->response)) {
    setResponse(state, "ERR RESPONSE_TOO_LARGE");
    return;
  }
  state->response_length = static_cast<uint16_t>(length);
  state->response_offset = 0;
}

bool parseSetTime(const char* line, size_t length,
                  clock_command::DateTime* value) {
  static const char command[] = "SET_TIME";
  if (!hasPrefix(line, length, command, sizeof(command) - 1) ||
      length != 28 || line[8] != ' ' || line[13] != '-' ||
      line[16] != '-' || line[19] != ' ' || line[22] != ':' ||
      line[25] != ':') {
    return false;
  }

  uint16_t year, month, day, hour, minute, second;
  if (!parseDigits(line, 9, 4, &year) ||
      !parseDigits(line, 14, 2, &month) ||
      !parseDigits(line, 17, 2, &day) ||
      !parseDigits(line, 20, 2, &hour) ||
      !parseDigits(line, 23, 2, &minute) ||
      !parseDigits(line, 26, 2, &second)) {
    return false;
  }

  value->year = year;
  value->month = static_cast<uint8_t>(month);
  value->day = static_cast<uint8_t>(day);
  value->hour = static_cast<uint8_t>(hour);
  value->minute = static_cast<uint8_t>(minute);
  value->second = static_cast<uint8_t>(second);
  return true;
}

void processLine(State* state, clock_command::RtcWriter& rtc,
                 DirectoryReader* directory, DisplaySettingsControl* display,
                 EncoderControl* encoder, LedControl* leds,
                 AlarmControl* alarm_control, audio::Control* audio_control,
                 sleep_timer::Control* sleep_control) {
  state->input[state->input_length] = '\0';
  if (memchr(state->input, '\0', state->input_length) != nullptr) {
    setResponse(state, "ERR UNKNOWN_COMMAND");
    return;
  }
  const bool get_sleep = strcmp(state->input, "GET_SLEEP") == 0 ||
      hasPrefix(state->input, state->input_length, "GET_SLEEP ", 10);
  if (strcmp(state->input, "TAP") == 0) {
    setResponse(state, encoder && encoder->tap()
        ? "OK TAP" : "ERR TAP UNAVAILABLE");
    return;
  }
  if (strcmp(state->input, "GET_TAP") == 0) {
    if (!encoder) {
      setResponse(state, "ERR TAP UNAVAILABLE");
    } else {
      const tap_input::Status status = encoder->tapStatus();
      char response[256];
      snprintf(response, sizeof(response),
          "OK TAP AVAILABLE %u ERROR %s COUNT %lu LAST_MS %lu DELTA_MG %u PEAK_MG %u FILTERED_PEAK_MG %u THRESHOLD_MG %u SAMPLES %lu COOLDOWN_MS %lu",
          status.available ? 1u : 0u, tap_input::errorName(status.error),
          static_cast<unsigned long>(status.state.count),
          static_cast<unsigned long>(status.state.last_ms),
          status.state.delta_mg, status.state.peak_mg,
          status.state.filtered_peak_mg, status.threshold_mg,
          static_cast<unsigned long>(status.state.samples),
          static_cast<unsigned long>(tap_input::COOLDOWN_MS));
      setResponse(state, response);
    }
    return;
  }
  if (hasPrefix(state->input, state->input_length, "SET_TAP_THRESHOLD ", 18)) {
    uint16_t threshold = 0;
    const size_t count = state->input_length - 18;
    if (count < 1 || count > 3 ||
        !parseDigits(state->input, 18, count, &threshold) ||
        threshold < 10 || threshold > 500) {
      setResponse(state, "ERR TAP INVALID_VALUE");
    } else {
      setResponse(state, encoder && encoder->setTapThreshold(threshold)
          ? "OK SET_TAP_THRESHOLD" : "ERR TAP UNAVAILABLE");
    }
    return;
  }
  const bool start_sleep = strcmp(state->input, "START_SLEEP") == 0 ||
      hasPrefix(state->input, state->input_length, "START_SLEEP ", 12);
  const bool stop_sleep = strcmp(state->input, "STOP_SLEEP") == 0;
  const bool set_sleep = hasPrefix(state->input, state->input_length, "SET_SLEEP ", 10);
  const bool load_sleep = hasPrefix(state->input, state->input_length, "LOAD_SLEEP ", 11);
  const bool save_sleep = hasPrefix(state->input, state->input_length, "SAVE_SLEEP ", 11);
  if (get_sleep || start_sleep || stop_sleep || set_sleep || load_sleep || save_sleep) {
    if (sleep_control == nullptr) {
      setResponse(state, "ERR SLEEP UNAVAILABLE");
      return;
    }
    uint8_t slot = sleep_timer::NO_SLOT;
    size_t field_offset = 10;
    if (set_sleep && state->input[10] >= '0' && state->input[10] <= '9') {
      if (state->input_length < 12 || state->input[11] != ' ' ||
          state->input[10] < '1' || state->input[10] > '3') {
        setResponse(state, "ERR SLEEP INVALID_VALUE");
        return;
      }
      slot = static_cast<uint8_t>(state->input[10] - '1');
      field_offset = 12;
    } else if (load_sleep || save_sleep ||
               (get_sleep && state->input_length != 9) ||
               (start_sleep && state->input_length != 11)) {
      const size_t prefix = get_sleep ? 10 : start_sleep ? 12 : 11;
      if (state->input_length != prefix + 1 ||
          state->input[prefix] < '1' || state->input[prefix] > '3') {
        setResponse(state, "ERR SLEEP INVALID_VALUE");
        return;
      }
      slot = static_cast<uint8_t>(state->input[prefix] - '1');
    }
    if (slot != sleep_timer::NO_SLOT && !sleep_control->available()) {
      setResponse(state, "ERR SLEEP STORAGE");
      return;
    }
    sleep_timer::Config config = slot != sleep_timer::NO_SLOT && !save_sleep
        ? sleep_control->getSlot(slot) : sleep_control->get();
    if (get_sleep) {
      const sleep_timer::Status status = sleep_control->status();
      const bool active = status.active &&
          (slot == sleep_timer::NO_SLOT || status.slot == slot);
      const audio::Status sound = audio_control ? audio_control->status() : audio::Status{};
      char response[640];
      snprintf(response, sizeof(response),
          "OK SLEEP SLOT %u ACTIVE_SLOT %u STATE %s REMAINING %lu ELAPSED_MS %lu ACTIVE_LIGHT %u ACTIVE_AUDIO %u "
          "HOLD %u TOTAL_SECONDS %lu OUTPUT %s EFFECT %s COLOR %u %u %u %u BRIGHTNESS %u "
          "FADE_IN_SECONDS %u FADE_OUT %u VOLUME %u AUDIO_FADE_IN_SECONDS %u "
          "AUDIO_FADE_OUT %u AUDIO %s AUDIO_RESULT %s SOUND %s",
          slot == sleep_timer::NO_SLOT ? 0u : static_cast<unsigned>(slot + 1),
          status.active && status.slot != sleep_timer::NO_SLOT
              ? static_cast<unsigned>(status.slot + 1) : 0u,
          active ? "RUNNING" : "OFF",
          static_cast<unsigned long>(active ? status.remaining_seconds : 0),
          static_cast<unsigned long>(active ? status.elapsed_ms : 0),
          active && status.light_enabled ? 1u : 0u, active && status.audio_enabled ? 1u : 0u,
          config.hold_minutes,
          static_cast<unsigned long>(sleep_timer::durationMilliseconds(config) / 1000),
          config.light_enabled
              ? (config.audio_enabled ? "BOTH" : "LIGHT") : "AUDIO",
          lighting::effectName(config.light.effect), config.light.color.red,
          config.light.color.green, config.light.color.blue, config.light.color.white,
          config.light.brightness, sleep_timer::fadeInMilliseconds(config) / 1000u,
          config.light.fade_out_minutes,
          config.sound.volume, sleep_timer::fadeInMilliseconds(config) / 1000u, config.light.fade_out_minutes,
          active && sound.sleep ? audio::stateName(sound.state) : "OFF",
          active && sound.sleep ? audio::errorName(sound.result) : "NONE",
          config.sound.path[0] ? config.sound.path : "NONE");
      setResponse(state, response);
      return;
    }
    sleep_timer::Result result = sleep_timer::Result::invalid_value;
    if (set_sleep) {
      const char* field = state->input + field_offset;
      const char* separator = strchr(field, ' ');
      if (separator == nullptr) {
        setResponse(state, "ERR SLEEP INVALID_VALUE");
        return;
      }
      const size_t field_length = static_cast<size_t>(separator - field);
      const size_t offset = static_cast<size_t>(separator - state->input) + 1;
      const char* value = state->input + offset;
      if ((field_length == 7 && memcmp(field, "FADE_IN", 7) == 0) ||
          (field_length == 13 && memcmp(field, "AUDIO_FADE_IN", 13) == 0)) {
        setResponse(state, "ERR SLEEP FIXED_FADE_IN");
        return;
      }
      if (field_length == 8 && memcmp(field, "DURATION", 8) == 0) {
        setResponse(state, "ERR SLEEP USE_HOLD");
        return;
      }
      uint16_t numbers[4] = {};
      const bool numeric = parseUnsignedFields(
          state->input, state->input_length, offset, numbers, 1);
      const uint16_t number = numbers[0];
      bool good = true;
      if (field_length == 6 && memcmp(field, "OUTPUT", 6) == 0) {
        good = strcmp(value, "LIGHT") == 0 || strcmp(value, "AUDIO") == 0 ||
            strcmp(value, "BOTH") == 0;
        if (good) {
          config.light_enabled = strcmp(value, "AUDIO") != 0;
          config.audio_enabled = strcmp(value, "LIGHT") != 0;
        }
      } else if (field_length == 6 && memcmp(field, "EFFECT", 6) == 0) {
        good = lighting::parseEffect(value, strlen(value), config.light.effect);
      } else if (field_length == 5 && memcmp(field, "SOUND", 5) == 0) {
        good = strlen(value) > 0 && strlen(value) < audio::PATH_CAPACITY;
        if (good) {
          memset(config.sound.path, 0, audio::PATH_CAPACITY);
          if (strcmp(value, "NONE") != 0) strcpy(config.sound.path, value);
        }
      } else if (field_length == 5 && memcmp(field, "COLOR", 5) == 0) {
        good = parseUnsignedFields(state->input, state->input_length, offset, numbers, 4);
        for (uint16_t component : numbers) if (component > 255) good = false;
        if (good) config.light.color = {static_cast<uint8_t>(numbers[0]),
            static_cast<uint8_t>(numbers[1]), static_cast<uint8_t>(numbers[2]),
            static_cast<uint8_t>(numbers[3])};
      } else if (!numeric) good = false;
      else if (field_length == 4 && memcmp(field, "HOLD", 4) == 0)
        config.hold_minutes = number;
      else if (number > 255) good = false;
      else if (field_length == 10 && memcmp(field, "BRIGHTNESS", 10) == 0)
        config.light.brightness = static_cast<uint8_t>(number);
      else if (field_length == 8 && memcmp(field, "FADE_OUT", 8) == 0)
        config.light.fade_out_minutes = static_cast<uint8_t>(number);
      else if (field_length == 6 && memcmp(field, "VOLUME", 6) == 0)
        config.sound.volume = static_cast<uint8_t>(number);
      else if (field_length == 14 && memcmp(field, "AUDIO_FADE_OUT", 14) == 0)
        config.light.fade_out_minutes = static_cast<uint8_t>(number);
      else good = false;
      if (good) result = slot == sleep_timer::NO_SLOT ? sleep_control->set(config)
          : sleep_control->saveSlot(slot, config);
    } else if (start_sleep) result = sleep_control->start(slot);
    else if (load_sleep) result = sleep_control->loadSlot(slot);
    else if (save_sleep) result = sleep_control->saveSlot(slot, config);
    else result = sleep_control->stop();
    char response[80];
    if (result == sleep_timer::Result::ok) {
      const char* name = set_sleep ? "SET_SLEEP" : start_sleep ? "START_SLEEP" :
          load_sleep ? "LOAD_SLEEP" : save_sleep ? "SAVE_SLEEP" : "STOP_SLEEP";
      if (slot == sleep_timer::NO_SLOT) snprintf(response, sizeof(response), "OK %s", name);
      else snprintf(response, sizeof(response), "OK %s %u", name, static_cast<unsigned>(slot + 1));
    }
    else snprintf(response, sizeof(response), "ERR SLEEP %s", sleep_timer::resultName(result));
    setResponse(state, response);
    return;
  }
  const bool get_audio = strcmp(state->input, "GET_AUDIO") == 0;
  const bool start_audio = hasPrefix(state->input, state->input_length, "START_AUDIO ", 12);
  const bool volume_audio = hasPrefix(state->input, state->input_length, "SET_AUDIO_VOLUME ", 17);
  const bool stop_audio = strcmp(state->input, "STOP_AUDIO") == 0;
  const bool fixture_audio = hasPrefix(state->input, state->input_length, "AUDIO_FIXTURE ", 14);
  const bool cleanup_audio = hasPrefix(state->input, state->input_length, "AUDIO_CLEAN ", 12);
  const bool spi_audio = hasPrefix(state->input, state->input_length, "AUDIO_SPI ", 10);
  if (fixture_audio || cleanup_audio || spi_audio) {
    audio::Request request;
    request.action = fixture_audio ? audio::Action::fixture :
        cleanup_audio ? audio::Action::cleanup : audio::Action::spi;
    size_t offset = fixture_audio ? 14 : cleanup_audio ? 12 : 10;
    uint32_t fields[4] = {};
    bool good = true;
    if (!cleanup_audio) {
      const uint8_t count = fixture_audio ? 4 : 1;
      for (uint8_t field = 0; field < count; ++field) {
        const size_t start = offset;
        while (offset < state->input_length && state->input[offset] >= '0' &&
               state->input[offset] <= '9') {
          if (offset - start < 6) fields[field] = fields[field] * 10 +
              static_cast<uint32_t>(state->input[offset] - '0');
          ++offset;
        }
        if (offset == start || offset - start > 6) good = false;
        if ((fixture_audio || field + 1 < count) &&
            (offset >= state->input_length || state->input[offset++] != ' ')) good = false;
      }
      request.rate = fields[0];
      good = good && fields[1] <= UINT8_MAX && fields[2] <= UINT8_MAX &&
          fields[3] <= UINT8_MAX;
      request.bits = static_cast<uint8_t>(fields[1]);
      request.channels = static_cast<uint8_t>(fields[2]);
      request.seconds = static_cast<uint8_t>(fields[3]);
    }
    if (!spi_audio && offset <= state->input_length &&
        state->input_length - offset < audio::PATH_CAPACITY)
      memcpy(request.path, state->input + offset, state->input_length - offset);
    else if (!spi_audio || offset != state->input_length) good = false;
    uint32_t id = 0;
    const audio::Error error = !good || !audio::validRequest(request)
        ? audio::Error::invalid_value : audio_control == nullptr
        ? audio::Error::unavailable : audio_control->submit(request, id);
    char response[80];
    if (error != audio::Error::none)
      snprintf(response, sizeof(response), "ERR AUDIO %s", audio::errorName(error));
    else snprintf(response, sizeof(response), "OK %s QUEUED %lu",
        fixture_audio ? "AUDIO_FIXTURE" : cleanup_audio ? "AUDIO_CLEAN" : "AUDIO_SPI",
        static_cast<unsigned long>(id));
    setResponse(state, response);
    return;
  }
  if (get_audio || start_audio || volume_audio || stop_audio) {
    if (audio_control == nullptr) {
      setResponse(state, "ERR AUDIO UNAVAILABLE");
      return;
    }
    char response[640];
    if (get_audio) {
      const audio::Status status = audio_control->status();
      if (status.core != 1) {
        setResponse(state, "ERR AUDIO UNAVAILABLE");
        return;
      }
      const int length = snprintf(response, sizeof(response),
          "OK AUDIO CORE %u IRQ_CORE %u STATE %s REQUEST %lu RESULT %s VOLUME %u LIMIT %u "
          "RATE %lu CHANNELS %u BITS %u FRAMES %lu TOTAL %lu BYTES %llu "
          "UNDERRUNS %lu PEAK %u BLOCK_PEAK %u GAIN_BPS %u ALARM %u SLEEP %u "
          "READ_US %llu WAIT_US %llu MAX_READ_US %lu SPI_MHZ %lu PATH %s",
          status.core, status.irq_core, audio::stateName(status.state),
          static_cast<unsigned long>(status.request), audio::errorName(status.result),
          status.volume, audio::VOLUME_LIMIT, static_cast<unsigned long>(status.rate),
          status.channels, status.bits, static_cast<unsigned long>(status.frames),
          static_cast<unsigned long>(status.total_frames),
          static_cast<unsigned long long>(status.bytes),
          static_cast<unsigned long>(status.underruns), status.peak,
          status.block_peak, status.gain_bps, status.alarm ? 1u : 0u,
          status.sleep ? 1u : 0u,
          static_cast<unsigned long long>(status.read_us),
          static_cast<unsigned long long>(status.wait_us),
          static_cast<unsigned long>(status.max_read_us),
          static_cast<unsigned long>(status.spi_mhz), status.path);
      if (length < 0 || static_cast<size_t>(length) >= sizeof(response)) {
        setResponse(state, "ERR RESPONSE_TOO_LARGE");
        return;
      }
      setResponse(state, response);
      return;
    }
    audio::Request request;
    if (!stop_audio) {
      size_t offset = start_audio ? 12 : 17;
      const size_t start = offset;
      while (offset < state->input_length &&
             state->input[offset] >= '0' && state->input[offset] <= '9') ++offset;
      uint16_t volume = 0;
      if (offset == start || offset - start > 3 ||
          !parseDigits(state->input, start, offset - start, &volume) ||
          volume > audio::VOLUME_LIMIT ||
          (volume_audio && offset != state->input_length) ||
          (start_audio && (offset >= state->input_length || state->input[offset++] != ' '))) {
        setResponse(state, "ERR AUDIO INVALID_VALUE");
        return;
      }
      request.volume = static_cast<uint8_t>(volume);
      request.action = start_audio ? audio::Action::start : audio::Action::volume;
      if (start_audio) {
        const size_t length = state->input_length - offset;
        if (length >= audio::PATH_CAPACITY) {
          setResponse(state, "ERR AUDIO INVALID_VALUE");
          return;
        }
        memcpy(request.path, state->input + offset, length);
      }
      if (!audio::validRequest(request)) {
        setResponse(state, "ERR AUDIO INVALID_VALUE");
        return;
      }
    }
    uint32_t id = 0;
    const audio::Error result = audio_control->submit(request, id);
    if (result != audio::Error::none) {
      snprintf(response, sizeof(response), "ERR AUDIO %s", audio::errorName(result));
    } else {
      snprintf(response, sizeof(response), "OK %s QUEUED %lu",
               start_audio ? "START_AUDIO" : stop_audio ? "STOP_AUDIO" : "SET_AUDIO_VOLUME",
               static_cast<unsigned long>(id));
    }
    setResponse(state, response);
    return;
  }
  if (strcmp(state->input, "GET_TIME") == 0) {
    clock_command::DateTime value = {};
    if (!rtc.getDateTime(value)) {
      setResponse(state, "ERR TIME INVALID");
    } else {
      char response[64];
      snprintf(response, sizeof(response), "OK TIME %04u-%02u-%02u %02u:%02u:%02u",
               value.year, value.month, value.day, value.hour, value.minute, value.second);
      setResponse(state, response);
    }
    return;
  }
  if (strcmp(state->input, "GET_SCREEN") == 0) {
    const char* text = display == nullptr ? nullptr : display->screenText();
    if (text == nullptr) setResponse(state, "ERR SCREEN UNAVAILABLE");
    else {
      char response[80];
      snprintf(response, sizeof(response), "OK SCREEN %s", text);
      setResponse(state, response);
    }
    return;
  }
  const bool get_alarm = hasPrefix(state->input, state->input_length, "GET_ALARM ", 10);
  const bool start_alarm = hasPrefix(state->input, state->input_length, "START_ALARM ", 12);
  const bool set_alarm = hasPrefix(state->input, state->input_length, "SET_ALARM ", 10);
  const bool snooze_alarm = strcmp(state->input, "SNOOZE_ALARM") == 0;
  const bool dismiss_alarm = strcmp(state->input, "DISMISS_ALARM") == 0;
  const bool reduce_alarm = strcmp(state->input, "HALF_ALARM") == 0;
  if (get_alarm || start_alarm || set_alarm || snooze_alarm ||
      dismiss_alarm || reduce_alarm) {
    uint8_t slot = 0;
    const size_t offset = start_alarm ? 12 : 10;
    if (get_alarm || start_alarm || set_alarm) {
      if (state->input_length < offset + 1 ||
          state->input[offset] < '1' || state->input[offset] > '5' ||
          ((get_alarm || start_alarm) && state->input_length != offset + 1) ||
          (set_alarm && (state->input_length <= offset + 3 ||
                        state->input[offset + 1] != ' '))) {
        setResponse(state, "ERR ALARM INVALID_VALUE");
        return;
      }
      slot = static_cast<uint8_t>(state->input[offset] - '1');
    }
    if (alarm_control == nullptr) {
      setResponse(state, "ERR ALARM UNAVAILABLE");
      return;
    }
    if ((get_alarm || set_alarm || start_alarm) && !alarm_control->available()) {
      setResponse(state, "ERR ALARM STORAGE");
      return;
    }
    if (get_alarm) {
      const alarms::Config config = alarm_control->get(slot);
      const alarms::Status status = alarm_control->status();
      const char* phase = status.slot != slot ? "OFF" :
          status.phase == alarms::Phase::ramping ? "RAMPING" :
          status.phase == alarms::Phase::ringing ? "RINGING" :
          status.phase == alarms::Phase::fading ? "FADING" :
          status.phase == alarms::Phase::snoozed ? "SNOOZED" : "OFF";
      const char* effect = lighting::effectName(config.light.effect);
      const audio::Status sound = audio_control ? audio_control->status() : audio::Status{};
      char response[640];
      snprintf(response, sizeof(response),
          "OK ALARM %u ENABLED %u ONESHOT %u TIME %02u:%02u DAYS %u EFFECT %s "
          "COLOR %u %u %u %u BRIGHTNESS %u FADE_IN %u FADE_OUT %u "
          "HOLD %u RUN %u STATE %s REMAINING %lu VOLUME %u AUDIO_FADE_IN %u "
          "AUDIO_FADE_OUT %u AUDIO %s AUDIO_RESULT %s SOUND %s",
          static_cast<unsigned>(slot + 1), config.enabled ? 1u : 0u,
          config.one_shot ? 1u : 0u, static_cast<unsigned>(config.minute / 60),
          static_cast<unsigned>(config.minute % 60), static_cast<unsigned>(config.days),
          effect, config.light.color.red, config.light.color.green,
          config.light.color.blue, config.light.color.white, config.light.brightness,
          config.light.fade_in_minutes, config.light.fade_out_minutes,
          config.hold_minutes,
          config.hold_minutes + config.light.fade_out_minutes, phase,
          static_cast<unsigned long>(status.slot == slot ? status.remaining_seconds : 0),
          config.sound.volume, config.light.fade_in_minutes, config.light.fade_out_minutes,
          status.slot == slot && sound.alarm ? audio::stateName(sound.state) : "OFF",
          status.slot == slot && sound.alarm ? audio::errorName(sound.result) : "NONE",
          config.sound.path[0] ? config.sound.path : "NONE");
      setResponse(state, response);
      return;
    }
    alarms::Result result = alarms::Result::invalid_value;
    if (set_alarm) {
      alarms::Config config = alarm_control->get(slot);
      const char* field = state->input + offset + 2;
      const char* separator = strchr(field, ' ');
      if (separator == nullptr) {
        setResponse(state, "ERR ALARM INVALID_VALUE");
        return;
      }
      const size_t field_length = static_cast<size_t>(separator - field);
      const size_t value_offset = static_cast<size_t>(separator - state->input) + 1;
      const char* value = state->input + value_offset;
      uint16_t numbers[4] = {};
      const bool numeric = parseUnsignedFields(
          state->input, state->input_length, value_offset, numbers, 1);
      const uint16_t number = numbers[0];
      bool good = true;
      if (field_length == 4 && memcmp(field, "TIME", 4) == 0) {
        uint16_t hour = 0, minute = 0;
        good = strlen(value) == 5 && value[2] == ':' &&
            parseDigits(value, 0, 2, &hour) && parseDigits(value, 3, 2, &minute) &&
            hour < 24 && minute < 60;
        if (good) config.minute = hour * 60 + minute;
      } else if (field_length == 6 && memcmp(field, "EFFECT", 6) == 0) {
        good = lighting::parseEffect(value, strlen(value), config.light.effect);
      } else if (field_length == 5 && memcmp(field, "COLOR", 5) == 0) {
        good = parseUnsignedFields(state->input, state->input_length, value_offset, numbers, 4);
        for (uint16_t component : numbers) if (component > 255) good = false;
        if (good) config.light.color = {static_cast<uint8_t>(numbers[0]),
            static_cast<uint8_t>(numbers[1]), static_cast<uint8_t>(numbers[2]),
            static_cast<uint8_t>(numbers[3])};
      } else if (field_length == 5 && memcmp(field, "SOUND", 5) == 0) {
        good = strlen(value) > 0 && strlen(value) < audio::PATH_CAPACITY;
        if (good) {
          memset(config.sound.path, 0, audio::PATH_CAPACITY);
          if (strcmp(value, "NONE") != 0) strcpy(config.sound.path, value);
        }
      } else if (!numeric) good = false;
      else if (field_length == 6 && memcmp(field, "VOLUME", 6) == 0) {
        good = number <= audio::VOLUME_LIMIT;
        config.sound.volume = static_cast<uint8_t>(number);
      } else if (field_length == 13 && memcmp(field, "AUDIO_FADE_IN", 13) == 0) {
        good = number <= 255;
        config.light.fade_in_minutes = static_cast<uint8_t>(number);
      } else if (field_length == 14 && memcmp(field, "AUDIO_FADE_OUT", 14) == 0) {
        good = number <= 255;
        config.light.fade_out_minutes = static_cast<uint8_t>(number);
      }
      else if (field_length == 7 && memcmp(field, "ENABLED", 7) == 0) {
        good = number <= 1;
        config.enabled = number == 1;
      } else if (field_length == 7 && memcmp(field, "ONESHOT", 7) == 0) {
        good = number <= 1;
        config.one_shot = number == 1;
      } else if (field_length == 4 && memcmp(field, "DAYS", 4) == 0) {
        good = number > 0 && number <= 127;
        config.days = static_cast<uint8_t>(number);
      } else if (field_length == 10 && memcmp(field, "BRIGHTNESS", 10) == 0) {
        good = number <= 255;
        config.light.brightness = static_cast<uint8_t>(number);
      } else if (field_length == 7 && memcmp(field, "FADE_IN", 7) == 0) {
        good = number <= 255;
        config.light.fade_in_minutes = static_cast<uint8_t>(number);
      } else if (field_length == 8 && memcmp(field, "FADE_OUT", 8) == 0) {
        good = number <= 255;
        config.light.fade_out_minutes = static_cast<uint8_t>(number);
      } else if (field_length == 4 && memcmp(field, "HOLD", 4) == 0) {
        config.hold_minutes = number;
      } else if (field_length == 3 && memcmp(field, "RUN", 3) == 0) {
        good = number >= config.light.fade_out_minutes && number <= 180;
        if (good) config.hold_minutes = number - config.light.fade_out_minutes;
      } else good = false;
      if (good) result = alarm_control->set(slot, config);
    } else if (start_alarm) result = alarm_control->start(slot);
    else if (snooze_alarm) result = alarm_control->snooze();
    else if (dismiss_alarm) result = alarm_control->dismiss();
    else result = alarm_control->reduce();
    if (result != alarms::Result::ok) {
      setResponse(state, result == alarms::Result::invalid_value
          ? "ERR ALARM INVALID_VALUE" : result == alarms::Result::no_active
          ? "ERR ALARM NO_ACTIVE" : "ERR ALARM STORAGE");
    } else {
      char response[48];
      if (set_alarm || start_alarm) {
        snprintf(response, sizeof(response), "OK %s %u",
                 set_alarm ? "SET_ALARM" : "START_ALARM", static_cast<unsigned>(slot + 1));
      } else snprintf(response, sizeof(response), "OK %s",
                      snooze_alarm ? "SNOOZE_ALARM" :
                      dismiss_alarm ? "DISMISS_ALARM" : "HALF_ALARM");
      setResponse(state, response);
    }
    return;
  }
  if (strcmp(state->input, "BOOTLOADER") == 0) {
    setResponse(state, "OK BOOTLOADER");
    state->bootloader_requested = true;
    return;
  }
  if (state->input_length == 10 && strcmp(state->input, "STOP_LIGHT") == 0) {
    const LedResult result = leds == nullptr ? LedResult::unavailable
                                             : leds->stopEffect();
    setResponse(state, result == LedResult::ok ? "OK STOP_LIGHT" :
        result == LedResult::busy ? "ERR LIGHT BUSY" : "ERR LIGHT UNAVAILABLE");
    return;
  }
  if (state->input_length == 9 && strcmp(state->input, "GET_LIGHT") == 0) {
    lighting::AnimationConfig config = {};
    lighting::RunState running = lighting::RunState::off;
    if (leds == nullptr || !leds->getLight(config, running)) {
      setResponse(state, "ERR LIGHT UNAVAILABLE");
      return;
    }
    const char* effect = lighting::effectName(config.effect);
    const char* run_state = running == lighting::RunState::running ? "RUNNING"
                          : running == lighting::RunState::fading_out ? "FADING"
                          : "OFF";
    char response[160];
    snprintf(response, sizeof(response),
             "OK LIGHT %s %s %u %u %u %u %u %u %u", run_state, effect,
             static_cast<unsigned>(config.color.red),
             static_cast<unsigned>(config.color.green),
             static_cast<unsigned>(config.color.blue),
             static_cast<unsigned>(config.color.white),
             static_cast<unsigned>(config.brightness),
             static_cast<unsigned>(config.fade_in_minutes),
             static_cast<unsigned>(config.fade_out_minutes));
    setResponse(state, response);
    return;
  }
  if (hasPrefix(state->input, state->input_length, "START_LIGHT ", 12)) {
    const char* effect = state->input + 12;
    const char* separator = strchr(effect, ' ');
    if (separator == nullptr) {
      setResponse(state, "ERR LIGHT INVALID_VALUE");
      return;
    }
    const size_t effect_length = static_cast<size_t>(separator - effect);
    lighting::Effect selected;
    if (!lighting::parseEffect(effect, effect_length, selected)) {
      setResponse(state, "ERR LIGHT INVALID_VALUE");
      return;
    }
    uint16_t values[7] = {};
    const size_t offset = static_cast<size_t>(separator - state->input) + 1;
    if (!parseUnsignedFields(state->input, state->input_length, offset,
                             values, 7)) {
      setResponse(state, "ERR LIGHT INVALID_VALUE");
      return;
    }
    for (size_t i = 0; i < 7; ++i) {
      if (values[i] > 255) {
        setResponse(state, "ERR LIGHT INVALID_VALUE");
        return;
      }
    }
    if (!lighting::validFadeMinutes(static_cast<uint8_t>(values[5])) ||
        !lighting::validFadeMinutes(static_cast<uint8_t>(values[6]))) {
      setResponse(state, "ERR LIGHT INVALID_VALUE");
      return;
    }
    const lighting::AnimationConfig config = {
        selected,
        {static_cast<uint8_t>(values[0]), static_cast<uint8_t>(values[1]),
         static_cast<uint8_t>(values[2]), static_cast<uint8_t>(values[3])},
        static_cast<uint8_t>(values[4]), static_cast<uint8_t>(values[5]),
        static_cast<uint8_t>(values[6])};
    const LedResult result = leds == nullptr ? LedResult::unavailable
                                             : leds->startEffect(config);
    setResponse(state, result == LedResult::ok ? "OK START_LIGHT" :
                       result == LedResult::invalid_value ?
                           "ERR LIGHT INVALID_VALUE" :
                       result == LedResult::busy ? "ERR LIGHT BUSY" : "ERR LIGHT UNAVAILABLE");
    return;
  }
  if (strcmp(state->input, "CLEAR_LEDS") == 0) {
    const LedResult result = leds == nullptr ? LedResult::unavailable
                                             : leds->clear();
    setResponse(state, result == LedResult::ok ? "OK CLEAR_LEDS" :
        result == LedResult::busy ? "ERR LEDS BUSY" : "ERR LEDS UNAVAILABLE");
    return;
  }
  if (strcmp(state->input, "GET_LEDS") == 0) {
    lighting::Frame frame = {};
    if (leds == nullptr || !leds->getFrame(frame)) {
      setResponse(state, "ERR LEDS UNAVAILABLE");
      return;
    }
    static const char digits[] = "0123456789ABCDEF";
    static const char prefix[] = "OK LEDS ";
    static_assert(sizeof(prefix) - 1 + lighting::PIXEL_COUNT * 8 + 1 <=
                      RESPONSE_CAPACITY, "LED response exceeds capacity");
    size_t position = sizeof(prefix) - 1;
    memcpy(state->response, prefix, position);
    for (size_t i = 0; i < lighting::PIXEL_COUNT; ++i) {
      const lighting::Color color = frame.pixels[i];
      const uint8_t components[] = {color.red, color.green, color.blue,
                                    color.white};
      for (uint8_t component : components) {
        state->response[position++] = digits[component >> 4];
        state->response[position++] = digits[component & 15];
      }
    }
    state->response[position++] = '\n';
    state->response_length = static_cast<uint16_t>(position);
    state->response_offset = 0;
    return;
  }
  if (hasPrefix(state->input, state->input_length, "SET_LED", 7)) {
    uint16_t values[6] = {};
    const bool valid = state->input_length > 8 && state->input[7] == ' ' &&
        parseUnsignedFields(state->input, state->input_length, 8, values, 6);
    if (!valid || values[0] >= 8 || values[1] >= 8 ||
        values[2] > 255 || values[3] > 255 ||
        values[4] > 255 || values[5] > 255) {
      setResponse(state, "ERR LEDS INVALID_VALUE");
      return;
    }
    const lighting::Color color = {static_cast<uint8_t>(values[2]),
                                   static_cast<uint8_t>(values[3]),
                                   static_cast<uint8_t>(values[4]),
                                   static_cast<uint8_t>(values[5])};
    const LedResult result = leds == nullptr ? LedResult::unavailable
        : leds->setPixel(static_cast<uint8_t>(values[0]),
                         static_cast<uint8_t>(values[1]), color);
    setResponse(state, result == LedResult::ok ? "OK SET_LED" :
                       result == LedResult::invalid_value ?
                           "ERR LEDS INVALID_VALUE" :
                       result == LedResult::busy ? "ERR LEDS BUSY" : "ERR LEDS UNAVAILABLE");
    return;
  }
  if (hasPrefix(state->input, state->input_length, "ENCODER ", 8)) {
    MenuEvent event = MENU_EVENT_ROTATE;
    int32_t delta = 0;
    if (state->input_length == 12 &&
        memcmp(state->input, "ENCODER LEFT", 12) == 0) delta = -1;
    else if (state->input_length == 13 &&
             memcmp(state->input, "ENCODER RIGHT", 13) == 0) delta = 1;
    else if (state->input_length == 13 &&
             memcmp(state->input, "ENCODER CLICK", 13) == 0)
      event = MENU_EVENT_SELECT;
    else {
      setResponse(state, "ERR ENCODER INVALID_VALUE");
      return;
    }
    if (encoder == nullptr || !encoder->send(event, delta)) {
      setResponse(state, "ERR ENCODER UNAVAILABLE");
    } else {
      setResponse(state, "OK ENCODER");
    }
    return;
  }
  if (strcmp(state->input, "GET_DISPLAY") == 0 &&
      state->input_length == 11) {
    if (display == nullptr) {
      setResponse(state, "ERR DISPLAY UNAVAILABLE");
    } else {
      const settings::DisplaySettings value = display->get();
      char response[64];
      snprintf(response, sizeof(response),
               "OK DISPLAY FORMAT %u BRIGHTNESS %u WAKE_SECONDS %u",
               value.use_12_hour ? 12u : 24u,
               static_cast<unsigned>(value.brightness),
               static_cast<unsigned>(value.wake_seconds));
      setResponse(state, response);
    }
    return;
  }
  if (state->input_length == 8 && strcmp(state->input, "GET_DARK") == 0) {
    if (display == nullptr) {
      setResponse(state, "ERR DARK UNAVAILABLE");
    } else {
      const settings::DisplaySettings value = display->get();
      char response[80];
      snprintf(response, sizeof(response),
               "OK DARK ENABLED %u START %02u:%02u END %02u:%02u BLANK %u",
               value.dark_enabled ? 1u : 0u,
               static_cast<unsigned>(value.dark_start / 60),
               static_cast<unsigned>(value.dark_start % 60),
               static_cast<unsigned>(value.dark_end / 60),
               static_cast<unsigned>(value.dark_end % 60),
               display->clockBlanked() ? 1u : 0u);
      setResponse(state, response);
    }
    return;
  }
  if (hasPrefix(state->input, state->input_length, "SET_DARK", 8)) {
    if (display == nullptr) {
      setResponse(state, "ERR DARK UNAVAILABLE");
      return;
    }
    settings::DisplaySettings candidate = display->get();
    if (state->input_length == 18 &&
        memcmp(state->input, "SET_DARK ENABLED ", 17) == 0 &&
        (state->input[17] == '0' || state->input[17] == '1')) {
      candidate.dark_enabled = state->input[17] == '1';
    } else {
      const bool start = hasPrefix(state->input, state->input_length,
                                   "SET_DARK START ", 15);
      const bool end = hasPrefix(state->input, state->input_length,
                                 "SET_DARK END ", 13);
      const size_t offset = start ? 15 : 13;
      uint16_t hour = 0;
      uint16_t minute = 0;
      if ((!start && !end) || state->input_length != offset + 5 ||
          state->input[offset + 2] != ':' ||
          !parseDigits(state->input, offset, 2, &hour) ||
          !parseDigits(state->input, offset + 3, 2, &minute) ||
          hour >= 24 || minute >= 60) {
        setResponse(state, "ERR DARK INVALID_VALUE");
        return;
      }
      if (start) candidate.dark_start = hour * 60 + minute;
      else candidate.dark_end = hour * 60 + minute;
    }
    const settings_command::Result result = display->set(candidate);
    setResponse(state, result == settings_command::Result::success
                           ? "OK SET_DARK"
                           : result == settings_command::Result::invalid_value
                                 ? "ERR DARK INVALID_VALUE"
                                 : "ERR DARK STORAGE");
    return;
  }
  if (hasPrefix(state->input, state->input_length, "SET_DISPLAY", 11)) {
    if (display == nullptr) {
      setResponse(state, "ERR DISPLAY UNAVAILABLE");
      return;
    }
    settings::DisplaySettings candidate = display->get();
    if (strcmp(state->input, "SET_DISPLAY FORMAT 12") == 0 &&
        state->input_length == 21) {
      candidate.use_12_hour = true;
    } else if (strcmp(state->input, "SET_DISPLAY FORMAT 24") == 0 &&
               state->input_length == 21) {
      candidate.use_12_hour = false;
    } else if (hasPrefix(state->input, state->input_length,
                         "SET_DISPLAY BRIGHTNESS ", 23)) {
      uint16_t brightness = 0;
      const size_t count = state->input_length - 23;
      if (count < 1 || count > 2 ||
          !parseDigits(state->input, 23, count, &brightness) ||
          brightness > 15) {
        setResponse(state, "ERR DISPLAY INVALID_VALUE");
        return;
      }
      candidate.brightness = static_cast<uint8_t>(brightness);
    } else if (hasPrefix(state->input, state->input_length,
                         "SET_DISPLAY WAKE_SECONDS ", 25)) {
      uint16_t seconds = 0;
      const size_t count = state->input_length - 25;
      if (count < 1 || count > 3 ||
          !parseDigits(state->input, 25, count, &seconds) ||
          seconds < 1 || seconds > 300) {
        setResponse(state, "ERR DISPLAY INVALID_VALUE");
        return;
      }
      candidate.wake_seconds = seconds;
    } else {
      setResponse(state, "ERR DISPLAY INVALID_VALUE");
      return;
    }
    const settings_command::Result result = display->set(candidate);
    setResponse(state, result == settings_command::Result::success
                           ? "OK SET_DISPLAY"
                           : result == settings_command::Result::invalid_value
                                 ? "ERR DISPLAY INVALID_VALUE"
                                 : "ERR DISPLAY STORAGE");
    return;
  }
  if (state->input_length > 8 &&
      memcmp(state->input, "SD_READ ", 8) == 0) {
    const char* path = state->input + 8;
    bool invalid_path = path[0] != '/';
    for (size_t i = 8; i < state->input_length; ++i) {
      if (static_cast<unsigned char>(state->input[i]) < 32 ||
          state->input[i] == 127) invalid_path = true;
    }
    if (invalid_path) {
      setResponse(state, "ERR SD_READ INVALID_PATH");
    } else if (directory == nullptr || !directory->openFile(path)) {
      setResponse(state, "ERR SD_READ UNAVAILABLE");
    } else {
      state->reading_file = true;
      setResponse(state, "OK SD_READ BEGIN");
    }
    return;
  }
  if ((state->input_length == 7 &&
       memcmp(state->input, "SD_LIST", 7) == 0) ||
      (state->input_length > 8 &&
       memcmp(state->input, "SD_LIST ", 8) == 0)) {
    const char* path = state->input_length == 7 ? "/" : state->input + 8;
    bool invalid_path = path[0] != '/';
    for (size_t i = 7; i < state->input_length; ++i) {
      if (static_cast<unsigned char>(state->input[i]) < 32 ||
          state->input[i] == 127) invalid_path = true;
    }
    if (invalid_path) {
      setResponse(state, "ERR SD_LIST INVALID_PATH");
    } else if (directory == nullptr || !directory->open(path)) {
      setResponse(state, "ERR SD_LIST UNAVAILABLE");
    } else {
      state->listing_directory = true;
      setResponse(state, "OK SD_LIST BEGIN");
    }
    return;
  }
  clock_command::DateTime value = {};
  if (!hasPrefix(state->input, state->input_length, "SET_TIME", 8)) {
    setResponse(state, "ERR UNKNOWN_COMMAND");
    return;
  }
  if (!parseSetTime(state->input, state->input_length, &value)) {
    setResponse(state, "ERR SET_TIME INVALID_FORMAT");
    return;
  }

  const clock_command::Result result = clock_command::setDateTime(rtc, value);
  if (result == clock_command::Result::invalid_date_time) {
    setResponse(state, "ERR SET_TIME INVALID_DATE_TIME");
    return;
  }
  if (result == clock_command::Result::rtc_write_failed) {
    setResponse(state, "ERR SET_TIME RTC_WRITE_FAILED");
    return;
  }

  setTimeResponse(state, "OK", value);
  queueTimeChangedEvent(state, value);
}

}

void init(State* state) {
  memset(state, 0, sizeof(*state));
}

bool canAcceptInput(const State* state) {
  return state->response_length == 0 && !state->listing_directory &&
         !state->reading_file && !state->bootloader_requested &&
         state->event_offset == 0;
}

bool bootloaderReady(const State* state) {
  return state->bootloader_requested && state->response_length == 0;
}

void cancelBootloaderRequest(State* state) {
  if (!state->bootloader_requested || state->response_length == 0) return;
  state->bootloader_requested = false;
  state->response_length = 0;
  state->response_offset = 0;
}

void receiveByte(State* state, char value, clock_command::RtcWriter& rtc,
                 DirectoryReader* directory, DisplaySettingsControl* display,
                 EncoderControl* encoder, LedControl* leds, AlarmControl* alarm_control,
                 audio::Control* audio_control, sleep_timer::Control* sleep_control) {
  if (!canAcceptInput(state)) {
    return;
  }

  if (value == '\n') {
    if (state->discard_line) {
      setResponse(state, "ERR INPUT_TOO_LONG");
    } else {
      if (state->input_length != 0 &&
          state->input[state->input_length - 1] == '\r') {
        --state->input_length;
      }
    }
    if (!state->discard_line && state->input_length != 0) {
      processLine(state, rtc, directory, display, encoder, leds, alarm_control,
                  audio_control, sleep_control);
    }
    state->input_length = 0;
    state->discard_line = false;
    return;
  }
  if (state->discard_line) {
    return;
  }
  if (state->input_length >= sizeof(state->input) - 1) {
    state->input_length = 0;
    state->discard_line = true;
    return;
  }
  state->input[state->input_length++] = value;
}

void advanceDirectory(State* state, DirectoryReader* directory) {
  if (state->response_length != 0) {
    return;
  }
  if (state->reading_file) {
    uint8_t bytes[32];
    const int count = directory->readFile(bytes, sizeof(bytes));
    if (count <= 0 || count > static_cast<int>(sizeof(bytes))) {
      directory->close();
      state->reading_file = false;
      setResponse(state, count == 0 ? "OK SD_READ END" : "ERR SD_READ IO");
      return;
    }
    static const char digits[] = "0123456789ABCDEF";
    memcpy(state->response, "DATA ", 5);
    for (int i = 0; i < count; ++i) {
      state->response[5 + i * 2] = digits[bytes[i] >> 4];
      state->response[6 + i * 2] = digits[bytes[i] & 15];
    }
    state->response[5 + count * 2] = '\n';
    state->response_length = static_cast<uint16_t>(6 + count * 2);
    state->response_offset = 0;
    return;
  }
  if (!state->listing_directory) {
    return;
  }
  char name[768];
  bool is_directory = false;
  uint64_t size = 0;
  const DirectoryResult result =
      directory->next(name, sizeof(name), &is_directory, &size);
  if (result != DirectoryResult::entry) {
    directory->close();
    state->listing_directory = false;
    setResponse(state, result == DirectoryResult::end ? "OK SD_LIST END"
                                                     : "ERR SD_LIST READ");
    return;
  }
  for (const char* p = name; *p; ++p) {
    if (static_cast<unsigned char>(*p) < 32 || *p == 127) {
      directory->close();
      state->listing_directory = false;
      setResponse(state, "ERR SD_LIST INVALID_NAME");
      return;
    }
  }
  const int length = snprintf(state->response, sizeof(state->response),
                              "ENTRY %c %llu %s\n", is_directory ? 'D' : 'F',
                              static_cast<unsigned long long>(size), name);
  if (length < 0 || static_cast<size_t>(length) >= sizeof(state->response)) {
    directory->close();
    state->listing_directory = false;
    setResponse(state, "ERR SD_LIST NAME_TOO_LONG");
    return;
  }
  state->response_length = static_cast<uint16_t>(length);
  state->response_offset = 0;
}

void abortDirectory(State* state, DirectoryReader* directory) {
  if (state->listing_directory || state->reading_file) {
    directory->close();
    state->listing_directory = false;
    state->reading_file = false;
    state->response_length = 0;
    state->response_offset = 0;
  }
}

const char* pendingOutput(const State* state, size_t* length) {
  if (state->response_offset < state->response_length) {
    *length = state->response_length - state->response_offset;
    return state->response + state->response_offset;
  }
  if (!state->listing_directory && !state->reading_file &&
      state->event_offset < state->event_length) {
    *length = state->event_length - state->event_offset;
    return state->event + state->event_offset;
  }
  *length = 0;
  return NULL;
}

void consumeOutput(State* state, size_t length) {
  if (state->response_offset < state->response_length) {
    const size_t remaining = state->response_length - state->response_offset;
    const size_t consumed = length < remaining ? length : remaining;
    state->response_offset += static_cast<uint16_t>(consumed);
    if (state->response_offset == state->response_length) {
      state->response_offset = 0;
      state->response_length = 0;
    }
    length -= consumed;
  }
  if (length != 0 && state->event_offset < state->event_length) {
    const size_t remaining = state->event_length - state->event_offset;
    const size_t consumed = length < remaining ? length : remaining;
    state->event_offset += static_cast<uint8_t>(consumed);
    if (state->event_offset == state->event_length) {
      state->event_offset = 0;
      state->event_length = 0;
    }
  }
}

void queueEvent(State* state, const char* line) {
  if (state->event_offset < state->event_length) {
    ++state->dropped_events;
    return;
  }

  const size_t length = strlen(line);
  if (length >= sizeof(state->event)) {
    ++state->dropped_events;
    return;
  }
  memcpy(state->event, line, length + 1);
  state->event_length = static_cast<uint8_t>(length);
  state->event_offset = 0;
}

void queueTimeChangedEvent(State* state,
                           const clock_command::DateTime& value) {
  char event[EVENT_CAPACITY];
  const int length =
      snprintf(event, sizeof(event), "EVENT TIME_SET %04u-%02u-%02u "
                                     "%02u:%02u:%02u\n",
               static_cast<unsigned int>(value.year),
               static_cast<unsigned int>(value.month),
               static_cast<unsigned int>(value.day),
               static_cast<unsigned int>(value.hour),
               static_cast<unsigned int>(value.minute),
               static_cast<unsigned int>(value.second));
  if (length >= 0 && static_cast<size_t>(length) < sizeof(event)) {
    queueEvent(state, event);
  } else {
    ++state->dropped_events;
  }
}

void queueDroppedEventSummary(State* state) {
  if (state->dropped_events == 0 ||
      state->event_offset < state->event_length) {
    return;
  }

  char summary[EVENT_CAPACITY];
  const int length = snprintf(summary, sizeof(summary),
                              "EVENT LOG_DROPPED %lu\n",
                              static_cast<unsigned long>(state->dropped_events));
  if (length >= 0 && static_cast<size_t>(length) < sizeof(summary)) {
    const uint32_t reported = state->dropped_events;
    queueEvent(state, summary);
    if (state->event_length != 0) {
      state->dropped_events -= reported;
    }
  }
}

void resetPartialInput(State* state) {
  state->input_length = 0;
  state->discard_line = false;
}

}
