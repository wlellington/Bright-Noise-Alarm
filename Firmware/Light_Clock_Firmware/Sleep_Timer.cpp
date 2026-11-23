#include "Sleep_Timer.hpp"

#include <string.h>

namespace sleep_timer {
namespace {
using namespace settings::record;

void encode(const Config* slots, uint32_t sequence, uint8_t* data) {
  memset(data, 0, RECORD_SIZE);
  memcpy(data, "BNSL", 4);
  data[4] = 2;
  put32(data + 8, sequence);
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    const Config& config = slots[slot];
    uint8_t* entry = data + 16 + slot * 128;
    entry[0] = config.light_enabled;
    entry[1] = config.audio_enabled;
    entry[2] = static_cast<uint8_t>(config.light.effect);
    entry[3] = config.light.brightness;
    put16(entry + 4, config.hold_minutes);
    entry[6] = config.light.fade_in_minutes;
    entry[7] = config.light.fade_out_minutes;
    entry[8] = 1;  // Reserved; was the removed Forest Green option.
    entry[9] = config.light.color.red;
    entry[10] = config.light.color.green;
    entry[11] = config.light.color.blue;
    entry[12] = config.light.color.white;
    entry[13] = config.sound.volume;
    memcpy(entry + 16, config.sound.path, audio::PATH_CAPACITY);
  }
  put32(data + 508, checksum(data, 508));
}

bool decode(const uint8_t* data, size_t length, Config* slots, uint32_t& sequence) {
  if (length != RECORD_SIZE || memcmp(data, "BNSL", 4) != 0 ||
      (data[4] != 1 && data[4] != 2) ||
      get32(data + 508) != checksum(data, 508)) return false;
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    const uint8_t* entry = data + 16 + slot * 128;
    if (entry[0] > 1 || entry[1] > 1 || entry[8] > 1) return false;
    const uint16_t duration = get16(entry + 4);
    if (data[4] == 1 &&
        (duration < 1 || duration > 180 ||
         !lighting::validFadeMinutes(entry[6]) ||
         !lighting::validFadeMinutes(entry[7]) ||
         entry[6] >= duration || entry[6] + entry[7] > duration))
      return false;
    Config& config = slots[slot];
    config = {static_cast<uint16_t>(data[4] == 1
                  ? duration - entry[6] - entry[7] : duration),
        entry[0] != 0, entry[1] != 0,
        {static_cast<lighting::Effect>(entry[2]),
         {entry[9], entry[10], entry[11], entry[12]}, entry[3],
         0, entry[7]}, {}};
    if (data[4] == 2 && entry[6] != 0) return false;
    config.sound.volume = entry[13];
    memcpy(config.sound.path, entry + 16, audio::PATH_CAPACITY);
    if (!valid(config)) return false;
  }
  sequence = get32(data + 8);
  return true;
}

}

Config defaults() {
  Config config = {30, true, false,
      {lighting::Effect::beach, {255, 180, 64, 255}, 128, 0, 5}, {}};
  config.sound.volume = 5;
  return config;
}

Config slotDefaults(uint8_t slot) {
  Config config = defaults();
  config.light.effect = lighting::animationEffect(slot + 3);
  return config;
}

bool valid(const Config& config) {
  if (config.hold_minutes > 180 ||
      (!config.light_enabled && !config.audio_enabled) ||
      !lighting::validAnimationEffect(config.light.effect) ||
      config.light.fade_in_minutes != 0 ||
      !lighting::validFadeMinutes(config.light.fade_out_minutes) ||
      config.sound.volume > audio::VOLUME_LIMIT) return false;
  if (config.audio_enabled && config.sound.path[0] == '\0') return false;
  if (config.sound.path[0] != '\0') {
    audio::Request request;
    request.action = audio::Action::start;
    request.volume = config.sound.volume;
    memcpy(request.path, config.sound.path, audio::PATH_CAPACITY);
    if (!audio::validRequest(request)) return false;
  }
  return true;
}

