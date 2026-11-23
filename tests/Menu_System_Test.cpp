#include "../Firmware/Light_Clock_Firmware/Menu_System.hpp"
#include "../Firmware/Light_Clock_Firmware/Clock_Display.hpp"
#include "../Firmware/Light_Clock_Firmware/Menu_Text.hpp"
#include "../Firmware/Light_Clock_Firmware/Settings_Command.hpp"

#include <assert.h>
#include <string.h>

class FakeClockWriter : public clock_command::RtcWriter {
public:
  explicit FakeClockWriter(const clock_command::DateTime& initial)
      : current(initial), writes(0) {}

  bool setDateTime(const clock_command::DateTime& value) override {
    ++writes;
    current = value;
    return true;
  }

  clock_command::DateTime current;
  unsigned int writes;
};

class MenuStorage : public settings::Storage {
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

static void finish_sleep_save(MenuState* menu) {
  assert(menu->sleep_action == SLEEP_ACTION_SAVE);
  completeSleepSave(menu, sleep_timer::Result::ok, menu->sleep_candidate);
}

static void finish_alarm_save(MenuState* menu) {
  assert(takeAlarmAction(menu) == ALARM_ACTION_SAVE);
  completeAlarmSave(menu, alarms::Result::ok, menu->alarm_candidate);
}

static void finish_clock_save(MenuState* menu) {
  clock_command::DateTime candidate = {};
  assert(takeClockSaveRequest(menu, &candidate));
  assert(clock_command::isValid(candidate));
  completeClockSave(menu, clock_command::Result::success);
}

// REQ-003: the encoder-driven menu reaches each category and returns to clock.
static void test_req_003_rotation_wraps_and_reaches_every_category() {
  MenuState menu;
  init_MenuState(&menu);

  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_LIST);

  const char* expected_labels[MENU_ITEM_COUNT] = {
      "Clock", "Alarms", "Sleep timers", "Live lights", "Settings", "Back"};
  for (uint8_t index = 0; index < MENU_ITEM_COUNT; ++index) {
    assert(menu.selected_item == static_cast<MenuItem>(index));
    assert(strcmp(menu_item_text(static_cast<MenuItem>(index)),
                  expected_labels[index]) == 0);
    update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  }
  assert(menu.selected_item == MENU_ITEM_CLOCK);

  update_MenuState(&menu, MENU_EVENT_ROTATE, -1);
  assert(menu.selected_item == MENU_ITEM_BACK);
}

static void test_req_003_each_category_and_back_navigation() {
  for (uint8_t index = 0; index < MENU_ITEM_BACK; ++index) {
    MenuState menu;
    init_MenuState(&menu);

    update_MenuState(&menu, MENU_EVENT_SELECT, 0);
    update_MenuState(&menu, MENU_EVENT_ROTATE, index);
    update_MenuState(&menu, MENU_EVENT_SELECT, 0);
    assert(menu.selected_item == static_cast<MenuItem>(index));

    if (index == MENU_ITEM_CLOCK) {
      assert(menu.screen == MENU_SCREEN_CLOCK);
      continue;
    }

    if (index == MENU_ITEM_SETTINGS) {
      assert(menu.screen == MENU_SCREEN_SETTINGS);
      update_MenuState(&menu, MENU_EVENT_BACK, 0);
      assert(menu.screen == MENU_SCREEN_LIST);
      continue;
    }
    if (index == MENU_ITEM_LIGHT) {
      assert(menu.screen == MENU_SCREEN_LIGHT);
      update_MenuState(&menu, MENU_EVENT_BACK, 0);
      assert(menu.screen == MENU_SCREEN_LIST);
      continue;
    }
    if (index == MENU_ITEM_ALARMS) {
      assert(menu.screen == MENU_SCREEN_ALARMS);
      update_MenuState(&menu, MENU_EVENT_BACK, 0);
      assert(menu.screen == MENU_SCREEN_LIST);
      continue;
    }

    assert(menu.screen == MENU_SCREEN_SLEEP_SLOTS);
    update_MenuState(&menu, MENU_EVENT_BACK, 0);
    assert(menu.screen == MENU_SCREEN_LIST);
    update_MenuState(&menu, MENU_EVENT_BACK, 0);
    assert(menu.screen == MENU_SCREEN_CLOCK);
  }
}

static void test_light_menu_start_stop_and_fade_options() {
  MenuState menu;
  init_MenuState(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, MENU_ITEM_LIGHT);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_LIGHT);
  assert(strcmp(menu_display_text(&menu), "Sunrise") == 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 4);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_LIGHT_EDITOR);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 7);
  assert(menu.light_config.fade_in_minutes == 60);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.light_config.fade_in_minutes == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 2);
  assert(menu.light_config.fade_out_minutes == 5);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, -3);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  lighting::AnimationConfig config = {};
  assert(takeLightAction(&menu, &config) == LIGHT_ACTION_START);
  assert(config.effect == lighting::Effect::forest);
  assert(config.fade_in_minutes == 0 && config.fade_out_minutes == 5);
  assert(takeLightAction(&menu, &config) == LIGHT_ACTION_NONE);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(takeLightAction(&menu, &config) == LIGHT_ACTION_STOP);
  completeLightAction(&menu, LightResult::busy);
  assert(strcmp(menu_display_text(&menu), "Timer active") == 0);
  completeLightAction(&menu, LightResult::failed);
  assert(strcmp(menu_display_text(&menu), "Light error") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_LIGHT);
  update_MenuState(&menu, MENU_EVENT_ROTATE, -4);
  assert(strcmp(menu_display_text(&menu), "Back") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_LIST);
}

