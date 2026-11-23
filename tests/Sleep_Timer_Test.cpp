#include "../Firmware/Light_Clock_Firmware/Sleep_Timer.hpp"

#include <assert.h>
#include <initializer_list>
#include <string.h>

class FakeAudio : public audio::Control {
public:
  audio::Request last;
  audio::Error result = audio::Error::none;
  uint32_t count = 0;
  audio::Error submit(const audio::Request& request, uint32_t& id) override {
    assert(audio::validRequest(request));
    last = request;
    ++count;
    id = count;
    return result;
  }
  audio::Status status() const override { return {}; }
};

static sleep_timer::Config shortConfig() {
  sleep_timer::Config config = sleep_timer::defaults();
  config.hold_minutes = 1;
  config.light = {lighting::Effect::solid, {255, 0, 0, 0}, 255, 0, 1};
  strcpy(config.sound.path, "/quiet morning.wav");
  config.sound.volume = 10;
  return config;
}

class SleepMemory : public settings::Storage {
public:
  uint8_t data[2][sleep_timer::RECORD_SIZE] = {};
  size_t lengths[2] = {};
  bool fail = false;
  bool bad_readback = false;
  uint32_t writes = 0;
  settings::ReadResult read(uint8_t slot, uint8_t* output, size_t capacity,
                            size_t* length) override {
    if (lengths[slot] == 0) return settings::ReadResult::missing;
    assert(lengths[slot] <= capacity);
    *length = lengths[slot];
    memcpy(output, data[slot], *length);
    if (bad_readback) output[*length - 1] ^= 1;
    return settings::ReadResult::ok;
  }
  bool write(uint8_t slot, const uint8_t* input, size_t length) override {
    ++writes;
    lengths[slot] = fail ? length / 2 : length;
    memcpy(data[slot], input, lengths[slot]);
    return !fail;
  }
};

static void test_three_saved_sleep_slots_and_corruption_recovery() {
  SleepMemory memory;
  sleep_timer::Store store(memory);
  assert(store.load() == settings::Result::blank);
  for (uint8_t slot = 0; slot < 3; ++slot) {
    sleep_timer::Config config = sleep_timer::slotDefaults(slot);
    config.hold_minutes = 10 + slot * 5;
    config.audio_enabled = slot != 0;
    config.light_enabled = slot != 1;
    strcpy(config.sound.path, "/Quiet Night.wav");
    config.sound.volume = 1 + slot;
    config.light.brightness = 64 + slot * 32;
    config.light.color = {static_cast<uint8_t>(21 + slot), 43, 65, 87};
    config.light.fade_in_minutes = 0;
    config.light.fade_out_minutes = slot == 2 ? 10 : 5;
    assert(store.save(slot, config));
  }
  sleep_timer::Store restarted(memory);
  assert(restarted.load() == settings::Result::ok);
  for (uint8_t slot = 0; slot < 3; ++slot) {
    assert(restarted.get(slot).hold_minutes == 10 + slot * 5);
    assert(restarted.get(slot).sound.volume == 1 + slot);
    assert(restarted.get(slot).light.brightness == 64 + slot * 32);
    assert(strcmp(restarted.get(slot).sound.path, "/Quiet Night.wav") == 0);
    assert(restarted.get(slot).light.effect == sleep_timer::slotDefaults(slot).light.effect);
    assert(restarted.get(slot).light_enabled == (slot != 1));
    assert(restarted.get(slot).audio_enabled == (slot != 0));
    const lighting::AnimationConfig& light = restarted.get(slot).light;
    assert(light.color.red == 21 + slot && light.color.green == 43 &&
           light.color.blue == 65 && light.color.white == 87);
    assert(light.fade_in_minutes == 0);
    assert(light.fade_out_minutes == (slot == 2 ? 10 : 5));
  }
  sleep_timer::Engine engine;
  assert(!engine.active());
  assert(engine.set(restarted.get(0)) == sleep_timer::Result::ok);
  assert(engine.start(1000, false, 0) == sleep_timer::Result::ok);
  sleep_timer::Config edited = restarted.get(0);
  edited.hold_minutes = 45;
  assert(restarted.save(0, edited));
  assert(engine.activeConfig().hold_minutes == 10);
  memory.fail = true;
  assert(!restarted.save(1, edited) && !restarted.ready());
  const uint32_t writes = memory.writes;
  assert(!restarted.save(2, edited) && memory.writes == writes);
  sleep_timer::Store recovered(memory);
  assert(recovered.load() == settings::Result::ok);
  assert(recovered.get(0).hold_minutes == 45);
  memory.data[0][4] = 3;
  assert(recovered.load() == settings::Result::corrupt && !recovered.ready());
  SleepMemory readback_memory;
  sleep_timer::Store unverified(readback_memory);
  assert(unverified.load() == settings::Result::blank);
  readback_memory.bad_readback = true;
  assert(!unverified.save(0, edited) && !unverified.ready());
  assert(unverified.get(0).hold_minutes == 30);
  readback_memory.bad_readback = false;
  assert(unverified.load() == settings::Result::ok);
  assert(unverified.get(0).hold_minutes == 45);
}