uint32_t fadeInMilliseconds(const Config& config) {
  return config.hold_minutes == 0 ? 0u : FADE_IN_MS;
}

uint32_t durationMilliseconds(const Config& config) {
  return (config.hold_minutes + config.light.fade_out_minutes) * 60000u;
}

const char* resultName(Result result) {
  switch (result) {
    case Result::ok: return "OK";
    case Result::invalid_value: return "INVALID_VALUE";
    case Result::busy: return "BUSY";
    case Result::unavailable: return "UNAVAILABLE";
    case Result::storage: return "STORAGE";
  }
  return "INVALID_VALUE";
}

Store::Store(settings::Storage& storage) : storage_(storage) {
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) slots_[slot] = slotDefaults(slot);
}

settings::Result Store::load() {
  ready_ = false;
  Config candidates[2][SLOT_COUNT];
  uint32_t sequences[2] = {};
  bool good[2] = {};
  bool present = false;
  for (uint8_t record = 0; record < 2; ++record) {
    uint8_t data[RECORD_SIZE] = {};
    size_t length = 0;
    const settings::ReadResult result = storage_.read(record, data, RECORD_SIZE, &length);
    if (result == settings::ReadResult::error) return settings::Result::unavailable;
    if (result == settings::ReadResult::ok) {
      present = true;
      if (length >= 5 && memcmp(data, "BNSL", 4) == 0 &&
          data[4] != 1 && data[4] != 2)
        return settings::Result::corrupt;
      good[record] = decode(data, length, candidates[record], sequences[record]);
    }
  }
  if (!good[0] && !good[1]) {
    if (present) return settings::Result::corrupt;
    for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) slots_[slot] = slotDefaults(slot);
    active_record_ = -1;
    sequence_ = 0;
    ready_ = true;
    return settings::Result::blank;
  }
  active_record_ = good[1] && (!good[0] ||
      static_cast<int32_t>(sequences[1] - sequences[0]) > 0) ? 1 : 0;
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot)
    slots_[slot] = candidates[active_record_][slot];
  sequence_ = sequences[active_record_];
  ready_ = true;
  return settings::Result::ok;
}

bool Store::save(uint8_t slot, const Config& config) {
  if (!ready_ || slot >= SLOT_COUNT || !valid(config)) return false;
  Config candidate[SLOT_COUNT];
  for (uint8_t i = 0; i < SLOT_COUNT; ++i) candidate[i] = slots_[i];
  candidate[slot] = config;
  uint8_t data[RECORD_SIZE];
  encode(candidate, sequence_ + 1, data);
  const uint8_t target = active_record_ == 0 ? 1 : 0;
  uint8_t verify[RECORD_SIZE];
  size_t length = 0;
  if (!storage_.write(target, data, RECORD_SIZE) ||
      storage_.read(target, verify, RECORD_SIZE, &length) != settings::ReadResult::ok ||
      length != RECORD_SIZE || memcmp(data, verify, RECORD_SIZE) != 0) {
    ready_ = false;
    return false;
  }
  slots_[slot] = config;
  ++sequence_;
  active_record_ = target;
  return true;
}

Result Engine::set(const Config& config) {
  if (!valid(config)) return Result::invalid_value;
  config_ = config;
  return Result::ok;
}

Result Engine::start(uint32_t now, bool alarm_active, uint8_t slot) {
  if (alarm_active) return Result::busy;
  if (slot != NO_SLOT && slot >= SLOT_COUNT) return Result::invalid_value;
  if (!valid(config_)) return Result::invalid_value;
  active_config_ = config_;
  reductions_ = 0;
  tap_reductions_ = 0;
  audio_reductions_ = 0;
  started_at_ = now;
  ++generation_;
  active_ = durationMilliseconds(active_config_) != 0;
  silenced_ = false;
  audio_pending_ = true;
  slot_ = slot;
  return Result::ok;
}

void Engine::stop() {
  if (active_) {
    active_ = false;
    audio_pending_ = true;
  }
}

void Engine::preempt() {
  active_ = false;
  audio_pending_ = false;
}

