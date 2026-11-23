// ===================== Libraries + Includes =====================
// Package/lib Includes

#include <gamma.h>
#include <Wire.h>
#include <Adafruit_NeoMatrix.h>
#include <atomic>

// User Includes
#include "Defines.hpp"
#include "Audio_Playback.hpp"
#include "Audio_Service_RP2040.hpp"
#include "Display_Driver.hpp"
#include "File_Management.hpp"
#include "Settings_Storage_SD.hpp"
#include "Settings_Command.hpp"
#include "Input_Processing.hpp"
#include "Matrix_Driver.hpp"
#include "Menu_System.hpp"
#include "Serial_Diagnostics.hpp"
#include "Time_Keeping.hpp"

// ===================== Global State Objects =====================

// Time Keeping System
TimeKeeper time_keeper;

// Input System
InputState inputs;
InputGestures gestures;

// Menu System
MenuState menu;

// Display System
DisplayState display;

// SD Card - Settings
SettingsStorageSD settings_storage;
settings::Store settings_store(settings_storage);
AlarmStorageSD alarm_storage(settings_storage);
alarms::Store alarm_store(alarm_storage);
alarms::Engine alarm_engine(alarm_store);
uint32_t wall_time = 0;
bool wall_valid = false;
alarms::AudioBridge alarm_audio;
sleep_timer::Engine sleep_engine;
SleepStorageSD sleep_storage(settings_storage);
sleep_timer::Store sleep_store(sleep_storage);

class AudioAdapter : public audio::Control {
public:
  audio::Error submit(const audio::Request& request, uint32_t& id) override {
    if (!request.alarm) {
      const bool alarm_owns = alarm_engine.active() || alarm_audio.pending();
      const bool live_volume = request.action == audio::Action::volume &&
          !alarm_audio.pending() && !sleep_engine.pending();
      if ((request.sleep && alarm_owns) ||
          (!request.sleep && request.action != audio::Action::stop &&
           !live_volume &&
           (alarm_owns || sleep_engine.active() || sleep_engine.pending()))) {
        return audio::Error::busy;
      }
    }
    const audio::Error result = audio::submit(request, id);
    if (result == audio::Error::none && !request.alarm && !request.sleep &&
        request.action == audio::Action::stop) {
      alarm_audio.silence(alarm_engine, static_cast<uint32_t>(millis()));
      sleep_engine.silence();
    }
    return result;
  }
  audio::Status status() const override { return audio::snapshot(); }
};

AudioAdapter audio_adapter;

class AlarmAdapter : public serial_command::AlarmControl {
public:
  bool available() const override { return alarm_store.ready(); }
  alarms::Config get(uint8_t slot) const override { return alarm_engine.get(slot); }
  alarms::Status status() const override {
    return alarm_engine.status(static_cast<uint32_t>(millis()));
  }
  alarms::Result set(uint8_t slot, const alarms::Config& config) override {
    return alarm_engine.set(slot, config, wall_time);
  }
  alarms::Result start(uint8_t slot) override {
    return alarm_engine.start(slot, wall_time, static_cast<uint32_t>(millis()));
  }
  alarms::Result snooze() override {
    const alarms::Result result = alarm_engine.snooze(static_cast<uint32_t>(millis()));
    if (result == alarms::Result::ok) menu.screen = MENU_SCREEN_CLOCK;
    return result;
  }
  alarms::Result dismiss() override { return alarm_engine.dismiss(); }
  alarms::Result reduce() override { return alarm_engine.reduce(); }
};

AlarmAdapter alarm_adapter;

class DisplaySettingsAdapter : public serial_command::DisplaySettingsControl {
public:
  settings::DisplaySettings get() const override {
    return menu.display_settings;
  }

  bool clockBlanked() const override {
    return display.clock_blanked;
  }
  const char* screenText() const override { return display.screen_text; }

  settings_command::Result set(
      const settings::DisplaySettings& candidate) override {
    return settings_command::setDisplay(settings_store, &menu.display_settings,
                                        candidate);
  }
};

DisplaySettingsAdapter display_settings_adapter;

bool handle_MenuInput(MenuEvent event, int32_t delta);
bool handle_Tap(bool physical = false);

