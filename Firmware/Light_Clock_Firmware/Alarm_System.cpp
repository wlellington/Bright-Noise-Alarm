#include "Alarm_System.hpp"

#include <string.h>

namespace alarms {
namespace {
using namespace settings::record;

uint8_t peakDelay(const Config& config) {
  return config.light.fade_in_minutes;
}

void encodeSound(const Sound& sound, const lighting::AnimationConfig& light,
                 uint8_t* data) {
  memcpy(data, sound.path, audio::PATH_CAPACITY);
  data[96] = sound.volume;
  data[97] = light.fade_in_minutes;
  data[98] = light.fade_out_minutes;
}

void decodeSound(const uint8_t* data, Sound& sound) {
  memcpy(sound.path, data, audio::PATH_CAPACITY);
  sound.volume = data[96];
}
void encodeConfig(const Config& config, uint8_t* data) {
  data[0] = config.enabled;
  data[1] = config.one_shot;
  data[2] = config.days;
  data[3] = static_cast<uint8_t>(config.light.effect);
  put16(data + 4, config.minute);
  put16(data + 6, config.hold_minutes);
  data[8] = config.light.color.red;
  data[9] = config.light.color.green;
  data[10] = config.light.color.blue;
  data[11] = config.light.color.white;
  data[12] = config.light.brightness;
  data[13] = config.light.fade_in_minutes;
  data[14] = config.light.fade_out_minutes;
  data[15] = 1;  // Reserved; was the removed Forest Green option.
}

bool decodeConfig(const uint8_t* data, Config& config, bool legacy) {
  if (data[0] > 1 || data[1] > 1 || data[15] > 1) return false;
  const uint16_t duration = get16(data + 6);
  if (legacy && (duration < 1 || duration > 180 || data[14] > duration))
    return false;
  config = {data[0] != 0, data[1] != 0, data[2], get16(data + 4),
      static_cast<uint16_t>(legacy ? duration - data[14] : duration),
      {static_cast<lighting::Effect>(data[3]),
       {data[8], data[9], data[10], data[11]}, data[12], data[13],
       data[14]}};
  return valid(config);
}

void encode(const Bank& bank, uint32_t sequence, uint8_t* data) {
  memset(data, 0, RECORD_SIZE);
  memcpy(data, "BNAL", 4);
  data[4] = 4;
  put32(data + 8, sequence);
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    encodeConfig(bank.slots[slot], data + 16 + slot * 16);
    put32(data + 96 + slot * 4, bank.consumed[slot]);
    encodeSound(bank.slots[slot].sound, bank.slots[slot].light,
                data + 256 + slot * 104);
  }
  data[116] = bank.resume_slot;
  put32(data + 120, bank.resume_peak);
  encodeConfig(bank.resume_config, data + 124);
  encodeSound(bank.resume_config.sound, bank.resume_config.light, data + 800);
  put32(data + 1020, checksum(data, 1020));
}

bool decode(const uint8_t* data, size_t length, Bank& bank,
            uint32_t& sequence) {
  const bool legacy = length == 256 && data[4] == 1;
  const size_t crc = legacy ? 252 : 1020;
  if ((!legacy && (length != RECORD_SIZE ||
       (data[4] != 2 && data[4] != 3 && data[4] != 4))) ||
      memcmp(data, "BNAL", 4) != 0 || get32(data + crc) != checksum(data, crc) ||
      (data[116] != NO_SLOT && data[116] >= SLOT_COUNT)) return false;
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    if (!decodeConfig(data + 16 + slot * 16, bank.slots[slot], data[4] < 4))
      return false;
    if (!legacy) decodeSound(data + 256 + slot * 104, bank.slots[slot].sound);
    if (!valid(bank.slots[slot])) return false;
    bank.consumed[slot] = get32(data + 96 + slot * 4);
  }
  bank.resume_slot = data[116];
  bank.resume_peak = get32(data + 120);
  sequence = get32(data + 8);
  if (!decodeConfig(data + 124, bank.resume_config, data[4] < 4)) return false;
  if (!legacy) decodeSound(data + 800, bank.resume_config.sound);
  return valid(bank.resume_config);
}