static void test_req_003_encoder_select_returns_to_menu() {
  MenuState menu;
  init_MenuState(&menu);

  update_MenuState(&menu, MENU_EVENT_ROTATE, 2);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_SLEEP_SLOTS);
  assert(menu.selected_item == MENU_ITEM_SLEEP);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  assert(menu.screen == MENU_SCREEN_LIST);
}

static void test_req_015_016_040_sleep_menu_edits_audio_selection_and_actions() {
  MenuState menu;
  init_MenuState(&menu);
  menu.screen = MENU_SCREEN_SLEEP;
  menu.sleep_field = SLEEP_HOLD;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, -29);
  assert(menu.sleep_candidate.hold_minutes == 1);
  finish_sleep_save(&menu);
  update_MenuState(&menu, MENU_EVENT_ROTATE, -1);
  assert(menu.sleep_candidate.hold_minutes == 0);
  finish_sleep_save(&menu);
  update_MenuState(&menu, MENU_EVENT_ROTATE, -1);
  assert(menu.sleep_candidate.hold_minutes == 180);
  finish_sleep_save(&menu);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  assert(menu.screen == MENU_SCREEN_SLEEP);
  menu.sleep_field = SLEEP_OUTPUT;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(strcmp(menu_display_text(&menu), "Audio only") == 0);
  finish_sleep_save(&menu);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(strcmp(menu_display_text(&menu), "Both") == 0);
  finish_sleep_save(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  menu.sleep_field = SLEEP_EFFECT;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(strcmp(menu_display_text(&menu), "Fire") == 0);
  finish_sleep_save(&menu);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(strcmp(menu_display_text(&menu), "Nebula") == 0);
  finish_sleep_save(&menu);
  lighting::AnimationConfig unused;
  assert(takeLightAction(&menu, &unused) == LIGHT_ACTION_NONE);
  assert(menu.screen == MENU_SCREEN_SLEEP_EDITOR);
  assert(menu.sleep_candidate.light.effect == lighting::Effect::nebula);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  menu.sleep_field = SLEEP_SOUND;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_SLEEP_SOUND);
  menu.audio_count = 1;
  strcpy(menu.audio_paths[0], "/Quiet Night.wav");
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(strcmp(menu.sleep_candidate.sound.path, "/Quiet Night.wav") == 0);
  assert(menu.alarm_candidate.sound.path[0] == '\0');
  finish_sleep_save(&menu);
  menu.sleep_field = SLEEP_START;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.sleep_action == SLEEP_ACTION_START);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.sleep_field == SLEEP_START);
  menu.sleep_action = SLEEP_ACTION_NONE;
  menu.sleep_field = SLEEP_STATUS;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_CLOCK);
  menu.screen = MENU_SCREEN_SLEEP_SLOTS;
  menu.sleep_slot = 0;
  assert(strcmp(menu_display_text(&menu), "SLT1") == 0);
  menu.sleep_slot = 2;
  assert(strcmp(menu_display_text(&menu), "SLT3") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_SLEEP);
  assert(menu.sleep_field == SLEEP_START);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.sleep_action == SLEEP_ACTION_STOP);
  char text[6];
  format_sleep_countdown(3599, text);
  assert(strcmp(text, "59:59") == 0);
  format_sleep_countdown(3600, text);
  assert(strcmp(text, "01:00") == 0);
  format_sleep_countdown(10800, text);
  assert(strcmp(text, "03:00") == 0);
}

static void test_sleep_dark_wake_goodnight_and_next_noon() {
  settings::DisplaySettings settings = settings::defaults();
  ClockWakeState wake = {false, 0};
  assert(!clock_should_blank(settings, 14, 0, wake, 100));
  assert(clock_should_blank(settings, 14, 0, wake, 100, true));
  const uint32_t start = UINT32_MAX - 100;
  wake_clock(&wake, start);
  assert(!clock_should_blank(settings, 14, 0, wake, start + 29999u, true));
  assert(clock_should_blank(settings, 14, 0, wake, start + 30000u, true));
  SleepClockState state;
  const uint32_t midnight = 946857600;
  finish_sleep_clock(state, midnight + 1800, true, true, start);
  assert(state.goodnight && state.held && state.release_at == midnight + 43200);
  update_sleep_clock(state, midnight + 1801, true, start + 3999u);
  assert(state.goodnight);
  update_sleep_clock(state, midnight + 1804, true, start + 4000u);
  assert(!state.goodnight && state.held);
  update_sleep_clock(state, midnight + 43199, true, start + 5000u);
  assert(state.held);
  update_sleep_clock(state, midnight + 43200, true, start + 5001u);
  assert(!state.held);
  finish_sleep_clock(state, midnight + 14 * 3600, true, false, 10000);
  assert(!state.goodnight && state.release_at == midnight + 86400 + 43200);
  disturb_sleep_clock(state);
  assert(!state.held && !state.goodnight);
  finish_sleep_clock(state, 0, false, true, 11000);
  assert(state.held && !state.goodnight);
  update_sleep_clock(state, midnight + 1800, true, 12000);
  assert(state.release_at == midnight + 43200);
}

