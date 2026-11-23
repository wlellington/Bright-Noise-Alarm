#include "Matrix_Driver.hpp"
#include <string.h>

namespace lighting {
namespace {

constexpr uint8_t FADE_MINUTES[] = {0, 1, 5, 10, 15, 20, 30, 45, 60};

uint8_t scale(uint8_t value, uint8_t factor) {
  return static_cast<uint8_t>(static_cast<uint32_t>(value) * factor / 255);
}

Color scaleColor(Color color, uint8_t brightness) {
  return {scale(color.red, brightness), scale(color.green, brightness),
          scale(color.blue, brightness), scale(color.white, brightness)};
}

uint8_t progress(uint32_t elapsed, uint32_t duration) {
  if (duration == 0 || elapsed >= duration) return 255;
  return static_cast<uint8_t>(static_cast<uint64_t>(elapsed) * 255 / duration);
}

uint8_t mix(uint8_t from, uint8_t to, uint8_t amount) {
  return static_cast<uint8_t>(
      (static_cast<uint32_t>(from) * (255 - amount) +
       static_cast<uint32_t>(to) * amount) / 255);
}

int32_t forestSway(uint64_t motion, uint32_t divisor, uint32_t offset,
                   uint32_t extent) {
  const uint32_t half_period = 255 * divisor;
  const uint32_t phase = static_cast<uint32_t>(
      (motion + static_cast<uint64_t>(offset) * divisor) % (2 * half_period));
  const uint32_t position = phase <= half_period
      ? phase : 2 * half_period - phase;
  return static_cast<int32_t>(position * extent / half_period);
}

uint32_t forestRandom(uint32_t value) {
  value ^= value >> 16;
  value *= 0x7feb352du;
  value ^= value >> 15;
  value *= 0x846ca68bu;
  return value ^ (value >> 16);
}

uint64_t forestMotion(uint32_t elapsed, uint32_t duration) {
  if (duration == 0) return static_cast<uint64_t>(elapsed) * 4;
  const uint64_t ramp = elapsed < duration ? elapsed : duration;
  // Integrate a 1x -> 4x speed ramp so motion stays continuous at fade completion.
  return ramp + 3 * (ramp * ramp / duration) / 2 +
         4ull * (elapsed - ramp);
}

void forestFrame(const EffectConfig& config, uint32_t elapsed, Frame& frame) {
  const uint8_t openness = progress(elapsed, config.duration_ms);
  const uint8_t level = scale(config.brightness, openness);
  const uint64_t motion = forestMotion(elapsed, config.duration_ms);
  uint8_t shadows[PIXEL_COUNT] = {};
  constexpr int32_t feather = 384;
  for (uint32_t patch = 0; patch < 16; ++patch) {
    const uint32_t seed = forestRandom(patch + 71);
    const uint32_t vertical_divisor = (16 + seed % 17) * 5 / 4;
    const int32_t center_row = -128 +
        forestSway(motion, vertical_divisor, seed >> 16, 2048);
    const int32_t center_column = static_cast<int32_t>(
        (patch % 8) * 256 + (seed >> 8) % 129) - 64 +
        forestSway(motion, 37 + seed % 29, seed >> 8, 192) - 96;
    const int32_t height = mix(216, 96 + (seed % 4) * 40, openness);
    const int32_t width = mix(216, 96 + ((seed >> 8) % 4) * 40, openness);
    for (size_t i = 0; i < PIXEL_COUNT; ++i) {
      int32_t dx = static_cast<int32_t>(i % 8) * 256 - center_column;
      int32_t dy = static_cast<int32_t>(i / 8) * 256 - center_row;
      if (dx < 0) dx = -dx;
      if (dy < 0) dy = -dy;
      const int32_t edge_x = dx - width;
      const int32_t edge_y = dy - height;
      const int32_t edge = edge_x > edge_y ? edge_x : edge_y;
      const uint8_t shadow = edge <= 0 ? 255 :
          edge >= feather ? 0 : 255 - progress(edge, feather);
      if (shadow > shadows[i]) shadows[i] = shadow;
    }
  }

  const uint32_t green_step = static_cast<uint32_t>(motion / 2000);
  const uint8_t green_phase = progress(
      static_cast<uint32_t>(motion % 2000), 2000);
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    uint8_t green = 0;
    {
      uint8_t neighbor_shadow = shadows[i];
      const size_t row = i / 8;
      const size_t column = i % 8;
      const size_t neighbors[] = {row > 0 ? i - 8 : i,
          row < 7 ? i + 8 : i, column > 0 ? i - 1 : i,
          column < 7 ? i + 1 : i};
      for (size_t neighbor : neighbors) {
        if (shadows[neighbor] > neighbor_shadow) {
          neighbor_shadow = shadows[neighbor];
        }
      }
      const uint8_t scattered = mix(
          static_cast<uint8_t>(forestRandom(i * 97 + green_step)),
          static_cast<uint8_t>(forestRandom(i * 97 + green_step + 1)),
          green_phase);
      const uint8_t edge_green = scattered > 144
          ? progress(scattered - 144, 111) : 0;
      const uint8_t rim = scale(neighbor_shadow, scale(edge_green, 230));
      const uint8_t spot = scattered > 200
          ? progress(scattered - 200, 55) : 0;
      green = rim > spot ? rim : spot;
    }
    const Color color = {mix(255, 70, green), mix(180, 230, green), 0,
                         mix(180, 50, green)};
    frame.pixels[i] = scaleColor(color, scale(level, 255 - shadows[i]));
  }
}

Color sunriseColor(uint8_t amount) {
  if (amount <= 64) {
    return {255, mix(8, 80, progress(amount, 64)), 0, 0};
  }
  if (amount <= 128) {
    return {255, mix(80, 180, progress(amount - 64, 64)), 0, 0};
  }
  if (amount <= 224) {
    return {255, 180, 0, progress(amount - 128, 96)};
  }
  const uint8_t remaining = 255 - progress(amount - 224, 31);
  return {scale(255, remaining), scale(180, remaining), 0, 255};
}

uint8_t smooth(uint8_t value) {
  return static_cast<uint8_t>(static_cast<uint32_t>(value) * value *
                             (765 - 2 * value) / (255 * 255));
}

Color blend(Color from, Color to, uint8_t amount) {
  return {mix(from.red, to.red, amount), mix(from.green, to.green, amount),
          mix(from.blue, to.blue, amount), mix(from.white, to.white, amount)};
}

uint8_t driftingNoise(uint32_t seed, uint32_t elapsed, uint32_t period) {
  const uint32_t step = elapsed / period;
  return mix(static_cast<uint8_t>(forestRandom(seed + step)),
             static_cast<uint8_t>(forestRandom(seed + step + 1)),
             smooth(progress(elapsed % period, period)));
}

uint8_t cloudNoise(uint32_t x, uint32_t y) {
  const uint32_t seed = (x / 256) * 71 + (y / 256) * 197;
  const uint8_t dx = smooth(x % 256);
  const uint8_t dy = smooth(y % 256);
  return mix(mix(forestRandom(seed) & 255, forestRandom(seed + 71) & 255, dx),
             mix(forestRandom(seed + 197) & 255,
                 forestRandom(seed + 268) & 255, dx), dy);
}

void sleepScene(const EffectConfig& config, uint32_t elapsed, Frame& frame) {
  const uint8_t level = scale(config.brightness,
                              progress(elapsed, config.duration_ms));
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    const uint32_t row = i / 8;
    const uint32_t column = i % 8;
    const uint32_t seed = forestRandom(i + 101);
    Color color = {};
    if (config.effect == Effect::beach) {
      const int32_t shore = forestSway(elapsed, 45, 0, 2560) - 384 +
          forestSway(elapsed, 23, column * 61, 224) - 112;
      const int32_t distance = static_cast<int32_t>(row * 256) - shore;
      const uint8_t sand = distance <= -384 ? 0 :
          distance >= 384 ? 255 : smooth(progress(distance + 384, 768));
      const uint8_t wave = driftingNoise(row * 37 + column * 7, elapsed, 6000);
      const Color ocean = blend({0, 62, 112, 0}, {4, 182, 192, 0}, wave);
      color = blend(ocean, {170, 112, 40, 130}, sand);
      const int32_t edge = distance < 0 ? -distance : distance;
      const uint8_t foam = edge >= 320 ? 0 :
          scale(180, smooth(255 - progress(edge, 320)));
      color = blend(color, {80, 152, 154, 180}, foam);
    } else if (config.effect == Effect::fire) {
      const uint8_t flicker = driftingNoise(seed, elapsed, 9000 + seed % 7000);
      const uint8_t ember = driftingNoise(i / 2 + 401, elapsed, 18000);
      const uint8_t warmth = mix(24, 146, flicker);
      color = {255, warmth, 0, scale(12, ember)};
      const uint8_t height = static_cast<uint8_t>(255 - row * 18);
      color = scaleColor(color, scale(height, mix(100, 255, ember)));
    } else {
      const uint32_t x = column * 112 + forestSway(elapsed, 100, 31, 768);
      const uint32_t y = row * 112 + forestSway(elapsed, 135, 137, 768);
      const uint8_t cloud = cloudNoise(x, y);
      color = cloud < 85 ? blend({8, 14, 210, 0}, {58, 8, 200, 0},
                                 progress(cloud, 85)) :
          cloud < 170 ? blend({58, 8, 200, 0}, {116, 16, 210, 0},
                              progress(cloud - 85, 85)) :
          blend({116, 16, 210, 0}, {136, 24, 188, 0},
                progress(cloud - 170, 85));
      if (seed % 23 == 0) color = blend(color, {180, 112, 32, 0}, 48);
      else if (seed % 17 == 0) color = blend(color, {168, 40, 156, 0}, 48);
      color = scaleColor(color, mix(72, 190, cloud));
      if (seed % 5 == 0) {
        const uint8_t pulse = static_cast<uint8_t>(
            forestSway(elapsed, 55 + seed % 41, seed >> 16, 255));
        const uint8_t star = pulse <= 160 ? 0 :
            smooth(progress(pulse - 160, 95));
        color = blend(color, {120, 72, 224, 0}, scale(star, 160));
      }
    }
    frame.pixels[i] = scaleColor(color, level);
  }
}

}  // namespace

