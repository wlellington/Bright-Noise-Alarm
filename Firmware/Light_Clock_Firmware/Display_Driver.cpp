#include "Display_Driver.hpp"
#include "Clock_Display.hpp"
#include "Serial_Diagnostics.hpp"

#include <string.h>

void display_formatted_text(DisplayState* display, uint16_t* display_buffer,
                            bool show_colon) {
  display->sev_segment.writeDigitRaw(0, display_buffer[0]);
  display->sev_segment.writeDigitRaw(1, display_buffer[1]);
  display->sev_segment.writeDigitRaw(3, display_buffer[3]);
  display->sev_segment.writeDigitRaw(4, display_buffer[4]);
  display->sev_segment.drawColon(show_colon);
  display->sev_segment.writeDisplay();
}

void display_scrolling_text(DisplayState* display, const char* full_text,
                            bool show_colon) {
  static const uint16_t scroll_delays[] = {350, 250, 167};
  const unsigned long now = millis();
  const size_t full_length = strlen(full_text);
  const bool text_changed = strcmp(display->scroll_text, full_text) != 0;
  bool update_display = text_changed ||
                        display->last_scroll_colon != show_colon;

  if (text_changed) {
    strncpy(display->scroll_text, full_text,
            sizeof(display->scroll_text) - 1);
    display->scroll_text[sizeof(display->scroll_text) - 1] = '\0';
    display->scroll_position = 0;
    display->last_scroll = now;
  }

  char scroll_window[5];
  if (full_length <= 4) {
    menu_text_window(full_text, 0, scroll_window);
  } else {
    const uint8_t speed = display->scroll_speed <= SCROLL_SPEED_FAST
                              ? display->scroll_speed
                              : SCROLL_SPEED_MEDIUM;
    const uint16_t delay_ms = display->scroll_position == 0 ||
        display->scroll_position == full_length - 4 ? 900 : scroll_delays[speed];
    if (!text_changed && now - display->last_scroll >= delay_ms) {
      ++display->scroll_position;
      display->last_scroll = now;
      if (display->scroll_position > full_length - 4) {
        display->scroll_position = 0;
      }
      update_display = true;
    }

    menu_text_window(full_text, display->scroll_position, scroll_window);
  }

  if (!update_display) {
    return;
  }

  uint16_t display_buffer[5];
  format_menu_text(display_buffer, scroll_window);
  display_formatted_text(display, display_buffer, show_colon);
  memcpy(display->screen_text, scroll_window, sizeof(scroll_window));
  display->last_scroll_colon = show_colon;
}

static void display_clock_edit_time(DisplayState* display,
                                    const char text[6], bool timer = false) {
  const uint8_t positions[] = {0, 1, 3, 4};
  for (uint8_t index = 0; index < 4; ++index) {
    const uint8_t position = positions[index];
    const char digit = text[position];
    if (digit == ' ') {
      display->sev_segment.writeDigitRaw(position, 0);
    } else {
      display->sev_segment.writeDigitNum(
          position, static_cast<uint8_t>(digit - '0'), timer && position == 4);
    }
  }
  display->sev_segment.drawColon(true);
  display->sev_segment.writeDisplay();
  memcpy(display->screen_text, text, 6);
}

