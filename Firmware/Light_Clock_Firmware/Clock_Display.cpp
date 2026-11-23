#include "Clock_Display.hpp"

namespace {
uint32_t next_noon(uint32_t wall) {
  const uint32_t noon = wall / 86400u * 86400u + 43200u;
  return noon > wall ? noon : noon + 86400u;
}
}

void finish_sleep_clock(SleepClockState& state, uint32_t wall, bool clock_valid,
                        bool show_goodnight, uint32_t now) {
  state = {true, show_goodnight && clock_valid,
           clock_valid ? next_noon(wall) : 0, now};
}

void update_sleep_clock(SleepClockState& state, uint32_t wall, bool clock_valid,
                        uint32_t now) {
  if (state.goodnight && now - state.message_started >= 4000u) state.goodnight = false;
  if (state.held && clock_valid) {
    if (state.release_at == 0) state.release_at = next_noon(wall);
    if (wall >= state.release_at) state.held = false;
  }
}

void disturb_sleep_clock(SleepClockState& state) {
  state = {};
}

bool clock_dark_hours(const settings::DisplaySettings& settings,
                      uint8_t hour, uint8_t minute) {
  if (!settings.dark_enabled || settings.dark_start == settings.dark_end) {
    return false;
  }
  const uint16_t now = static_cast<uint16_t>(hour * 60 + minute);
  if (settings.dark_start < settings.dark_end) {
    return now >= settings.dark_start && now < settings.dark_end;
  }
  return now >= settings.dark_start || now < settings.dark_end;
}

void wake_clock(ClockWakeState* wake, uint32_t now, uint32_t duration_ms) {
  wake->awake = true;
  wake->started_at = now;
  wake->duration_ms = duration_ms;
}

bool clock_should_blank(const settings::DisplaySettings& settings,
                        uint8_t hour, uint8_t minute,
                        const ClockWakeState& wake, uint32_t now, bool force_dark) {
  return (force_dark || clock_dark_hours(settings, hour, minute)) && !clock_awake(wake, now);
}

bool clock_awake(const ClockWakeState& wake, uint32_t now) {
  return wake.awake && now - wake.started_at < wake.duration_ms;
}

uint8_t clock_display_hour(uint8_t hour, bool use_12_hour) {
  if (!use_12_hour) {
    return hour;
  }
  const uint8_t result = hour % 12;
  return result == 0 ? 12 : result;
}

void format_sleep_countdown(uint32_t seconds, char output[6]) {
  const uint8_t left = seconds >= 3600 ? seconds / 3600 : seconds / 60;
  const uint8_t right = seconds >= 3600 ? (seconds / 60) % 60 : seconds % 60;
  format_clock_display_text(true, left, right, output);
}

void format_clock_display_text(bool clock_valid, uint8_t hour,
                               uint8_t minute, char output[6]) {
  if (!clock_valid) {
    output[0] = 'S';
    output[1] = 'E';
    output[2] = 'T';
    output[3] = ' ';
    output[4] = '\0';
    output[5] = '\0';
    return;
  }

  output[0] = '0' + hour / 10;
  output[1] = '0' + hour % 10;
  output[2] = ':';
  output[3] = '0' + minute / 10;
  output[4] = '0' + minute % 10;
  output[5] = '\0';
}

void format_clock_edit_display_text(uint8_t hour, uint8_t minute,
                                    bool editing_hour, bool blink_visible,
                                    char output[6]) {
  format_clock_display_text(true, hour, minute, output);
  if (blink_visible) {
    return;
  }

  const uint8_t first_digit = editing_hour ? 0 : 3;
  output[first_digit] = ' ';
  output[first_digit + 1] = ' ';
}
