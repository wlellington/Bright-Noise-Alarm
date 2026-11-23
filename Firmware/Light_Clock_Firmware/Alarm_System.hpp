#pragma once

#include "File_Management.hpp"
#include "Matrix_Driver.hpp"
#include "Audio_Playback.hpp"

namespace alarms {

constexpr uint8_t SLOT_COUNT = 5;
constexpr uint8_t NO_SLOT = 255;
constexpr size_t RECORD_SIZE = 1024;

struct Sound {
  char path[audio::PATH_CAPACITY] = {};
  uint8_t volume = 5;
};

struct Config {
  bool enabled;
  bool one_shot;
  uint8_t days;
  uint16_t minute;
  uint16_t hold_minutes;
  lighting::AnimationConfig light;
  Sound sound = {};
};

struct Bank {
  Config slots[SLOT_COUNT];
  uint32_t consumed[SLOT_COUNT];
  uint8_t resume_slot;
  uint32_t resume_peak;
  Config resume_config;
};

Config defaults();
bool valid(const Config& config);
bool occurrence(const Config& config, uint32_t now, uint32_t& peak);

class Store {
public:
  explicit Store(settings::Storage& storage);
  settings::Result load();
  bool save(const Bank& bank);
  const Bank& bank() const { return bank_; }
  bool ready() const { return ready_; }
private:
  settings::Storage& storage_;
  Bank bank_ = {};
  uint32_t sequence_ = 0;
  int8_t active_slot_ = -1;
  bool ready_ = false;
};

enum class Result { ok, invalid_value, no_active, storage };
enum class Phase { off, ramping, ringing, fading, snoozed };

struct Status {
  uint8_t slot;
  Phase phase;
  uint32_t peak;
  uint32_t remaining_seconds;
  bool hold_reminder;
};

class Engine {
public:
  explicit Engine(Store& store);
  Result set(uint8_t slot, const Config& config, uint32_t wall);
  Result start(uint8_t slot, uint32_t wall, uint32_t now);
  Result snooze(uint32_t now);
  Result dismiss();
  Result reduce(bool temporary = false);
  void clearTapReduction() { tap_reductions_ = 0; }
  Result tick(uint32_t wall, bool clock_valid, uint32_t now);
  Result clockChanged(uint32_t wall);
  Result button(bool pressed, uint32_t now);
  bool frame(uint32_t now, lighting::Frame& frame);
  Status status(uint32_t now) const;
  const Config& get(uint8_t slot) const { return store_.bank().slots[slot]; }
  bool active() const { return slot_ != NO_SLOT; }
  const Config& activeConfig() const { return config_; }
  uint32_t generation() const { return generation_; }
  uint32_t elapsedMilliseconds(uint32_t now) const;
  audio::Request audioRequest(uint32_t now) const;
  uint8_t reductions() const {
    const uint8_t total = reductions_ + tap_reductions_;
    return total > 8 ? 8 : total;
  }
private:
  void activate(uint8_t slot, const Config& config, uint32_t peak,
                uint32_t elapsed_seconds, uint32_t now);
  Store& store_;
  uint8_t slot_ = NO_SLOT;
  Config config_ = {};
  uint32_t peak_ = 0;
  uint32_t started_at_ = 0;
  uint32_t paused_at_ = 0;
  uint32_t pressed_at_ = 0;
  uint8_t reductions_ = 0;
  uint8_t tap_reductions_ = 0;
  uint32_t generation_ = 0;
  bool manual_ = false;
  bool snoozed_ = false;
  bool initialized_ = false;
  bool pressed_ = false;
  bool held_ = false;
};

class AudioBridge {
public:
  audio::Error update(const Engine& engine, audio::Control& control, uint32_t now);
  void silence(const Engine& engine, uint32_t now);
  bool pending() const { return pending_; }
private:
  uint32_t generation_ = 0;
  bool active_ = false;
  bool snoozed_ = false;
  uint8_t reductions_ = 0;
  bool pending_ = false;
  audio::Request request_;
};

}  // namespace alarms