static void test_readable_labels_and_value_only_fade_editors() {
  MenuState menu;
  init_MenuState(&menu);
  menu.screen = MENU_SCREEN_SLEEP;
  menu.sleep_field = SLEEP_EFFECT;
  assert(strcmp(menu_display_text(&menu), "Animation") == 0);
  char window[5];
  menu_text_window(menu_display_text(&menu), 5, window);
  assert(strcmp(window, "tion") == 0);
  menu.sleep_field = SLEEP_FADE_OUT;
  assert(strcmp(menu_display_text(&menu), "Fade out") == 0);
  menu.sleep_candidate.light.fade_out_minutes = 0;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  const char* expected[] = {"0", "1", "5", "10", "15", "20", "30", "45", "60"};
  for (const char* value : expected) {
    assert(strcmp(menu_display_text(&menu), value) == 0);
    update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
    finish_sleep_save(&menu);
  }
  assert(strcmp(menu_display_text(&menu), "0") == 0);
}

static void test_alarm_editor_autosaves_days_and_actions() {
  MenuState menu;
  init_MenuState(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, MENU_ITEM_ALARMS);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  beginAlarmEditor(&menu, alarms::defaults());
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.alarm_candidate.enabled);
  finish_alarm_save(&menu);
  assert(menu.screen == MENU_SCREEN_ALARM_EDITOR);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, ALARM_DAYS);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.alarm_candidate.days == 126);
  finish_alarm_save(&menu);
  assert(menu.screen == MENU_SCREEN_ALARM_DAYS);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 7);
  assert(strcmp(menu_display_text(&menu), "Back") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, ALARM_TEST - ALARM_DAYS);
  assert(takeAlarmAction(&menu) == ALARM_ACTION_NONE);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(takeAlarmAction(&menu) == ALARM_ACTION_START);
  completeAlarmAction(&menu, alarms::Result::ok);
  assert(strcmp(menu_display_text(&menu), "Done") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, ALARM_BACK - ALARM_TEST);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_ALARMS);
  assert(takeAlarmAction(&menu) == ALARM_ACTION_NONE);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 5);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(takeAlarmAction(&menu) == ALARM_ACTION_SNOOZE);
}

static void test_alarm_sound_selection_volume_and_fades() {
  MenuState menu;
  init_MenuState(&menu);
  beginAlarmEditor(&menu, alarms::defaults());
  menu.screen = MENU_SCREEN_ALARM;
  menu.alarm_field = ALARM_SOUND;
  menu.audio_count = 1;
  strcpy(menu.audio_paths[0], "/morning.wav");
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(strcmp(menu_display_text(&menu), "morning.wav") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(strcmp(menu.alarm_candidate.sound.path, "/morning.wav") == 0);
  finish_alarm_save(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  assert(strcmp(menu.alarm_candidate.sound.path, "/morning.wav") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.alarm_candidate.sound.path[0] == '\0');
  finish_alarm_save(&menu);
  menu.alarm_field = ALARM_VOLUME;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 5);
  assert(menu.alarm_candidate.sound.volume == 10);
  finish_alarm_save(&menu);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 40);
  assert(menu.alarm_candidate.sound.volume == 50);
  finish_alarm_save(&menu);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.alarm_candidate.sound.volume == 0);
  finish_alarm_save(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  menu.alarm_field = ALARM_FADE_IN;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.alarm_candidate.light.fade_in_minutes == 45);
  assert(strcmp(menu_display_text(&menu), "45") == 0);
}

