#pragma once

#include "Time_Keeping.hpp"
#include "Serial_Command.hpp"

void init_SerialDiagnostics();
void update_SerialDiagnostics(TimeKeeper* time_keeper,
                              serial_command::DirectoryReader* directory,
                              serial_command::DisplaySettingsControl* display,
                              serial_command::EncoderControl* encoder,
                              serial_command::LedControl* leds,
                              serial_command::AlarmControl* alarms,
                              audio::Control* audio,
                              sleep_timer::Control* sleep);
void serial_diagnostics_log(const char* line);
void serial_diagnostics_time_changed(const clock_command::DateTime& value);