class EncoderAdapter : public serial_command::EncoderControl {
public:
  bool send(MenuEvent event, int32_t delta) override {
    return handle_MenuInput(event, delta);
  }
  bool tap() override { return handle_Tap(); }
  tap_input::Status tapStatus() const override { return inputs.tap_status; }
  bool setTapThreshold(uint16_t threshold) override {
    if (!inputs.tap_status.available || threshold < 10 || threshold > 500)
      return false;
    inputs.tap_status.threshold_mg = threshold;
    inputs.tap_status.state.peak_mg = 0;
    inputs.tap_status.state.filtered_peak_mg = 0;
    return true;
  }
};

EncoderAdapter encoder_adapter;

// LED Matrix
class LedAdapter : public serial_command::LedControl {
public:
  bool begin() {
    if (matrix.getPixels() == nullptr || !matrix.begin()) return false;
    matrix.clear();
    matrix.show();
    ready = true;
    return true;
  }

  serial_command::LedResult setPixel(uint8_t row, uint8_t column,
                                     lighting::Color color) override {
    if (alarm_engine.active() || sleep_engine.active()) return serial_command::LedResult::busy;
    if (!ready) return serial_command::LedResult::unavailable;
    lighting::Frame candidate = logical_frame;
    if (!lighting::setPixelFrame(row, column, color, candidate)) {
      return serial_command::LedResult::invalid_value;
    }
    player.cancel();
    return setFrame(candidate);
  }

  serial_command::LedResult clear() override {
    if (alarm_engine.active() || sleep_engine.active()) return serial_command::LedResult::busy;
    if (!ready) return serial_command::LedResult::unavailable;
    player.cancel();
    return setFrame(lighting::Frame{});
  }

  serial_command::LedResult startEffect(
      const lighting::AnimationConfig& config) override {
    if (alarm_engine.active() || sleep_engine.active()) return serial_command::LedResult::busy;
    if (!ready) return serial_command::LedResult::unavailable;
    const uint32_t now = static_cast<uint32_t>(millis());
    if (!player.start(config, now)) return serial_command::LedResult::invalid_value;
    lighting::Frame candidate = {};
    if (!player.frame(now, candidate)) {
      player.cancel();
      return serial_command::LedResult::invalid_value;
    }
    const serial_command::LedResult result = setFrame(candidate);
    if (result == serial_command::LedResult::ok) {
      menu.light_config = config;
      menu.light_applied_config = config;
      last_effect_update = now;
    } else {
      player.cancel();
    }
    return result;
  }

  serial_command::LedResult stopEffect() override {
    if (alarm_engine.active() || sleep_engine.active()) return serial_command::LedResult::busy;
    if (!ready) return serial_command::LedResult::unavailable;
    const uint32_t now = static_cast<uint32_t>(millis());
    lighting::Frame candidate = {};
    if (!player.stop(now, menu.light_config.fade_out_minutes) ||
        !player.frame(now, candidate)) {
      return serial_command::LedResult::invalid_value;
    }
    last_effect_update = now;
    const serial_command::LedResult result = setFrame(candidate);
    if (result == serial_command::LedResult::ok)
      menu.light_applied_config.fade_out_minutes = menu.light_config.fade_out_minutes;
    return result;
  }

  bool getLight(lighting::AnimationConfig& config,
                lighting::RunState& state) const override {
    if (!ready) return false;
    if (alarm_engine.active()) {
      config = alarm_engine.activeConfig().light;
      const alarms::Phase phase = alarm_engine.status(millis()).phase;
      state = phase == alarms::Phase::snoozed ? lighting::RunState::off :
          phase == alarms::Phase::fading ? lighting::RunState::fading_out :
          lighting::RunState::running;
      return true;
    }
    if (sleep_engine.active()) {
      config = sleep_engine.activeConfig().light;
      const sleep_timer::Status status = sleep_engine.status(millis());
      state = !status.light_enabled ? lighting::RunState::off :
          status.remaining_seconds <= config.fade_out_minutes * 60u
              ? lighting::RunState::fading_out : lighting::RunState::running;
      return true;
    }
    state = player.state();
    config = state == lighting::RunState::off ? menu.light_config
                                              : player.config();
    if (state == lighting::RunState::running) {
      config.fade_out_minutes = menu.light_config.fade_out_minutes;
    }
    return true;
  }

  bool update(uint32_t now) {
    if (alarm_engine.active() || sleep_engine.active()) return true;
    if (player.state() == lighting::RunState::off ||
        now - last_effect_update < 50) return true;
    lighting::Frame candidate = {};
    if (!player.frame(now, candidate) ||
        setFrame(candidate) != serial_command::LedResult::ok) {
      player.cancel();
      matrix.clear();
      matrix.show();
      logical_frame = {};
      output_frame = {};
      return false;
    }
    last_effect_update = now;
    return true;
  }