static void test_alarm_and_sleep_fields_autosave_and_survive_idle_reload() {
  MenuStorage alarm_storage;
  alarms::Store alarm_store(alarm_storage);
  assert(alarm_store.load() == settings::Result::blank);
  alarms::Engine alarm_engine(alarm_store);
  MenuState menu;
  init_MenuState(&menu);
  menu.screen = MENU_SCREEN_ALARMS;
  menu.alarm_slot = 0;
  assert(strcmp(menu_display_text(&menu), "ALM1") == 0);
  menu.alarm_slot = 4;
  assert(strcmp(menu_display_text(&menu), "ALM5") == 0);
  menu.alarm_slot = 2;
  beginAlarmEditor(&menu, alarm_engine.get(2));
  menu.screen = MENU_SCREEN_ALARM_SOUND;
  menu.audio_count = 1;
  menu.audio_selection = 1;
  strcpy(menu.audio_paths[0], "/morning.wav");
  update_MenuState(&menu, MENU_EVENT_SELECT, 0, 100);
  assert(takeAlarmAction(&menu) == ALARM_ACTION_SAVE);
  assert(alarm_engine.set(2, menu.alarm_candidate, 0) == alarms::Result::ok);
  completeAlarmSave(&menu, alarms::Result::ok, alarm_engine.get(2));
  menu.screen = MENU_SCREEN_ALARM_DAYS;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0, 100);
  assert(takeAlarmAction(&menu) == ALARM_ACTION_SAVE);
  assert(alarm_engine.set(2, menu.alarm_candidate, 0) == alarms::Result::ok);
  completeAlarmSave(&menu, alarms::Result::ok, alarm_engine.get(2));
  for (uint8_t field = ALARM_ENABLED; field <= ALARM_VOLUME; ++field) {
    if (field == ALARM_DAYS || field == ALARM_SOUND) continue;
    menu.screen = MENU_SCREEN_ALARM_EDITOR;
    menu.alarm_field = static_cast<AlarmField>(field);
    update_MenuState(&menu, MENU_EVENT_ROTATE, 1, 100);
    assert(takeAlarmAction(&menu) == ALARM_ACTION_SAVE);
    const alarms::Result result = alarm_engine.set(2, menu.alarm_candidate, 0);
    assert(result == alarms::Result::ok);
    completeAlarmSave(&menu, result, alarm_engine.get(2));
    assert(menu.screen == MENU_SCREEN_ALARM_EDITOR);
    char expected[32];
    strcpy(expected, menu_display_text(&menu));
    alarms::Store reloaded(alarm_storage);
    assert(reloaded.load() == settings::Result::ok);
    menu.alarm_candidate = reloaded.bank().slots[2];
    assert(strcmp(menu_display_text(&menu), expected) == 0);
  }
  const uint16_t saved_minute = menu.alarm_candidate.minute;
  assert(update_MenuIdle(&menu, 60100));
  assert(alarm_engine.get(2).minute == saved_minute);
  assert(alarm_engine.get(2).days == 126);
  assert(strcmp(alarm_engine.get(2).sound.path, "/morning.wav") == 0);
  assert(!alarm_engine.get(0).enabled);

  MenuStorage sleep_storage;
  sleep_timer::Store sleep_store(sleep_storage);
  assert(sleep_store.load() == settings::Result::blank);
  menu.sleep_slot = 1;
  menu.sleep_candidate = sleep_store.get(1);
  menu.screen = MENU_SCREEN_SLEEP_SOUND;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0, 70000);
  assert(menu.sleep_action == SLEEP_ACTION_SAVE);
  assert(sleep_store.save(1, menu.sleep_candidate));
  completeSleepSave(&menu, sleep_timer::Result::ok, sleep_store.get(1));
  for (uint8_t field = SLEEP_HOLD; field <= SLEEP_VOLUME; ++field) {
    if (field == SLEEP_SOUND) continue;
    menu.screen = MENU_SCREEN_SLEEP_EDITOR;
    menu.sleep_field = static_cast<SleepField>(field);
    update_MenuState(&menu, MENU_EVENT_ROTATE, 1, 70000);
    assert(menu.sleep_action == SLEEP_ACTION_SAVE);
    assert(sleep_store.save(1, menu.sleep_candidate));
    completeSleepSave(&menu, sleep_timer::Result::ok, sleep_store.get(1));
    assert(menu.screen == MENU_SCREEN_SLEEP_EDITOR);
    char expected[32];
    strcpy(expected, menu_display_text(&menu));
    sleep_timer::Store reloaded(sleep_storage);
    assert(reloaded.load() == settings::Result::ok);
    menu.sleep_candidate = reloaded.get(1);
    assert(strcmp(menu_display_text(&menu), expected) == 0);
  }
  const uint16_t saved_hold = menu.sleep_candidate.hold_minutes;
  update_MenuState(&menu, MENU_EVENT_BACK, 0, 70000);
  assert(update_MenuIdle(&menu, 130000));
  sleep_timer::Store reloaded(sleep_storage);
  assert(reloaded.load() == settings::Result::ok);
  assert(reloaded.get(1).hold_minutes == saved_hold);
  assert(strcmp(reloaded.get(1).sound.path, "/morning.wav") == 0);
  assert(reloaded.get(0).hold_minutes == 30);
}

