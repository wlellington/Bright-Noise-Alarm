#pragma once

#include <stdint.h>

namespace tap_input {

constexpr uint8_t ADDRESS = 0x18;
constexpr uint16_t THRESHOLD_MG = 120;
constexpr uint32_t COOLDOWN_MS = 400;

enum class Error { none, unavailable, configuration, read };

struct State {
  uint32_t count = 0;
  uint32_t last_ms = 0;
  bool armed = false;
  bool initialized = false;
  bool motion = false;
  int32_t baseline[3] = {};
  uint16_t delta_mg = 0;
  uint16_t peak_mg = 0;
  uint32_t samples = 0;
  uint32_t started_at = 0;
  uint16_t history[3] = {};
  uint8_t history_index = 0;
  uint16_t filtered_peak_mg = 0;
};

struct Status {
  bool available = false;
  Error error = Error::unavailable;
  State state;
  uint16_t threshold_mg = THRESHOLD_MG;
};

inline uint32_t squareRoot(uint64_t value) {
  uint64_t result = 0;
  uint64_t bit = uint64_t{1} << 62;
  while (bit > value) bit >>= 2;
  while (bit != 0) {
    if (value >= result + bit) {
      value -= result + bit;
      result = (result >> 1) + bit;
    } else result >>= 1;
    bit >>= 2;
  }
  return static_cast<uint32_t>(result);
}

inline bool update(State& state, const int16_t axes[3], uint32_t now,
                   uint16_t threshold_mg = THRESHOLD_MG) {
  ++state.samples;
  if (!state.initialized) {
    for (unsigned i = 0; i < 3; ++i) state.baseline[i] = axes[i] * 256;
    state.initialized = true;
    state.started_at = now;
    return false;
  }
  const bool calibrating = now - state.started_at < 1000u;
  uint64_t squared = 0;
  for (unsigned i = 0; i < 3; ++i) {
    const int32_t delta = axes[i] - state.baseline[i] / 256;
    squared += static_cast<uint64_t>(static_cast<int64_t>(delta) * delta);
    state.baseline[i] += (axes[i] * 256 - state.baseline[i]) /
        (calibrating ? 4 : 32);
  }
  // At 4g/high-resolution, left-aligned raw output is eight counts per milli-g.
  const uint16_t raw = static_cast<uint16_t>(squareRoot(squared) / 8);
  if (raw > state.peak_mg) state.peak_mg = raw;
  state.history[state.history_index] = raw;
  if (++state.history_index == 3) state.history_index = 0;
  const uint16_t a = state.history[0], b = state.history[1], c = state.history[2];
  state.delta_mg = a > b ? (b > c ? b : (a > c ? c : a))
                       : (a > c ? a : (b > c ? c : b));
  if (state.delta_mg > state.filtered_peak_mg)
    state.filtered_peak_mg = state.delta_mg;
  if (calibrating) return false;
  if (state.delta_mg < threshold_mg / 2) state.motion = false;
  if (state.delta_mg < threshold_mg || state.motion) return false;
  state.motion = true;
  if (state.armed && now - state.last_ms < COOLDOWN_MS) return false;
  state.armed = true;
  state.last_ms = now;
  ++state.count;
  return true;
}

inline const char* errorName(Error error) {
  switch (error) {
    case Error::none: return "NONE";
    case Error::unavailable: return "UNAVAILABLE";
    case Error::configuration: return "CONFIG";
    case Error::read: return "READ";
  }
  return "UNKNOWN";
}

}