  serial_command::LedResult setFrame(const lighting::Frame& candidate) {
    if (!ready) return serial_command::LedResult::unavailable;
    lighting::Frame output = {};
    lighting::outputFrame(candidate, output);
    for (uint8_t row = 0; row < 8; ++row) {
      for (uint8_t column = 0; column < 8; ++column) {
        uint16_t index = 0;
        if (!lighting::matrixIndex(row, column, index)) {
          return serial_command::LedResult::invalid_value;
        }
        const lighting::Color color = output.pixels[row * 8 + column];
        matrix.setPixelColor(index, color.red, color.green, color.blue,
                             color.white);
      }
    }
    matrix.show();
    logical_frame = candidate;
    output_frame = output;
    return serial_command::LedResult::ok;
  }

  bool getFrame(lighting::Frame& result) const override {
    if (!ready) return false;
    result = output_frame;
    return true;
  }
  void preempt() { player.cancel(); }

private:
  Adafruit_NeoMatrix matrix{8, 8, PIN_EXTERNAL_NEOPIXELS,
                           NEO_MATRIX_TOP + NEO_MATRIX_RIGHT +
                               NEO_MATRIX_COLUMNS + NEO_MATRIX_PROGRESSIVE,
                           NEO_GRBW + NEO_KHZ800};
  lighting::Frame logical_frame = {};
  lighting::Frame output_frame = {};
  lighting::Player player;
  uint32_t last_effect_update = 0;
  bool ready = false;
};

LedAdapter led_adapter;

class SleepAdapter : public sleep_timer::Control {
public:
  sleep_timer::Config get() const override { return sleep_engine.config(); }
  sleep_timer::Status status() const override { return sleep_engine.status(millis()); }
  sleep_timer::Result set(const sleep_timer::Config& config) override {
    const sleep_timer::Result result = sleep_engine.set(config);
    if (result == sleep_timer::Result::ok) menu.sleep_candidate = config;
    return result;
  }
  bool available() const override { return sleep_store.ready(); }
  sleep_timer::Config getSlot(uint8_t slot) const override { return sleep_store.get(slot); }
  sleep_timer::Result loadSlot(uint8_t slot) override {
    if (slot >= sleep_timer::SLOT_COUNT) return sleep_timer::Result::invalid_value;
    if (!available()) return sleep_timer::Result::storage;
    menu.sleep_slot = slot;
    return set(getSlot(slot));
  }
  sleep_timer::Result saveSlot(uint8_t slot, const sleep_timer::Config& config) override {
    if (slot >= sleep_timer::SLOT_COUNT || !sleep_timer::valid(config))
      return sleep_timer::Result::invalid_value;
    if (!sleep_store.save(slot, config)) return sleep_timer::Result::storage;
    menu.sleep_slot = slot;
    return set(config);
  }
  sleep_timer::Result start(uint8_t slot = sleep_timer::NO_SLOT) override {
    if (alarm_engine.active() || alarm_audio.pending()) return sleep_timer::Result::busy;
    if (slot != sleep_timer::NO_SLOT) {
      const sleep_timer::Result loaded = loadSlot(slot);
      if (loaded != sleep_timer::Result::ok) return loaded;
    }
    lighting::Frame frame = {};
    if ((get().light_enabled && !led_adapter.getFrame(frame)) ||
        (get().audio_enabled && audio_adapter.status().core != 1))
      return sleep_timer::Result::unavailable;
    const sleep_timer::Result result = sleep_engine.start(millis(), false, slot);
    if (result == sleep_timer::Result::ok) {
      led_adapter.preempt();
      disturb_sleep_clock(display.sleep_clock);
      serial_diagnostics_log("EVENT SLEEP START");
    }
    return result;
  }
  sleep_timer::Result stop() override {
    if (sleep_engine.active()) serial_diagnostics_log("EVENT SLEEP STOP");
    sleep_engine.stop();
    return sleep_timer::Result::ok;
  }
};

SleepAdapter sleep_adapter;

// ================== Synchronization logic ===============
std::atomic<bool> core_0_configured{false};
std::atomic<bool> core_1_configured{false};
bool core1_separate_stack = true;