static void test_autosave_failures_show_errors_and_restore_confirmed_values() {
  MenuStorage storage;
  alarms::Store store(storage);
  assert(store.load() == settings::Result::blank);
  alarms::Engine engine(store);
  MenuState menu;
  init_MenuState(&menu);
  menu.screen = MENU_SCREEN_ALARM_EDITOR;
  menu.alarm_field = ALARM_HOUR;
  beginAlarmEditor(&menu, engine.get(0));
  menu.alarm_field = ALARM_HOUR;
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(takeAlarmAction(&menu) == ALARM_ACTION_SAVE);
  storage.fail = true;
  const alarms::Result result = engine.set(0, menu.alarm_candidate, 0);
  assert(result == alarms::Result::storage);
  completeAlarmSave(&menu, result, engine.get(0));
  assert(menu.screen == MENU_SCREEN_ALARM_RESULT);
  assert(menu.alarm_candidate.minute == 7 * 60);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  assert(menu.screen == MENU_SCREEN_ALARM);

  // The final repeat day may not be removed from a valid alarm.
  menu.alarm_candidate.days = 1;
  const alarms::Config saved_alarm = menu.alarm_candidate;
  menu.screen = MENU_SCREEN_ALARM_DAYS;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(takeAlarmAction(&menu) == ALARM_ACTION_SAVE);
  assert(!alarms::valid(menu.alarm_candidate));
  completeAlarmSave(&menu, alarms::Result::invalid_value, saved_alarm);
  assert(menu.alarm_candidate.days == 1);
  assert(menu.screen == MENU_SCREEN_ALARM_RESULT);

  const sleep_timer::Config saved_sleep = sleep_timer::defaults();
  menu.screen = MENU_SCREEN_SLEEP_EDITOR;
  menu.sleep_candidate = saved_sleep;
  menu.sleep_field = SLEEP_HOLD;
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.sleep_action == SLEEP_ACTION_SAVE);
  completeSleepSave(&menu, sleep_timer::Result::storage, saved_sleep);
  assert(menu.screen == MENU_SCREEN_SLEEP_RESULT);
  assert(strcmp(menu_display_text(&menu), "SD error") == 0);
  assert(menu.sleep_candidate.hold_minutes == saved_sleep.hold_minutes);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  assert(menu.screen == MENU_SCREEN_SLEEP);

  // Select a sound before enabling audio on a light-only slot.
  menu.sleep_field = SLEEP_OUTPUT;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(!sleep_timer::valid(menu.sleep_candidate));
  completeSleepSave(&menu, sleep_timer::Result::invalid_value, saved_sleep);
  assert(menu.screen == MENU_SCREEN_SLEEP_RESULT);
  assert(!menu.sleep_candidate.audio_enabled);
  assert(menu.sleep_action == SLEEP_ACTION_NONE);
}

static void test_req_003_back_item_returns_home() {
  MenuState menu;
  init_MenuState(&menu);

  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, MENU_ITEM_BACK);
  assert(menu.selected_item == MENU_ITEM_BACK);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_CLOCK);
}

static void test_clock_category_returns_home() {
  MenuState menu;
  init_MenuState(&menu);

  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_CLOCK);
}

static void open_clock_settings(MenuState* menu) {
  update_MenuState(menu, MENU_EVENT_SELECT, 0);
  update_MenuState(menu, MENU_EVENT_ROTATE, MENU_ITEM_SETTINGS);
  update_MenuState(menu, MENU_EVENT_SELECT, 0);
  assert(menu->screen == MENU_SCREEN_SETTINGS);
  update_MenuState(menu, MENU_EVENT_SELECT, 0);
  assert(menu->screen == MENU_SCREEN_CLOCK_EDITOR);
}

static void test_settings_back_item_returns_to_main_menu() {
  MenuState menu;
  init_MenuState(&menu);

  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, MENU_ITEM_SETTINGS);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_SETTINGS);
  update_MenuState(&menu, MENU_EVENT_ROTATE, -1);
  assert(strcmp(menu_display_text(&menu), "Back") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_LIST);
  assert(menu.selected_item == MENU_ITEM_SETTINGS);
}

// REQ-002: each clock adjustment writes immediately, without a Save screen.
static void test_req_002_local_editor_saves_each_adjustment() {
  MenuState menu;
  init_MenuState(&menu);
  open_clock_settings(&menu);

  const clock_command::DateTime initial = {2024, 2, 28, 23, 59, 58};
  FakeClockWriter rtc(initial);
  beginClockEditor(&menu, initial, true);
  assert(strcmp(menu_display_text(&menu), "2024") == 0);
  for (uint8_t field = 0; field < CLOCK_EDIT_FIELD_COUNT; ++field) {
    update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
    clock_command::DateTime candidate = {};
    assert(takeClockSaveRequest(&menu, &candidate));
    assert(!takeClockSaveRequest(&menu, &candidate));
    const clock_command::Result result = clock_command::setDateTime(rtc, candidate);
    assert(result == clock_command::Result::success);
    completeClockSave(&menu, result);
    assert(menu.screen == MENU_SCREEN_CLOCK_EDITOR);
    assert(rtc.writes == field + 1u);
    update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  }
  assert(menu.screen == MENU_SCREEN_SETTINGS);
  assert(rtc.current.year == 2025 && rtc.current.month == 3 &&
         rtc.current.day == 29 && rtc.current.hour == 0 &&
         rtc.current.minute == 0 && rtc.current.second == 59);
  assert(!menu.clock_save_requested);

  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  beginClockEditor(&menu, rtc.current, true);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  assert(menu.screen == MENU_SCREEN_SETTINGS);
  assert(rtc.writes == CLOCK_EDIT_FIELD_COUNT);
}

