#include "../Firmware/Light_Clock_Firmware/Matrix_Driver.hpp"

#include <assert.h>
#include <stdint.h>

using namespace lighting;

static bool same(Color a, Color b) {
  return a.red == b.red && a.green == b.green && a.blue == b.blue &&
         a.white == b.white;
}

static void test_calming_sleep_scenes_palette_motion_and_smoothness() {
  const Effect effects[] = {Effect::beach, Effect::fire, Effect::nebula};
  for (Effect selected : effects) {
    EffectConfig config = {selected, {}, 255, 0};
    Frame previous = {};
    assert(render(config, 0, previous));
    bool varied_space = false, varied_time = false;
    bool blue = false, pink = false, purple = false, gold = false, sand = false;
    bool aqua = false;
    for (uint32_t now = 50; now <= 60000; now += 50) {
      Frame frame = {};
      assert(render(config, now, frame));
      Frame output = {};
      outputFrame(frame, output);
      for (size_t i = 0; i < PIXEL_COUNT; ++i) {
        const Color color = frame.pixels[i];
        varied_space |= !same(color, frame.pixels[0]);
        varied_time |= !same(color, previous.pixels[i]);
        blue |= color.blue > color.red && color.blue > color.green;
        purple |= color.red > 50 && color.blue > color.red && color.green < 30;
        pink |= color.red > color.blue && color.blue > 60 && color.green < 80;
        gold |= color.red > color.green && color.green > color.blue && color.green > 80;
        sand |= color.red > color.blue && color.white > 30;
        aqua |= color.red < 16 && color.green > 50 &&
                color.green * 2 >= color.blue && color.white < 20;
        const uint8_t channels[] = {color.red, color.green, color.blue, color.white};
        const Color old = previous.pixels[i];
        const uint8_t old_channels[] = {old.red, old.green, old.blue, old.white};
        for (size_t channel = 0; channel < 4; ++channel) {
          const int delta = channels[channel] - old_channels[channel];
          assert(delta >= -20 && delta <= 20);
          if (selected != Effect::beach) assert(delta >= -4 && delta <= 4);
        }
        const Color limited = output.pixels[i];
        assert(limited.red <= 15 && limited.green <= 15 &&
               limited.blue <= 15 && limited.white <= 15);
        if (selected == Effect::fire)
          assert(color.red >= color.green && color.blue == 0 && color.white <= 12);
        if (selected == Effect::nebula) assert(color.white == 0);
      }
      previous = frame;
    }
    assert(varied_space && varied_time);
    if (selected == Effect::beach) assert(blue && aqua && sand);
    if (selected == Effect::nebula) {
      assert(blue && purple && !pink && !gold);
      for (Color color : previous.pixels) assert(color.white == 0);
    }
    Player player;
    AnimationConfig animation = {selected, {}, 255, 1, 0};
    assert(player.start(animation, 0));
    Frame half = {};
    assert(player.frame(0, half));
    for (Color color : half.pixels) assert(same(color, {}));
    assert(player.frame(30000, half));
    Frame full = {};
    assert(render(config, 30000, full));
    for (size_t i = 0; i < PIXEL_COUNT; ++i) {
      assert(half.pixels[i].red == full.pixels[i].red * 127 / 255);
      assert(half.pixels[i].blue == full.pixels[i].blue * 127 / 255);
    }
  }
}

static void assertUniform(const Frame& frame, Color expected) {
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    assert(same(frame.pixels[i], expected));
  }
}

