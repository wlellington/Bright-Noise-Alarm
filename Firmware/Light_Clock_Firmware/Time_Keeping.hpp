// All structs and logic regarding the time keeping and alarm functionality
#pragma once

#include <Wire.h>
#include <RTClib.h>

#include "Clock_Command.hpp"
#include "Defines.hpp"

//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//char shortDayOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

typedef struct {
  RTC_PCF8523 rtc;
  bool rtc_available;
  bool clock_valid;
  unsigned long validity_check_timer;
  uint32_t revision;
} TimeKeeper;

class TimeKeeperRtcWriter : public clock_command::RtcWriter {
public:
  explicit TimeKeeperRtcWriter(TimeKeeper* time_keeper);
  bool setDateTime(const clock_command::DateTime& value) override;
  bool getDateTime(clock_command::DateTime& value) const override;

private:
  TimeKeeper* time_keeper_;
};

void init_TimeKeeper(TimeKeeper* time_keeper);
void update_TimeKeeper(TimeKeeper* time_keeper);
clock_command::Result set_TimeKeeperDateTime(
    TimeKeeper* time_keeper, const clock_command::DateTime& value);