static void test_req_002_invalid_clock_starts_with_editable_default() {
  MenuState menu;
  init_MenuState(&menu);
  open_clock_settings(&menu);

  const clock_command::DateTime invalid = {};
  beginClockEditor(&menu, invalid, false);
  assert(menu.clock_candidate.year == 2000);
  assert(menu.clock_candidate.month == 1);
  assert(menu.clock_candidate.day == 1);

  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  finish_clock_save(&menu);
  assert(menu.clock_candidate.year == 2001);

  completeClockSave(&menu, clock_command::Result::rtc_write_failed);
  assert(menu.screen == MENU_SCREEN_CLOCK_RESULT);
  assert(strcmp(menu_display_text(&menu), "RTC?") == 0);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  assert(menu.screen == MENU_SCREEN_SETTINGS);
  completeClockSave(&menu, clock_command::Result::invalid_date_time);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_CLOCK_EDITOR);
  assert(menu.clock_edit_field == CLOCK_EDIT_DAY);
}

static void test_clock_editor_wraps_field_ranges() {
  MenuState menu;
  init_MenuState(&menu);
  open_clock_settings(&menu);
  beginClockEditor(&menu, {2099, 12, 31, 23, 59, 59}, true);

  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.clock_candidate.year == 2000);
  finish_clock_save(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.clock_candidate.month == 1);
  finish_clock_save(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.clock_candidate.day == 1);
  finish_clock_save(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.clock_candidate.hour == 0);
  finish_clock_save(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.clock_candidate.minute == 0);
  finish_clock_save(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.clock_candidate.second == 0);
  finish_clock_save(&menu);
}

static void test_clock_autosave_clamps_short_months_and_leap_years() {
  MenuState menu;
  init_MenuState(&menu);
  menu.screen = MENU_SCREEN_CLOCK_EDITOR;
  beginClockEditor(&menu, {2024, 2, 29, 12, 0, 0}, true);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.clock_candidate.year == 2025 && menu.clock_candidate.day == 28);
  finish_clock_save(&menu);
  beginClockEditor(&menu, {2024, 1, 31, 12, 0, 0}, true);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.clock_candidate.month == 2 && menu.clock_candidate.day == 29);
  finish_clock_save(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(menu.clock_candidate.day == 1);
  finish_clock_save(&menu);
}

// REQ-050: invalid RTC time is presented as requiring a clock reset.
static void test_req_050_invalid_clock_shows_set_message() {
  char text[6];
  format_clock_display_text(false, 0, 0, text);
  assert(strcmp(text, "SET ") == 0);

  format_clock_display_text(true, 4, 7, text);
  assert(strcmp(text, "04:07") == 0);
}

static void test_req_002_hour_minute_editor_blinks_selected_digits() {
  char text[6];
  format_clock_edit_display_text(12, 34, true, true, text);
  assert(strcmp(text, "12:34") == 0);
  format_clock_edit_display_text(12, 34, true, false, text);
  assert(strcmp(text, "  :34") == 0);
  format_clock_edit_display_text(12, 34, false, false, text);
  assert(strcmp(text, "12:  ") == 0);
}

static void test_req_001_display_format_and_req_004_brightness_save() {
  MenuState menu;
  init_MenuState(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, MENU_ITEM_SETTINGS);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_DISPLAY_FORMAT);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  settings::DisplaySettings candidate;
  assert(takeDisplaySaveRequest(&menu, &candidate));
  assert(candidate.use_12_hour && !menu.display_settings.use_12_hour);
  completeDisplaySave(&menu, true);
  assert(menu.display_settings.use_12_hour);
  assert(clock_display_hour(0, true) == 12);
  assert(clock_display_hour(12, true) == 12);
  assert(clock_display_hour(23, true) == 11);
  assert(clock_display_hour(23, false) == 23);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 2);
  assert(takeDisplaySaveRequest(&menu, &candidate));
  assert(candidate.brightness == 5 && candidate.use_12_hour);
  completeDisplaySave(&menu, false);
  assert(menu.screen == MENU_SCREEN_STORAGE_ERROR);
  assert(menu.display_settings.brightness == 3);
  assert(menu.display_settings.use_12_hour);
  assert(strcmp(menu_display_text(&menu), "SD error") == 0);
}

static void test_req_018_menu_save_and_failed_write() {
  MenuStorage storage;
  settings::Store store(storage);
  MenuState menu;
  init_MenuState(&menu);
  assert(store.load(&menu.display_settings) == settings::Result::blank);
  assert(store.save(menu.display_settings) == settings::Result::ok);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, MENU_ITEM_SETTINGS);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 2);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  settings::DisplaySettings candidate;
  assert(takeDisplaySaveRequest(&menu, &candidate));
  completeDisplaySave(&menu, settings_command::setDisplay(
      store, &menu.display_settings, candidate) ==
      settings_command::Result::success);
  assert(menu.display_settings.brightness == 4);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_SETTINGS);
  settings::Store restarted(storage);
  settings::DisplaySettings loaded;
  assert(restarted.load(&loaded) == settings::Result::ok);
  assert(loaded.brightness == 4);

  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  assert(takeDisplaySaveRequest(&menu, &candidate));
  storage.fail = true;
  completeDisplaySave(&menu, settings_command::setDisplay(
      store, &menu.display_settings, candidate) ==
      settings_command::Result::success);
  assert(menu.display_settings.brightness == 4);
  assert(menu.screen == MENU_SCREEN_STORAGE_ERROR);
  settings::Store after_failure(storage);
  assert(after_failure.load(&loaded) == settings::Result::ok);
  assert(loaded.brightness == 4);
}