// REQ-012, REQ-021: independent logical channels and repeating breathing.
static void test_solid_and_breathe() {
  EffectConfig config = {Effect::solid, {200, 150, 100, 50}, 255, 1000};
  Frame frame = {};
  assert(render(config, 0, frame));
  assertUniform(frame, config.color);
  config.brightness = 0;
  assert(render(config, 0, frame));
  assertUniform(frame, {0, 0, 0, 0});
  config.brightness = 255;
  config.effect = Effect::breathe;
  assert(render(config, 0, frame));
  assertUniform(frame, {0, 0, 0, 0});
  assert(render(config, 500, frame));
  assertUniform(frame, config.color);
  assert(render(config, 1000, frame));
  assertUniform(frame, {0, 0, 0, 0});
  assert(render(config, 250, frame));
  const Color rising = frame.pixels[0];
  assert(render(config, 750, frame));
  assertUniform(frame, rising);
  for (size_t channel = 0; channel < 4; ++channel) {
    config.effect = Effect::solid;
    config.color = {static_cast<uint8_t>(channel == 0 ? 255 : 0),
                    static_cast<uint8_t>(channel == 1 ? 255 : 0),
                    static_cast<uint8_t>(channel == 2 ? 255 : 0),
                    static_cast<uint8_t>(channel == 3 ? 255 : 0)};
    assert(render(config, 0, frame));
    assertUniform(frame, config.color);
  }
}

// REQ-011, REQ-032: fade timing and a warm sunrise with a late white finish.
static void test_fade_and_sunrise() {
  EffectConfig config = {Effect::fade, {200, 150, 100, 50}, 255, 1000};
  Frame frame = {};
  assert(render(config, 0, frame));
  assertUniform(frame, {0, 0, 0, 0});
  assert(render(config, 500, frame));
  assert(frame.pixels[0].red > 0 && frame.pixels[0].red < config.color.red);
  assert(render(config, 1000, frame));
  assertUniform(frame, config.color);
  assert(render(config, UINT32_MAX, frame));
  assertUniform(frame, config.color);
  config.effect = Effect::sunrise;
  assert(render(config, 250, frame));
  assert(frame.pixels[0].red > frame.pixels[0].green);
  assert(frame.pixels[0].green > frame.pixels[0].blue);
  assert(frame.pixels[0].white == 0);
  assert(render(config, 500, frame));
  assert(frame.pixels[0].green > 70 && frame.pixels[0].white == 0);
  assert(render(config, 750, frame));
  assert(frame.pixels[0].red > 0 && frame.pixels[0].green > 0);
  assert(frame.pixels[0].white > 0 && frame.pixels[0].blue == 0);
  assert(render(config, 900, frame));
  assert(frame.pixels[0].red > 0 && frame.pixels[0].green > 0);
  assert(frame.pixels[0].white > 220);
  assert(render(config, 1000, frame));
  assertUniform(frame, {0, 0, 0, 255});
  config.duration_ms = 0;
  assert(render(config, 0, frame));
  assertUniform(frame, {0, 0, 0, 255});
  config.effect = Effect::fade;
  config.duration_ms = UINT32_MAX;
  assert(render(config, UINT32_MAX / 2, frame));
  assert(frame.pixels[0].red > 90 && frame.pixels[0].red < 110);
}

// REQ-031: forest light varies by pixel and time and always includes green.
static void test_forest() {
  EffectConfig config = {Effect::forest, {}, 255, 0};
  Frame first = {};
  Frame next = {};
  assert(render(config, 0, first));
  assert(render(config, 2000, next));
  bool different_pixels = false;
  bool different_times = false;
  bool green_present = false;
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    assert(first.pixels[i].blue == 0);
    different_pixels |= !same(first.pixels[i], first.pixels[0]);
    different_times |= !same(first.pixels[i], next.pixels[i]);
    green_present |= first.pixels[i].green > first.pixels[i].red;
  }
  assert(different_pixels && different_times && green_present);
  assert(render(config, 1999, first));
  assert(render(config, 2000, next));
  outputFrame(first, first);
  outputFrame(next, next);
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    const int difference = static_cast<int>(first.pixels[i].red) -
                           static_cast<int>(next.pixels[i].red);
    assert(difference >= -2 && difference <= 2);
  }
}