Effect animationEffect(uint8_t index) {
  static constexpr Effect effects[] = {Effect::sunrise, Effect::solid,
      Effect::forest, Effect::beach, Effect::fire, Effect::nebula};
  return index < ANIMATION_OPTION_COUNT ? effects[index] : Effect::off;
}

bool validAnimationEffect(Effect effect) {
  for (uint8_t i = 0; i < ANIMATION_OPTION_COUNT; ++i)
    if (animationEffect(i) == effect) return true;
  return false;
}

const char* effectName(Effect effect) {
  switch (effect) {
    case Effect::sunrise: return "SUNRISE";
    case Effect::solid: return "SOLID";
    case Effect::forest: return "FOREST";
    case Effect::beach: return "BEACH";
    case Effect::fire: return "FIRE";
    case Effect::nebula: return "NEBULA";
    default: return "OFF";
  }
}

bool parseEffect(const char* text, size_t length, Effect& effect) {
  for (uint8_t i = 0; i < ANIMATION_OPTION_COUNT; ++i) {
    const Effect candidate = animationEffect(i);
    const char* name = effectName(candidate);
    if (strlen(name) == length && memcmp(text, name, length) == 0) {
      effect = candidate;
      return true;
    }
  }
  return false;
}

bool fadeMinutes(uint8_t index, uint8_t& minutes) {
  if (index >= FADE_OPTION_COUNT) return false;
  minutes = FADE_MINUTES[index];
  return true;
}