static void save_menu_display(MenuState* menu, settings::Store* store) {
  settings::DisplaySettings candidate;
  assert(takeDisplaySaveRequest(menu, &candidate));
  const bool success = settings_command::setDisplay(
                           *store, &menu->display_settings, candidate) ==
                       settings_command::Result::success;
  completeDisplaySave(menu, success);
  assert(success);
}

static void test_req_005_dark_hours_menu_and_wake() {
  MenuStorage storage;
  settings::Store store(storage);
  MenuState menu;
  init_MenuState(&menu);
  assert(store.load(&menu.display_settings) == settings::Result::blank);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, MENU_ITEM_SETTINGS);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 3);
  assert(strcmp(menu_display_text(&menu), "Dark hours") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_DARK_SETTINGS);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  save_menu_display(&menu, &store);
  assert(menu.display_settings.dark_enabled);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  save_menu_display(&menu, &store);
  assert(menu.display_settings.dark_start == 23 * 60);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, -2);
  assert(strcmp(menu_display_text(&menu), "Back") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_SETTINGS);

  settings::Store restarted(storage);
  settings::DisplaySettings value = {};
  assert(restarted.load(&value) == settings::Result::ok);
  assert(value.dark_enabled && value.dark_start == 23 * 60);
  assert(!clock_dark_hours(value, 22, 59));
  assert(clock_dark_hours(value, 23, 0));
  assert(clock_dark_hours(value, 0, 0));
  assert(clock_dark_hours(value, 6, 59));
  assert(!clock_dark_hours(value, 7, 0));
  ClockWakeState wake = {false, 0};
  assert(clock_should_blank(value, 1, 0, wake, 100));
  wake_clock(&wake, 0xfffffff0u);
  assert(!clock_should_blank(value, 1, 0, wake, 0x10u));
  assert(clock_should_blank(value, 1, 0, wake, 30000u));
  value.dark_start = value.dark_end;
  assert(!clock_dark_hours(value, 7, 0));
  value.dark_start = 8 * 60;
  value.dark_end = 10 * 60;
  assert(clock_dark_hours(value, 9, 0));
  assert(!clock_dark_hours(value, 10, 0));
}

static void test_menu_text_scroll_window_and_font() {
  char window[5];
  menu_text_window("Sleep Timer", 0, window);
  assert(strcmp(window, "Slee") == 0);

  menu_text_window("Sleep Timer", 4, window);
  assert(strcmp(window, "p Ti") == 0);

  menu_text_window("Sleep Timer", 8, window);
  assert(strcmp(window, "mer ") == 0);

  uint16_t display_buffer[5];
  format_menu_text(display_buffer, "Back");
  assert(display_buffer[0] == 0x7C);
  assert(display_buffer[1] == 0x77);
  assert(display_buffer[2] == 0);
  assert(display_buffer[3] == 0x39);
  assert(display_buffer[4] == 0x75);
}

static void test_menu_idle_boundary_activity_and_wrap() {
  MenuState menu;
  init_MenuState(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0, 100);
  assert(!update_MenuIdle(&menu, 60099));
  update_MenuState(&menu, MENU_EVENT_NONE, 0, 60099);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 0, 60099);
  assert(update_MenuIdle(&menu, 60100));
  assert(menu.screen == MENU_SCREEN_CLOCK);

  update_MenuState(&menu, MENU_EVENT_SELECT, 0, 70000);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1, 100000);
  assert(!update_MenuIdle(&menu, 130000));
  assert(!update_MenuIdle(&menu, 159999));
  assert(update_MenuIdle(&menu, 160000));

  update_MenuState(&menu, MENU_EVENT_SELECT, 0, 0xfffffff0u);
  assert(!update_MenuIdle(&menu, 59983));
  assert(update_MenuIdle(&menu, 59984));
}