static void test_forest_moving_shadows_and_opening_canopy() {
  EffectConfig config = {Effect::forest, {}, 255, 60000};
  Frame early = {};
  Frame late = {};
  assert(render(config, 12000, early));
  config.duration_ms = 15000;
  assert(render(config, 12000, late));
  size_t early_lit = 0;
  size_t late_lit = 0;
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    early_lit += early.pixels[i].red != 0;
    late_lit += late.pixels[i].red != 0;
    assert(early.pixels[i].red <= 51 && late.pixels[i].red <= 204);
  }
  assert(early_lit > 0 && early_lit < PIXEL_COUNT);
  assert(late_lit > early_lit);

  config.duration_ms = 0;
  bool shadowed[PIXEL_COUNT] = {};
  bool exposed[PIXEL_COUNT] = {};
  for (uint32_t now = 0; now < 30000; now += 100) {
    assert(render(config, now, late));
    for (size_t i = 0; i < PIXEL_COUNT; ++i) {
      shadowed[i] |= same(late.pixels[i], {0, 0, 0, 0});
      exposed[i] |= late.pixels[i].red > 0;
    }
  }
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    assert(shadowed[i] && exposed[i]);
  }
}

static uint32_t forest_motion_change(uint32_t start) {
  EffectConfig config = {Effect::forest, {}, 255, 60000};
  Frame previous = {};
  uint32_t previous_level = start * 255 / 60000;
  assert(render(config, start, previous));
  uint32_t change = 0;
  for (uint32_t now = start + 100; now <= start + 5000; now += 100) {
    Frame next = {};
    assert(render(config, now, next));
    const uint32_t level = now * 255 / 60000;
    for (size_t i = 0; i < PIXEL_COUNT; ++i) {
      const int difference = static_cast<int>(next.pixels[i].red * 255 / level) -
          static_cast<int>(previous.pixels[i].red * 255 / previous_level);
      change += difference < 0 ? -difference : difference;
    }
    previous = next;
    previous_level = level;
  }
  return change;
}

static void test_forest_acceleration_and_fade_completion() {
  assert(forest_motion_change(50000) > forest_motion_change(6000) * 3 / 2);
  AnimationConfig config = {Effect::forest, {}, 255, 1, 0};
  Player player;
  Frame frame = {};
  assert(player.start(config, 0));
  assert(player.frame(0, frame));
  assertUniform(frame, {0, 0, 0, 0});
  assert(player.frame(60000, frame));
  Frame next = {};
  assert(player.frame(60001, next));
  outputFrame(frame, frame);
  outputFrame(next, next);
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    const int difference = frame.pixels[i].red - next.pixels[i].red;
    assert(difference >= -2 && difference <= 2);
  }
  EffectConfig expected = {Effect::forest, {}, 255, 60000};
  assert(render(expected, 61000, frame));
  assert(player.frame(61000, next));
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    assert(same(frame.pixels[i], next.pixels[i]));
  }
}

static size_t shadow_components(const Frame& frame) {
  bool visited[PIXEL_COUNT] = {};
  size_t count = 0;
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    if (visited[i] || frame.pixels[i].red != 0) continue;
    ++count;
    size_t queue[PIXEL_COUNT] = {i};
    size_t size = 1;
    visited[i] = true;
    for (size_t head = 0; head < size; ++head) {
      const size_t pixel = queue[head];
      const size_t row = pixel / 8;
      const size_t column = pixel % 8;
      const size_t neighbors[] = {row > 0 ? pixel - 8 : pixel,
          row < 7 ? pixel + 8 : pixel, column > 0 ? pixel - 1 : pixel,
          column < 7 ? pixel + 1 : pixel};
      for (size_t neighbor : neighbors) {
        if (!visited[neighbor] && frame.pixels[neighbor].red == 0) {
          visited[neighbor] = true;
          queue[size++] = neighbor;
        }
      }
    }
  }
  return count;
}