// ===================== Core 0 Logic =====================
void setup() {
  audio::initControl();

  init_SerialDiagnostics();
  serial_diagnostics_log("C0-INIT: Serial Started");

  // ----- Configure Power Pins
  pinMode(PIN_EXTERNAL_POWER, OUTPUT);
  digitalWrite(PIN_EXTERNAL_POWER, HIGH);
  pinMode(PIN_EXTERNAL_BUTTON, INPUT_PULLUP);
  serial_diagnostics_log("C0-INIT: NEOPIXEL Pins and Power Configured");
  if (!led_adapter.begin()) {
    serial_diagnostics_log("MATRIX-ERROR: initialization failed");
  }

  // ----- Init Time Keeping System
  init_TimeKeeper(&time_keeper);
  serial_diagnostics_log("C0-INIT: Time System + RTC Initialized");

  // ----- Init Display
  init_DisplayState(&display);
  serial_diagnostics_log("C0-INIT: Display Initialized");

  // ----- Init Encoder + Pots
  init_InputState(&inputs);
  init_InputGestures(&gestures);
  serial_diagnostics_log("C0-INIT: Input System Initialized");

  // ----- Init Menu System
  init_MenuState(&menu);
  serial_diagnostics_log("C0-INIT: Menu/UI System Initialized");

  settings::Result storage_result = settings::Result::unavailable;
  if (settings_storage.begin()) {
    storage_result = settings_store.load(&menu.display_settings);
    if (storage_result == settings::Result::blank) {
      storage_result = settings::Result::ok;
    }
  }
  if (storage_result != settings::Result::ok) {
    menu.screen = MENU_SCREEN_STORAGE_ERROR;
    serial_diagnostics_log("STORAGE-ERROR: settings unavailable");
  }
  const settings::Result alarm_result = alarm_store.load();
  if (alarm_result != settings::Result::ok &&
      alarm_result != settings::Result::blank) {
    menu.screen = MENU_SCREEN_STORAGE_ERROR;
    serial_diagnostics_log("ALARM-STORAGE-ERROR: alarms unavailable");
  }
  serial_diagnostics_log("ALARM: shared light and core-1 audio fade times");
  const settings::Result sleep_result = sleep_store.load();
  if (sleep_result == settings::Result::ok || sleep_result == settings::Result::blank) {
    sleep_adapter.loadSlot(0);
  } else {
    menu.sleep_result = sleep_timer::Result::storage;
    menu.screen = MENU_SCREEN_SLEEP_RESULT;
    serial_diagnostics_log("SLEEP-STORAGE-ERROR: saved slots unavailable");
  }

  // Unblock core 1 setup
  core_0_configured.store(true, std::memory_order_release);
  serial_diagnostics_log("C0-INIT: Core 0 configured, waiting for Core 1");

  // Mark Init as complete once both cores are in lock step...
  while (!core_1_configured.load(std::memory_order_acquire)) {
    delay(1);
  }

  serial_diagnostics_log("C0-INIT: All Cores Ready to Go");
  serial_diagnostics_log("AUDIO: core 1 ready; digital volume limit 50%");
}

bool handle_Tap(bool physical) {
  wake_clock(&display.clock_wake, static_cast<uint32_t>(millis()),
             menu.display_settings.wake_seconds * 1000u);
  disturb_sleep_clock(display.sleep_clock);
  const char* action = "WAKE";
  if (alarm_engine.active()) {
    if (alarm_engine.reduce(true) != alarms::Result::ok) {
      serial_diagnostics_log("INPUT-ERROR: tap alarm reduction failed");
      return false;
    }
    action = "ALARM_HALF";
  } else if (sleep_engine.active()) {
    if (sleep_engine.reduce(true) != sleep_timer::Result::ok) {
      serial_diagnostics_log("INPUT-ERROR: tap sleep reduction failed");
      return false;
    }
    action = "SLEEP_HALF";
  }
  char event[56];
  snprintf(event, sizeof(event), "EVENT TAP %s %s",
           physical ? "PHYSICAL" : "SERIAL", action);
  serial_diagnostics_log(event);
  return true;
}

