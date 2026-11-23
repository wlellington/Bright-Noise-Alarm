#pragma once

#include <stddef.h>
#include <stdint.h>

namespace lighting {

constexpr size_t PIXEL_COUNT = 64;

struct Color {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t white;
};

struct Frame {
  // Row-major, starting at bottom left.
  Color pixels[PIXEL_COUNT];
};

enum class Effect {
  off,
  solid,
  breathe,
  fade,
  sunrise,
  forest,
  beach,
  fire,
  nebula
};

constexpr uint8_t ANIMATION_OPTION_COUNT = 6;
Effect animationEffect(uint8_t index);
bool validAnimationEffect(Effect effect);
const char* effectName(Effect effect);
bool parseEffect(const char* text, size_t length, Effect& effect);

struct EffectConfig {
  Effect effect;
  Color color;
  uint8_t brightness;
  uint32_t duration_ms;
};

// Returns false for an unknown effect or a breathing period below 2 ms.
bool render(const EffectConfig& config, uint32_t elapsed_ms, Frame& frame);

constexpr uint8_t FADE_OPTION_COUNT = 9;
bool fadeMinutes(uint8_t index, uint8_t& minutes);
bool validFadeMinutes(uint8_t minutes);

struct AnimationConfig {
  Effect effect;
  Color color;
  uint8_t brightness;
  uint8_t fade_in_minutes;
  uint8_t fade_out_minutes;
};

enum class RunState { off, running, fading_out };

class Player {
public:
  bool start(const AnimationConfig& config, uint32_t now);
  bool stop(uint32_t now, uint8_t fade_out_minutes);
  void cancel();
  bool frame(uint32_t now, Frame& result);
  RunState state() const { return state_; }
  const AnimationConfig& config() const { return config_; }

private:
  AnimationConfig config_ = {};
  RunState state_ = RunState::off;
  uint32_t started_at_ = 0;
  uint32_t stopped_at_ = 0;
  bool fade_in_complete_ = false;
  Frame stop_frame_ = {};
};

// Coordinates use row 0 at the bottom and column 0 at the left.
bool matrixIndex(uint8_t row, uint8_t column, uint16_t& index);
bool setPixelFrame(uint8_t row, uint8_t column, Color color, Frame& frame);
void outputFrame(const Frame& logical, Frame& output);

}  // namespace lighting
