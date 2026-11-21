#include "Clock_Command.hpp"

namespace clock_command {
namespace {

bool isLeapYear(uint16_t year) {
  return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

uint8_t daysInMonth(uint16_t year, uint8_t month) {
  switch (month) {
    case 2:
      return isLeapYear(year) ? 29 : 28;
    case 4:
    case 6:
    case 9:
    case 11:
      return 30;
    default:
      return 31;
  }
}

}

bool isValid(const DateTime& value) {
  if (value.year < 2000 || value.year > 2099 ||
      value.month < 1 || value.month > 12 ||
      value.day < 1 || value.day > daysInMonth(value.year, value.month)) {
    return false;
  }

  return value.hour <= 23 && value.minute <= 59 && value.second <= 59;
}

Result setDateTime(RtcWriter& rtc, const DateTime& value) {
  if (!isValid(value)) {
    return Result::invalid_date_time;
  }

  return rtc.setDateTime(value) ? Result::success : Result::rtc_write_failed;
}

}