bool handle_MenuInput(MenuEvent event, int32_t delta) {
  if (menu.clock_save_requested || menu.display_save_requested ||
      menu.alarm_action != ALARM_ACTION_NONE ||
      menu.sleep_action != SLEEP_ACTION_NONE) {
    return false;
  }
  const MenuScreen previous_screen = menu.screen;
  const bool wake_only = menu.screen == MENU_SCREEN_CLOCK &&
                         display.clock_blanked;
  wake_clock(&display.clock_wake, static_cast<uint32_t>(millis()),
             menu.display_settings.wake_seconds * 1000u);
  disturb_sleep_clock(display.sleep_clock);
  if (!wake_only) {
    if (previous_screen == MENU_SCREEN_CLOCK_EDITOR &&
        event == MENU_EVENT_ROTATE && time_keeper.clock_valid &&
        !time_keeper.rtc.lostPower()) {
      const DateTime current = time_keeper.rtc.now();
      const clock_command::DateTime value = {
          current.year(), current.month(), current.day(), current.hour(),
          current.minute(), current.second()};
      if (clock_command::isValid(value)) menu.clock_candidate = value;
    }
    update_MenuState(&menu, event, delta, static_cast<uint32_t>(millis()));
  }
  if (previous_screen == MENU_SCREEN_SETTINGS &&
      menu.screen == MENU_SCREEN_CLOCK_EDITOR) {
    clock_command::DateTime initial = {2000, 1, 1, 0, 0, 0};
    bool initial_valid = false;
    if (time_keeper.clock_valid && !time_keeper.rtc.lostPower()) {
      const DateTime current = time_keeper.rtc.now();
      initial = {current.year(), current.month(), current.day(), current.hour(),
                 current.minute(), current.second()};
      initial_valid = clock_command::isValid(initial);
    }
    beginClockEditor(&menu, initial, initial_valid);
  }
  if (previous_screen == MENU_SCREEN_ALARMS && menu.screen == MENU_SCREEN_ALARM) {
    if (alarm_store.ready()) beginAlarmEditor(&menu, alarm_engine.get(menu.alarm_slot));
    else completeAlarmAction(&menu, alarms::Result::storage);
  }
  if (previous_screen == MENU_SCREEN_SLEEP_SLOTS && menu.screen == MENU_SCREEN_SLEEP) {
    const sleep_timer::Result loaded = sleep_adapter.loadSlot(menu.sleep_slot);
    if (loaded != sleep_timer::Result::ok) {
      menu.sleep_result = loaded;
      menu.screen = MENU_SCREEN_SLEEP_RESULT;
      serial_diagnostics_log("SLEEP-STORAGE-ERROR: loading slot failed");
    }
  }
  if (previous_screen != menu.screen &&
      (menu.screen == MENU_SCREEN_ALARM_SOUND || menu.screen == MENU_SCREEN_SLEEP_SOUND) &&
      !settings_storage.audioCatalog(menu.audio_paths, MenuState::AUDIO_CHOICES,
                                     menu.audio_count, menu.audio_more)) {
    if (menu.screen == MENU_SCREEN_SLEEP_SOUND) {
      menu.sleep_result = sleep_timer::Result::unavailable;
      menu.screen = MENU_SCREEN_SLEEP_RESULT;
    } else completeAlarmAction(&menu, alarms::Result::storage);
    serial_diagnostics_log("AUDIO-ERROR: sound directory unavailable");
  }
  return true;
}

void log_AlarmEvent(const char* action, uint8_t slot) {
  char line[serial_command::EVENT_CAPACITY];
  clock_command::DateTime stamp = {};
  TimeKeeperRtcWriter rtc(&time_keeper);
  if (rtc.getDateTime(stamp)) {
    snprintf(line, sizeof(line), "EVENT ALARM %s %u %04u-%02u-%02u %02u:%02u:%02u",
             action, static_cast<unsigned>(slot + 1), stamp.year, stamp.month,
             stamp.day, stamp.hour, stamp.minute, stamp.second);
  } else snprintf(line, sizeof(line), "EVENT ALARM %s %u INVALID_TIME",
                  action, static_cast<unsigned>(slot + 1));
  serial_diagnostics_log(line);
}