bool validFadeMinutes(uint8_t minutes) {
  for (uint8_t value : FADE_MINUTES) {
    if (value == minutes) return true;
  }
  return false;
}

bool render(const EffectConfig& config, uint32_t elapsed_ms, Frame& frame) {
  Color color = config.color;
  uint8_t level = config.brightness;
  switch (config.effect) {
    case Effect::off:
      level = 0;
      break;
    case Effect::solid:
      break;
    case Effect::breathe: {
      if (config.duration_ms < 2) return false;
      const uint64_t phase = elapsed_ms % config.duration_ms;
      const uint64_t twice_phase = phase * 2;
      const uint64_t triangle = twice_phase <= config.duration_ms
                                    ? twice_phase
                                    : 2ull * config.duration_ms - twice_phase;
      level = scale(level, static_cast<uint8_t>(
          triangle * 255 / config.duration_ms));
      break;
    }
    case Effect::fade:
      level = scale(level, progress(elapsed_ms, config.duration_ms));
      break;
    case Effect::sunrise: {
      const uint8_t amount = progress(elapsed_ms, config.duration_ms);
      color = sunriseColor(amount);
      level = scale(level, amount);
      break;
    }
    case Effect::forest:
      forestFrame(config, elapsed_ms, frame);
      return true;
    case Effect::beach:
    case Effect::fire:
    case Effect::nebula:
      sleepScene(config, elapsed_ms, frame);
      return true;
    default:
      return false;
  }

  const Color pixel = scaleColor(color, level);
  for (size_t i = 0; i < PIXEL_COUNT; ++i) frame.pixels[i] = pixel;
  return true;
}

