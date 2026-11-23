#pragma once

#include <stdint.h>

#include "File_Management.hpp"

struct ClockWakeState {
  bool awake;
  uint32_t started_at;
  uint32_t duration_ms = 30000;
};

struct SleepClockState {
  bool held = false;
  bool goodnight = false;
  uint32_t release_at = 0;
  uint32_t message_started = 0;
};

void finish_sleep_clock(SleepClockState& state, uint32_t wall, bool clock_valid,
                        bool show_goodnight, uint32_t now);
void update_sleep_clock(SleepClockState& state, uint32_t wall, bool clock_valid,
                        uint32_t now);
void disturb_sleep_clock(SleepClockState& state);

bool clock_dark_hours(const settings::DisplaySettings& settings,
                      uint8_t hour, uint8_t minute);
void wake_clock(ClockWakeState* wake, uint32_t now, uint32_t duration_ms = 30000);
bool clock_should_blank(const settings::DisplaySettings& settings,
                        uint8_t hour, uint8_t minute,
                        const ClockWakeState& wake, uint32_t now,
                        bool force_dark = false);
bool clock_awake(const ClockWakeState& wake, uint32_t now);
void format_clock_display_text(bool clock_valid, uint8_t hour,
                               uint8_t minute, char output[6]);
void format_clock_edit_display_text(uint8_t hour, uint8_t minute,
                                    bool editing_hour, bool blink_visible,
                                    char output[6]);
uint8_t clock_display_hour(uint8_t hour, bool use_12_hour);
void format_sleep_countdown(uint32_t seconds, char output[6]);