void update_Alarms() {
  static uint32_t clock_poll = 0;
  static uint32_t revision = 0;
  static uint32_t output_timer = 0;
  static uint32_t generation = 0;
  static bool storage_fault_notified = false;
  static bool audio_stop_fault_notified = false;
  static alarms::Status previous = {alarms::NO_SLOT, alarms::Phase::off, 0, 0, false};
  const uint32_t now = static_cast<uint32_t>(millis());
  if (now - clock_poll >= 200 || revision != time_keeper.revision) {
    clock_poll = now;
    clock_command::DateTime value = {};
    TimeKeeperRtcWriter rtc(&time_keeper);
    wall_valid = rtc.getDateTime(value);
    if (wall_valid) {
      wall_time = DateTime(value.year, value.month, value.day, value.hour,
                           value.minute, value.second).unixtime();
    }
    if (revision != time_keeper.revision) {
      revision = time_keeper.revision;
      if (wall_valid && alarm_engine.clockChanged(wall_time) != alarms::Result::ok) {
        serial_diagnostics_log("ALARM-STORAGE-ERROR: clock suppression failed");
        completeAlarmAction(&menu, alarms::Result::storage);
      }
    }
  }
  const alarms::Result result = alarm_engine.tick(wall_time, wall_valid, now);
  if (result != alarms::Result::ok && !storage_fault_notified) {
    storage_fault_notified = true;
    serial_diagnostics_log("ALARM-STORAGE-ERROR: scheduler update failed");
    completeAlarmAction(&menu, result);
  }
  if (result == alarms::Result::ok) storage_fault_notified = false;
  const alarms::Status status = alarm_engine.status(now);
  const bool new_alarm = generation != alarm_engine.generation();
  if (new_alarm) {
    generation = alarm_engine.generation();
    if (sleep_engine.active() || sleep_engine.pending()) {
      sleep_engine.preempt();
      serial_diagnostics_log("EVENT SLEEP PREEMPT");
      if (menu.screen == MENU_SCREEN_SLEEP_STATUS) menu.screen = MENU_SCREEN_CLOCK;
    }
    led_adapter.preempt();
    log_AlarmEvent("START", status.slot);
  } else if (previous.slot != alarms::NO_SLOT && status.slot == alarms::NO_SLOT) {
    log_AlarmEvent("END", previous.slot);
  } else if (previous.phase != status.phase && status.slot != alarms::NO_SLOT) {
    if (status.phase == alarms::Phase::snoozed) menu.screen = MENU_SCREEN_CLOCK;
    log_AlarmEvent(status.phase == alarms::Phase::snoozed ? "SNOOZE" :
                   previous.phase == alarms::Phase::snoozed ? "RESUME" : "PHASE",
                   status.slot);
  }
  const audio::Error audio_result = alarm_audio.update(alarm_engine, audio_adapter, now);
  if (audio_result == audio::Error::none) audio_stop_fault_notified = false;
  else if (!audio_stop_fault_notified) {
    audio_stop_fault_notified = true;
    serial_diagnostics_log("ALARM-AUDIO-ERROR: command pending; queue unavailable");
  }
  if (alarm_engine.active() &&
      (new_alarm || status.phase != previous.phase || now - output_timer >= 50)) {
    lighting::Frame frame = {};
    if (!alarm_engine.frame(now, frame) ||
        led_adapter.setFrame(frame) != serial_command::LedResult::ok) {
      serial_diagnostics_log("ALARM-MATRIX-ERROR: output failed");
      const alarms::Result stop = alarm_engine.dismiss();
      completeAlarmAction(&menu, stop == alarms::Result::ok
          ? alarms::Result::invalid_value : stop);
    }
    output_timer = now;
  } else if (!alarm_engine.active() && previous.slot != alarms::NO_SLOT) {
    if (led_adapter.clear() != serial_command::LedResult::ok) {
      serial_diagnostics_log("ALARM-MATRIX-ERROR: clearing failed");
    }
  }
  display.alarm_active = alarm_engine.active();
  display.alarm_snoozed = status.phase == alarms::Phase::snoozed;
  display.alarm_hold = status.hold_reminder;
  const audio::Status sound = audio::snapshot();
  display.alarm_audio_fault = alarm_engine.active() && sound.alarm &&
      (sound.state == audio::State::error ||
       (sound.state == audio::State::playing && sound.underruns != 0));
  update_sleep_clock(display.sleep_clock, wall_time, wall_valid, now);
  previous = status;
}