uint32_t startTime(const Config& config, uint32_t peak) {
  return peak - static_cast<uint32_t>(peakDelay(config)) * 60;
}

bool inWindow(const Config& config, uint32_t peak, uint32_t wall) {
  return peak >= static_cast<uint32_t>(peakDelay(config)) * 60 &&
      wall >= startTime(config, peak) &&
      static_cast<uint64_t>(wall) <
          static_cast<uint64_t>(peak) +
              (config.hold_minutes + config.light.fade_out_minutes) * 60u;
}
}

Config defaults() {
  return {false, false, 127, 7 * 60, 30,
          {lighting::Effect::sunrise, {255, 180, 64, 255}, 255, 30, 0}};
}

bool valid(const Config& config) {
  audio::Request request;
  memcpy(request.path, config.sound.path, audio::PATH_CAPACITY);
  request.action = audio::Action::start;
  request.volume = config.sound.volume;
  return config.days > 0 && config.days <= 127 && config.minute < 1440 &&
      config.hold_minutes <= 180 &&
      config.hold_minutes + config.light.fade_out_minutes +
          config.light.fade_in_minutes > 0 &&
      lighting::validFadeMinutes(config.light.fade_in_minutes) &&
      lighting::validFadeMinutes(config.light.fade_out_minutes) &&
      config.sound.volume <= audio::VOLUME_LIMIT &&
      (config.sound.path[0] == '\0' || audio::validRequest(request)) &&
      lighting::validAnimationEffect(config.light.effect);
}

bool occurrence(const Config& config, uint32_t now, uint32_t& peak) {
  if (!config.enabled || !valid(config)) return false;
  const int64_t day = now / 86400;
  for (int8_t offset = 1; offset >= -1; --offset) {
    const int64_t candidate = (day + offset) * 86400 + config.minute * 60u;
    if (candidate < 0 || candidate > UINT32_MAX) continue;
    const uint32_t value = static_cast<uint32_t>(candidate);
    const uint8_t weekday = static_cast<uint8_t>((value / 86400 + 4) % 7);
    if ((config.days & (1u << weekday)) && inWindow(config, value, now)) {
      peak = value;
      return true;
    }
  }
  return false;
}

Store::Store(settings::Storage& storage) : storage_(storage) {
  for (Config& config : bank_.slots) config = defaults();
  bank_.resume_slot = NO_SLOT;
  bank_.resume_config = defaults();
}

settings::Result Store::load() {
  ready_ = false;
  uint8_t data[2][RECORD_SIZE] = {};
  Bank candidates[2] = {};
  uint32_t sequences[2] = {};
  bool good[2] = {};
  bool present = false;
  for (uint8_t slot = 0; slot < 2; ++slot) {
    size_t length = 0;
    const settings::ReadResult result =
        storage_.read(slot, data[slot], RECORD_SIZE, &length);
    if (result == settings::ReadResult::error) return settings::Result::unavailable;
    if (result == settings::ReadResult::ok) {
      present = true;
      if (length >= 5 && memcmp(data[slot], "BNAL", 4) == 0 &&
          data[slot][4] != 1 && data[slot][4] != 2 &&
          data[slot][4] != 3 && data[slot][4] != 4)
        return settings::Result::corrupt;
      good[slot] = decode(data[slot], length, candidates[slot], sequences[slot]);
    }
  }
  if (!good[0] && !good[1]) {
    if (present) return settings::Result::corrupt;
    bank_ = {};
    for (Config& config : bank_.slots) config = defaults();
    bank_.resume_config = defaults();
    bank_.resume_slot = NO_SLOT;
    sequence_ = 0;
    active_slot_ = -1;
    ready_ = true;
    return settings::Result::blank;
  }
  active_slot_ = good[1] && (!good[0] ||
      static_cast<int32_t>(sequences[1] - sequences[0]) > 0) ? 1 : 0;
  bank_ = candidates[active_slot_];
  sequence_ = sequences[active_slot_];
  ready_ = true;
  return settings::Result::ok;
}