static void test_req_015_040_outputs_expiry_and_synchronized_fades() {
  for (unsigned output = 0; output < 3; ++output) {
    sleep_timer::Config config = shortConfig();
    config.light_enabled = output != 1;
    config.audio_enabled = output != 0;
    sleep_timer::Engine engine;
    FakeAudio audio;
    assert(engine.set(config) == sleep_timer::Result::ok);
    const uint32_t start = UINT32_MAX - 1000;
    assert(engine.start(start, false) == sleep_timer::Result::ok);
    assert(engine.updateAudio(audio, start) == audio::Error::none);
    assert(audio.last.sleep && !audio.last.alarm);
    assert(audio.last.action == (config.audio_enabled
        ? audio::Action::start : audio::Action::stop));
    if (config.audio_enabled) {
      assert(audio.last.loop && audio.last.timeline.enabled);
      assert(audio::gainBasisPoints(audio.last, 0) == 0);
      assert(audio::gainBasisPoints(audio.last, 15000) == 500);
      assert(audio::gainBasisPoints(audio.last, 30000) == 1000);
      assert(audio::gainBasisPoints(audio.last, 60000) == 1000);
      assert(audio::gainBasisPoints(audio.last, 90000) == 500);
      assert(audio::gainBasisPoints(audio.last, 120000) == 0);
    }
    lighting::Frame frame = {};
    assert(engine.frame(start + 30000u, frame));
    for (lighting::Color color : frame.pixels)
      assert(color.red == (config.light_enabled ? 255 : 0));
    assert(engine.frame(start + 15000u, frame));
    for (lighting::Color color : frame.pixels)
      assert(color.red == (config.light_enabled ? 127 : 0));
    assert(engine.frame(start + 75000u, frame));
    for (lighting::Color color : frame.pixels)
      assert(color.red == (config.light_enabled ? 127 : 0));
    assert(engine.frame(start + 90000u, frame));
    for (lighting::Color color : frame.pixels) assert(color.red == 0);
    assert(engine.status(start + 30000u).remaining_seconds == 90);
    engine.tick(start + 119999u);
    assert(engine.active());
    engine.tick(start + 120000u);
    assert(!engine.active() && engine.pending());
    assert(engine.frame(start + 120000u, frame));
    for (lighting::Color color : frame.pixels) assert(color.red == 0);
    assert(engine.updateAudio(audio, start + 120000u) == audio::Error::none);
    assert(audio.last.action == audio::Action::stop && !engine.pending());
  }
}

static void test_req_037_048_053_preemption_stop_and_no_resume() {
  sleep_timer::Engine engine;
  FakeAudio audio;
  assert(engine.set(shortConfig()) == sleep_timer::Result::ok);
  assert(engine.start(100, true) == sleep_timer::Result::busy);
  assert(!engine.active() && !engine.pending());
  assert(engine.start(100, false) == sleep_timer::Result::ok);
  audio.result = audio::Error::busy;
  assert(engine.updateAudio(audio, 500) == audio::Error::busy);
  assert(engine.pending());
  engine.preempt();
  const uint32_t count = audio.count;
  assert(engine.updateAudio(audio, 1000) == audio::Error::none);
  engine.tick(10000);
  assert(!engine.active() && !engine.pending() && audio.count == count);
  assert(engine.start(10000, false) == sleep_timer::Result::ok);
  sleep_timer::Engine restarted;
  assert(!restarted.active() && !restarted.pending());
  engine.stop();
  audio.result = audio::Error::none;
  assert(engine.updateAudio(audio, 10001) == audio::Error::none);
  assert(audio.last.action == audio::Action::stop);
  engine.stop();
  assert(!engine.pending());
}