void init_DisplayState(DisplayState* display) {
  display->sev_segment = Adafruit_7segment();
  display->sev_segment.begin(DISPLAY_ADDRESS);
  display->screen_brighness = DISPLAY_BRIGTNESS;
  display->sev_segment.setBrightness(display->screen_brighness);
  serial_diagnostics_log("INIT: 7 Segment Display Configured");

  display->encoder_pixel =
      seesaw_NeoPixel(1, SEESAW_NEOPIX, NEO_GRB + NEO_KHZ800);
  display->encoder_pixel_ready = display->encoder_pixel.begin(SEESAW_ADDR);
  if (display->encoder_pixel_ready) {
    serial_diagnostics_log("INIT: Encoder Pixel Configured");
  } else {
    serial_diagnostics_log("DISPLAY-ERROR: encoder pixel unavailable");
  }

  display->screen_mode = SCREEN_MODE_TIME;
  display->clock_wake = {false, 0};
  display->sleep_clock = {};
  display->clock_blanked = false;
  display->alarm_active = false;
  display->alarm_snoozed = false;
  display->alarm_hold = false;
  display->alarm_audio_fault = false;
  memset(display->screen_text, 0, sizeof(display->screen_text));
  display->front_button_light = LIGHT_MODE_OFF;
  display->side_button_1_light = LIGHT_MODE_OFF;
  display->side_button_2_light = LIGHT_MODE_OFF;
  display->pause_timer = 0;
  display->last_scroll = 0;
  display->scroll_speed = SCROLL_SPEED_MEDIUM;
  display->scroll_position = 0;
  memset(display->scroll_text, 0, sizeof(display->scroll_text));
  display->last_scroll_colon = false;
  display->last_menu_screen = MENU_SCREEN_CLOCK;
  display->front_button_light_timer = 0;
  display->side_button_1_light_timer = 0;
  display->side_button_2_light_timer = 0;
  display->refresh_timer = 0;
}