void update_Sleep() {
  static bool previous_active = false;
  static bool queue_fault_notified = false;
  static uint32_t generation = 0;
  static uint32_t output_timer = 0;
  const uint32_t now = static_cast<uint32_t>(millis());
  if (alarm_engine.active() || alarm_audio.pending()) {
    sleep_engine.preempt();
  } else {
    const bool running = sleep_engine.active();
    sleep_engine.tick(now);
    if (running && !sleep_engine.active()) {
      serial_diagnostics_log("EVENT SLEEP EXPIRE");
      finish_sleep_clock(display.sleep_clock, wall_time, wall_valid,
          !clock_dark_hours(menu.display_settings,
              static_cast<uint8_t>((wall_time / 3600) % 24),
              static_cast<uint8_t>((wall_time / 60) % 60)), now);
      menu.screen = MENU_SCREEN_CLOCK;
    }
    const audio::Error result = sleep_engine.updateAudio(audio_adapter, now);
    if (result == audio::Error::none) queue_fault_notified = false;
    else if (!queue_fault_notified) {
      queue_fault_notified = true;
      serial_diagnostics_log("SLEEP-AUDIO-ERROR: command pending; queue unavailable");
    }
    const bool new_start = generation != sleep_engine.generation();
    if ((sleep_engine.active() && (new_start || now - output_timer >= 50)) ||
        (!sleep_engine.active() && (previous_active || new_start))) {
      lighting::Frame frame = {};
      const bool ready = led_adapter.getFrame(frame);
      if (!sleep_engine.frame(now, frame) ||
          (ready && led_adapter.setFrame(frame) != serial_command::LedResult::ok)) {
        sleep_engine.stop();
        menu.sleep_result = sleep_timer::Result::unavailable;
        menu.screen = MENU_SCREEN_SLEEP_RESULT;
        serial_diagnostics_log("SLEEP-MATRIX-ERROR: output failed");
      }
      output_timer = now;
    }
  }
  generation = sleep_engine.generation();
  previous_active = sleep_engine.active();
  menu.sleep_status = sleep_engine.status(now);
  const audio::Status sound = audio_adapter.status();
  menu.sleep_audio_fault = sleep_engine.active() && sound.sleep &&
      (sound.state == audio::State::error ||
       (sound.state == audio::State::playing && sound.underruns != 0));
}