static void test_forest_split_patches_and_scattered_green_rims() {
  EffectConfig config = {Effect::forest, {}, 255, 0};
  size_t split_frames = 0;
  size_t rim_green = 0;
  size_t isolated_green = 0;
  size_t previous_rows[8] = {};
  size_t previous_count[8] = {};
  bool moved_up[8] = {};
  bool moved_down[8] = {};
  for (uint32_t now = 0; now < 8000; now += 250) {
    Frame frame = {};
    assert(render(config, now, frame));
    split_frames += shadow_components(frame) >= 3;
    size_t green_count = 0;
    for (size_t i = 0; i < PIXEL_COUNT; ++i) {
      if (frame.pixels[i].green <= frame.pixels[i].red) continue;
      ++green_count;
      const size_t row = i / 8;
      const size_t column = i % 8;
      const bool beside_hole =
          (row > 0 && frame.pixels[i - 8].red == 0) ||
          (row < 7 && frame.pixels[i + 8].red == 0) ||
          (column > 0 && frame.pixels[i - 1].red == 0) ||
          (column < 7 && frame.pixels[i + 1].red == 0);
      rim_green += beside_hole;
      isolated_green += !beside_hole;
    }
    assert(green_count > 0 && green_count < PIXEL_COUNT / 2);
    for (size_t column = 0; column < 8; ++column) {
      size_t rows = 0;
      size_t count = 0;
      for (size_t row = 0; row < 8; ++row) {
        if (frame.pixels[row * 8 + column].red == 0) {
          rows += row;
          ++count;
        }
      }
      if (count != 0 && previous_count[column] != 0) {
        moved_up[column] |= rows * previous_count[column] >
                            previous_rows[column] * count;
        moved_down[column] |= rows * previous_count[column] <
                              previous_rows[column] * count;
      }
      previous_rows[column] = rows;
      previous_count[column] = count;
    }
  }
  assert(split_frames >= 16);
  assert(rim_green > 0 && isolated_green > 0);
  size_t bidirectional_columns = 0;
  for (size_t column = 0; column < 8; ++column) {
    bidirectional_columns += moved_up[column] && moved_down[column];
  }
  assert(bidirectional_columns >= 6);
}

static void test_forest_soft_edges_larger_holes_and_lateral_motion() {
  EffectConfig config = {Effect::forest, {}, 255, 0};
  Frame previous = {};
  assert(render(config, 0, previous));
  outputFrame(previous, previous);
  uint16_t levels[PIXEL_COUNT] = {};
  size_t previous_columns[8] = {};
  size_t previous_count[8] = {};
  bool moved_left[8] = {};
  bool moved_right[8] = {};
  bool two_by_two = false;
  for (uint32_t now = 50; now < 30000; now += 50) {
    Frame frame = {};
    assert(render(config, now, frame));
    outputFrame(frame, frame);
    for (size_t i = 0; i < PIXEL_COUNT; ++i) {
      const int difference = frame.pixels[i].red - previous.pixels[i].red;
      assert(difference >= -5 && difference <= 5);
      const int green_difference = frame.pixels[i].green -
                                   previous.pixels[i].green;
      const int white_difference = frame.pixels[i].white -
                                   previous.pixels[i].white;
      assert(green_difference >= -5 && green_difference <= 5);
      assert(white_difference >= -5 && white_difference <= 5);
      levels[i] |= static_cast<uint16_t>(1u << frame.pixels[i].red);
    }
    for (size_t row = 0; row < 8; ++row) {
      size_t columns = 0;
      size_t count = 0;
      for (size_t column = 0; column < 8; ++column) {
        const size_t i = row * 8 + column;
        if (frame.pixels[i].red != 0) continue;
        columns += column;
        ++count;
        if (row < 7 && column < 7) {
          two_by_two |= frame.pixels[i + 1].red == 0 &&
                        frame.pixels[i + 8].red == 0 &&
                        frame.pixels[i + 9].red == 0;
        }
      }
      if (count != 0 && previous_count[row] != 0) {
        moved_left[row] |= columns * previous_count[row] <
                           previous_columns[row] * count;
        moved_right[row] |= columns * previous_count[row] >
                            previous_columns[row] * count;
      }
      previous_columns[row] = columns;
      previous_count[row] = count;
    }
    previous = frame;
  }
  assert(two_by_two);
  size_t bidirectional_rows = 0;
  for (size_t row = 0; row < 8; ++row) {
    bidirectional_rows += moved_left[row] && moved_right[row];
  }
  assert(bidirectional_rows >= 4);
  for (uint16_t seen : levels) {
    size_t distinct = 0;
    for (; seen != 0; seen >>= 1) distinct += seen & 1;
    assert(distinct >= 5);
  }
}