bool Store::save(const Bank& bank) {
  if (!ready_ || (bank.resume_slot != NO_SLOT && bank.resume_slot >= SLOT_COUNT) ||
      !valid(bank.resume_config)) return false;
  for (const Config& config : bank.slots) if (!valid(config)) return false;
  uint8_t data[RECORD_SIZE];
  encode(bank, sequence_ + 1, data);
  const uint8_t target = active_slot_ == 0 ? 1 : 0;
  uint8_t verify[RECORD_SIZE];
  size_t length = 0;
  if (!storage_.write(target, data, RECORD_SIZE) ||
      storage_.read(target, verify, RECORD_SIZE, &length) != settings::ReadResult::ok ||
      length != RECORD_SIZE || memcmp(data, verify, RECORD_SIZE) != 0) {
    ready_ = false;
    return false;
  }
  bank_ = bank;
  ++sequence_;
  active_slot_ = target;
  return true;
}

Engine::Engine(Store& store) : store_(store) {}

Result Engine::set(uint8_t slot, const Config& config, uint32_t wall) {
  if (slot >= SLOT_COUNT || !valid(config)) return Result::invalid_value;
  Bank bank = store_.bank();
  const bool stop = active() && slot_ == slot &&
                    bank.slots[slot].enabled && !config.enabled;
  bank.slots[slot] = config;
  uint32_t peak = 0;
  if (occurrence(config, wall, peak) && peak != bank.consumed[slot]) {
    bank.consumed[slot] = peak;
  }
  if (stop) bank.resume_slot = NO_SLOT;
  if (!store_.save(bank)) return Result::storage;
  if (stop) slot_ = NO_SLOT;
  return Result::ok;
}

void Engine::activate(uint8_t slot, const Config& config, uint32_t peak,
                      uint32_t elapsed_seconds, uint32_t now) {
  slot_ = slot;
  config_ = config;
  peak_ = peak;
  started_at_ = now - elapsed_seconds * 1000u;
  reductions_ = 0;
  tap_reductions_ = 0;
  snoozed_ = false;
  manual_ = false;
  ++generation_;
}

Result Engine::start(uint8_t slot, uint32_t wall, uint32_t now) {
  if (slot >= SLOT_COUNT) return Result::invalid_value;
  Bank bank = store_.bank();
  bank.resume_slot = NO_SLOT;
  if (!store_.save(bank)) return Result::storage;
  const Config config = bank.slots[slot];
  activate(slot, config, wall + peakDelay(config) * 60u, 0, now);
  manual_ = true;
  return Result::ok;
}

Result Engine::dismiss() {
  if (!active()) return Result::no_active;
  slot_ = NO_SLOT;
  snoozed_ = false;
  Bank bank = store_.bank();
  bank.resume_slot = NO_SLOT;
  return store_.save(bank) ? Result::ok : Result::storage;
}

Result Engine::snooze(uint32_t now) {
  if (!active() || snoozed_) return Result::no_active;
  Bank bank = store_.bank();
  bank.resume_slot = NO_SLOT;
  snoozed_ = true;
  paused_at_ = now;
  return store_.save(bank) ? Result::ok : Result::storage;
}

Result Engine::reduce(bool temporary) {
  if (!active()) return Result::no_active;
  if (temporary) tap_reductions_ = 1;
  else if (reductions_ < 8) ++reductions_;
  return Result::ok;
}

Result Engine::clockChanged(uint32_t wall) {
  Bank bank = store_.bank();
  bool changed = false;
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    uint32_t peak = 0;
    if (occurrence(bank.slots[slot], wall, peak) && peak != bank.consumed[slot]) {
      bank.consumed[slot] = peak;
      changed = true;
    }
  }
  initialized_ = true;
  return !changed || store_.save(bank) ? Result::ok : Result::storage;
}