static void test_delayed_queue_snapshot_and_manual_audio_silence() {
  sleep_timer::Config config = shortConfig();
  config.audio_enabled = true;
  sleep_timer::Engine engine;
  FakeAudio audio;
  assert(engine.set(config) == sleep_timer::Result::ok);
  assert(engine.start(1000, false) == sleep_timer::Result::ok);
  config.hold_minutes = 10;
  config.sound.volume = 1;
  assert(engine.set(config) == sleep_timer::Result::ok);
  assert(engine.updateAudio(audio, 31000) == audio::Error::none);
  assert(audio.last.timeline.elapsed_ms == 30000 && audio.last.timeline.reference_ms == 31000);
  assert(audio.last.volume == 10 && audio.last.timeline.end_ms == 120000);
  assert(engine.status(31000).remaining_seconds == 90);
  assert(engine.start(32000, false) == sleep_timer::Result::ok);
  engine.silence();
  const uint32_t count = audio.count;
  assert(engine.updateAudio(audio, 32001) == audio::Error::none);
  assert(audio.count == count && engine.active());
  assert(!engine.status(32001).audio_enabled);
}

static void test_invalid_configuration_preserves_selected_timer() {
  sleep_timer::Engine engine;
  sleep_timer::Config config = shortConfig();
  assert(engine.set(config) == sleep_timer::Result::ok);
  config.light_enabled = false;
  config.audio_enabled = false;
  assert(engine.set(config) == sleep_timer::Result::invalid_value);
  config = shortConfig();
  config.light.fade_in_minutes = 1;
  assert(!sleep_timer::valid(config));
  config = shortConfig();
  config.hold_minutes = 181;
  assert(!sleep_timer::valid(config));
  config = shortConfig();
  config.audio_enabled = true;
  config.sound.path[0] = '\0';
  assert(!sleep_timer::valid(config));
  config = shortConfig();
  config.sound.volume = 50;
  assert(sleep_timer::valid(config));
  config.sound.volume = 51;
  assert(!sleep_timer::valid(config));
  config = shortConfig();
  config.hold_minutes = 2;
  assert(sleep_timer::valid(config));
  assert(engine.config().hold_minutes == 1 && engine.config().light_enabled);
}

static void test_five_minute_sleep_fade_dark_midpoint_and_zero_fade() {
  sleep_timer::Engine engine;
  sleep_timer::Config config = shortConfig();
  config.hold_minutes = 1;
  config.light.fade_out_minutes = 5;
  config.audio_enabled = true;
  FakeAudio sound;
  assert(engine.set(config) == sleep_timer::Result::ok);
  assert(engine.start(1000, false) == sleep_timer::Result::ok);
  assert(engine.updateAudio(sound, 1000) == audio::Error::none);
  lighting::Frame frame;
  assert(engine.frame(16000, frame) && frame.pixels[0].red == 127);
  assert(audio::gainBasisPoints(sound.last, 15000) == 500);
  assert(engine.frame(31000, frame) && frame.pixels[0].red == 255);
  assert(audio::gainBasisPoints(sound.last, 30000) == 1000);
  assert(engine.frame(61000, frame) && frame.pixels[0].red == 255);
  assert(audio::gainBasisPoints(sound.last, 60000) == 1000);
  assert(engine.frame(136000, frame) && frame.pixels[0].red == 127);
  assert(audio::gainBasisPoints(sound.last, 135000) == 750);
  assert(engine.frame(211000, frame) && frame.pixels[0].red == 0);
  assert(audio::gainBasisPoints(sound.last, 210000) == 500);
  assert(engine.frame(286000, frame) && frame.pixels[0].red == 0);
  assert(audio::gainBasisPoints(sound.last, 285000) == 250);
  assert(engine.frame(361000, frame) && frame.pixels[0].red == 0);
  assert(audio::gainBasisPoints(sound.last, 360000) == 0);
  config.light.fade_out_minutes = 0;
  assert(engine.set(config) == sleep_timer::Result::ok);
  assert(engine.start(1000, false) == sleep_timer::Result::ok);
  assert(engine.frame(60999, frame) && frame.pixels[0].red == 255);
  assert(engine.frame(61000, frame) && frame.pixels[0].red == 0);
}

