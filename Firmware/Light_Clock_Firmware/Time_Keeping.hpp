// All structs and logic regarding the time keeping and alarm functionality
#include <Wire.h>
#include <RTClib.h>

#include "Defines.hpp"

//char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
//char shortDayOfTheWeek[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

typedef struct {
  RTC_PCF8523 rtc;
} TimeKeeper;

void init_TimeKeeper(TimeKeeper* time_keeper);
void update_TimeKeeper(TimeKeeper* time_keeper);