static void test_menu_idle_cancels_pending_changes_not_applied_settings() {
  MenuState menu;
  init_MenuState(&menu);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0, 0);
  menu.display_candidate = menu.display_settings;
  menu.display_candidate.brightness = 3;
  menu.display_save_requested = true;
  completeDisplaySave(&menu, true);
  menu.light_applied_config.brightness = 80;
  menu.light_config.brightness = 200;
  menu.display_candidate.brightness = 12;
  menu.clock_candidate.hour = 23;
  menu.alarm_candidate.minute = 23 * 60;
  menu.sleep_candidate.hold_minutes = 99;
  menu.clock_save_requested = true;
  menu.display_save_requested = true;
  menu.light_action = LIGHT_ACTION_START;
  menu.alarm_action = ALARM_ACTION_SAVE;
  menu.sleep_action = SLEEP_ACTION_SAVE;
  update_MenuState(&menu, MENU_EVENT_SELECT, 0, 59000);
  assert(update_MenuIdle(&menu, 60000));
  assert(menu.screen == MENU_SCREEN_CLOCK);
  assert(menu.display_settings.brightness == 3);
  assert(menu.display_candidate.brightness == 3);
  assert(menu.light_config.brightness == 80);
  assert(menu.clock_candidate.hour != 23);
  assert(menu.alarm_candidate.minute != 23 * 60);
  assert(menu.sleep_candidate.hold_minutes != 99);
  clock_command::DateTime clock;
  settings::DisplaySettings display;
  lighting::AnimationConfig light;
  assert(!takeClockSaveRequest(&menu, &clock));
  assert(!takeDisplaySaveRequest(&menu, &display));
  assert(takeLightAction(&menu, &light) == LIGHT_ACTION_NONE);
  assert(takeAlarmAction(&menu) == ALARM_ACTION_NONE);
  assert(menu.sleep_action == SLEEP_ACTION_NONE);
}

static void test_programmatic_menu_screens_and_sleep_cancel_share_cleanup() {
  MenuState menu;
  init_MenuState(&menu);
  menu.screen = MENU_SCREEN_STORAGE_ERROR;
  assert(!update_MenuIdle(&menu, 500));
  assert(!update_MenuIdle(&menu, 60499));
  assert(update_MenuIdle(&menu, 60500));
  menu.screen = MENU_SCREEN_LIGHT_EDITOR;
  menu.light_config.brightness = 10;
  menu.light_action = LIGHT_ACTION_START;
  cancel_MenuState(&menu);
  assert(menu.screen == MENU_SCREEN_CLOCK);
  assert(menu.light_config.brightness == menu.light_applied_config.brightness);
  assert(menu.light_action == LIGHT_ACTION_NONE);
  assert(!update_MenuIdle(&menu, 200000));
}

static void test_configurable_wake_editor_and_wraparound_expiry() {
  MenuState menu;
  init_MenuState(&menu);
  menu.screen = MENU_SCREEN_DARK_SETTINGS;
  menu.dark_selection = 5;
  assert(strcmp(menu_display_text(&menu), "Wake seconds") == 0);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(strcmp(menu_display_text(&menu), "30") == 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, -29);
  settings::DisplaySettings saved;
  assert(takeDisplaySaveRequest(&menu, &saved) && saved.wake_seconds == 1);
  menu.display_settings = saved;
  completeDisplaySave(&menu, true);
  assert(strcmp(menu_display_text(&menu), "1") == 0);
  update_MenuState(&menu, MENU_EVENT_BACK, 0);
  update_MenuState(&menu, MENU_EVENT_ROTATE, 1);
  update_MenuState(&menu, MENU_EVENT_SELECT, 0);
  assert(menu.screen == MENU_SCREEN_SETTINGS);
  ClockWakeState wake = {false, 0};
  const uint32_t start = UINT32_MAX - 500;
  wake_clock(&wake, start, 1000);
  assert(clock_awake(wake, start + 999u));
  assert(!clock_awake(wake, start + 1000u));
  wake_clock(&wake, start + 900u, 5000);
  assert(clock_awake(wake, start + 5899u));
  assert(!clock_awake(wake, start + 5900u));
}

int main() {
  test_configurable_wake_editor_and_wraparound_expiry();
  test_menu_idle_boundary_activity_and_wrap();
  test_menu_idle_cancels_pending_changes_not_applied_settings();
  test_programmatic_menu_screens_and_sleep_cancel_share_cleanup();
  test_sleep_dark_wake_goodnight_and_next_noon();
  test_readable_labels_and_value_only_fade_editors();
  test_req_015_016_040_sleep_menu_edits_audio_selection_and_actions();
  test_alarm_sound_selection_volume_and_fades();
  test_alarm_and_sleep_fields_autosave_and_survive_idle_reload();
  test_autosave_failures_show_errors_and_restore_confirmed_values();
  test_req_003_rotation_wraps_and_reaches_every_category();
  test_req_003_each_category_and_back_navigation();
  test_light_menu_start_stop_and_fade_options();
  test_alarm_editor_autosaves_days_and_actions();
  test_req_003_encoder_select_returns_to_menu();
  test_req_003_back_item_returns_home();
  test_clock_category_returns_home();
  test_settings_back_item_returns_to_main_menu();
  test_req_002_local_editor_saves_each_adjustment();
  test_clock_autosave_clamps_short_months_and_leap_years();
  test_req_002_invalid_clock_starts_with_editable_default();
  test_clock_editor_wraps_field_ranges();
  test_req_050_invalid_clock_shows_set_message();
  test_req_002_hour_minute_editor_blinks_selected_digits();
  test_req_001_display_format_and_req_004_brightness_save();
  test_req_018_menu_save_and_failed_write();
  test_req_005_dark_hours_menu_and_wake();
  test_menu_text_scroll_window_and_font();
}