static void test_legacy_sleep_duration_migrates_hold_and_fixed_rise() {
  SleepMemory memory;
  sleep_timer::Store store(memory);
  assert(store.load() == settings::Result::blank);
  sleep_timer::Config config = sleep_timer::defaults();
  config.hold_minutes = 10;
  assert(store.save(0, config));
  for (uint8_t record = 0; record < 2; ++record) {
    if (!memory.lengths[record]) continue;
    uint8_t* data = memory.data[record];
    data[4] = 1;
    for (uint8_t slot = 0; slot < sleep_timer::SLOT_COUNT; ++slot) {
      uint8_t* entry = data + 16 + slot * 128;
      entry[6] = 1;
      settings::record::put16(entry + 4,
          settings::record::get16(entry + 4) + entry[6] + entry[7]);
    }
    settings::record::put32(data + 508, settings::record::checksum(data, 508));
  }
  sleep_timer::Store migrated(memory);
  assert(migrated.load() == settings::Result::ok);
  assert(migrated.get(0).hold_minutes == 10);
  assert(migrated.get(0).light.fade_in_minutes == 0);
  assert(sleep_timer::durationMilliseconds(migrated.get(0)) == 900000);
  assert(migrated.save(0, migrated.get(0)));
  sleep_timer::Store restarted(memory);
  assert(restarted.load() == settings::Result::ok);
  assert(restarted.get(0).hold_minutes == 10);
}

static void test_sleep_hold_zero_phases_and_longest_timer() {
  sleep_timer::Config config = shortConfig();
  config.hold_minutes = 10;
  config.light.fade_out_minutes = 10;
  config.audio_enabled = true;
  sleep_timer::Engine engine;
  FakeAudio sound;
  assert(engine.set(config) == sleep_timer::Result::ok);
  assert(engine.start(0, false) == sleep_timer::Result::ok);
  assert(engine.updateAudio(sound, 0) == audio::Error::none);
  assert(sound.last.timeline.end_ms == 1200000);
  assert(engine.status(0).remaining_seconds == 1200);
  assert(audio::gainBasisPoints(sound.last, 15000) == 500);
  lighting::Frame frame;
  for (uint32_t elapsed : {30000u, 300000u, 599999u, 600000u}) {
    assert(engine.frame(elapsed, frame) && frame.pixels[0].red == 255);
    assert(audio::gainBasisPoints(sound.last, elapsed) == 1000);
  }
  assert(engine.frame(750000, frame) && frame.pixels[0].red == 127);
  assert(audio::gainBasisPoints(sound.last, 750000) == 750);
  assert(engine.frame(900000, frame) && frame.pixels[0].red == 0);
  assert(audio::gainBasisPoints(sound.last, 900000) == 500);
  assert(engine.frame(1200000, frame) && frame.pixels[0].red == 0);
  engine.tick(1199999);
  assert(engine.active());
  engine.tick(1200000);
  assert(!engine.active());
  config.hold_minutes = 0;
  config.light.fade_out_minutes = 0;
  assert(engine.set(config) == sleep_timer::Result::ok);
  assert(engine.start(0, false) == sleep_timer::Result::ok);
  assert(engine.updateAudio(sound, 0) == audio::Error::none);
  assert(sound.last.action == audio::Action::stop);
  assert(!engine.active());
  engine.tick(0);
  assert(!engine.active());
  config.light.fade_out_minutes = 1;
  assert(engine.set(config) == sleep_timer::Result::ok);
  assert(engine.start(0, false) == sleep_timer::Result::ok);
  assert(engine.updateAudio(sound, 0) == audio::Error::none);
  assert(sound.last.timeline.fade_in_ms == 0);
  assert(sound.last.timeline.end_ms == 60000);
  assert(engine.frame(0, frame) && frame.pixels[0].red == 255);
  assert(audio::gainBasisPoints(sound.last, 0) == 1000);
  assert(engine.frame(30000, frame) && frame.pixels[0].red == 0);
  assert(audio::gainBasisPoints(sound.last, 30000) == 500);
  config.hold_minutes = 180;
  config.light.fade_out_minutes = 60;
  assert(engine.set(config) == sleep_timer::Result::ok);
  assert(engine.start(0, false) == sleep_timer::Result::ok);
  assert(engine.updateAudio(sound, 0) == audio::Error::none);
  assert(sound.last.timeline.end_ms == 14400000);
}