void update_Display(DisplayState* display, const MenuState* menu,
                    TimeKeeper* time_keeper) {
  const unsigned long now = millis();
  if (display->screen_brighness != menu->display_settings.brightness) {
    display->screen_brighness = menu->display_settings.brightness;
    display->sev_segment.setBrightness(display->screen_brighness);
  }
  if (display->alarm_hold ||
      (menu->screen == MENU_SCREEN_CLOCK && display->alarm_snoozed &&
       (now / 2000) % 2 == 0) ||
      (menu->screen == MENU_SCREEN_CLOCK && display->alarm_audio_fault &&
       (now / 2000) % 2 == 0)) {
    display->clock_blanked = false;
    display->screen_mode = SCREEN_MODE_STATIC;
    display_scrolling_text(display, display->alarm_hold ? "HLD2" :
        display->alarm_snoozed ? "SNZ" : "AErr", false);
    return;
  }
  if (menu->screen == MENU_SCREEN_CLOCK) {
    if (now - display->refresh_timer < 200) {
      return;
    }
    display->refresh_timer = now;
    display->scroll_text[0] = '\0';
    display->scroll_position = 0;
    display->last_menu_screen = MENU_SCREEN_CLOCK;

    if (!display->alarm_active && display->sleep_clock.goodnight) {
      char window[5];
      menu_text_window("Goodnight",
          static_cast<uint8_t>((now - display->sleep_clock.message_started) / 400u),
          window);
      uint16_t buffer[5];
      format_menu_text(buffer, window);
      display_formatted_text(display, buffer, false);
      memcpy(display->screen_text, window, sizeof(window));
      display->clock_blanked = false;
      display->screen_mode = SCREEN_MODE_SCOLL;
      return;
    }

    if (!display->alarm_active &&
        (display->sleep_clock.held || (menu->sleep_status.active &&
         !clock_awake(display->clock_wake, static_cast<uint32_t>(now))))) {
      display->sev_segment.clear();
      display->sev_segment.writeDisplay();
      memcpy(display->screen_text, "    ", 5);
      display->screen_mode = SCREEN_MODE_OFF;
      display->clock_blanked = true;
      return;
    }

    if (!time_keeper->clock_valid) {
      display->clock_blanked = false;
      uint16_t display_buffer[5];
      char text[6];
      format_clock_display_text(false, 0, 0, text);
      format_menu_text(display_buffer, text);
      display_formatted_text(display, display_buffer, false);
      memcpy(display->screen_text, text, sizeof(text));
      display->screen_mode = SCREEN_MODE_STATIC;
      return;
    }

    const DateTime current_time = time_keeper->rtc.now();
    if (!display->alarm_active &&
        clock_should_blank(menu->display_settings, current_time.hour(),
                           current_time.minute(), display->clock_wake,
                           static_cast<uint32_t>(now))) {
      display->sev_segment.clear();
      display->sev_segment.writeDisplay();
      memcpy(display->screen_text, "    ", 5);
      display->screen_mode = SCREEN_MODE_OFF;
      display->clock_blanked = true;
      return;
    }
    display->clock_blanked = false;
    if (menu->sleep_status.active && !display->alarm_active && (now / 2000) % 2 != 0) {
      if (menu->sleep_audio_fault) {
        display_scrolling_text(display, "AErr", false);
      } else {
        char text[6];
        format_sleep_countdown(menu->sleep_status.remaining_seconds, text);
        display_clock_edit_time(display, text, true);
      }
      display->screen_mode = SCREEN_MODE_STATIC;
      return;
    }
    const uint8_t hour = clock_display_hour(
        current_time.hour(), menu->display_settings.use_12_hour);
    const uint8_t minute = current_time.minute();
    format_clock_display_text(true, hour, minute, display->screen_text);
    display->sev_segment.writeDigitNum(0, hour / 10);
    display->sev_segment.writeDigitNum(1, hour % 10);
    display->sev_segment.writeDigitNum(3, minute / 10);
    display->sev_segment.writeDigitNum(4, minute % 10);
    display->sev_segment.drawColon(current_time.second() % 2 == 0);
    display->sev_segment.writeDisplay();
    display->screen_mode = SCREEN_MODE_TIME;
    return;
  }

  if (display->last_menu_screen != menu->screen) {
    display->scroll_text[0] = '\0';
    display->scroll_position = 0;
    display->last_menu_screen = menu->screen;
  }
  display->clock_blanked = false;
  display->screen_mode = SCREEN_MODE_STATIC;
  if (menu->screen == MENU_SCREEN_SLEEP_STATUS && menu->sleep_status.active &&
      !display->alarm_active) {
    if ((now / 2000) % 2 == 0) {
      display_scrolling_text(display, menu->sleep_audio_fault ? "AErr" : "SLP", false);
    } else {
      if (now - display->refresh_timer < 200) return;
      display->refresh_timer = now;
      char text[6];
      format_sleep_countdown(menu->sleep_status.remaining_seconds, text);
      display_clock_edit_time(display, text);
      display->scroll_text[0] = '\0';
    }
    return;
  }
  if (menu->screen == MENU_SCREEN_ALARM_EDITOR &&
      (menu->alarm_field == ALARM_HOUR || menu->alarm_field == ALARM_MINUTE)) {
    if (now - display->refresh_timer < 200) return;
    display->refresh_timer = now;
    char text[6];
    format_clock_edit_display_text(
        menu->alarm_candidate.minute / 60, menu->alarm_candidate.minute % 60,
        menu->alarm_field == ALARM_HOUR, (now / 500) % 2 == 0, text);
    display_clock_edit_time(display, text);
    return;
  }
  if (menu->screen == MENU_SCREEN_DARK_EDITOR && menu->dark_selection >= 1 &&
      menu->dark_selection <= 4) {
    if (now - display->refresh_timer < 200) {
      return;
    }
    display->refresh_timer = now;
    const uint16_t time = menu->dark_selection <= 2
                              ? menu->display_settings.dark_start
                              : menu->display_settings.dark_end;
    char text[6];
    format_clock_edit_display_text(
        time / 60, time % 60,
        menu->dark_selection == 1 || menu->dark_selection == 3,
        (now / 500) % 2 == 0, text);
    display_clock_edit_time(display, text);
    return;
  }
  if (menu->screen == MENU_SCREEN_CLOCK_EDITOR &&
      (menu->clock_edit_field == CLOCK_EDIT_HOUR ||
       menu->clock_edit_field == CLOCK_EDIT_MINUTE)) {
    if (now - display->refresh_timer < 200) {
      return;
    }
    display->refresh_timer = now;
    char text[6];
    format_clock_edit_display_text(
        menu->clock_candidate.hour, menu->clock_candidate.minute,
        menu->clock_edit_field == CLOCK_EDIT_HOUR, (now / 500) % 2 == 0,
        text);
    display_clock_edit_time(display, text);
    return;
  }
  display_scrolling_text(display, menu_display_text(menu),
                         menu_screen_shows_colon(menu->screen));
}
