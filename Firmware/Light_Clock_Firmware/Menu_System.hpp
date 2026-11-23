#pragma once

#include <stdint.h>

#include "Clock_Command.hpp"
#include "File_Management.hpp"
#include "Matrix_Driver.hpp"
#include "Alarm_System.hpp"
#include "Sleep_Timer.hpp"

enum MenuScreen : uint8_t {
  MENU_SCREEN_CLOCK,
  MENU_SCREEN_LIST,
  MENU_SCREEN_CATEGORY,
  MENU_SCREEN_SETTINGS,
  MENU_SCREEN_CLOCK_EDITOR,
  MENU_SCREEN_CLOCK_RESULT,
  MENU_SCREEN_DISPLAY_FORMAT,
  MENU_SCREEN_DISPLAY_BRIGHTNESS,
  MENU_SCREEN_DARK_SETTINGS,
  MENU_SCREEN_DARK_EDITOR,
  MENU_SCREEN_STORAGE_ERROR,
  MENU_SCREEN_LIGHT,
  MENU_SCREEN_LIGHT_EDITOR,
  MENU_SCREEN_LIGHT_ERROR,
  MENU_SCREEN_ALARMS,
  MENU_SCREEN_ALARM,
  MENU_SCREEN_ALARM_EDITOR,
  MENU_SCREEN_ALARM_DAYS,
  MENU_SCREEN_ALARM_RESULT,
  MENU_SCREEN_ALARM_SOUND,
  MENU_SCREEN_SLEEP,
  MENU_SCREEN_SLEEP_EDITOR,
  MENU_SCREEN_SLEEP_SOUND,
  MENU_SCREEN_SLEEP_STATUS,
  MENU_SCREEN_SLEEP_RESULT,
  MENU_SCREEN_SLEEP_SLOTS
};

enum MenuEvent : uint8_t {
  MENU_EVENT_NONE,
  MENU_EVENT_ROTATE,
  MENU_EVENT_SELECT,
  MENU_EVENT_BACK
};

enum MenuItem : uint8_t {
  MENU_ITEM_CLOCK,
  MENU_ITEM_ALARMS,
  MENU_ITEM_SLEEP,
  MENU_ITEM_LIGHT,
  MENU_ITEM_SETTINGS,
  MENU_ITEM_BACK,
  MENU_ITEM_COUNT
};

enum ClockEditField : uint8_t {
  CLOCK_EDIT_YEAR,
  CLOCK_EDIT_MONTH,
  CLOCK_EDIT_DAY,
  CLOCK_EDIT_HOUR,
  CLOCK_EDIT_MINUTE,
  CLOCK_EDIT_SECOND,
  CLOCK_EDIT_FIELD_COUNT
};

enum LightAction : uint8_t {
  LIGHT_ACTION_NONE,
  LIGHT_ACTION_START,
  LIGHT_ACTION_STOP
};

enum class LightResult { ok, busy, failed };

enum AlarmField : uint8_t {
  ALARM_ENABLED, ALARM_HOUR, ALARM_MINUTE, ALARM_ONESHOT, ALARM_DAYS,
  ALARM_EFFECT, ALARM_FADE_IN, ALARM_HOLD, ALARM_FADE_OUT, ALARM_RED, ALARM_GREEN,
  ALARM_BLUE, ALARM_WHITE, ALARM_BRIGHTNESS,
  ALARM_SOUND, ALARM_VOLUME,
  ALARM_TEST, ALARM_BACK, ALARM_FIELD_COUNT
};

enum AlarmAction : uint8_t {
  ALARM_ACTION_NONE, ALARM_ACTION_SAVE, ALARM_ACTION_START,
  ALARM_ACTION_SNOOZE, ALARM_ACTION_DISMISS, ALARM_ACTION_DIM
};

enum SleepField : uint8_t {
  SLEEP_START, SLEEP_STATUS, SLEEP_HOLD, SLEEP_OUTPUT,
  SLEEP_EFFECT, SLEEP_FADE_OUT, SLEEP_RED, SLEEP_GREEN,
  SLEEP_BLUE, SLEEP_WHITE, SLEEP_BRIGHTNESS,
  SLEEP_SOUND, SLEEP_VOLUME,
  SLEEP_BACK, SLEEP_FIELD_COUNT
};

enum SleepAction : uint8_t {
  SLEEP_ACTION_NONE, SLEEP_ACTION_START, SLEEP_ACTION_STOP, SLEEP_ACTION_SAVE
};

// REQ-003: menu items share one state machine with a path back to clock.
struct MenuState {
  MenuScreen screen;
  MenuItem selected_item;
  uint8_t settings_selection;
  uint8_t dark_selection;
  ClockEditField clock_edit_field;
  clock_command::DateTime clock_candidate;
  bool clock_save_requested;
  clock_command::Result clock_save_result;
  settings::DisplaySettings display_settings;
  settings::DisplaySettings display_candidate;
  bool display_save_requested;
  bool storage_error_from_editor;
  uint8_t light_selection;
  lighting::AnimationConfig light_config;
  lighting::AnimationConfig light_applied_config;
  LightAction light_action;
  LightResult light_result = LightResult::ok;
  uint8_t alarm_slot;
  AlarmField alarm_field;
  uint8_t alarm_day;
  alarms::Config alarm_candidate;
  AlarmAction alarm_action;
  alarms::Result alarm_result;
  static constexpr uint8_t AUDIO_CHOICES = 12;
  char audio_paths[AUDIO_CHOICES][audio::PATH_CAPACITY] = {};
  uint8_t audio_count = 0;
  uint8_t audio_selection = 0;
  bool audio_more = false;
  SleepField sleep_field = SLEEP_START;
  uint8_t sleep_slot = 0;
  sleep_timer::Config sleep_candidate = sleep_timer::defaults();
  SleepAction sleep_action = SLEEP_ACTION_NONE;
  sleep_timer::Result sleep_result = sleep_timer::Result::ok;
  sleep_timer::Status sleep_status = {};
  bool sleep_audio_fault = false;
  uint32_t last_interaction_ms = 0;
  bool idle_armed = false;
};

void init_MenuState(MenuState* menu);
void update_MenuState(MenuState* menu, MenuEvent event, int32_t encoder_delta,
                      uint32_t now = 0);
bool update_MenuIdle(MenuState* menu, uint32_t now);
void cancel_MenuState(MenuState* menu);
void beginClockEditor(MenuState* menu,
                      const clock_command::DateTime& initial_value,
                      bool initial_value_valid);
bool takeClockSaveRequest(MenuState* menu,
                          clock_command::DateTime* candidate);
void completeClockSave(MenuState* menu, clock_command::Result result);
bool takeDisplaySaveRequest(MenuState* menu, settings::DisplaySettings* value);
void completeDisplaySave(MenuState* menu, bool success);
LightAction takeLightAction(MenuState* menu, lighting::AnimationConfig* config);
void completeLightAction(MenuState* menu, LightResult result);
void beginAlarmEditor(MenuState* menu, const alarms::Config& config);
AlarmAction takeAlarmAction(MenuState* menu);
void completeAlarmAction(MenuState* menu, alarms::Result result);
void completeAlarmSave(MenuState* menu, alarms::Result result,
                       const alarms::Config& stored);
void completeSleepSave(MenuState* menu, sleep_timer::Result result,
                       const sleep_timer::Config& stored);
const char* menu_item_text(MenuItem item);
const char* menu_display_text(const MenuState* menu);
bool menu_screen_shows_colon(MenuScreen screen);