void loop() {
  if (!clock_awake(display.clock_wake, static_cast<uint32_t>(millis()))) {
    alarm_engine.clearTapReduction();
    sleep_engine.clearTapReduction();
  }
  update_Alarms();
  update_Sleep();
  update_InputState(&inputs);
  update_GestureState(&inputs, &gestures);
  if (gestures.tap_detected) handle_Tap(true);

  const alarms::Result button_result = alarm_engine.button(
      inputs.front_button_val, static_cast<uint32_t>(millis()));
  if (button_result != alarms::Result::ok) {
    completeAlarmAction(&menu, button_result);
    serial_diagnostics_log("ALARM-STORAGE-ERROR: button action failed");
  }
  if (gestures.front_button_press == BUTTON_TAP && !alarm_engine.active()) {
    handle_MenuInput(MENU_EVENT_BACK, 0);
  } else {
    if (gestures.encoder_delta != 0) {
      handle_MenuInput(MENU_EVENT_ROTATE, gestures.encoder_delta);
    }
    if (gestures.encoder_button_press == BUTTON_TAP &&
        !menu.display_save_requested) {
      handle_MenuInput(MENU_EVENT_SELECT, 0);
    }
  }

  if (update_MenuIdle(&menu, static_cast<uint32_t>(millis())))
    serial_diagnostics_log("EVENT MENU IDLE_CANCEL");

  const AlarmAction alarm_action = takeAlarmAction(&menu);
  if (alarm_action != ALARM_ACTION_NONE) {
    const alarms::Result result = alarm_action == ALARM_ACTION_SAVE
        ? alarm_adapter.set(menu.alarm_slot, menu.alarm_candidate) :
        alarm_action == ALARM_ACTION_START ? alarm_adapter.start(menu.alarm_slot) :
        alarm_action == ALARM_ACTION_SNOOZE ? alarm_adapter.snooze() :
        alarm_action == ALARM_ACTION_DISMISS ? alarm_adapter.dismiss() :
        alarm_adapter.reduce();
    if (alarm_action == ALARM_ACTION_SAVE)
      completeAlarmSave(&menu, result, alarm_engine.get(menu.alarm_slot));
    else completeAlarmAction(&menu, result);
    if (result == alarms::Result::ok && alarm_action == ALARM_ACTION_SNOOZE) {
      if (menu.screen != MENU_SCREEN_CLOCK) cancel_MenuState(&menu);
    }
    if (result == alarms::Result::ok && alarm_action == ALARM_ACTION_SAVE) {
      char event[40];
      snprintf(event, sizeof(event), "EVENT ALARM_SAVE %u",
               static_cast<unsigned>(menu.alarm_slot + 1));
      serial_diagnostics_log(event);
    }
    if (result != alarms::Result::ok) {
      serial_diagnostics_log("ALARM-ERROR: menu action failed");
    }
  }

  settings::DisplaySettings display_candidate;
  if (takeDisplaySaveRequest(&menu, &display_candidate)) {
    const settings_command::Result result = display_settings_adapter.set(
        display_candidate);
    completeDisplaySave(&menu, result == settings_command::Result::success);
    if (result != settings_command::Result::success) {
      serial_diagnostics_log("STORAGE-ERROR: settings save failed");
    }
  }

  clock_command::DateTime candidate = {};
  if (takeClockSaveRequest(&menu, &candidate)) {
    const clock_command::Result result =
        set_TimeKeeperDateTime(&time_keeper, candidate);
    completeClockSave(&menu, result);
    if (result == clock_command::Result::success) {
      serial_diagnostics_time_changed(candidate);
    } else serial_diagnostics_log("RTC-ERROR: menu time save failed");
  }

  lighting::AnimationConfig light_config = {};
  const LightAction light_action = takeLightAction(&menu, &light_config);
  if (light_action != LIGHT_ACTION_NONE) {
    const serial_command::LedResult result = light_action == LIGHT_ACTION_START
        ? led_adapter.startEffect(light_config) : led_adapter.stopEffect();
    completeLightAction(&menu, result == serial_command::LedResult::ok
        ? LightResult::ok : result == serial_command::LedResult::busy
            ? LightResult::busy : LightResult::failed);
    if (result != serial_command::LedResult::ok) {
      serial_diagnostics_log(result == serial_command::LedResult::busy
          ? "MATRIX-BUSY: alarm or sleep timer owns lights"
          : "MATRIX-ERROR: light action failed");
    }
  }
  const SleepAction sleep_action = menu.sleep_action;
  menu.sleep_action = SLEEP_ACTION_NONE;
  if (sleep_action != SLEEP_ACTION_NONE) {
    sleep_timer::Result result = sleep_timer::Result::ok;
    if (sleep_action == SLEEP_ACTION_START) {
      result = sleep_adapter.start(menu.sleep_slot);
    } else if (sleep_action == SLEEP_ACTION_SAVE) {
      result = sleep_adapter.saveSlot(menu.sleep_slot, menu.sleep_candidate);
    } else result = sleep_adapter.stop();
    if (sleep_action == SLEEP_ACTION_SAVE) {
      completeSleepSave(&menu, result, sleep_adapter.getSlot(menu.sleep_slot));
    } else {
      menu.sleep_result = result;
      menu.screen = result != sleep_timer::Result::ok ? MENU_SCREEN_SLEEP_RESULT :
          sleep_action == SLEEP_ACTION_START ? MENU_SCREEN_CLOCK : MENU_SCREEN_SLEEP_SLOTS;
    }
    if (result != sleep_timer::Result::ok)
      serial_diagnostics_log("SLEEP-ERROR: menu action failed");
  }
  if (!led_adapter.update(static_cast<uint32_t>(millis()))) {
    serial_diagnostics_log("MATRIX-ERROR: light update failed");
    completeLightAction(&menu, LightResult::failed);
  }

  update_TimeKeeper(&time_keeper);
  if ((sleep_engine.active() &&
       !clock_awake(display.clock_wake, static_cast<uint32_t>(millis()))) ||
      display.sleep_clock.held || display.sleep_clock.goodnight)
    menu.screen = MENU_SCREEN_CLOCK;
  update_Display(&display, &menu, &time_keeper);
  static audio::Status previous_audio;
  const audio::Status audio_status = audio_adapter.status();
  if (audio_status.request != previous_audio.request ||
      audio_status.state != previous_audio.state ||
      audio_status.result != previous_audio.result ||
      (audio_status.underruns != 0 && previous_audio.underruns == 0)) {
    if (audio_status.underruns != 0 && previous_audio.underruns == 0)
      serial_diagnostics_log("AUDIO-ERROR: underrun detected");
    char event[serial_command::EVENT_CAPACITY];
    snprintf(event, sizeof(event), "EVENT AUDIO %lu %s %s",
             static_cast<unsigned long>(audio_status.request),
             audio::stateName(audio_status.state), audio::errorName(audio_status.result));
    serial_diagnostics_log(event);
    previous_audio = audio_status;
  }
  update_SerialDiagnostics(&time_keeper, &settings_storage,
                           &display_settings_adapter, &encoder_adapter,
                           &led_adapter, &alarm_adapter, &audio_adapter, &sleep_adapter);
  delay(5);

}


// ===================== Core 1 Logic =====================
void setup1() {
  while (!core_0_configured.load(std::memory_order_acquire)) {
    delay(1);
  }
  audio::beginCore1(settings_storage);
  core_1_configured.store(true, std::memory_order_release);
}

void loop1() {
  if (!audio::updateCore1()) delay(1);
}
