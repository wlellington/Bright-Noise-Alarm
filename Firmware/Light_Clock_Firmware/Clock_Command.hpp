#pragma once

#include <stdint.h>

namespace clock_command {

struct DateTime {
  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
};

class RtcWriter {
public:
  virtual ~RtcWriter() {}
  virtual bool setDateTime(const DateTime& value) = 0;
};

enum class Result {
  success,
  invalid_date_time,
  rtc_write_failed
};

bool isValid(const DateTime& value);
Result setDateTime(RtcWriter& rtc, const DateTime& value);

}