static void test_invalid_config_does_not_change_frame() {
  EffectConfig config = {Effect::solid, {1, 2, 3, 4}, 255, 0};
  Frame frame = {};
  assert(render(config, 0, frame));
  config.effect = Effect::breathe;
  assert(!render(config, 0, frame));
  assertUniform(frame, {1, 2, 3, 4});
  config.effect = static_cast<Effect>(99);
  assert(!render(config, 0, frame));
  assertUniform(frame, {1, 2, 3, 4});
  config.effect = Effect::off;
  assert(render(config, 0, frame));
  assertUniform(frame, {0, 0, 0, 0});
}

static void test_row_column_mapping_and_output_scaling() {
  uint16_t index = 0;
  assert(matrixIndex(7, 0, index) && index == 0);
  assert(matrixIndex(7, 1, index) && index == 1);
  assert(matrixIndex(6, 0, index) && index == 8);
  assert(matrixIndex(0, 7, index) && index == 63);
  assert(!matrixIndex(8, 0, index));
  assert(!matrixIndex(0, 8, index));

  Frame frame = {};
  assert(setPixelFrame(7, 0, {255, 31, 16, 15}, frame));
  assert(same(frame.pixels[56], {255, 31, 16, 15}));
  assert(!setPixelFrame(8, 0, {1, 1, 1, 1}, frame));
  assert(setPixelFrame(0, 0, {0, 1, 15, 255}, frame));
  Frame output = {};
  outputFrame(frame, output);
  assert(same(output.pixels[56], {15, 1, 1, 0}));
  assert(same(output.pixels[0], {0, 0, 0, 15}));
  assert(same(frame.pixels[56], {255, 31, 16, 15}));

  for (unsigned int value = 0; value <= 255; ++value) {
    const uint8_t input = static_cast<uint8_t>(value);
    for (size_t i = 0; i < PIXEL_COUNT; ++i) {
      frame.pixels[i] = {input, input, input, input};
    }
    outputFrame(frame, output);
    const uint8_t scaled = static_cast<uint8_t>(value / 16);
    assertUniform(output, {scaled, scaled, scaled, scaled});
    assertUniform(frame, {input, input, input, input});
  }
}

