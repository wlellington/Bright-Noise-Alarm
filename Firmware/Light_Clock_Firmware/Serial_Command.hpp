#pragma once

#include <stddef.h>
#include <stdint.h>

#include "Clock_Command.hpp"
#include "Settings_Command.hpp"
#include "Menu_System.hpp"
#include "Matrix_Driver.hpp"
#include "Audio_Playback.hpp"
#include "Sleep_Timer.hpp"
#include "Tap_Detection.hpp"

namespace serial_command {

static const size_t INPUT_CAPACITY = 280;
static const size_t RESPONSE_CAPACITY = 832;
static const size_t EVENT_CAPACITY = 56;

enum class DirectoryResult { entry, end, error };

class DirectoryReader {
public:
  virtual ~DirectoryReader() {}
  virtual bool open(const char* path) = 0;
  virtual DirectoryResult next(char* name, size_t capacity, bool* is_directory,
                               uint64_t* size) = 0;
  virtual void close() = 0;
  virtual bool openFile(const char* path) = 0;
  virtual int readFile(uint8_t* data, size_t capacity) = 0;
};

class DisplaySettingsControl {
public:
  virtual ~DisplaySettingsControl() {}
  virtual settings::DisplaySettings get() const = 0;
  virtual bool clockBlanked() const = 0;
  virtual const char* screenText() const { return nullptr; }
  virtual settings_command::Result set(
      const settings::DisplaySettings& candidate) = 0;
};

class EncoderControl {
public:
  virtual ~EncoderControl() {}
  virtual bool send(MenuEvent event, int32_t delta) = 0;
  virtual bool tap() { return false; }
  virtual tap_input::Status tapStatus() const { return {}; }
  virtual bool setTapThreshold(uint16_t) { return false; }
};

enum class LedResult { ok, invalid_value, unavailable, busy };

class AlarmControl {
public:
  virtual ~AlarmControl() {}
  virtual bool available() const = 0;
  virtual alarms::Config get(uint8_t slot) const = 0;
  virtual alarms::Status status() const = 0;
  virtual alarms::Result set(uint8_t slot, const alarms::Config& config) = 0;
  virtual alarms::Result start(uint8_t slot) = 0;
  virtual alarms::Result snooze() = 0;
  virtual alarms::Result dismiss() = 0;
  virtual alarms::Result reduce() = 0;
};

class LedControl {
public:
  virtual ~LedControl() {}
  virtual LedResult setPixel(uint8_t row, uint8_t column,
                             lighting::Color color) = 0;
  virtual LedResult clear() = 0;
  virtual bool getFrame(lighting::Frame& frame) const = 0;
  virtual LedResult startEffect(const lighting::AnimationConfig& config) = 0;
  virtual LedResult stopEffect() = 0;
  virtual bool getLight(lighting::AnimationConfig& config,
                        lighting::RunState& state) const = 0;
};

struct State {
  char input[INPUT_CAPACITY];
  uint16_t input_length;
  bool discard_line;
  char response[RESPONSE_CAPACITY];
  uint16_t response_length;
  uint16_t response_offset;
  char event[EVENT_CAPACITY];
  uint8_t event_length;
  uint8_t event_offset;
  uint32_t dropped_events;
  bool listing_directory;
  bool reading_file;
  bool bootloader_requested;
};

void init(State* state);
bool canAcceptInput(const State* state);
bool bootloaderReady(const State* state);
void cancelBootloaderRequest(State* state);
void receiveByte(State* state, char value, clock_command::RtcWriter& rtc,
                 DirectoryReader* directory = nullptr,
                 DisplaySettingsControl* display = nullptr,
                 EncoderControl* encoder = nullptr,
                 LedControl* leds = nullptr, AlarmControl* alarms = nullptr,
                 audio::Control* audio = nullptr,
                 sleep_timer::Control* sleep = nullptr);
void advanceDirectory(State* state, DirectoryReader* directory);
void abortDirectory(State* state, DirectoryReader* directory);
const char* pendingOutput(const State* state, size_t* length);
void consumeOutput(State* state, size_t length);
void queueEvent(State* state, const char* line);
void queueTimeChangedEvent(State* state,
                           const clock_command::DateTime& value);
void queueDroppedEventSummary(State* state);
void resetPartialInput(State* state);

}
