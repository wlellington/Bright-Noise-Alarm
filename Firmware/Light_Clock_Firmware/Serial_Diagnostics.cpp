#include "Serial_Diagnostics.hpp"

#include <Arduino.h>
#include <stdio.h>

#include "Serial_Command.hpp"

namespace {

static const unsigned long SERIAL_STARTUP_WINDOW_MS = 1500;
static const unsigned long BOOTLOADER_RESPONSE_DELAY_MS = 100;

serial_command::State command_state;
bool diagnostics_enabled = false;
bool bootloader_armed = false;
unsigned long bootloader_armed_at = 0;

static void flush_output() {
  size_t pending_length = 0;
  const char* pending =
      serial_command::pendingOutput(&command_state, &pending_length);
  if (pending == NULL || pending_length == 0) {
    return;
  }

  const int writable = Serial.availableForWrite();
  if (writable <= 0) {
    return;
  }

  const size_t chunk =
      pending_length < static_cast<size_t>(writable)
          ? pending_length
          : static_cast<size_t>(writable);
  const size_t written = Serial.write(
      reinterpret_cast<const uint8_t*>(pending), chunk);
  serial_command::consumeOutput(&command_state, written);
}

}

void init_SerialDiagnostics() {
  serial_command::init(&command_state);
  Serial.begin(115200);

  const unsigned long started_at = millis();
  while (!Serial.dtr() &&
         millis() - started_at < SERIAL_STARTUP_WINDOW_MS) {
    yield();
    delay(1);
  }
  diagnostics_enabled = Serial.dtr();
  if (diagnostics_enabled) {
    serial_command::queueEvent(&command_state, "EVENT SERIAL_READY\n");
  }
}

void serial_diagnostics_log(const char* line) {
  if (!diagnostics_enabled) {
    return;
  }

  char formatted[serial_command::EVENT_CAPACITY];
  const int length = snprintf(formatted, sizeof(formatted), "%s\n", line);
  if (length < 0 || static_cast<size_t>(length) >= sizeof(formatted)) {
    ++command_state.dropped_events;
    return;
  }
  serial_command::queueEvent(&command_state, formatted);
}

void serial_diagnostics_time_changed(const clock_command::DateTime& value) {
  if (diagnostics_enabled) {
    serial_command::queueTimeChangedEvent(&command_state, value);
  }
}

void update_SerialDiagnostics(TimeKeeper* time_keeper,
                              serial_command::DirectoryReader* directory,
                              serial_command::DisplaySettingsControl* display,
                              serial_command::EncoderControl* encoder,
                              serial_command::LedControl* leds,
                              serial_command::AlarmControl* alarms,
                              audio::Control* audio,
                              sleep_timer::Control* sleep) {
  if (!diagnostics_enabled) {
    return;
  }

  if (serial_command::bootloaderReady(&command_state)) {
    if (!bootloader_armed) {
      Serial.flush();
      bootloader_armed = true;
      bootloader_armed_at = millis();
    }
    if (millis() - bootloader_armed_at >= BOOTLOADER_RESPONSE_DELAY_MS) {
      digitalWrite(PIN_EXTERNAL_POWER, LOW);
      rp2040.rebootToBootloader();
    }
    return;
  }

  if (!Serial.dtr()) {
    serial_command::cancelBootloaderRequest(&command_state);
    serial_command::abortDirectory(&command_state, directory);
    serial_command::resetPartialInput(&command_state);
    return;
  }

  serial_command::queueDroppedEventSummary(&command_state);
  serial_command::advanceDirectory(&command_state, directory);
  flush_output();

  TimeKeeperRtcWriter rtc_writer(time_keeper);
  while (serial_command::canAcceptInput(&command_state) &&
         Serial.available() > 0) {
    const int value = Serial.read();
    if (value < 0) {
      break;
    }
    serial_command::receiveByte(&command_state, static_cast<char>(value),
                                rtc_writer, directory, display, encoder, leds, alarms, audio, sleep);
  }

  flush_output();
}