static void test_fade_options_and_player_timing() {
  const uint8_t expected[] = {0, 1, 5, 10, 15, 20, 30, 45, 60};
  for (uint8_t i = 0; i < FADE_OPTION_COUNT; ++i) {
    uint8_t minutes = 255;
    assert(fadeMinutes(i, minutes) && minutes == expected[i]);
    assert(validFadeMinutes(minutes));
    Player choice;
    AnimationConfig settings = {Effect::solid, {255, 0, 0, 0}, 255,
                                minutes, minutes};
    Frame test = {};
    assert(choice.start(settings, 0));
    const uint32_t duration = static_cast<uint32_t>(minutes) * 60000u;
    if (duration != 0) {
      assert(choice.frame(duration / 2, test));
      assert(test.pixels[0].red == 127);
      assert(choice.frame(duration - 1, test));
      assert(test.pixels[0].red == 254);
    }
    assert(choice.frame(duration, test));
    assert(test.pixels[0].red == 255);
    assert(choice.stop(duration, minutes));
    if (duration != 0) {
      assert(choice.frame(duration + duration / 2, test));
      assert(test.pixels[0].red == 128);
      assert(choice.frame(duration + duration - 1, test));
      assert(test.pixels[0].red == 1);
    }
    assert(choice.frame(duration + duration, test));
    assert(test.pixels[0].red == 0);
    assert(choice.state() == RunState::off);
  }
  uint8_t untouched = 42;
  assert(!fadeMinutes(FADE_OPTION_COUNT, untouched) && untouched == 42);
  assert(!validFadeMinutes(2) && !validFadeMinutes(61));

  Player player;
  AnimationConfig config = {Effect::solid, {255, 128, 0, 64}, 255, 1, 1};
  Frame frame = {};
  assert(player.start(config, 1000));
  assert(player.frame(1000, frame));
  assertUniform(frame, {0, 0, 0, 0});
  assert(player.frame(31000, frame));
  const Color halfway = frame.pixels[0];
  assert(halfway.red > 120 && halfway.red < 130);
  assert(player.stop(31000, 1));
  assert(player.frame(31000, frame));
  assertUniform(frame, halfway);
  assert(player.stop(41000, 1));
  assert(player.frame(61000, frame));
  assert(frame.pixels[0].red > 55 && frame.pixels[0].red < 70);
  assert(player.frame(91000, frame));
  assertUniform(frame, {0, 0, 0, 0});
  assert(player.state() == RunState::off);

  config.fade_in_minutes = 0;
  config.fade_out_minutes = 0;
  assert(player.start(config, 0));
  assert(player.frame(0, frame));
  assertUniform(frame, config.color);
  assert(player.stop(0, 0));
  assert(player.frame(0, frame));
  assertUniform(frame, {0, 0, 0, 0});

  assert(player.start(config, 0));
  assert(!player.stop(0, 2));
  assert(player.state() == RunState::running);
  assert(player.stop(0, 1));
  assert(player.state() == RunState::fading_out);
  assert(player.frame(60000, frame));
  assertUniform(frame, {0, 0, 0, 0});

  config.fade_in_minutes = 2;
  assert(!player.start(config, 0));
  assert(player.state() == RunState::off);
}

static void test_sunrise_forest_and_clock_wrap() {
  Player player;
  AnimationConfig config = {Effect::sunrise, {255, 180, 64, 255}, 255, 1,
                            0};
  Frame early = {};
  Frame peak = {};
  const uint32_t start = UINT32_MAX - 10000;
  assert(player.start(config, start));
  assert(player.frame(start + 15000u, early));
  assert(early.pixels[0].red > early.pixels[0].green);
  assert(player.frame(start + 60000u, peak));
  assertUniform(peak, {0, 0, 0, 255});
  assert(player.frame(start + 70000u, peak));
  assertUniform(peak, {0, 0, 0, 255});

  config.effect = Effect::forest;
  config.fade_in_minutes = 0;
  assert(player.start(config, 0));
  assert(player.frame(0, early));
  assert(player.frame(2000, peak));
  bool changed = false;
  for (size_t i = 0; i < PIXEL_COUNT; ++i) {
    changed |= !same(early.pixels[i], peak.pixels[i]);
  }
  assert(changed);
  player.cancel();
  assert(player.frame(2000, peak));
  assertUniform(peak, {0, 0, 0, 0});
}

int main() {
  test_calming_sleep_scenes_palette_motion_and_smoothness();
  test_solid_and_breathe();
  test_fade_and_sunrise();
  test_forest();
  test_forest_moving_shadows_and_opening_canopy();
  test_forest_acceleration_and_fade_completion();
  test_forest_split_patches_and_scattered_green_rims();
  test_forest_soft_edges_larger_holes_and_lateral_motion();
  test_invalid_config_does_not_change_frame();
  test_row_column_mapping_and_output_scaling();
  test_fade_options_and_player_timing();
  test_sunrise_forest_and_clock_wrap();
}
