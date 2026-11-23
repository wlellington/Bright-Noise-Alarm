#include "Menu_System.hpp"

#include <stdio.h>
#include <string.h>

namespace {

static const char* SETTING_LABELS[] = {
    "Set time", "Time format", "Brightness", "Dark hours", "Back"};
static const char* DARK_LABELS[] = {
    "Enabled", "Start hour", "Start minute", "End hour", "End minute",
    "Wake seconds", "Back"};
static const char* LIGHT_LABELS[] = {
    "Sunrise", "Solid color", "Forest", "Stop lights", "Fade in", "Fade out",
    "Red", "Green", "Blue", "White", "Brightness",
    "Waves", "Fire", "Nebula", "Back"};
static const uint8_t LIGHT_COUNT = sizeof(LIGHT_LABELS) / sizeof(LIGHT_LABELS[0]);
static const char* ALARM_LABELS[] = {
    "Enabled", "Hour", "Minute", "Repeat mode", "Days", "Animation",
    "Fade in", "Hold minutes", "Fade out", "Red", "Green", "Blue", "White", "Brightness",
    "Sound", "Volume", "Start alarm", "Back"};
static const char* DAY_LABELS[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static const char* SLEEP_LABELS[] = {
    "Start sleep", "Time left", "Hold minutes", "Outputs", "Animation",
    "Fade out", "Red", "Green", "Blue", "White", "Brightness",
    "Sound", "Volume", "Back"};

static const char* effect_label(lighting::Effect effect) {
  if (effect == lighting::Effect::sunrise) return "Sunrise";
  if (effect == lighting::Effect::solid) return "Solid color";
  if (effect == lighting::Effect::forest) return "Forest";
  if (effect == lighting::Effect::beach) return "Waves";
  if (effect == lighting::Effect::fire) return "Fire";
  if (effect == lighting::Effect::nebula) return "Nebula";
  return "Invalid effect";
}

static int32_t wrap_value(int32_t value, int32_t delta, int32_t minimum,
                          int32_t maximum) {
  const int32_t range = maximum - minimum + 1;
  const int32_t offset = delta % range;
  return minimum + (value - minimum + offset + range) % range;
}

static bool adjust_fade(uint8_t& value, int32_t delta) {
  for (uint8_t index = 0; index < lighting::FADE_OPTION_COUNT; ++index) {
    uint8_t choice = 0;
    if (lighting::fadeMinutes(index, choice) && choice == value) {
      return lighting::fadeMinutes(static_cast<uint8_t>(wrap_value(
          index, delta, 0, lighting::FADE_OPTION_COUNT - 1)), value);
    }
  }
  return false;
}

static bool adjust_effect(lighting::Effect& effect, int32_t delta) {
  for (uint8_t i = 0; i < lighting::ANIMATION_OPTION_COUNT; ++i) {
    if (effect == lighting::animationEffect(i)) {
      effect = lighting::animationEffect(static_cast<uint8_t>(
          wrap_value(i, delta, 0, lighting::ANIMATION_OPTION_COUNT - 1)));
      return true;
    }
  }
  return false;
}

static void adjust_sleep(MenuState* menu, int32_t delta) {
  sleep_timer::Config& config = menu->sleep_candidate;
  bool good = true;
  switch (menu->sleep_field) {
    case SLEEP_HOLD:
      config.hold_minutes = static_cast<uint16_t>(
          wrap_value(config.hold_minutes, delta, 0, 180));
      break;
    case SLEEP_OUTPUT: {
      const int32_t choice = config.light_enabled ? (config.audio_enabled ? 2 : 0) : 1;
      const int32_t selected = wrap_value(choice, delta, 0, 2);
      config.light_enabled = selected != 1;
      config.audio_enabled = selected != 0;
      break;
    }
    case SLEEP_EFFECT: good = adjust_effect(config.light.effect, delta); break;
    case SLEEP_FADE_OUT: good = adjust_fade(config.light.fade_out_minutes, delta); break;
    case SLEEP_VOLUME:
      config.sound.volume = static_cast<uint8_t>(
          wrap_value(config.sound.volume, delta, 0, audio::VOLUME_LIMIT));
      break;
    default: {
      uint8_t* value = nullptr;
      switch (menu->sleep_field) {
        case SLEEP_RED: value = &config.light.color.red; break;
        case SLEEP_GREEN: value = &config.light.color.green; break;
        case SLEEP_BLUE: value = &config.light.color.blue; break;
        case SLEEP_WHITE: value = &config.light.color.white; break;
        case SLEEP_BRIGHTNESS: value = &config.light.brightness; break;
        default: good = false; break;
      }
      if (value) *value = static_cast<uint8_t>(wrap_value(*value, delta, 0, 255));
      break;
    }
  }
  if (!good) {
    menu->sleep_result = sleep_timer::Result::invalid_value;
    menu->screen = MENU_SCREEN_SLEEP_RESULT;
  }
}

static void adjust_alarm(MenuState* menu, int32_t delta) {
  alarms::Config& config = menu->alarm_candidate;
  switch (menu->alarm_field) {
    case ALARM_ENABLED:
      if (delta % 2) config.enabled = !config.enabled;
      break;
    case ALARM_ONESHOT:
      if (delta % 2) config.one_shot = !config.one_shot;
      break;
    case ALARM_HOUR:
      config.minute = static_cast<uint16_t>(
          wrap_value(config.minute / 60, delta, 0, 23) * 60 + config.minute % 60);
      break;
    case ALARM_MINUTE:
      config.minute = static_cast<uint16_t>(
          config.minute / 60 * 60 + wrap_value(config.minute % 60, delta, 0, 59));
      break;
    case ALARM_HOLD:
      config.hold_minutes = static_cast<uint16_t>(
          wrap_value(config.hold_minutes, delta, 0, 180));
      break;
    case ALARM_VOLUME:
      config.sound.volume = static_cast<uint8_t>(
          wrap_value(config.sound.volume, delta, 0, audio::VOLUME_LIMIT));
      break;
    case ALARM_FADE_IN:
    case ALARM_FADE_OUT:
      if (!adjust_fade(menu->alarm_field == ALARM_FADE_IN
                           ? config.light.fade_in_minutes
                           : config.light.fade_out_minutes, delta)) {
        menu->alarm_result = alarms::Result::invalid_value;
        menu->screen = MENU_SCREEN_ALARM_RESULT;
      }
      break;
    case ALARM_EFFECT: {
      if (!adjust_effect(config.light.effect, delta)) {
        menu->alarm_result = alarms::Result::invalid_value;
        menu->screen = MENU_SCREEN_ALARM_RESULT;
      }
      break;
    }
    default: {
      uint8_t* value = nullptr;
      switch (menu->alarm_field) {
        case ALARM_RED: value = &config.light.color.red; break;
        case ALARM_GREEN: value = &config.light.color.green; break;
        case ALARM_BLUE: value = &config.light.color.blue; break;
        case ALARM_WHITE: value = &config.light.color.white; break;
        case ALARM_BRIGHTNESS: value = &config.light.brightness; break;
        default: break;
      }
      if (value) *value = static_cast<uint8_t>(wrap_value(*value, delta, 0, 255));
      break;
    }
  }
}

static void adjust_clock_field(MenuState* menu, int32_t delta) {
  clock_command::DateTime& value = menu->clock_candidate;
  switch (menu->clock_edit_field) {
    case CLOCK_EDIT_YEAR:
      value.year = static_cast<uint16_t>(
          wrap_value(value.year, delta, 2000, 2099));
      break;
    case CLOCK_EDIT_MONTH:
      value.month = static_cast<uint8_t>(
          wrap_value(value.month, delta, 1, 12));
      break;
    case CLOCK_EDIT_DAY:
      value.day = static_cast<uint8_t>(
          wrap_value(value.day, delta, 1,
                     clock_command::daysInMonth(value.year, value.month)));
      break;
    case CLOCK_EDIT_HOUR:
      value.hour = static_cast<uint8_t>(
          wrap_value(value.hour, delta, 0, 23));
      break;
    case CLOCK_EDIT_MINUTE:
      value.minute = static_cast<uint8_t>(
          wrap_value(value.minute, delta, 0, 59));
      break;
    case CLOCK_EDIT_SECOND:
      value.second = static_cast<uint8_t>(
          wrap_value(value.second, delta, 0, 59));
      break;
    case CLOCK_EDIT_FIELD_COUNT:
      break;
  }
  const uint8_t last_day = clock_command::daysInMonth(value.year, value.month);
  if (value.day > last_day) value.day = last_day;
}

static const char* clock_editor_text(const MenuState* menu) {
  static char text[24];
  const clock_command::DateTime& value = menu->clock_candidate;
  switch (menu->clock_edit_field) {
    case CLOCK_EDIT_YEAR:
      snprintf(text, sizeof(text), "%04u",
               static_cast<unsigned int>(value.year));
      break;
    case CLOCK_EDIT_MONTH:
      snprintf(text, sizeof(text), "Mo%02u",
               static_cast<unsigned int>(value.month));
      break;
    case CLOCK_EDIT_DAY:
      snprintf(text, sizeof(text), "Dy%02u",
               static_cast<unsigned int>(value.day));
      break;
    case CLOCK_EDIT_HOUR:
      snprintf(text, sizeof(text), "%02u",
               static_cast<unsigned int>(value.hour));
      break;
    case CLOCK_EDIT_MINUTE:
      snprintf(text, sizeof(text), "%02u",
               static_cast<unsigned int>(value.minute));
      break;
    case CLOCK_EDIT_SECOND:
      snprintf(text, sizeof(text), "Sc%02u",
               static_cast<unsigned int>(value.second));
      break;
    case CLOCK_EDIT_FIELD_COUNT:
      text[0] = '\0';
      break;
  }
  return text;
}

}

static void rotate_selection(MenuState* menu, int32_t encoder_delta) {
  const int32_t item_count = MENU_ITEM_COUNT;
  const int32_t offset = encoder_delta % item_count;
  const int32_t current = menu->selected_item;
  menu->selected_item =
      static_cast<MenuItem>((current + offset + item_count) % item_count);
}

void init_MenuState(MenuState* menu) {
  menu->screen = MENU_SCREEN_CLOCK;
  menu->selected_item = MENU_ITEM_CLOCK;
  menu->settings_selection = 0;
  menu->dark_selection = 0;
  menu->clock_edit_field = CLOCK_EDIT_YEAR;
  menu->clock_candidate = {2000, 1, 1, 0, 0, 0};
  menu->clock_save_requested = false;
  menu->clock_save_result = clock_command::Result::success;
  menu->display_settings = settings::defaults();
  menu->display_candidate = menu->display_settings;
  menu->display_save_requested = false;
  menu->storage_error_from_editor = false;
  menu->light_selection = 0;
  menu->light_config = {lighting::Effect::sunrise, {255, 180, 64, 255},
                        255, 1, 0};
  menu->light_applied_config = menu->light_config;
  menu->light_action = LIGHT_ACTION_NONE;
  menu->light_result = LightResult::ok;
  menu->alarm_slot = 0;
  menu->alarm_field = ALARM_ENABLED;
  menu->alarm_day = 0;
  menu->alarm_candidate = alarms::defaults();
  menu->alarm_action = ALARM_ACTION_NONE;
  menu->alarm_result = alarms::Result::ok;
  menu->sleep_candidate = sleep_timer::defaults();
  menu->sleep_field = SLEEP_START;
  menu->sleep_slot = 0;
  menu->sleep_action = SLEEP_ACTION_NONE;
  menu->sleep_result = sleep_timer::Result::ok;
  menu->sleep_status = {};
  menu->sleep_audio_fault = false;
  menu->last_interaction_ms = 0;
  menu->idle_armed = false;
}

void cancel_MenuState(MenuState* menu) {
  menu->screen = MENU_SCREEN_CLOCK;
  menu->idle_armed = false;
  menu->clock_save_requested = false;
  menu->display_save_requested = false;
  menu->light_action = LIGHT_ACTION_NONE;
  menu->alarm_action = ALARM_ACTION_NONE;
  menu->sleep_action = SLEEP_ACTION_NONE;
  menu->display_candidate = menu->display_settings;
  menu->light_config = menu->light_applied_config;
  menu->clock_candidate = {2000, 1, 1, 0, 0, 0};
  menu->alarm_candidate = alarms::defaults();
  menu->sleep_candidate = sleep_timer::defaults();
}

bool update_MenuIdle(MenuState* menu, uint32_t now) {
  if (menu->screen == MENU_SCREEN_CLOCK) {
    menu->idle_armed = false;
  } else if (!menu->idle_armed) {
    menu->last_interaction_ms = now;
    menu->idle_armed = true;
  } else if (static_cast<uint32_t>(now - menu->last_interaction_ms) >= 60000u) {
    cancel_MenuState(menu);
    return true;
  }
  return false;
}

void update_MenuState(MenuState* menu, MenuEvent event, int32_t encoder_delta,
                      uint32_t now) {
  if (menu->clock_save_requested || menu->display_save_requested ||
      menu->alarm_action != ALARM_ACTION_NONE ||
      menu->sleep_action != SLEEP_ACTION_NONE) {
    return;
  }
  if (event == MENU_EVENT_NONE ||
      (event == MENU_EVENT_ROTATE && encoder_delta == 0)) return;
  menu->last_interaction_ms = now;
  menu->idle_armed = true;
  switch (menu->screen) {
    case MENU_SCREEN_CLOCK:
      if (event == MENU_EVENT_SELECT) {
        menu->screen = MENU_SCREEN_LIST;
      } else if (event == MENU_EVENT_ROTATE && encoder_delta != 0) {
        menu->screen = MENU_SCREEN_LIST;
        rotate_selection(menu, encoder_delta);
      }
      break;

    case MENU_SCREEN_LIST:
      if (event == MENU_EVENT_BACK) {
        menu->screen = MENU_SCREEN_CLOCK;
      } else if (event == MENU_EVENT_ROTATE) {
        rotate_selection(menu, encoder_delta);
      } else if (event == MENU_EVENT_SELECT) {
        if (menu->selected_item == MENU_ITEM_CLOCK ||
            menu->selected_item == MENU_ITEM_BACK) {
          menu->screen = MENU_SCREEN_CLOCK;
        } else if (menu->selected_item == MENU_ITEM_SETTINGS) {
          menu->settings_selection = 0;
          menu->screen = MENU_SCREEN_SETTINGS;
        } else if (menu->selected_item == MENU_ITEM_LIGHT) {
          menu->light_selection = 0;
          menu->screen = MENU_SCREEN_LIGHT;
        } else if (menu->selected_item == MENU_ITEM_ALARMS) {
          menu->alarm_slot = 0;
          menu->screen = MENU_SCREEN_ALARMS;
        } else if (menu->selected_item == MENU_ITEM_SLEEP) {
          menu->sleep_field = SLEEP_START;
          menu->sleep_slot = 0;
          menu->screen = MENU_SCREEN_SLEEP_SLOTS;
        } else {
          menu->screen = MENU_SCREEN_CATEGORY;
        }
      }
      break;

    case MENU_SCREEN_ALARMS:
      if (event == MENU_EVENT_BACK) menu->screen = MENU_SCREEN_LIST;
      else if (event == MENU_EVENT_ROTATE) {
        menu->alarm_slot = static_cast<uint8_t>(
            wrap_value(menu->alarm_slot, encoder_delta, 0, 8));
      } else if (event == MENU_EVENT_SELECT) {
        if (menu->alarm_slot < alarms::SLOT_COUNT) {
          menu->alarm_field = ALARM_ENABLED;
          menu->screen = MENU_SCREEN_ALARM;
        } else if (menu->alarm_slot == 8) {
          menu->screen = MENU_SCREEN_LIST;
        } else {
          menu->alarm_action = menu->alarm_slot == 5 ? ALARM_ACTION_SNOOZE :
              menu->alarm_slot == 6 ? ALARM_ACTION_DISMISS : ALARM_ACTION_DIM;
        }
      }
      break;

    case MENU_SCREEN_ALARM:
      if (event == MENU_EVENT_BACK) menu->screen = MENU_SCREEN_ALARMS;
      else if (event == MENU_EVENT_ROTATE) {
        menu->alarm_field = static_cast<AlarmField>(wrap_value(
            menu->alarm_field, encoder_delta, 0, ALARM_FIELD_COUNT - 1));
      } else if (event == MENU_EVENT_SELECT) {
        if (menu->alarm_field == ALARM_BACK) menu->screen = MENU_SCREEN_ALARMS;
        else if (menu->alarm_field == ALARM_TEST) {
          menu->alarm_action = ALARM_ACTION_START;
        } else if (menu->alarm_field == ALARM_DAYS) {
          menu->alarm_day = 0;
          menu->screen = MENU_SCREEN_ALARM_DAYS;
        } else if (menu->alarm_field == ALARM_SOUND) {
          menu->audio_selection = 0;
          menu->screen = MENU_SCREEN_ALARM_SOUND;
        } else menu->screen = MENU_SCREEN_ALARM_EDITOR;
      }
      break;

    case MENU_SCREEN_ALARM_SOUND:
    case MENU_SCREEN_SLEEP_SOUND: {
      const bool sleep = menu->screen == MENU_SCREEN_SLEEP_SOUND;
      const MenuScreen parent = sleep ? MENU_SCREEN_SLEEP : MENU_SCREEN_ALARM;
      if (event == MENU_EVENT_BACK) menu->screen = parent;
      else if (event == MENU_EVENT_ROTATE) {
        menu->audio_selection = static_cast<uint8_t>(wrap_value(
            menu->audio_selection, encoder_delta, 0,
            menu->audio_count + 1 + (menu->audio_more ? 1 : 0)));
      } else if (event == MENU_EVENT_SELECT) {
        if (menu->audio_selection <= menu->audio_count) {
          char* path = sleep ? menu->sleep_candidate.sound.path :
                              menu->alarm_candidate.sound.path;
          memset(path, 0, audio::PATH_CAPACITY);
          if (menu->audio_selection != 0)
            strcpy(path, menu->audio_paths[menu->audio_selection - 1]);
          menu->screen = parent;
          if (sleep) menu->sleep_action = SLEEP_ACTION_SAVE;
          else menu->alarm_action = ALARM_ACTION_SAVE;
        } else if (menu->audio_selection == menu->audio_count + 1)
          menu->screen = parent;
      }
      break;
    }

    case MENU_SCREEN_SLEEP:
      if (event == MENU_EVENT_BACK) menu->screen = MENU_SCREEN_SLEEP_SLOTS;
      else if (event == MENU_EVENT_ROTATE)
        menu->sleep_field = static_cast<SleepField>(wrap_value(
            menu->sleep_field, encoder_delta, 0, SLEEP_FIELD_COUNT - 1));
      else if (event == MENU_EVENT_SELECT) {
        if (menu->sleep_field == SLEEP_BACK) menu->screen = MENU_SCREEN_SLEEP_SLOTS;
        else if (menu->sleep_field == SLEEP_START) menu->sleep_action = SLEEP_ACTION_START;
        else if (menu->sleep_field == SLEEP_STATUS) menu->screen = MENU_SCREEN_CLOCK;
        else if (menu->sleep_field == SLEEP_SOUND) {
          menu->audio_selection = 0;
          menu->screen = MENU_SCREEN_SLEEP_SOUND;
        } else menu->screen = MENU_SCREEN_SLEEP_EDITOR;
      }
      break;

    case MENU_SCREEN_SLEEP_SLOTS:
      if (event == MENU_EVENT_BACK) menu->screen = MENU_SCREEN_LIST;
      else if (event == MENU_EVENT_ROTATE)
        menu->sleep_slot = static_cast<uint8_t>(
            wrap_value(menu->sleep_slot, encoder_delta, 0, 5));
      else if (event == MENU_EVENT_SELECT) {
        if (menu->sleep_slot < sleep_timer::SLOT_COUNT) {
          menu->sleep_field = SLEEP_START;
          menu->screen = MENU_SCREEN_SLEEP;
        } else if (menu->sleep_slot == 3) menu->sleep_action = SLEEP_ACTION_STOP;
        else if (menu->sleep_slot == 4) menu->screen = MENU_SCREEN_CLOCK;
        else menu->screen = MENU_SCREEN_LIST;
      }
      break;

    case MENU_SCREEN_SLEEP_EDITOR:
      if (event == MENU_EVENT_SELECT || event == MENU_EVENT_BACK)
        menu->screen = MENU_SCREEN_SLEEP;
      else if (event == MENU_EVENT_ROTATE) {
        adjust_sleep(menu, encoder_delta);
        if (menu->screen == MENU_SCREEN_SLEEP_EDITOR)
          menu->sleep_action = SLEEP_ACTION_SAVE;
      }
      break;

    case MENU_SCREEN_SLEEP_STATUS:
    case MENU_SCREEN_SLEEP_RESULT:
      if (event == MENU_EVENT_SELECT || event == MENU_EVENT_BACK)
        menu->screen = menu->sleep_slot < sleep_timer::SLOT_COUNT
            ? MENU_SCREEN_SLEEP : MENU_SCREEN_SLEEP_SLOTS;
      break;

    case MENU_SCREEN_ALARM_EDITOR:
      if (event == MENU_EVENT_SELECT || event == MENU_EVENT_BACK) {
        menu->screen = MENU_SCREEN_ALARM;
      } else if (event == MENU_EVENT_ROTATE) {
        adjust_alarm(menu, encoder_delta);
        if (menu->screen == MENU_SCREEN_ALARM_EDITOR)
          menu->alarm_action = ALARM_ACTION_SAVE;
      }
      break;

    case MENU_SCREEN_ALARM_DAYS:
      if (event == MENU_EVENT_BACK) menu->screen = MENU_SCREEN_ALARM;
      else if (event == MENU_EVENT_ROTATE) {
        menu->alarm_day = static_cast<uint8_t>(
            wrap_value(menu->alarm_day, encoder_delta, 0, 7));
      } else if (event == MENU_EVENT_SELECT) {
        if (menu->alarm_day == 7) menu->screen = MENU_SCREEN_ALARM;
        else {
          menu->alarm_candidate.days ^= static_cast<uint8_t>(1u << menu->alarm_day);
          menu->alarm_action = ALARM_ACTION_SAVE;
        }
      }
      break;

    case MENU_SCREEN_ALARM_RESULT:
      if (event == MENU_EVENT_SELECT || event == MENU_EVENT_BACK) {
        menu->screen = menu->alarm_slot < alarms::SLOT_COUNT
            ? MENU_SCREEN_ALARM : MENU_SCREEN_ALARMS;
      }
      break;

    case MENU_SCREEN_CATEGORY:
      if (event == MENU_EVENT_BACK || event == MENU_EVENT_SELECT) {
        menu->screen = MENU_SCREEN_LIST;
      }
      break;

    case MENU_SCREEN_LIGHT:
      if (event == MENU_EVENT_BACK) {
        menu->screen = MENU_SCREEN_LIST;
      } else if (event == MENU_EVENT_ROTATE) {
        menu->light_selection = static_cast<uint8_t>(
            wrap_value(menu->light_selection, encoder_delta, 0,
                       LIGHT_COUNT - 1));
      } else if (event == MENU_EVENT_SELECT) {
        if (menu->light_selection < 3 ||
            (menu->light_selection >= 11 && menu->light_selection <= 13)) {
          menu->light_config.effect = lighting::animationEffect(
              menu->light_selection < 3 ? menu->light_selection :
                                         menu->light_selection - 8);
          menu->light_action = LIGHT_ACTION_START;
        } else if (menu->light_selection == 3) {
          menu->light_action = LIGHT_ACTION_STOP;
        } else if (menu->light_selection == LIGHT_COUNT - 1) {
          menu->screen = MENU_SCREEN_LIST;
        } else {
          menu->screen = MENU_SCREEN_LIGHT_EDITOR;
        }
      }
      break;

    case MENU_SCREEN_LIGHT_EDITOR:
      if (event == MENU_EVENT_BACK || event == MENU_EVENT_SELECT) {
        menu->screen = MENU_SCREEN_LIGHT;
      } else if (event == MENU_EVENT_ROTATE && encoder_delta != 0) {
        if (menu->light_selection == 4 || menu->light_selection == 5) {
          uint8_t& value = menu->light_selection == 4
                               ? menu->light_config.fade_in_minutes
                               : menu->light_config.fade_out_minutes;
          if (!adjust_fade(value, encoder_delta)) {
            menu->screen = MENU_SCREEN_LIGHT_ERROR;
          }
        } else {
          uint8_t* value = nullptr;
          switch (menu->light_selection) {
            case 6: value = &menu->light_config.color.red; break;
            case 7: value = &menu->light_config.color.green; break;
            case 8: value = &menu->light_config.color.blue; break;
            case 9: value = &menu->light_config.color.white; break;
            case 10: value = &menu->light_config.brightness; break;
            default: menu->screen = MENU_SCREEN_LIGHT_ERROR; break;
          }
          if (value != nullptr) {
            *value = static_cast<uint8_t>(
                wrap_value(*value, encoder_delta, 0, 255));
          }
        }
      }
      break;

    case MENU_SCREEN_LIGHT_ERROR:
      if (event == MENU_EVENT_SELECT || event == MENU_EVENT_BACK) {
        menu->screen = MENU_SCREEN_LIGHT;
      }
      break;

    case MENU_SCREEN_SETTINGS:
      if (event == MENU_EVENT_BACK) {
        menu->screen = MENU_SCREEN_LIST;
      } else if (event == MENU_EVENT_ROTATE) {
        menu->settings_selection = static_cast<uint8_t>(
            (menu->settings_selection + encoder_delta % 5 + 5) % 5);
      } else if (event == MENU_EVENT_SELECT) {
        if (menu->settings_selection == 0) {
          menu->screen = MENU_SCREEN_CLOCK_EDITOR;
          menu->clock_edit_field = CLOCK_EDIT_YEAR;
        } else if (menu->settings_selection == 1) {
          menu->screen = MENU_SCREEN_DISPLAY_FORMAT;
        } else if (menu->settings_selection == 2) {
          menu->screen = MENU_SCREEN_DISPLAY_BRIGHTNESS;
        } else if (menu->settings_selection == 3) {
          menu->dark_selection = 0;
          menu->screen = MENU_SCREEN_DARK_SETTINGS;
        } else {
          menu->screen = MENU_SCREEN_LIST;
        }
      }
      break;

    case MENU_SCREEN_DARK_SETTINGS:
      if (event == MENU_EVENT_BACK) {
        menu->screen = MENU_SCREEN_SETTINGS;
      } else if (event == MENU_EVENT_ROTATE) {
        menu->dark_selection = static_cast<uint8_t>(
            (menu->dark_selection + encoder_delta % 7 + 7) % 7);
      } else if (event == MENU_EVENT_SELECT) {
        menu->screen = menu->dark_selection == 6
                           ? MENU_SCREEN_SETTINGS
                           : MENU_SCREEN_DARK_EDITOR;
      }
      break;

    case MENU_SCREEN_DARK_EDITOR:
      if (event == MENU_EVENT_BACK || event == MENU_EVENT_SELECT) {
        menu->screen = MENU_SCREEN_DARK_SETTINGS;
      } else if (event == MENU_EVENT_ROTATE && encoder_delta != 0) {
        menu->display_candidate = menu->display_settings;
        if (menu->dark_selection == 0) {
          if (encoder_delta % 2 == 0) break;
          menu->display_candidate.dark_enabled =
              !menu->display_candidate.dark_enabled;
        } else if (menu->dark_selection == 5) {
          menu->display_candidate.wake_seconds = static_cast<uint16_t>(
              wrap_value(menu->display_settings.wake_seconds,
                         encoder_delta, 1, 300));
          if (menu->display_candidate.wake_seconds ==
              menu->display_settings.wake_seconds) break;
        } else {
          uint16_t& time = menu->dark_selection <= 2
                               ? menu->display_candidate.dark_start
                               : menu->display_candidate.dark_end;
          const uint16_t old_time = time;
          const bool hour = menu->dark_selection == 1 ||
                            menu->dark_selection == 3;
          const uint16_t hours = time / 60;
          const uint16_t minutes = time % 60;
          time = hour ? static_cast<uint16_t>(
                            wrap_value(hours, encoder_delta, 0, 23) * 60 +
                            minutes)
                      : static_cast<uint16_t>(
                            hours * 60 +
                            wrap_value(minutes, encoder_delta, 0, 59));
          if (time == old_time) break;
        }
        menu->display_save_requested = true;
      }
      break;

    case MENU_SCREEN_CLOCK_EDITOR:
      if (event == MENU_EVENT_BACK) {
        menu->screen = MENU_SCREEN_SETTINGS;
      } else if (event == MENU_EVENT_ROTATE) {
        adjust_clock_field(menu, encoder_delta);
        menu->clock_save_requested = true;
      } else if (event == MENU_EVENT_SELECT) {
        if (menu->clock_edit_field == CLOCK_EDIT_SECOND) {
          menu->screen = MENU_SCREEN_SETTINGS;
        } else {
          menu->clock_edit_field = static_cast<ClockEditField>(
              menu->clock_edit_field + 1);
        }
      }
      break;

    case MENU_SCREEN_CLOCK_RESULT:
      if (event == MENU_EVENT_BACK) {
        menu->screen = MENU_SCREEN_SETTINGS;
      } else if (event == MENU_EVENT_SELECT) {
        menu->screen =
            menu->clock_save_result == clock_command::Result::invalid_date_time
                ? MENU_SCREEN_CLOCK_EDITOR
                : MENU_SCREEN_SETTINGS;
      }
      break;

    case MENU_SCREEN_DISPLAY_FORMAT:
    case MENU_SCREEN_DISPLAY_BRIGHTNESS:
      if (event == MENU_EVENT_BACK || event == MENU_EVENT_SELECT) {
        menu->screen = MENU_SCREEN_SETTINGS;
      } else if (event == MENU_EVENT_ROTATE && encoder_delta != 0) {
        menu->display_candidate = menu->display_settings;
        if (menu->screen == MENU_SCREEN_DISPLAY_FORMAT) {
          if (encoder_delta % 2 == 0) {
            break;
          }
          menu->display_candidate.use_12_hour =
              !menu->display_candidate.use_12_hour;
        } else {
          menu->display_candidate.brightness = static_cast<uint8_t>(
              wrap_value(menu->display_settings.brightness, encoder_delta,
                         0, 15));
          if (menu->display_candidate.brightness ==
              menu->display_settings.brightness) {
            break;
          }
        }
        menu->display_save_requested = true;
      }
      break;

    case MENU_SCREEN_STORAGE_ERROR:
      if (event == MENU_EVENT_SELECT || event == MENU_EVENT_BACK) {
        menu->screen = menu->storage_error_from_editor
                           ? MENU_SCREEN_SETTINGS
                           : MENU_SCREEN_CLOCK;
      }
      break;
  }
}

void beginClockEditor(MenuState* menu,
                      const clock_command::DateTime& initial_value,
                      bool initial_value_valid) {
  menu->clock_candidate =
      initial_value_valid ? initial_value
                          : clock_command::DateTime{2000, 1, 1, 0, 0, 0};
  menu->clock_edit_field = CLOCK_EDIT_YEAR;
  menu->clock_save_requested = false;
  menu->clock_save_result = clock_command::Result::success;
}

bool takeClockSaveRequest(MenuState* menu,
                          clock_command::DateTime* candidate) {
  if (!menu->clock_save_requested) {
    return false;
  }
  menu->clock_save_requested = false;
  *candidate = menu->clock_candidate;
  return true;
}

void completeClockSave(MenuState* menu, clock_command::Result result) {
  menu->clock_save_result = result;
  if (result == clock_command::Result::invalid_date_time) {
    menu->clock_edit_field = CLOCK_EDIT_DAY;
  }
  if (result != clock_command::Result::success)
    menu->screen = MENU_SCREEN_CLOCK_RESULT;
}

bool takeDisplaySaveRequest(MenuState* menu, settings::DisplaySettings* value) {
  if (!menu->display_save_requested) {
    return false;
  }
  *value = menu->display_candidate;
  return true;
}

void completeDisplaySave(MenuState* menu, bool success) {
  menu->display_save_requested = false;
  if (success) {
    menu->display_settings = menu->display_candidate;
  } else {
    menu->storage_error_from_editor = true;
    menu->screen = MENU_SCREEN_STORAGE_ERROR;
  }
}

LightAction takeLightAction(MenuState* menu, lighting::AnimationConfig* config) {
  const LightAction action = menu->light_action;
  if (action == LIGHT_ACTION_START) *config = menu->light_config;
  menu->light_action = LIGHT_ACTION_NONE;
  return action;
}

void completeLightAction(MenuState* menu, LightResult result) {
  menu->light_result = result;
  if (result != LightResult::ok) menu->screen = MENU_SCREEN_LIGHT_ERROR;
}

void beginAlarmEditor(MenuState* menu, const alarms::Config& config) {
  menu->alarm_candidate = config;
  menu->alarm_field = ALARM_ENABLED;
  menu->alarm_result = alarms::Result::ok;
}

AlarmAction takeAlarmAction(MenuState* menu) {
  const AlarmAction action = menu->alarm_action;
  menu->alarm_action = ALARM_ACTION_NONE;
  return action;
}

void completeAlarmAction(MenuState* menu, alarms::Result result) {
  menu->alarm_result = result;
  menu->screen = MENU_SCREEN_ALARM_RESULT;
}

void completeAlarmSave(MenuState* menu, alarms::Result result,
                       const alarms::Config& stored) {
  menu->alarm_result = result;
  if (result != alarms::Result::ok) {
    menu->alarm_candidate = stored;
    menu->screen = MENU_SCREEN_ALARM_RESULT;
  }
}

void completeSleepSave(MenuState* menu, sleep_timer::Result result,
                       const sleep_timer::Config& stored) {
  menu->sleep_action = SLEEP_ACTION_NONE;
  menu->sleep_result = result;
  if (result != sleep_timer::Result::ok) {
    menu->sleep_candidate = stored;
    menu->screen = MENU_SCREEN_SLEEP_RESULT;
  }
}

const char* menu_item_text(MenuItem item) {
  switch (item) {
    case MENU_ITEM_CLOCK:
      return "Clock";
    case MENU_ITEM_ALARMS:
      return "Alarms";
    case MENU_ITEM_SLEEP:
      return "Sleep timers";
    case MENU_ITEM_LIGHT:
      return "Live lights";
    case MENU_ITEM_SETTINGS:
      return "Settings";
    case MENU_ITEM_BACK:
      return "Back";
    case MENU_ITEM_COUNT:
      break;
  }
  return "";
}

const char* menu_display_text(const MenuState* menu) {
  switch (menu->screen) {
    case MENU_SCREEN_SLEEP_SLOTS: {
      static const char* const labels[] = {"SLT1", "SLT2", "SLT3", "Stop sleep", "Time left", "Back"};
      return menu->sleep_slot < 6 ? labels[menu->sleep_slot] : "Err";
    }
    case MENU_SCREEN_SLEEP: return SLEEP_LABELS[menu->sleep_field];
    case MENU_SCREEN_SLEEP_STATUS:
      return menu->sleep_status.active ? "Sleep active" : "Off";
    case MENU_SCREEN_SLEEP_RESULT:
      return menu->sleep_result == sleep_timer::Result::ok ? "Saved" :
          menu->sleep_result == sleep_timer::Result::storage ? "SD error" :
          menu->sleep_result == sleep_timer::Result::unavailable ? "Unavailable" :
          menu->sleep_result == sleep_timer::Result::busy ? "Alarm active" : "Invalid setting";
    case MENU_SCREEN_SLEEP_EDITOR: {
      const sleep_timer::Config& config = menu->sleep_candidate;
      static char text[32];
      unsigned value = 0;
      switch (menu->sleep_field) {
        case SLEEP_OUTPUT: return config.light_enabled
            ? (config.audio_enabled ? "Both" : "Lights only") : "Audio only";
        case SLEEP_EFFECT: return effect_label(config.light.effect);
        case SLEEP_HOLD: value = config.hold_minutes; break;
        case SLEEP_FADE_OUT: value = config.light.fade_out_minutes; break;
        case SLEEP_RED: value = config.light.color.red; break;
        case SLEEP_GREEN: value = config.light.color.green; break;
        case SLEEP_BLUE: value = config.light.color.blue; break;
        case SLEEP_WHITE: value = config.light.color.white; break;
        case SLEEP_BRIGHTNESS: value = config.light.brightness; break;
        case SLEEP_VOLUME: value = config.sound.volume; break;
        default: return "Err";
      }
      snprintf(text, sizeof(text), "%u", value);
      return text;
    }
    case MENU_SCREEN_ALARMS: {
      static char text[16];
      if (menu->alarm_slot == 5) return "Snooze";
      if (menu->alarm_slot == 6) return "Dismiss";
      if (menu->alarm_slot == 7) return "Dim half";
      if (menu->alarm_slot == 8) return "Back";
      snprintf(text, sizeof(text), "ALM%u", static_cast<unsigned>(menu->alarm_slot + 1));
      return text;
    }
    case MENU_SCREEN_ALARM:
      return ALARM_LABELS[menu->alarm_field];
    case MENU_SCREEN_ALARM_SOUND:
    case MENU_SCREEN_SLEEP_SOUND:
      if (menu->audio_selection == 0) return "None";
      if (menu->audio_selection == menu->audio_count + 1) return "Back";
      if (menu->audio_selection > menu->audio_count) return "More";
      return menu->audio_paths[menu->audio_selection - 1] + 1;
    case MENU_SCREEN_ALARM_EDITOR: {
      static char text[32];
      const alarms::Config& config = menu->alarm_candidate;
      unsigned value = 0;
      switch (menu->alarm_field) {
        case ALARM_ENABLED: return config.enabled ? "On" : "Off";
        case ALARM_ONESHOT: return config.one_shot ? "Once" : "Repeating";
        case ALARM_EFFECT:
          return effect_label(config.light.effect);
        case ALARM_HOUR: value = config.minute / 60; break;
        case ALARM_MINUTE: value = config.minute % 60; break;
        case ALARM_FADE_IN: value = config.light.fade_in_minutes; break;
        case ALARM_FADE_OUT: value = config.light.fade_out_minutes; break;
        case ALARM_RED: value = config.light.color.red; break;
        case ALARM_GREEN: value = config.light.color.green; break;
        case ALARM_BLUE: value = config.light.color.blue; break;
        case ALARM_WHITE: value = config.light.color.white; break;
        case ALARM_BRIGHTNESS: value = config.light.brightness; break;
        case ALARM_HOLD: value = config.hold_minutes; break;
        case ALARM_VOLUME: value = config.sound.volume; break;
        default: return "Err";
      }
      snprintf(text, sizeof(text), "%u", value);
      return text;
    }
    case MENU_SCREEN_ALARM_DAYS: {
      if (menu->alarm_day == 7) return "Back";
      static char text[12];
      snprintf(text, sizeof(text), "%.2s%s", DAY_LABELS[menu->alarm_day],
               menu->alarm_candidate.days & (1u << menu->alarm_day) ? "On" : "--");
      return text;
    }
    case MENU_SCREEN_ALARM_RESULT:
      return menu->alarm_result == alarms::Result::ok ? "Done" :
          menu->alarm_result == alarms::Result::invalid_value ? "Invalid setting" :
          menu->alarm_result == alarms::Result::no_active ? "None" : "SD error";
    case MENU_SCREEN_LIST:
      return menu_item_text(menu->selected_item);
    case MENU_SCREEN_CATEGORY:
      return menu_item_text(menu->selected_item);
    case MENU_SCREEN_SETTINGS:
      return SETTING_LABELS[menu->settings_selection];
    case MENU_SCREEN_CLOCK_EDITOR:
      return clock_editor_text(menu);
    case MENU_SCREEN_CLOCK_RESULT:
      if (menu->clock_save_result == clock_command::Result::success) {
        return "Done";
      }
      if (menu->clock_save_result ==
          clock_command::Result::invalid_date_time) {
        return "Date";
      }
      return "RTC?";
    case MENU_SCREEN_CLOCK:
      return "Clock";
    case MENU_SCREEN_DISPLAY_FORMAT:
      return menu->display_settings.use_12_hour ? "12h" : "24h";
    case MENU_SCREEN_DISPLAY_BRIGHTNESS: {
      static char text[20];
      snprintf(text, sizeof(text), "%u",
               static_cast<unsigned>(menu->display_settings.brightness));
      return text;
    }
    case MENU_SCREEN_DARK_SETTINGS:
      return DARK_LABELS[menu->dark_selection];
    case MENU_SCREEN_DARK_EDITOR:
      if (menu->dark_selection == 5) {
        static char text[6];
        snprintf(text, sizeof(text), "%u",
                 static_cast<unsigned>(menu->display_settings.wake_seconds));
        return text;
      }
      return menu->display_settings.dark_enabled ? "On" : "Off";
    case MENU_SCREEN_STORAGE_ERROR:
      return "SD error";
    case MENU_SCREEN_LIGHT:
      return LIGHT_LABELS[menu->light_selection];
    case MENU_SCREEN_LIGHT_EDITOR: {
      static char text[24];
      unsigned value = 0;
      switch (menu->light_selection) {
        case 4: value = menu->light_config.fade_in_minutes; break;
        case 5: value = menu->light_config.fade_out_minutes; break;
        case 6: value = menu->light_config.color.red; break;
        case 7: value = menu->light_config.color.green; break;
        case 8: value = menu->light_config.color.blue; break;
        case 9: value = menu->light_config.color.white; break;
        case 10: value = menu->light_config.brightness; break;
        default: return "Invalid setting";
      }
      snprintf(text, sizeof(text), "%u", value);
      return text;
    }
    case MENU_SCREEN_LIGHT_ERROR:
      return menu->light_result == LightResult::busy
          ? "Timer active" : "Light error";
  }
  return "";
}

bool menu_screen_shows_colon(MenuScreen screen) {
  return screen == MENU_SCREEN_LIST || screen == MENU_SCREEN_SETTINGS ||
         screen == MENU_SCREEN_DARK_SETTINGS || screen == MENU_SCREEN_LIGHT ||
         screen == MENU_SCREEN_ALARMS || screen == MENU_SCREEN_ALARM ||
         screen == MENU_SCREEN_SLEEP || screen == MENU_SCREEN_SLEEP_SLOTS;
}
