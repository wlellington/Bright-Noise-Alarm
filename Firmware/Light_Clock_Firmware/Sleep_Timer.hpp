#pragma once

#include "Alarm_System.hpp"

namespace sleep_timer {

constexpr uint8_t SLOT_COUNT = 3;
constexpr uint8_t NO_SLOT = 255;
constexpr size_t RECORD_SIZE = 512;
constexpr uint32_t FADE_IN_MS = 30000;

struct Config {
  uint16_t hold_minutes;
  bool light_enabled;
  bool audio_enabled;
  lighting::AnimationConfig light;
  alarms::Sound sound;
};

struct Status {
  bool active;
  uint32_t remaining_seconds;
  uint32_t elapsed_ms;
  bool light_enabled;
  bool audio_enabled;
  uint8_t slot = NO_SLOT;
  uint8_t reductions = 0;
};

enum class Result { ok, invalid_value, busy, unavailable, storage };

Config defaults();
Config slotDefaults(uint8_t slot);
bool valid(const Config& config);
uint32_t fadeInMilliseconds(const Config& config);
uint32_t durationMilliseconds(const Config& config);
const char* resultName(Result result);

class Store {
public:
  explicit Store(settings::Storage& storage);
  settings::Result load();
  bool save(uint8_t slot, const Config& config);
  const Config& get(uint8_t slot) const { return slots_[slot]; }
  bool ready() const { return ready_; }
private:
  settings::Storage& storage_;
  Config slots_[SLOT_COUNT];
  uint32_t sequence_ = 0;
  int8_t active_record_ = -1;
  bool ready_ = false;
};

class Control {
public:
  virtual ~Control() {}
  virtual Config get() const = 0;
  virtual Status status() const = 0;
  virtual Result set(const Config& config) = 0;
  virtual Result start(uint8_t slot = NO_SLOT) = 0;
  virtual Result stop() = 0;
  virtual Config getSlot(uint8_t slot) const = 0;
  virtual Result loadSlot(uint8_t slot) = 0;
  virtual Result saveSlot(uint8_t slot, const Config& config) = 0;
  virtual bool available() const = 0;
};

class Engine {
public:
  Result set(const Config& config);
  Result start(uint32_t now, bool alarm_active, uint8_t slot = NO_SLOT);
  void stop();
  void preempt();
  void silence();
  Result reduce(bool temporary = false);
  void clearTapReduction() { tap_reductions_ = 0; }
  uint8_t reductions() const {
    const uint8_t total = reductions_ + tap_reductions_;
    return total > 8 ? 8 : total;
  }
  void tick(uint32_t now);
  bool frame(uint32_t now, lighting::Frame& frame) const;
  audio::Error updateAudio(audio::Control& control, uint32_t now);
  Status status(uint32_t now) const;
  const Config& config() const { return config_; }
  const Config& activeConfig() const { return active_config_; }
  bool active() const { return active_; }
  bool pending() const { return audio_pending_; }
  uint32_t generation() const { return generation_; }

private:
  Config config_ = defaults();
  Config active_config_ = defaults();
  uint32_t started_at_ = 0;
  uint32_t generation_ = 0;
  bool active_ = false;
  bool audio_pending_ = false;
  bool silenced_ = false;
  uint8_t slot_ = NO_SLOT;
  uint8_t reductions_ = 0;
  uint8_t tap_reductions_ = 0;
  uint8_t audio_reductions_ = 0;
};

}
