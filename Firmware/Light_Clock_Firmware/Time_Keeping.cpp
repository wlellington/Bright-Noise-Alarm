#include "Time_Keeping.hpp"
#include "Serial_Diagnostics.hpp"

static bool has_valid_clock(TimeKeeper* time_keeper) {
  if (!time_keeper->rtc_available || !time_keeper->rtc.initialized() ||
      time_keeper->rtc.lostPower()) {
    return false;
  }
  return time_keeper->rtc.now().isValid();
}

void init_TimeKeeper(TimeKeeper* time_keeper) {
  time_keeper->rtc_available = time_keeper->rtc.begin();
  time_keeper->clock_valid = false;
  time_keeper->revision = 0;
  time_keeper->validity_check_timer = millis();

  if (!time_keeper->rtc_available) {
    serial_diagnostics_log("RTC-ERROR: clock unavailable");
    return;
  }

  time_keeper->clock_valid = has_valid_clock(time_keeper);
  if (time_keeper->clock_valid) {
    serial_diagnostics_log("INIT: RTC Connection Established");
  } else {
    // REQ-050: do not replace invalid RTC time with the build timestamp.
    serial_diagnostics_log("RTC-ERROR: clock needs to be set");
  }
}

void update_TimeKeeper(TimeKeeper* time_keeper) {
  const unsigned long now = millis();
  if (now - time_keeper->validity_check_timer < 1000) {
    return;
  }
  time_keeper->validity_check_timer = now;
  time_keeper->clock_valid = has_valid_clock(time_keeper);
}

TimeKeeperRtcWriter::TimeKeeperRtcWriter(TimeKeeper* time_keeper)
    : time_keeper_(time_keeper) {}

bool TimeKeeperRtcWriter::setDateTime(
    const clock_command::DateTime& value) {
  if (time_keeper_ == NULL || !time_keeper_->rtc_available) {
    return false;
  }
  const DateTime requested(value.year, value.month, value.day, value.hour,
                           value.minute, value.second);
  time_keeper_->rtc.adjust(requested);
  const DateTime written = time_keeper_->rtc.now();
  if (!written.isValid()) {
    time_keeper_->clock_valid = has_valid_clock(time_keeper_);
    return false;
  }

  const uint32_t requested_epoch = requested.unixtime();
  const uint32_t written_epoch = written.unixtime();
  if (written_epoch < requested_epoch || written_epoch > requested_epoch + 2) {
    time_keeper_->clock_valid = has_valid_clock(time_keeper_);
    return false;
  }

  time_keeper_->clock_valid = true;
  time_keeper_->validity_check_timer = millis();
  ++time_keeper_->revision;
  return true;
}

bool TimeKeeperRtcWriter::getDateTime(clock_command::DateTime& value) const {
  if (time_keeper_ == nullptr || !has_valid_clock(time_keeper_)) return false;
  const DateTime now = time_keeper_->rtc.now();
  value = {now.year(), now.month(), now.day(), now.hour(), now.minute(), now.second()};
  return true;
}

clock_command::Result set_TimeKeeperDateTime(
    TimeKeeper* time_keeper, const clock_command::DateTime& value) {
  TimeKeeperRtcWriter rtc_writer(time_keeper);
  return clock_command::setDateTime(rtc_writer, value);
}