static void test_taps_halve_sleep_without_restarting_fades_and_retry_audio() {
  sleep_timer::Config config = shortConfig();
  config.hold_minutes = 1;
  config.audio_enabled = true;
  sleep_timer::Engine engine, reference;
  assert(engine.set(config) == sleep_timer::Result::ok);
  assert(reference.set(config) == sleep_timer::Result::ok);
  assert(engine.start(100, false) == sleep_timer::Result::ok);
  assert(reference.start(100, false) == sleep_timer::Result::ok);
  assert(engine.reduce(true) == sleep_timer::Result::ok);
  FakeAudio audio;
  audio.result = audio::Error::busy;
  assert(engine.updateAudio(audio, 100) == audio::Error::busy);
  assert(audio.last.action == audio::Action::start && audio.last.reductions == 1);
  assert(engine.reduce(true) == sleep_timer::Result::ok);
  audio.result = audio::Error::none;
  assert(engine.updateAudio(audio, 1100) == audio::Error::none);
  assert(audio.last.timeline.elapsed_ms == 1000 && audio.last.reductions == 1);
  const uint32_t submitted = audio.count;
  for (unsigned i = 0; i < 10; ++i)
    assert(engine.reduce(true) == sleep_timer::Result::ok);
  audio.result = audio::Error::busy;
  assert(engine.updateAudio(audio, 45100) == audio::Error::none);
  assert(audio.count == submitted && engine.reductions() == 1);
  audio.result = audio::Error::none;
  for (uint32_t elapsed : {15000u, 45000u, 105000u, 120000u}) {
    lighting::Frame baseline, reduced;
    assert(reference.frame(100 + elapsed, baseline));
    assert(engine.frame(100 + elapsed, reduced));
    for (unsigned pixel = 0; pixel < lighting::PIXEL_COUNT; ++pixel)
      assert(reduced.pixels[pixel].red == baseline.pixels[pixel].red / 2);
    assert(engine.status(100 + elapsed).remaining_seconds ==
           reference.status(100 + elapsed).remaining_seconds);
  }
  assert(engine.activeConfig().sound.volume == config.sound.volume);
  assert(engine.activeConfig().light.brightness == config.light.brightness);
  engine.clearTapReduction();
  audio.result = audio::Error::busy;
  assert(engine.updateAudio(audio, 90100) == audio::Error::busy);
  assert(audio.last.action == audio::Action::reduce && audio.last.reductions == 0);
  audio.result = audio::Error::none;
  assert(engine.updateAudio(audio, 91100) == audio::Error::none);
  assert(audio.last.action == audio::Action::reduce && audio.last.reductions == 0);
  lighting::Frame expected, restored;
  assert(reference.frame(91100, expected) && engine.frame(91100, restored));
  assert(restored.pixels[0].red == expected.pixels[0].red);
  assert(engine.status(91100).remaining_seconds == reference.status(91100).remaining_seconds);
  assert(engine.reduce() == sleep_timer::Result::ok);
  assert(engine.reduce(true) == sleep_timer::Result::ok);
  engine.clearTapReduction();
  assert(engine.reductions() == 1);
  engine.tick(150100);
  assert(!engine.active());
  assert(engine.start(150101, false) == sleep_timer::Result::ok);
  assert(engine.status(150101).reductions == 0);
}

int main() {
  test_taps_halve_sleep_without_restarting_fades_and_retry_audio();
  test_legacy_sleep_duration_migrates_hold_and_fixed_rise();
  test_sleep_hold_zero_phases_and_longest_timer();
  test_five_minute_sleep_fade_dark_midpoint_and_zero_fade();
  test_three_saved_sleep_slots_and_corruption_recovery();
  test_req_015_040_outputs_expiry_and_synchronized_fades();
  test_req_037_048_053_preemption_stop_and_no_resume();
  test_delayed_queue_snapshot_and_manual_audio_silence();
  test_invalid_configuration_preserves_selected_timer();
}
