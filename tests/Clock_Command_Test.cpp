#include "../Firmware/Light_Clock_Firmware/Clock_Command.hpp"

#include <stdio.h>

using clock_command::DateTime;
using clock_command::Result;
using clock_command::RtcWriter;

namespace {

int failures = 0;

bool equal(const DateTime& left, const DateTime& right) {
  return left.year == right.year &&
         left.month == right.month &&
         left.day == right.day &&
         left.hour == right.hour &&
         left.minute == right.minute &&
         left.second == right.second;
}

void expect(bool condition, const char* message) {
  if (!condition) {
    fprintf(stderr, "FAIL: %s\n", message);
    ++failures;
  }
}

class FakeRtc : public RtcWriter {
public:
  explicit FakeRtc(const DateTime& initial)
      : current(initial), write_succeeds(true), write_count(0) {}

  bool setDateTime(const DateTime& value) override {
    ++write_count;
    if (!write_succeeds) {
      return false;
    }

    current = value;
    return true;
  }

  DateTime current;
  bool write_succeeds;
  unsigned int write_count;
};

Result localCaller(RtcWriter& rtc, const DateTime& value) {
  return clock_command::setDateTime(rtc, value);
}

Result serialCaller(RtcWriter& rtc, const DateTime& value) {
  return clock_command::setDateTime(rtc, value);
}

void testMonthBoundaries() {
  const uint8_t month_lengths[] = {31, 29, 31, 30, 31, 30,
                                   31, 31, 30, 31, 30, 31};

  expect(clock_command::isValid({2024, 1, 1, 0, 0, 0}),
         "first day of the year is valid");
  expect(clock_command::isValid({2024, 12, 31, 23, 59, 59}),
         "last day of the year is valid");

  for (uint8_t month = 1; month <= 12; ++month) {
    const DateTime last_day = {2024, month, month_lengths[month - 1], 12, 0, 0};
    expect(clock_command::isValid(last_day), "month's final calendar day is valid");

    if (month_lengths[month - 1] < 31) {
      const DateTime past_end = {
          2024, month, static_cast<uint8_t>(month_lengths[month - 1] + 1), 12, 0, 0};
      expect(!clock_command::isValid(past_end), "day after month end is invalid");
    }
  }

  expect(!clock_command::isValid({2024, 0, 1, 0, 0, 0}), "month zero is invalid");
  expect(!clock_command::isValid({2024, 13, 1, 0, 0, 0}), "month thirteen is invalid");
  expect(!clock_command::isValid({2024, 1, 0, 0, 0, 0}), "day zero is invalid");
}

void testLeapYearsAndSupportedRange() {
  expect(clock_command::isValid({2000, 2, 29, 0, 0, 0}),
         "year 2000 is a leap year");
  expect(clock_command::isValid({2024, 2, 29, 0, 0, 0}),
         "year 2024 is a leap year");
  expect(!clock_command::isValid({2023, 2, 29, 0, 0, 0}),
         "year 2023 is not a leap year");
  expect(!clock_command::isValid({1999, 12, 31, 23, 59, 59}),
         "year before RTC range is invalid");
  expect(!clock_command::isValid({2100, 1, 1, 0, 0, 0}),
         "year after RTC range is invalid");
}

void testInvalidTimesDoNotWrite() {
  const DateTime initial = {2025, 6, 15, 10, 20, 30};
  FakeRtc rtc(initial);
  const DateTime invalid_values[] = {
      {2025, 6, 15, 24, 0, 0},
      {2025, 6, 15, 0, 60, 0},
      {2025, 6, 15, 0, 0, 60},
      {2025, 4, 31, 10, 20, 30},
  };

  for (unsigned int index = 0;
       index < sizeof(invalid_values) / sizeof(invalid_values[0]);
       ++index) {
    expect(clock_command::setDateTime(rtc, invalid_values[index]) ==
               Result::invalid_date_time,
           "invalid date/time returns explicit validation error");
    expect(rtc.write_count == 0, "invalid date/time does not write RTC");
    expect(equal(rtc.current, initial), "invalid date/time preserves RTC value");
  }
}

void testRtcWriteFailureIsReported() {
  const DateTime initial = {2025, 6, 15, 10, 20, 30};
  const DateTime requested = {2026, 1, 1, 0, 0, 0};
  FakeRtc rtc(initial);
  rtc.write_succeeds = false;

  expect(clock_command::setDateTime(rtc, requested) == Result::rtc_write_failed,
         "RTC write failure is returned explicitly");
  expect(rtc.write_count == 1, "RTC write is attempted exactly once");
  expect(equal(rtc.current, initial), "failed write does not change fake RTC");
}

void testLocalAndSerialCallersShareCommand() {
  const DateTime initial = {2025, 6, 15, 10, 20, 30};
  const DateTime valid = {2026, 2, 28, 8, 9, 10};
  const DateTime invalid = {2026, 2, 29, 8, 9, 10};
  FakeRtc local_rtc(initial);
  FakeRtc serial_rtc(initial);

  const Result local_result = localCaller(local_rtc, valid);
  const Result serial_result = serialCaller(serial_rtc, valid);
  expect(local_result == Result::success && local_result == serial_result,
         "local and serial callers report the same successful result");
  expect(local_rtc.write_count == 1 && serial_rtc.write_count == 1,
         "both callers execute one shared RTC operation");
  expect(equal(local_rtc.current, serial_rtc.current),
         "both callers store the same value");

  FakeRtc invalid_local_rtc(initial);
  FakeRtc invalid_serial_rtc(initial);
  const Result invalid_local_result = localCaller(invalid_local_rtc, invalid);
  const Result invalid_serial_result = serialCaller(invalid_serial_rtc, invalid);
  expect(invalid_local_result == Result::invalid_date_time &&
             invalid_local_result == invalid_serial_result,
         "local and serial callers share validation failures");
  expect(invalid_local_rtc.write_count == 0 &&
             invalid_serial_rtc.write_count == 0,
         "neither caller writes an invalid value");
}

void testCancelDoesNotWriteAndSaveWritesOnce() {
  const DateTime initial = {2025, 6, 15, 10, 20, 30};
  const DateTime saved = {2025, 12, 31, 23, 59, 59};
  FakeRtc rtc(initial);

  {
    const DateTime candidate = saved;
    expect(equal(candidate, saved), "edited candidate holds the draft value");
  }
  expect(rtc.write_count == 0, "editing and canceling a candidate does not write");
  expect(equal(rtc.current, initial), "cancel leaves RTC unchanged");

  const DateTime candidate = saved;
  expect(localCaller(rtc, candidate) == Result::success,
         "saving a valid candidate succeeds");
  expect(rtc.write_count == 1, "saving writes RTC exactly once");
  expect(equal(rtc.current, saved), "saving stores the candidate value");
}

}

int main() {
  testMonthBoundaries();
  testLeapYearsAndSupportedRange();
  testInvalidTimesDoNotWrite();
  testRtcWriteFailureIsReported();
  testLocalAndSerialCallersShareCommand();
  testCancelDoesNotWriteAndSaveWritesOnce();

  if (failures != 0) {
    fprintf(stderr, "%d test assertion(s) failed\n", failures);
    return 1;
  }

  puts("All clock command tests passed");
  return 0;
}