bool Player::start(const AnimationConfig& config, uint32_t now) {
  if (!validAnimationEffect(config.effect) ||
      !validFadeMinutes(config.fade_in_minutes) ||
      !validFadeMinutes(config.fade_out_minutes)) {
    return false;
  }
  config_ = config;
  started_at_ = now;
  fade_in_complete_ = false;
  state_ = RunState::running;
  return true;
}

bool Player::stop(uint32_t now, uint8_t fade_out_minutes) {
  if (!validFadeMinutes(fade_out_minutes)) return false;
  if (state_ != RunState::running) return true;
  if (!frame(now, stop_frame_)) return false;
  config_.fade_out_minutes = fade_out_minutes;
  stopped_at_ = now;
  state_ = config_.fade_out_minutes == 0 ? RunState::off
                                         : RunState::fading_out;
  return true;
}

void Player::cancel() {
  state_ = RunState::off;
}

bool Player::frame(uint32_t now, Frame& result) {
  if (state_ == RunState::off) {
    result = {};
    return true;
  }
  if (state_ == RunState::fading_out) {
    const uint32_t duration = static_cast<uint32_t>(config_.fade_out_minutes) *
                              60000u;
    const uint32_t elapsed = now - stopped_at_;
    if (elapsed >= duration) {
      state_ = RunState::off;
      result = {};
      return true;
    }
    const uint8_t factor = static_cast<uint8_t>(255 - progress(elapsed, duration));
    for (size_t i = 0; i < PIXEL_COUNT; ++i) {
      result.pixels[i] = scaleColor(stop_frame_.pixels[i], factor);
    }
    return true;
  }

  const uint32_t duration = static_cast<uint32_t>(config_.fade_in_minutes) *
                            60000u;
  const uint32_t elapsed = now - started_at_;
  if (elapsed >= duration) fade_in_complete_ = true;
  const uint8_t fade = fade_in_complete_ ? 255 : progress(elapsed, duration);
  EffectConfig effect = {config_.effect, config_.color, config_.brightness,
                         duration};
  if (config_.effect == Effect::sunrise) {
    return render(effect, fade_in_complete_ ? duration : elapsed, result);
  }
  if (config_.effect != Effect::solid) {
    if (fade_in_complete_ && elapsed < duration) effect.duration_ms = 0;
    return render(effect, elapsed, result);
  }
  effect.brightness = scale(effect.brightness, fade);
  return render(effect, elapsed, result);
}

bool matrixIndex(uint8_t row, uint8_t column, uint16_t& index) {
  if (row >= 8 || column >= 8) return false;
  index = static_cast<uint16_t>((7 - row) * 8 + column);
  return true;
}

bool setPixelFrame(uint8_t row, uint8_t column, Color color, Frame& frame) {
  uint16_t index;
  if (!matrixIndex(row, column, index)) return false;
  frame.pixels[row * 8 + column] = color;
  return true;
}

void outputFrame(const Frame& logical, Frame& output) {
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    const Color color = logical.pixels[i];
    output.pixels[i] = {static_cast<uint8_t>(color.red >> 4),
                        static_cast<uint8_t>(color.green >> 4),
                        static_cast<uint8_t>(color.blue >> 4),
                        static_cast<uint8_t>(color.white >> 4)};
  }
}

}  // namespace lighting