Result Engine::tick(uint32_t wall, bool clock_valid, uint32_t now) {
  if (clock_valid && store_.ready()) {
    Bank bank = store_.bank();
    uint8_t selected = NO_SLOT;
    uint32_t selected_peak = 0;
    Config selected_config = {};
    bool changed = false;
    if (!initialized_) {
      initialized_ = true;
      if (bank.resume_slot != NO_SLOT &&
          inWindow(bank.resume_config, bank.resume_peak, wall)) {
        selected = bank.resume_slot;
        selected_peak = bank.resume_peak;
        selected_config = bank.resume_config;
      } else if (bank.resume_slot != NO_SLOT) {
        bank.resume_slot = NO_SLOT;
        changed = true;
      }
    }
    for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
      uint32_t peak = 0;
      if (!occurrence(bank.slots[slot], wall, peak) ||
          peak == bank.consumed[slot]) continue;
      bank.consumed[slot] = peak;
      changed = true;
      if (selected == NO_SLOT ||
          startTime(bank.slots[slot], peak) > startTime(selected_config, selected_peak) ||
          (startTime(bank.slots[slot], peak) == startTime(selected_config, selected_peak) &&
           slot < selected)) {
        selected = slot;
        selected_peak = peak;
        selected_config = bank.slots[slot];
      }
      if (bank.slots[slot].one_shot) bank.slots[slot].enabled = false;
    }
    if (selected != NO_SLOT) {
      bank.resume_slot = selected;
      bank.resume_peak = selected_peak;
      bank.resume_config = selected_config;
      if (!store_.save(bank)) return Result::storage;
      activate(selected, selected_config, selected_peak,
               wall - startTime(selected_config, selected_peak), now);
    } else if (changed && !store_.save(bank)) return Result::storage;
  }
  if (!active()) return Result::ok;
  if (snoozed_) {
    if (now - paused_at_ < 300000u) return Result::ok;
    Bank bank = store_.bank();
    bank.resume_slot = manual_ ? NO_SLOT : slot_;
    bank.resume_peak = peak_ + (now - paused_at_) / 1000;
    bank.resume_config = config_;
    if (!store_.save(bank)) return Result::storage;
    started_at_ += now - paused_at_;
    peak_ = bank.resume_peak;
    snoozed_ = false;
  }
  const uint32_t total = (peakDelay(config_) + config_.hold_minutes +
                         config_.light.fade_out_minutes) * 60000u;
  if (now - started_at_ >= total) return dismiss();
  return Result::ok;
}

bool Engine::frame(uint32_t now, lighting::Frame& result) {
  result = {};
  if (!active() || snoozed_) return true;
  const uint32_t elapsed = now - started_at_;
  const uint32_t ramp = peakDelay(config_) * 60000u;
  const uint32_t light_ramp = config_.light.fade_in_minutes * 60000u;
  const uint32_t lead = ramp - light_ramp;
  if (elapsed < lead) return true;
  const uint32_t light_elapsed = elapsed - lead;
  const uint32_t end = ramp +
      (config_.hold_minutes + config_.light.fade_out_minutes) * 60000u;
  const uint32_t fade = config_.light.fade_out_minutes * 60000u;
  const uint32_t fade_start = end - fade;
  if (elapsed >= end) return true;
  lighting::EffectConfig effect = {config_.light.effect, config_.light.color,
      config_.light.brightness, light_ramp};
  if (effect.effect == lighting::Effect::solid) {
    effect.brightness = static_cast<uint8_t>(effect.brightness *
        (light_ramp == 0 || light_elapsed >= light_ramp ? 255u :
         light_elapsed * 255u / light_ramp) / 255u);
  }
  if (!lighting::render(effect, light_elapsed, result)) return false;
  if (fade != 0 && elapsed >= fade_start) {
    const uint32_t factor = (end - elapsed) * 255u / fade;
    for (lighting::Color& color : result.pixels) {
      color.red = static_cast<uint8_t>(color.red * factor / 255);
      color.green = static_cast<uint8_t>(color.green * factor / 255);
      color.blue = static_cast<uint8_t>(color.blue * factor / 255);
      color.white = static_cast<uint8_t>(color.white * factor / 255);
    }
  }
  for (lighting::Color& color : result.pixels) {
    color.red = static_cast<uint8_t>(color.red >> reductions());
    color.green = static_cast<uint8_t>(color.green >> reductions());
    color.blue = static_cast<uint8_t>(color.blue >> reductions());
    color.white = static_cast<uint8_t>(color.white >> reductions());
  }
  return true;
}