void Engine::silence() {
  silenced_ = true;
  audio_pending_ = false;
}

Result Engine::reduce(bool temporary) {
  if (!active_) return Result::busy;
  if (temporary) tap_reductions_ = 1;
  else if (reductions_ < 8) ++reductions_;
  return Result::ok;
}

void Engine::tick(uint32_t now) {
  if (active_ && now - started_at_ >= durationMilliseconds(active_config_))
    stop();
}

Status Engine::status(uint32_t now) const {
  const uint32_t elapsed = active_ ? now - started_at_ : 0;
  const uint32_t duration = durationMilliseconds(active_config_);
  return {active_, active_ && elapsed < duration
      ? (duration - elapsed + 999) / 1000 : 0, elapsed,
      active_ && active_config_.light_enabled,
      active_ && active_config_.audio_enabled && !silenced_,
      active_ ? slot_ : NO_SLOT, active_ ? reductions() : static_cast<uint8_t>(0)};
}

bool Engine::frame(uint32_t now, lighting::Frame& frame) const {
  frame = {};
  if (!active_ || !active_config_.light_enabled) return true;
  const uint32_t elapsed = now - started_at_;
  const uint32_t duration = durationMilliseconds(active_config_);
  if (elapsed >= duration) return true;
  const lighting::AnimationConfig& light = active_config_.light;
  const uint32_t fade_out = light.fade_out_minutes * 60000u;
  const uint32_t light_fade_out = fade_out / 2;
  const uint32_t fade_start = duration - fade_out;
  if (fade_out != 0 && elapsed >= fade_start + light_fade_out) return true;
  lighting::EffectConfig effect = {light.effect, light.color, light.brightness,
      fadeInMilliseconds(active_config_)};
  if (effect.effect == lighting::Effect::solid && effect.duration_ms != 0 &&
      elapsed < effect.duration_ms) {
    effect.brightness = static_cast<uint8_t>(
        static_cast<uint64_t>(effect.brightness) * elapsed / effect.duration_ms);
  }
  if (!lighting::render(effect, elapsed, frame)) return false;
  if (fade_out != 0 && elapsed >= fade_start) {
    const uint32_t remaining = fade_start + light_fade_out - elapsed;
    for (lighting::Color& pixel : frame.pixels) {
      pixel.red = static_cast<uint8_t>(pixel.red * remaining / light_fade_out);
      pixel.green = static_cast<uint8_t>(pixel.green * remaining / light_fade_out);
      pixel.blue = static_cast<uint8_t>(pixel.blue * remaining / light_fade_out);
      pixel.white = static_cast<uint8_t>(pixel.white * remaining / light_fade_out);
    }
  }
  for (lighting::Color& pixel : frame.pixels) {
    pixel.red >>= reductions();
    pixel.green >>= reductions();
    pixel.blue >>= reductions();
    pixel.white >>= reductions();
  }
  return true;
}

audio::Error Engine::updateAudio(audio::Control& control, uint32_t now) {
  const bool reduce_pending = active_ && active_config_.audio_enabled &&
      !silenced_ && reductions() != audio_reductions_;
  if (!audio_pending_ && !reduce_pending) return audio::Error::none;
  audio::Request request;
  request.sleep = true;
  if (!audio_pending_) {
    request.action = audio::Action::reduce;
  } else if (active_ && active_config_.audio_enabled && !silenced_) {
    request.action = audio::Action::start;
    request.volume = active_config_.sound.volume;
    memcpy(request.path, active_config_.sound.path, audio::PATH_CAPACITY);
    request.loop = true;
    request.timeline = {true, now, now - started_at_,
        fadeInMilliseconds(active_config_),
        durationMilliseconds(active_config_),
        fadeInMilliseconds(active_config_),
        active_config_.light.fade_out_minutes * 60000u};
  }
  request.reductions = reductions();
  uint32_t id = 0;
  const audio::Error result = control.submit(request, id);
  if (result == audio::Error::none) {
    audio_pending_ = false;
    audio_reductions_ = reductions();
  }
  return result;
}

}
