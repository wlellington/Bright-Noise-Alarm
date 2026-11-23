#include "../Firmware/Light_Clock_Firmware/Clock_Command.hpp"
#include "../Firmware/Light_Clock_Firmware/Serial_Command.hpp"

#include <assert.h>
#include <stdio.h>
#include <string.h>

namespace {

class FakeRtc : public clock_command::RtcWriter {
public:
  explicit FakeRtc(const clock_command::DateTime& initial)
      : value(initial), writes(0), succeeds(true) {}

  bool setDateTime(const clock_command::DateTime& requested) override {
    ++writes;
    if (!succeeds) {
      return false;
    }
    value = requested;
    return true;
  }
  bool getDateTime(clock_command::DateTime& current) const override {
    current = value;
    return clock_command::isValid(value);
  }

  clock_command::DateTime value;
  unsigned int writes;
  bool succeeds;
};

bool sameDateTime(const clock_command::DateTime& left,
                  const clock_command::DateTime& right) {
  return left.year == right.year && left.month == right.month &&
         left.day == right.day && left.hour == right.hour &&
         left.minute == right.minute && left.second == right.second;
}

void receiveLine(serial_command::State* state, FakeRtc& rtc,
                const char* line,
                serial_command::DirectoryReader* directory = nullptr,
                serial_command::DisplaySettingsControl* display = nullptr,
                serial_command::EncoderControl* encoder = nullptr,
                serial_command::LedControl* leds = nullptr,
                serial_command::AlarmControl* alarms = nullptr,
                audio::Control* audio = nullptr,
                sleep_timer::Control* sleep = nullptr) {
  for (size_t index = 0; line[index] != '\0'; ++index) {
    serial_command::receiveByte(state, line[index], rtc, directory, display,
                                encoder, leds, alarms, audio, sleep);
  }
  serial_command::receiveByte(state, '\n', rtc, directory, display, encoder,
                              leds, alarms, audio, sleep);
}

class FakeEncoder : public serial_command::EncoderControl {
public:
  MenuEvent last_event = MENU_EVENT_NONE;
  int32_t last_delta = 0;
  unsigned taps = 0;
  bool tap() override { ++taps; return true; }
  uint16_t tap_threshold = tap_input::THRESHOLD_MG;
  bool setTapThreshold(uint16_t value) override {
    tap_threshold = value;
    return true;
  }
  bool send(MenuEvent event, int32_t delta) override {
    last_event = event;
    last_delta = delta;
    return true;
  }
};

class FakeLeds : public serial_command::LedControl {
public:
  lighting::Frame logical = {};
  lighting::Frame frame = {};
  lighting::Player player;
  lighting::AnimationConfig config = {
      lighting::Effect::sunrise, {255, 180, 64, 255}, 255, 1, 0};
  serial_command::LedResult setPixel(uint8_t row, uint8_t column,
                                     lighting::Color color) override {
    if (!lighting::setPixelFrame(row, column, color, logical)) {
      return serial_command::LedResult::invalid_value;
    }
    player.cancel();
    lighting::outputFrame(logical, frame);
    return serial_command::LedResult::ok;
  }
  serial_command::LedResult clear() override {
    player.cancel();
    logical = {};
    frame = {};
    return serial_command::LedResult::ok;
  }
  bool getFrame(lighting::Frame& result) const override {
    result = frame;
    return true;
  }
  serial_command::LedResult startEffect(
      const lighting::AnimationConfig& requested) override {
    if (!player.start(requested, 0)) return serial_command::LedResult::invalid_value;
    config = requested;
    return serial_command::LedResult::ok;
  }
  serial_command::LedResult stopEffect() override {
    return player.stop(0, config.fade_out_minutes) ? serial_command::LedResult::ok
                          : serial_command::LedResult::invalid_value;
  }
  bool getLight(lighting::AnimationConfig& result,
                lighting::RunState& state) const override {
    result = config;
    state = player.state();
    return true;
  }
};

class FakeSettingsStorage : public settings::Storage {
public:
  uint8_t bytes[2][alarms::RECORD_SIZE] = {};
  bool present[2] = {};
  size_t lengths[2] = {};
  bool fail = false;
  settings::ReadResult read(uint8_t slot, uint8_t* data, size_t capacity,
                            size_t* length) override {
    if (!present[slot]) return settings::ReadResult::missing;
    assert(lengths[slot] <= capacity);
    *length = lengths[slot];
    memcpy(data, bytes[slot], *length);
    return settings::ReadResult::ok;
  }
  bool write(uint8_t slot, const uint8_t* data, size_t size) override {
    if (fail) return false;
    present[slot] = true;
    lengths[slot] = size;
    memcpy(bytes[slot], data, size);
    return true;
  }
};

class FakeDisplaySettings : public serial_command::DisplaySettingsControl {
public:
  explicit FakeDisplaySettings(settings::Store& backing)
      : store(backing), value(settings::defaults()) {}
  settings::DisplaySettings get() const override { return value; }
  bool clockBlanked() const override { return false; }
  const char* screenText() const override { return "1234"; }
  settings_command::Result set(
      const settings::DisplaySettings& candidate) override {
    return settings_command::setDisplay(store, &value, candidate);
  }
  settings::Store& store;
  settings::DisplaySettings value;
};

class FakeAlarms : public serial_command::AlarmControl {
public:
  explicit FakeAlarms(alarms::Store& store) : engine(store), backing(store) {}
  alarms::Engine engine;
  alarms::Store& backing;
  bool available() const override { return backing.ready(); }
  alarms::Config get(uint8_t slot) const override { return engine.get(slot); }
  alarms::Status status() const override { return engine.status(0); }
  alarms::Result set(uint8_t slot, const alarms::Config& config) override {
    return engine.set(slot, config, 0);
  }
  alarms::Result start(uint8_t slot) override { return engine.start(slot, 0, 0); }
  alarms::Result snooze() override { return engine.snooze(0); }
  alarms::Result dismiss() override { return engine.dismiss(); }
  alarms::Result reduce() override { return engine.reduce(); }
};

class FakeDirectory : public serial_command::DirectoryReader {
public:
  int index = 0;
  bool opened = false;
  bool fail_read = false;
  char path[280] = {};
  bool open(const char* requested) override {
    snprintf(path, sizeof(path), "%s", requested);
    index = 0;
    opened = true;
    return true;
  }
  serial_command::DirectoryResult next(char* name, size_t capacity,
                                       bool* is_directory,
                                       uint64_t* size) override {
    if (fail_read) return serial_command::DirectoryResult::error;
    if (index == 2) return serial_command::DirectoryResult::end;
    snprintf(name, capacity, "%s", index == 0 ? "Music" : "alarm sound.wav");
    *is_directory = index == 0;
    *size = index == 0 ? 0 : 1234;
    ++index;
    return serial_command::DirectoryResult::entry;
  }
  void close() override { opened = false; }
  bool openFile(const char* requested) override {
    snprintf(path, sizeof(path), "%s", requested);
    index = 0;
    opened = true;
    return true;
  }
  int readFile(uint8_t* data, size_t capacity) override {
    if (fail_read) return -1;
    if (index++ != 0) return 0;
    assert(capacity >= 3);
    memcpy(data, "abc", 3);
    return 3;
  }
};

void expectOutput(const serial_command::State& state, const char* expected) {
  size_t length = 0;
  const char* output = serial_command::pendingOutput(&state, &length);
  assert(output != NULL);
  if (length != strlen(expected) || memcmp(output, expected, length) != 0) {
    fprintf(stderr, "Expected: %sActual: %.*s", expected,
            static_cast<int>(length), output);
  }
  assert(length == strlen(expected));
  assert(memcmp(output, expected, length) == 0);
}

void drainOutput(serial_command::State* state) {
  size_t length = 0;
  while (serial_command::pendingOutput(state, &length) != NULL) {
    serial_command::consumeOutput(state, length);
  }
}

void test_REQ_002_clock_command_validation() {
  const clock_command::DateTime initial = {2025, 6, 15, 10, 20, 30};
  const clock_command::DateTime leap_day = {2024, 2, 29, 23, 59, 59};
  FakeRtc rtc(initial);

  assert(clock_command::setDateTime(rtc, leap_day) ==
         clock_command::Result::success);
  assert(rtc.writes == 1);
  assert(sameDateTime(rtc.value, leap_day));

  const clock_command::DateTime invalid = {2023, 2, 29, 12, 0, 0};
  assert(clock_command::setDateTime(rtc, invalid) ==
         clock_command::Result::invalid_date_time);
  assert(rtc.writes == 1);
  assert(sameDateTime(rtc.value, leap_day));

  rtc.succeeds = false;
  const clock_command::DateTime write_failure = {2026, 1, 1, 0, 0, 0};
  assert(clock_command::setDateTime(rtc, write_failure) ==
         clock_command::Result::rtc_write_failed);
  assert(rtc.writes == 2);
  assert(sameDateTime(rtc.value, leap_day));
}

void test_REQ_024_025_set_time_command_uses_shared_operation() {
  const clock_command::DateTime initial = {2025, 6, 15, 10, 20, 30};
  const clock_command::DateTime requested = {2026, 11, 21, 15, 19, 45};
  FakeRtc rtc(initial);
  serial_command::State state;
  serial_command::init(&state);

  receiveLine(&state, rtc, "SET_TIME 2026-11-21 15:19:45\r");
  assert(rtc.writes == 1);
  assert(sameDateTime(rtc.value, requested));
  serial_command::resetPartialInput(&state);
  expectOutput(state, "OK SET_TIME 2026-11-21 15:19:45\n");
  assert(!serial_command::canAcceptInput(&state));

  size_t output_length = 0;
  const char* response = serial_command::pendingOutput(&state, &output_length);
  serial_command::consumeOutput(&state, output_length);
  assert(serial_command::pendingOutput(&state, &output_length) != response);
  expectOutput(state, "EVENT TIME_SET 2026-11-21 15:19:45\n");
  drainOutput(&state);
  assert(serial_command::canAcceptInput(&state));
}

void test_REQ_026_invalid_commands_do_not_change_clock() {
  const clock_command::DateTime initial = {2025, 6, 15, 10, 20, 30};
  FakeRtc rtc(initial);
  serial_command::State state;
  serial_command::init(&state);

  receiveLine(&state, rtc, "SET_TIME 2023-02-29 12:00:00");
  assert(rtc.writes == 0);
  assert(sameDateTime(rtc.value, initial));
  expectOutput(state, "ERR SET_TIME INVALID_DATE_TIME\n");
  drainOutput(&state);

  receiveLine(&state, rtc, "SET_TIME 2026-11-21 15:19");
  assert(rtc.writes == 0);
  expectOutput(state, "ERR SET_TIME INVALID_FORMAT\n");
  drainOutput(&state);

  receiveLine(&state, rtc, "SET_TIME 2026-\r11-21 15:19:45");
  assert(rtc.writes == 0);
  expectOutput(state, "ERR SET_TIME INVALID_FORMAT\n");
  drainOutput(&state);

  receiveLine(&state, rtc, "SET_ALARM 1");
  assert(rtc.writes == 0);
  expectOutput(state, "ERR ALARM INVALID_VALUE\n");
  drainOutput(&state);
}

void test_REQ_026_reports_write_failure() {
  const clock_command::DateTime initial = {2025, 6, 15, 10, 20, 30};
  FakeRtc rtc(initial);
  rtc.succeeds = false;
  serial_command::State state;
  serial_command::init(&state);

  receiveLine(&state, rtc, "SET_TIME 2026-11-21 15:19:45");
  assert(rtc.writes == 1);
  assert(sameDateTime(rtc.value, initial));
  expectOutput(state, "ERR SET_TIME RTC_WRITE_FAILED\n");
  drainOutput(&state);
}

void test_REQ_056_057_bounded_lines_and_queued_events() {
  const clock_command::DateTime initial = {2025, 6, 15, 10, 20, 30};
  FakeRtc rtc(initial);
  serial_command::State state;
  serial_command::init(&state);

  for (size_t index = 0; index < serial_command::INPUT_CAPACITY + 5; ++index) {
    serial_command::receiveByte(&state, 'X', rtc);
  }
  serial_command::receiveByte(&state, '\n', rtc);
  assert(rtc.writes == 0);
  expectOutput(state, "ERR INPUT_TOO_LONG\n");
  drainOutput(&state);

  serial_command::queueEvent(&state, "EVENT ONE\n");
  serial_command::queueEvent(&state, "EVENT TWO\n");
  assert(state.dropped_events == 1);
  expectOutput(state, "EVENT ONE\n");
  size_t length = 0;
  serial_command::pendingOutput(&state, &length);
  serial_command::consumeOutput(&state, 6);
  const char* remainder = serial_command::pendingOutput(&state, &length);
  assert(remainder != NULL);
  assert(length == strlen("ONE\n"));
  assert(memcmp(remainder, "ONE\n", length) == 0);
  drainOutput(&state);
  serial_command::queueDroppedEventSummary(&state);
  expectOutput(state, "EVENT LOG_DROPPED 1\n");
  assert(state.dropped_events == 0);
}

void test_sd_list_streams_entries_and_reports_errors() {
  FakeRtc rtc({2025, 6, 15, 10, 20, 30});
  FakeDirectory directory;
  serial_command::State state;
  serial_command::init(&state);
  receiveLine(&state, rtc, "SD_LIST /Audio Files", &directory);
  assert(strcmp(directory.path, "/Audio Files") == 0);
  expectOutput(state, "OK SD_LIST BEGIN\n");
  assert(!serial_command::canAcceptInput(&state));
  drainOutput(&state);
  serial_command::queueEvent(&state, "EVENT ONE\n");
  serial_command::advanceDirectory(&state, &directory);
  expectOutput(state, "ENTRY D 0 Music\n");
  drainOutput(&state);
  serial_command::advanceDirectory(&state, &directory);
  expectOutput(state, "ENTRY F 1234 alarm sound.wav\n");
  drainOutput(&state);
  serial_command::advanceDirectory(&state, &directory);
  expectOutput(state, "OK SD_LIST END\n");
  assert(!directory.opened);
  drainOutput(&state);

  receiveLine(&state, rtc, "SD_LIST relative", &directory);
  expectOutput(state, "ERR SD_LIST INVALID_PATH\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "SD_LIST", nullptr);
  expectOutput(state, "ERR SD_LIST UNAVAILABLE\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "SD_LIST", &directory);
  drainOutput(&state);
  directory.fail_read = true;
  serial_command::advanceDirectory(&state, &directory);
  expectOutput(state, "ERR SD_LIST READ\n");
  assert(!directory.opened);
  drainOutput(&state);
  directory.fail_read = false;
  receiveLine(&state, rtc, "SD_LIST", &directory);
  serial_command::abortDirectory(&state, &directory);
  assert(!directory.opened && serial_command::canAcceptInput(&state));
}

void test_sd_read() {
  FakeRtc rtc({2025, 6, 15, 10, 20, 30});
  FakeDirectory directory;
  serial_command::State state;
  serial_command::init(&state);
  receiveLine(&state, rtc, "SD_READ /alarm_settings.txt", &directory);
  expectOutput(state, "OK SD_READ BEGIN\n");
  drainOutput(&state);
  serial_command::advanceDirectory(&state, &directory);
  expectOutput(state, "DATA 616263\n");
  drainOutput(&state);
  serial_command::advanceDirectory(&state, &directory);
  expectOutput(state, "OK SD_READ END\n");
  drainOutput(&state);
}

void test_req_025_serial_display_uses_shared_persistent_command() {
  FakeRtc rtc({2025, 6, 15, 10, 20, 30});
  FakeSettingsStorage storage;
  settings::Store store(storage);
  FakeDisplaySettings display(store);
  assert(store.load(&display.value) == settings::Result::blank);
  serial_command::State state;
  serial_command::init(&state);
  receiveLine(&state, rtc, "SET_DISPLAY FORMAT 12", nullptr, &display);
  expectOutput(state, "OK SET_DISPLAY\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "SET_DISPLAY BRIGHTNESS 7", nullptr, &display);
  expectOutput(state, "OK SET_DISPLAY\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "GET_DISPLAY", nullptr, &display);
  expectOutput(state, "OK DISPLAY FORMAT 12 BRIGHTNESS 7 WAKE_SECONDS 30\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "SET_DISPLAY BRIGHTNESS 16", nullptr, &display);
  expectOutput(state, "ERR DISPLAY INVALID_VALUE\n");
  drainOutput(&state);
  settings::Store restarted(storage);
  settings::DisplaySettings restored = {};
  assert(restarted.load(&restored) == settings::Result::ok);
  assert(restored.use_12_hour && restored.brightness == 7);
  storage.fail = true;
  receiveLine(&state, rtc, "SET_DISPLAY FORMAT 24", nullptr, &display);
  expectOutput(state, "ERR DISPLAY STORAGE\n");
  assert(display.value.use_12_hour && display.value.brightness == 7);
}

void test_req_005_serial_dark_settings() {
  FakeRtc rtc({2025, 6, 15, 10, 20, 30});
  FakeSettingsStorage storage;
  settings::Store store(storage);
  FakeDisplaySettings display(store);
  assert(store.load(&display.value) == settings::Result::blank);
  serial_command::State state;
  serial_command::init(&state);
  receiveLine(&state, rtc, "SET_DARK START 23:15", nullptr, &display);
  expectOutput(state, "OK SET_DARK\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "SET_DARK END 07:10", nullptr, &display);
  expectOutput(state, "OK SET_DARK\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "SET_DARK ENABLED 1", nullptr, &display);
  expectOutput(state, "OK SET_DARK\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "GET_DARK", nullptr, &display);
  expectOutput(state, "OK DARK ENABLED 1 START 23:15 END 07:10 BLANK 0\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "SET_DARK START 24:00", nullptr, &display);
  expectOutput(state, "ERR DARK INVALID_VALUE\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "SET_DARK END 07:60", nullptr, &display);
  expectOutput(state, "ERR DARK INVALID_VALUE\n");
  drainOutput(&state);
  settings::Store restarted(storage);
  settings::DisplaySettings restored = {};
  assert(restarted.load(&restored) == settings::Result::ok);
  assert(restored.dark_enabled && restored.dark_start == 1395 &&
         restored.dark_end == 430 && restored.brightness == 3);
  storage.fail = true;
  receiveLine(&state, rtc, "SET_DARK ENABLED 0", nullptr, &display);
  expectOutput(state, "ERR DARK STORAGE\n");
  assert(display.value.dark_enabled);
}

void test_req_042_serial_encoder_commands() {
  FakeRtc rtc({2025, 6, 15, 10, 20, 30});
  FakeEncoder encoder;
  serial_command::State state;
  serial_command::init(&state);
  receiveLine(&state, rtc, "ENCODER LEFT", nullptr, nullptr, &encoder);
  expectOutput(state, "OK ENCODER\n");
  assert(encoder.last_event == MENU_EVENT_ROTATE && encoder.last_delta == -1);
  drainOutput(&state);
  receiveLine(&state, rtc, "ENCODER RIGHT", nullptr, nullptr, &encoder);
  expectOutput(state, "OK ENCODER\n");
  assert(encoder.last_event == MENU_EVENT_ROTATE && encoder.last_delta == 1);
  drainOutput(&state);
  receiveLine(&state, rtc, "ENCODER CLICK", nullptr, nullptr, &encoder);
  expectOutput(state, "OK ENCODER\n");
  assert(encoder.last_event == MENU_EVENT_SELECT && encoder.last_delta == 0);
  drainOutput(&state);
  receiveLine(&state, rtc, "ENCODER RIGHT 4", nullptr, nullptr, &encoder);
  expectOutput(state, "ERR ENCODER INVALID_VALUE\n");
}

// REQ-044 and REQ-058: serial reports all commanded pixels and enforces
// the final output scaling, including mixed-channel requests.
void test_led_serial_control() {
  FakeRtc rtc({2025, 6, 15, 10, 20, 30});
  FakeLeds leds;
  serial_command::State state;
  serial_command::init(&state);
  receiveLine(&state, rtc, "SET_LED 7 7 16 32 0 208", nullptr, nullptr, nullptr,
              &leds);
  expectOutput(state, "OK SET_LED\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "GET_LEDS", nullptr, nullptr, nullptr, &leds);
  char expected[sizeof("OK LEDS ") - 1 + lighting::PIXEL_COUNT * 8 + 2];
  memcpy(expected, "OK LEDS ", 8);
  memset(expected + 8, '0', lighting::PIXEL_COUNT * 8);
  memcpy(expected + 8 + 63 * 8, "0102000D", 8);
  expected[8 + lighting::PIXEL_COUNT * 8] = '\n';
  expected[9 + lighting::PIXEL_COUNT * 8] = '\0';
  expectOutput(state, expected);
  drainOutput(&state);
  const char* invalid[] = {"SET_LED 8 0 1 0 0 0", "SET_LED 0 8 1 0 0 0",
                           "SET_LED -1 0 1 0 0 0",
                           "SET_LED 0 0 256 0 0 0",
                           "SET_LED 0 0 1 0 0 0 extra",
                           "SET_LED 0  0 1 0 0 0"};
  for (const char* command : invalid) {
    receiveLine(&state, rtc, command, nullptr, nullptr, nullptr, &leds);
    expectOutput(state, "ERR LEDS INVALID_VALUE\n");
    drainOutput(&state);
  }
  assert(leds.frame.pixels[63].red == 1 && leds.frame.pixels[63].green == 2);
  receiveLine(&state, rtc, "SET_LED 0 0 255 255 255 255", nullptr, nullptr, nullptr,
              &leds);
  expectOutput(state, "OK SET_LED\n");
  assert(leds.logical.pixels[0].red == 255);
  assert(leds.frame.pixels[0].red == 15);
  assert(leds.frame.pixels[63].red == 1);
  drainOutput(&state);
  receiveLine(&state, rtc, "CLEAR_LEDS", nullptr, nullptr, nullptr, &leds);
  expectOutput(state, "OK CLEAR_LEDS\n");
  assert(leds.frame.pixels[0].red == 0 && leds.frame.pixels[63].red == 0);
  drainOutput(&state);
  receiveLine(&state, rtc, "GET_LEDS");
  expectOutput(state, "ERR LEDS UNAVAILABLE\n");
}

void test_bootloader_waits_for_response_and_rejects_extra_input() {
  FakeRtc rtc({2025, 6, 15, 10, 20, 30});
  serial_command::State state;
  serial_command::init(&state);

  receiveLine(&state, rtc, "BOOTLOADER EXTRA");
  expectOutput(state, "ERR UNKNOWN_COMMAND\n");
  drainOutput(&state);
  assert(!serial_command::bootloaderReady(&state));

  receiveLine(&state, rtc, "BOOTLOADER");
  expectOutput(state, "OK BOOTLOADER\n");
  assert(!serial_command::bootloaderReady(&state));
  assert(!serial_command::canAcceptInput(&state));
  serial_command::consumeOutput(&state, 3);
  assert(!serial_command::bootloaderReady(&state));
  drainOutput(&state);
  assert(serial_command::bootloaderReady(&state));
  assert(!serial_command::canAcceptInput(&state));
  serial_command::cancelBootloaderRequest(&state);
  assert(serial_command::bootloaderReady(&state));

  serial_command::init(&state);
  receiveLine(&state, rtc, "BOOTLOADER");
  serial_command::cancelBootloaderRequest(&state);
  assert(!serial_command::bootloaderReady(&state));
  assert(serial_command::canAcceptInput(&state));
}

static void test_light_serial_validation_and_state() {
  FakeRtc rtc({2025, 6, 15, 10, 20, 30});
  FakeLeds leds;
  serial_command::State state;
  serial_command::init(&state);
  receiveLine(&state, rtc, "START_LIGHT SOLID 255 16 32 64 255 1 5",
              nullptr, nullptr, nullptr, &leds);
  expectOutput(state, "OK START_LIGHT\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "GET_LIGHT", nullptr, nullptr, nullptr, &leds);
  expectOutput(state, "OK LIGHT RUNNING SOLID 255 16 32 64 255 1 5\n");
  drainOutput(&state);
  const char* invalid[] = {
      "START_LIGHT SUNRISE 255 0 0 0 255 2 0",
      "START_LIGHT FOREST 255 0 0 0 255 1 2",
      "START_LIGHT FOREST 255 0 0 0 255 0 0 1",
      "START_LIGHT SOLID 256 0 0 0 255 0 0",
      "START_LIGHT UNKNOWN 0 0 0 0 255 0 0",
      "START_LIGHT SOLID 0 0 0 0 255 0 0 extra"};
  for (const char* command : invalid) {
    receiveLine(&state, rtc, command, nullptr, nullptr, nullptr, &leds);
    expectOutput(state, "ERR LIGHT INVALID_VALUE\n");
    drainOutput(&state);
  }
  assert(leds.config.effect == lighting::Effect::solid);
  assert(leds.config.fade_out_minutes == 5);
  const char embedded_null[] = {'S', 'T', 'O', 'P', '_', 'L', 'I', 'G', 'H',
                                'T', '\0', 'X', '\n'};
  for (char value : embedded_null) {
    serial_command::receiveByte(&state, value, rtc, nullptr, nullptr, nullptr,
                                &leds);
  }
  expectOutput(state, "ERR UNKNOWN_COMMAND\n");
  assert(leds.player.state() == lighting::RunState::running);
  drainOutput(&state);
  receiveLine(&state, rtc, "STOP_LIGHT", nullptr, nullptr, nullptr, &leds);
  expectOutput(state, "OK STOP_LIGHT\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "GET_LIGHT", nullptr, nullptr, nullptr, &leds);
  expectOutput(state, "OK LIGHT FADING SOLID 255 16 32 64 255 1 5\n");
}

static void test_alarm_serial_settings_actions_and_queries() {
  FakeRtc rtc({2026, 11, 23, 6, 0, 0});
  FakeSettingsStorage memory;
  alarms::Store store(memory);
  assert(store.load() == settings::Result::blank);
  FakeAlarms alarms(store);
  serial_command::State state;
  serial_command::init(&state);
  auto send = [&](const char* command) {
    receiveLine(&state, rtc, command, nullptr, nullptr, nullptr, nullptr, &alarms);
  };
  const char* commands[] = {
      "SET_ALARM 1 TIME 08:15", "SET_ALARM 1 DAYS 34",
      "SET_ALARM 1 ONESHOT 1", "SET_ALARM 1 EFFECT SOLID",
      "SET_ALARM 1 COLOR 255 10 20 30", "SET_ALARM 1 BRIGHTNESS 128",
      "SET_ALARM 1 FADE_IN 0", "SET_ALARM 1 FADE_OUT 5",
      "SET_ALARM 1 RUN 10", "SET_ALARM 1 ENABLED 1"};
  for (const char* command : commands) {
    send(command);
    expectOutput(state, "OK SET_ALARM 1\n");
    drainOutput(&state);
  }
  send("GET_ALARM 1");
  expectOutput(state, "OK ALARM 1 ENABLED 1 ONESHOT 1 TIME 08:15 DAYS 34 "
      "EFFECT SOLID COLOR 255 10 20 30 BRIGHTNESS 128 FADE_IN 0 FADE_OUT 5 "
      "HOLD 5 RUN 10 STATE OFF REMAINING 0 VOLUME 5 AUDIO_FADE_IN 0 "
      "AUDIO_FADE_OUT 5 AUDIO OFF AUDIO_RESULT NONE SOUND NONE\n");
  drainOutput(&state);
  const char* bad[] = {"START_ALARM 0", "GET_ALARM 6", "SET_ALARM 1 DAYS 0",
      "SET_ALARM 1 COLOR 256 0 0 0", "SET_ALARM 1 TIME 25:00",
      "SET_ALARM 1 FADE_IN 2", "SET_ALARM 1 RUN 4",
      "SET_ALARM 1 ENABLED 2", "SET_ALARM 1 RUN 10 extra", "SET_ALARM 1 GREEN 1",
      "SET_ALARM 1 VOLUME 51", "SET_ALARM 1 AUDIO_FADE_IN 2",
      "SET_ALARM 1 HOLD 181", "SET_ALARM 1 SOUND relative.wav",
      "SET_ALARM 1 SOUND "};
  for (const char* command : bad) {
    send(command);
    expectOutput(state, "ERR ALARM INVALID_VALUE\n");
    drainOutput(&state);
  }
  assert(alarms.get(0).days == 34 && alarms.get(0).hold_minutes == 5);
  send("SET_ALARM 1 HOLD 10");
  expectOutput(state, "OK SET_ALARM 1\n");
  assert(alarms.get(0).hold_minutes == 10);
  drainOutput(&state);
  send("START_ALARM 1");
  expectOutput(state, "OK START_ALARM 1\n");
  assert(alarms.status().phase == alarms::Phase::ringing);
  assert(alarms.get(0).enabled);  // Manual tests do not consume a one-shot.
  drainOutput(&state);
  send("SET_ALARM 1 SOUND /morning song.wav");
  expectOutput(state, "OK SET_ALARM 1\n");
  assert(strcmp(alarms.get(0).sound.path, "/morning song.wav") == 0);
  drainOutput(&state);
  send("SET_ALARM 1 VOLUME 10");
  expectOutput(state, "OK SET_ALARM 1\n");
  assert(alarms.get(0).sound.volume == 10);
  drainOutput(&state);
  send("HALF_ALARM");
  expectOutput(state, "OK HALF_ALARM\n");
  drainOutput(&state);
  send("SNOOZE_ALARM");
  expectOutput(state, "OK SNOOZE_ALARM\n");
  assert(alarms.status().phase == alarms::Phase::snoozed);
  drainOutput(&state);
  send("DISMISS_ALARM");
  expectOutput(state, "OK DISMISS_ALARM\n");
  drainOutput(&state);
  send("DISMISS_ALARM");
  expectOutput(state, "ERR ALARM NO_ACTIVE\n");
  drainOutput(&state);
  send("GET_TIME");
  expectOutput(state, "OK TIME 2026-11-23 06:00:00\n");
  drainOutput(&state);
  memory.fail = true;
  send("SET_ALARM 1 ENABLED 0");
  expectOutput(state, "ERR ALARM STORAGE\n");
  assert(alarms.get(0).enabled);
  drainOutput(&state);
  send("GET_ALARM 1");
  expectOutput(state, "ERR ALARM STORAGE\n");
}

static void test_read_screen_and_invalid_time() {
  FakeSettingsStorage memory;
  settings::Store store(memory);
  FakeDisplaySettings display(store);
  FakeRtc rtc({1999, 1, 1, 0, 0, 0});
  serial_command::State state;
  serial_command::init(&state);
  receiveLine(&state, rtc, "GET_SCREEN", nullptr, &display);
  expectOutput(state, "OK SCREEN 1234\n");
  drainOutput(&state);
  receiveLine(&state, rtc, "GET_TIME");
  expectOutput(state, "ERR TIME INVALID\n");
}

class FakeAudio : public audio::Control {
public:
  audio::Request last;
  audio::Status current;
  audio::Error result = audio::Error::none;
  uint32_t count = 0;
  audio::Error submit(const audio::Request& request, uint32_t& id) override {
    if (result != audio::Error::none) return result;
    last = request;
    id = ++count;
    return audio::Error::none;
  }
  audio::Status status() const override { return current; }
};

static void test_audio_commands_cap_paths_and_async_status() {
  FakeRtc rtc({2026, 11, 22, 12, 0, 0});
  FakeAudio audio;
  serial_command::State state;
  serial_command::init(&state);
  auto send = [&](const char* text) {
    receiveLine(&state, rtc, text, nullptr, nullptr, nullptr, nullptr, nullptr, &audio);
  };
  send("START_AUDIO 5 /sounds/quiet morning.wav");
  expectOutput(state, "OK START_AUDIO QUEUED 1\n");
  assert(audio.last.volume == 5);
  assert(strcmp(audio.last.path, "/sounds/quiet morning.wav") == 0);
  drainOutput(&state);
  const char* invalid[] = {"START_AUDIO 51 /test.wav", "SET_AUDIO_VOLUME 51",
                         "START_AUDIO 5 relative.wav", "START_AUDIO 5 ",
                         "SET_AUDIO_VOLUME 5 extra", "START_AUDIO -1 /test.wav"};
  for (const char* bad : invalid) {
    send(bad);
    expectOutput(state, "ERR AUDIO INVALID_VALUE\n");
    drainOutput(&state);
  }
  assert(audio.count == 1);
  send("AUDIO_FIXTURE 96000 32 2 5 /BNTEST1234abcd");
  expectOutput(state, "OK AUDIO_FIXTURE QUEUED 2\n");
  assert(audio.last.action == audio::Action::fixture && audio.last.rate == 96000);
  assert(audio.last.bits == 32 && audio.last.channels == 2 && audio.last.seconds == 5);
  drainOutput(&state);
  send("AUDIO_CLEAN /Ooblets_32.wav");
  expectOutput(state, "ERR AUDIO INVALID_VALUE\n");
  drainOutput(&state);
  send("AUDIO_SPI 50");
  expectOutput(state, "ERR AUDIO INVALID_VALUE\n");
  drainOutput(&state);
  send("AUDIO_SPI 30");
  expectOutput(state, "ERR AUDIO INVALID_VALUE\n");
  drainOutput(&state);
  send("SET_AUDIO_VOLUME 0");
  expectOutput(state, "OK SET_AUDIO_VOLUME QUEUED 3\n");
  assert(audio.last.action == audio::Action::volume && audio.last.volume == 0);
  drainOutput(&state);
  audio.result = audio::Error::busy;
  send("STOP_AUDIO");
  expectOutput(state, "ERR AUDIO BUSY\n");
  drainOutput(&state);
  send("GET_AUDIO");
  expectOutput(state, "ERR AUDIO UNAVAILABLE\n");
  drainOutput(&state);
  audio.current.core = 1;
  audio.current.result = audio::Error::file;
  audio.current.state = audio::State::error;
  send("GET_AUDIO");
  expectOutput(state, "OK AUDIO CORE 1 IRQ_CORE 255 STATE ERROR REQUEST 0 RESULT FILE VOLUME 0 LIMIT 50 "
      "RATE 0 CHANNELS 0 BITS 0 FRAMES 0 TOTAL 0 BYTES 0 UNDERRUNS 0 PEAK 0 BLOCK_PEAK 0 "
      "GAIN_BPS 0 ALARM 0 SLEEP 0 READ_US 0 WAIT_US 0 MAX_READ_US 0 SPI_MHZ 10 PATH \n");
  drainOutput(&state);
  send("SET_AUDIO_VOLUME 50");
  expectOutput(state, "ERR AUDIO BUSY\n");
  drainOutput(&state);
  audio.result = audio::Error::none;
  send("SET_AUDIO_VOLUME 50");
  expectOutput(state, "OK SET_AUDIO_VOLUME QUEUED 4\n");
  assert(audio.last.action == audio::Action::volume && audio.last.volume == 50);
}

class FakeSleep : public sleep_timer::Control {
public:
  sleep_timer::Engine engine;
  sleep_timer::Config slots[3] = {sleep_timer::slotDefaults(0),
      sleep_timer::slotDefaults(1), sleep_timer::slotDefaults(2)};
  bool alarm_active = false;
  bool ready = true;
  sleep_timer::Config get() const override { return engine.config(); }
  sleep_timer::Status status() const override { return engine.status(1000); }
  sleep_timer::Result set(const sleep_timer::Config& config) override { return engine.set(config); }
  sleep_timer::Result start(uint8_t slot = sleep_timer::NO_SLOT) override {
    if (alarm_active) return sleep_timer::Result::busy;
    if (slot != sleep_timer::NO_SLOT) {
      const sleep_timer::Result result = loadSlot(slot);
      if (result != sleep_timer::Result::ok) return result;
    }
    return engine.start(1000, false, slot);
  }
  sleep_timer::Result stop() override { engine.stop(); return sleep_timer::Result::ok; }
  bool available() const override { return ready; }
  sleep_timer::Config getSlot(uint8_t slot) const override { return slots[slot]; }
  sleep_timer::Result loadSlot(uint8_t slot) override {
    if (!ready) return sleep_timer::Result::storage;
    return engine.set(slots[slot]);
  }
  sleep_timer::Result saveSlot(uint8_t slot, const sleep_timer::Config& config) override {
    if (!ready) return sleep_timer::Result::storage;
    if (!sleep_timer::valid(config)) return sleep_timer::Result::invalid_value;
    slots[slot] = config;
    return engine.set(config);
  }
};

void test_req_015_016_040_sleep_serial_configuration_status_and_busy() {
  FakeRtc rtc({2026, 11, 21, 12, 0, 0});
  FakeSleep sleep;
  serial_command::State state;
  serial_command::init(&state);
  auto send = [&](const char* text) {
    receiveLine(&state, rtc, text, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, &sleep);
  };
  send("GET_SLEEP");
  expectOutput(state, "OK SLEEP SLOT 0 ACTIVE_SLOT 0 STATE OFF REMAINING 0 ELAPSED_MS 0 ACTIVE_LIGHT 0 ACTIVE_AUDIO 0 "
      "HOLD 30 TOTAL_SECONDS 2100 OUTPUT LIGHT EFFECT BEACH COLOR 255 180 64 255 BRIGHTNESS 128 "
      "FADE_IN_SECONDS 30 FADE_OUT 5 VOLUME 5 AUDIO_FADE_IN_SECONDS 30 AUDIO_FADE_OUT 5 "
      "AUDIO OFF AUDIO_RESULT NONE SOUND NONE\n");
  drainOutput(&state);
  const char* invalid[] = {"SET_SLEEP HOLD 181",
      "SET_SLEEP BRIGHTNESS 256", "SET_SLEEP VOLUME 51", "SET_SLEEP OUTPUT AUDIO",
      "SET_SLEEP OUTPUT NONE", "SET_SLEEP EFFECT unknown",
      "SET_SLEEP SOUND relative.wav",
      "SET_SLEEP HOLD 10 extra", "SET_SLEEP GREEN 1"};
  for (const char* text : invalid) {
    send(text);
    expectOutput(state, "ERR SLEEP INVALID_VALUE\n");
    drainOutput(&state);
  }
  assert(sleep.get().hold_minutes == 30 && sleep.get().sound.path[0] == '\0');
  send("SET_SLEEP AUDIO_FADE_IN 5");
  expectOutput(state, "ERR SLEEP FIXED_FADE_IN\n");
  assert(sleep.get().light.fade_in_minutes == 0);
  drainOutput(&state);
  send("SET_SLEEP FADE_IN 0");
  expectOutput(state, "ERR SLEEP FIXED_FADE_IN\n");
  drainOutput(&state);
  send("SET_SLEEP DURATION 30");
  expectOutput(state, "ERR SLEEP USE_HOLD\n");
  drainOutput(&state);
  const char* valid[] = {"SET_SLEEP FADE_OUT 0", "SET_SLEEP HOLD 1",
      "SET_SLEEP AUDIO_FADE_OUT 1", "SET_SLEEP SOUND /Quiet Night.wav",
      "SET_SLEEP OUTPUT BOTH", "SET_SLEEP EFFECT NEBULA", "SET_SLEEP VOLUME 10"};
  for (const char* text : valid) {
    send(text);
    expectOutput(state, "OK SET_SLEEP\n");
    drainOutput(&state);
  }
  assert(sleep.get().light.fade_out_minutes == 1);
  sleep.alarm_active = true;
  send("START_SLEEP");
  expectOutput(state, "ERR SLEEP BUSY\n");
  drainOutput(&state);
  sleep.alarm_active = false;
  send("START_SLEEP");
  expectOutput(state, "OK START_SLEEP\n");
  drainOutput(&state);
  assert(sleep.status().remaining_seconds == 120 && sleep.status().audio_enabled);
  send("SET_SLEEP SOUND NONE");
  expectOutput(state, "ERR SLEEP INVALID_VALUE\n");
  drainOutput(&state);
  send("STOP_SLEEP");
  expectOutput(state, "OK STOP_SLEEP\n");
  assert(!sleep.status().active);
  drainOutput(&state);
  send("SAVE_SLEEP 2");
  expectOutput(state, "OK SAVE_SLEEP 2\n");
  drainOutput(&state);
  send("SET_SLEEP 2 VOLUME 3");
  expectOutput(state, "OK SET_SLEEP 2\n");
  assert(sleep.slots[1].sound.volume == 3 && sleep.slots[0].sound.volume == 5);
  drainOutput(&state);
  send("LOAD_SLEEP 2");
  expectOutput(state, "OK LOAD_SLEEP 2\n");
  drainOutput(&state);
  send("START_SLEEP 2");
  expectOutput(state, "OK START_SLEEP 2\n");
  assert(sleep.status().slot == 1);
  drainOutput(&state);
  const char* invalid_slots[] = {"GET_SLEEP 0", "START_SLEEP 4", "LOAD_SLEEP 10",
      "SAVE_SLEEP 2 extra", "SET_SLEEP 9 VOLUME 2"};
  for (const char* text : invalid_slots) {
    send(text);
    expectOutput(state, "ERR SLEEP INVALID_VALUE\n");
    drainOutput(&state);
  }
  sleep.ready = false;
  send("SAVE_SLEEP 1");
  expectOutput(state, "ERR SLEEP STORAGE\n");
}

}

static void test_tap_filter_and_serial_wake_setting() {
  tap_input::State filtered;
  const int16_t resting[] = {0, 0, 8000};
  const int16_t diagonal[] = {800, 800, 8000};
  const int16_t z_tap[] = {0, 0, 9280};
  assert(!tap_input::update(filtered, resting, UINT32_MAX - 2000));
  assert(!tap_input::update(filtered, diagonal, UINT32_MAX - 210));
  assert(tap_input::update(filtered, diagonal, UINT32_MAX - 205));
  assert(!tap_input::update(filtered, diagonal, UINT32_MAX - 200));
  assert(!tap_input::update(filtered, resting, UINT32_MAX - 195));
  assert(!tap_input::update(filtered, resting, UINT32_MAX - 190));
  assert(!tap_input::update(filtered, z_tap, 100));
  assert(!tap_input::update(filtered, z_tap, 105));
  assert(!tap_input::update(filtered, resting, 110));
  assert(!tap_input::update(filtered, resting, 115));
  assert(!tap_input::update(filtered, z_tap, 189));
  assert(tap_input::update(filtered, z_tap, 194));
  assert(filtered.count == 2);
  tap_input::State stationary;
  const int16_t tilted[] = {3200, -2400, 7200};
  assert(!tap_input::update(stationary, tilted, 0));
  for (uint32_t t = 5; t < 3000; t += 5) {
    const int16_t noisy[] = {static_cast<int16_t>(3200 + (t % 2 ? 16 : -16)),
                            -2400, 7200};
    assert(!tap_input::update(stationary, noisy, t));
  }
  assert(stationary.count == 0);
  tap_input::State extreme;
  const int16_t minimum[] = {-32768, -32768, -32768};
  const int16_t maximum[] = {32767, 32767, 32767};
  assert(!tap_input::update(extreme, minimum, 0));
  assert(!tap_input::update(extreme, maximum, 1500));
  assert(tap_input::update(extreme, maximum, 1505));
  assert(extreme.peak_mg == 14188);
  tap_input::State spike;
  const int16_t impulse[] = {960, 0, 8000};
  assert(!tap_input::update(spike, resting, 0));
  assert(!tap_input::update(spike, resting, 1001));
  assert(!tap_input::update(spike, impulse, 1010));
  assert(!tap_input::update(spike, resting, 1015));
  assert(!tap_input::update(spike, resting, 1020));
  assert(spike.count == 0 && spike.peak_mg == 120);
  tap_input::State bump;
  const int16_t small_bump[] = {800, 0, 8000};
  assert(!tap_input::update(bump, resting, 0));
  assert(!tap_input::update(bump, resting, 1000));
  assert(!tap_input::update(bump, small_bump, 1010));
  assert(!tap_input::update(bump, small_bump, 1015));
  assert(bump.count == 0 && bump.filtered_peak_mg >= 90);
  FakeRtc rtc({2026, 11, 22, 18, 0, 0});
  FakeEncoder encoder;
  serial_command::State state;
  serial_command::init(&state);
  receiveLine(&state, rtc, "TAP", nullptr, nullptr, &encoder);
  expectOutput(state, "OK TAP\n");
  assert(encoder.taps == 1 && encoder.last_event == MENU_EVENT_NONE);
  drainOutput(&state);
  receiveLine(&state, rtc, "TAP extra", nullptr, nullptr, &encoder);
  expectOutput(state, "ERR UNKNOWN_COMMAND\n");
  assert(encoder.taps == 1);
  drainOutput(&state);
  receiveLine(&state, rtc, "SET_TAP_THRESHOLD 20", nullptr, nullptr, &encoder);
  expectOutput(state, "OK SET_TAP_THRESHOLD\n");
  assert(encoder.tap_threshold == 20);
  drainOutput(&state);
  receiveLine(&state, rtc, "SET_TAP_THRESHOLD 9", nullptr, nullptr, &encoder);
  expectOutput(state, "ERR TAP INVALID_VALUE\n");
  assert(encoder.tap_threshold == 20);
  drainOutput(&state);
  FakeSettingsStorage storage;
  settings::Store store(storage);
  FakeDisplaySettings display(store);
  assert(store.load(&display.value) == settings::Result::blank);
  receiveLine(&state, rtc, "SET_DISPLAY WAKE_SECONDS 300", nullptr, &display);
  expectOutput(state, "OK SET_DISPLAY\n");
  assert(display.value.wake_seconds == 300);
  drainOutput(&state);
  receiveLine(&state, rtc, "SET_DISPLAY WAKE_SECONDS 0", nullptr, &display);
  expectOutput(state, "ERR DISPLAY INVALID_VALUE\n");
  assert(display.value.wake_seconds == 300);
}

int main() {
  test_tap_filter_and_serial_wake_setting();
  test_req_015_016_040_sleep_serial_configuration_status_and_busy();
  test_REQ_002_clock_command_validation();
  test_REQ_024_025_set_time_command_uses_shared_operation();
  test_REQ_026_invalid_commands_do_not_change_clock();
  test_REQ_026_reports_write_failure();
  test_REQ_056_057_bounded_lines_and_queued_events();
  test_sd_list_streams_entries_and_reports_errors();
  test_sd_read();
  test_req_025_serial_display_uses_shared_persistent_command();
  test_req_005_serial_dark_settings();
  test_req_042_serial_encoder_commands();
  test_led_serial_control();
  test_bootloader_waits_for_response_and_rejects_extra_input();
  test_light_serial_validation_and_state();
  test_alarm_serial_settings_actions_and_queries();
  test_read_screen_and_invalid_time();
  test_audio_commands_cap_paths_and_async_status();
}