Status Engine::status(uint32_t now) const {
  if (!active()) return {NO_SLOT, Phase::off, 0, 0, false};
  const uint32_t elapsed = elapsedMilliseconds(now);
  const uint32_t ramp = peakDelay(config_) * 60000u;
  const uint32_t end = ramp +
      (config_.hold_minutes + config_.light.fade_out_minutes) * 60000u;
  const Phase phase = snoozed_ ? Phase::snoozed :
      elapsed < ramp ? Phase::ramping :
      elapsed >= end - config_.light.fade_out_minutes * 60000u &&
          config_.light.fade_out_minutes != 0
          ? Phase::fading : Phase::ringing;
  const uint32_t remaining = snoozed_
      ? (now - paused_at_ >= 300000u ? 0 : (300000u - (now - paused_at_)) / 1000)
      : (elapsed >= end ? 0 : (end - elapsed) / 1000);
  return {slot_, phase, peak_, remaining, pressed_ && !held_};
}

uint32_t Engine::elapsedMilliseconds(uint32_t now) const {
  return (snoozed_ ? paused_at_ : now) - started_at_;
}

audio::Request Engine::audioRequest(uint32_t now) const {
  audio::Request request;
  request.alarm = true;
  if (!active() || snoozed_ || config_.sound.path[0] == '\0') return request;
  request.action = audio::Action::start;
  request.volume = config_.sound.volume;
  memcpy(request.path, config_.sound.path, audio::PATH_CAPACITY);
  request.loop = true;
  request.reductions = reductions();
  request.timeline = {true, now, elapsedMilliseconds(now),
      peakDelay(config_) * 60000u,
      (peakDelay(config_) + config_.hold_minutes) * 60000u +
          config_.light.fade_out_minutes * 30000u,
      config_.light.fade_in_minutes * 30000u,
      config_.light.fade_out_minutes * 30000u};
  return request;
}

Result Engine::button(bool pressed, uint32_t now) {
  if (!pressed) {
    const bool short_press = pressed_ && !held_ && active() && !snoozed_;
    pressed_ = false;
    held_ = false;
    return short_press ? snooze(now) : Result::ok;
  }
  if (!active()) return Result::ok;
  if (!pressed_) {
    pressed_ = true;
    pressed_at_ = now;
    held_ = false;
  }
  if (!held_ && now - pressed_at_ >= 2000u) {
    held_ = true;
    return dismiss();
  }
  return Result::ok;
}

void AudioBridge::silence(const Engine& engine, uint32_t now) {
  generation_ = engine.generation();
  active_ = engine.active();
  snoozed_ = engine.status(now).phase == Phase::snoozed;
  reductions_ = engine.reductions();
  pending_ = false;
}

audio::Error AudioBridge::update(const Engine& engine, audio::Control& control,
                                uint32_t now) {
  const bool active = engine.active();
  const bool snoozed = engine.status(now).phase == Phase::snoozed;
  if ((active && (generation_ != engine.generation() || snoozed != snoozed_)) ||
      (!active && active_)) {
    request_ = engine.audioRequest(now);
    pending_ = true;
  } else if (active && !snoozed && reductions_ != engine.reductions()) {
    if (!pending_ || request_.action != audio::Action::start) {
      request_ = {};
      request_.action = audio::Action::reduce;
      request_.reductions = engine.reductions();
      request_.alarm = true;
    }
    pending_ = true;
  }
  generation_ = engine.generation();
  active_ = active;
  snoozed_ = snoozed;
  reductions_ = engine.reductions();
  if (!pending_) return audio::Error::none;
  if (request_.action == audio::Action::start) request_ = engine.audioRequest(now);
  uint32_t id = 0;
  const audio::Error result = control.submit(request_, id);
  if (result == audio::Error::none) pending_ = false;
  return result;
}

}  // namespace alarms
