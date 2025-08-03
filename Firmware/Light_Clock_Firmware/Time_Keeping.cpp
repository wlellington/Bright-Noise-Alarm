// All structs and logic regarding the time keeping and alarm functionality
#include "Time_Keeping.hpp"

void init_TimeKeeper(TimeKeeper* time_keeper){
   // Setup the RTC
  if (! time_keeper->rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }

  if (! time_keeper->rtc.initialized() || time_keeper->rtc.lostPower()) {
    Serial.println("RTC is NOT initialized, let's set the time!");
    // When time needs to be set on a new device, or after a battery power loss, the
    // following line sets the RTC to the date & time this sketch was compiled
    time_keeper->rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
  Serial.println("\tINIT: RTC Connection Established");
}