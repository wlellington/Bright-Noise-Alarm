#include "../Firmware/Light_Clock_Firmware/Alarm_System.hpp"

#include <assert.h>
#include <string.h>
#include <vector>

using namespace alarms;

class AlarmMemory : public settings::Storage {
public:
  uint8_t data[2][RECORD_SIZE] = {};
  size_t lengths[2] = {};
  bool fail = false;
  settings::ReadResult read(uint8_t slot, uint8_t* output, size_t capacity,
                            size_t* length) override {
    if (lengths[slot] == 0) return settings::ReadResult::missing;
    assert(lengths[slot] <= capacity);
    *length = lengths[slot];
    memcpy(output, data[slot], *length);
    return settings::ReadResult::ok;
  }
  bool write(uint8_t slot, const uint8_t* input, size_t length) override {
    lengths[slot] = fail ? length / 2 : length;
    memcpy(data[slot], input, lengths[slot]);
    return !fail;
  }
};

constexpr uint32_t MONDAY = 946857600;
constexpr uint32_t PEAK = MONDAY + 7 * 3600;

static Config configured(bool one_shot = false) {
  Config config = defaults();
  config.enabled = true;
  config.one_shot = one_shot;
  config.days = (1 << 1) | (1 << 5);
  return config;
}

// REQ-006/007/018: independent saved slots and selected weekdays.
static void test_saved_slots_and_days() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    Config config = configured();
    config.minute += slot;
    if (slot < 3) config.light.effect = lighting::animationEffect(slot + 3);
    assert(engine.set(slot, config, MONDAY) == Result::ok);
  }
  Config config = engine.get(2);
  config.enabled = false;
  assert(engine.set(2, config, MONDAY) == Result::ok);
  Store restarted(memory);
  assert(restarted.load() == settings::Result::ok);
  for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
    assert(restarted.bank().slots[slot].enabled == (slot != 2));
    assert(restarted.bank().slots[slot].minute == 420 + slot);
    if (slot < 3)
      assert(restarted.bank().slots[slot].light.effect == lighting::animationEffect(slot + 3));
  }
  uint32_t peak = 0;
  config = configured();
  assert(occurrence(config, PEAK, peak) && peak == PEAK);
  assert(!occurrence(config, PEAK + 86400, peak));
  assert(occurrence(config, PEAK + 4 * 86400, peak));
  config.minute = 5;
  config.light.fade_in_minutes = 30;
  assert(occurrence(config, MONDAY - 10 * 60, peak) && peak == MONDAY + 300);
}

// REQ-008/030/049: pre-alarm ramp, durable one-shot and elapsed restart recovery.
static void test_ramp_one_shot_and_recovery() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = configured(true);
  config.hold_minutes = 1;
  config.light.fade_out_minutes = 1;
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.tick(PEAK - 1801, true, 0) == Result::ok);
  assert(!engine.active());
  assert(engine.tick(PEAK - 1800, true, 1000) == Result::ok);
  assert(engine.status(1000).phase == Phase::ramping);
  assert(!engine.get(0).enabled);
  lighting::Frame frame = {};
  assert(engine.frame(1000, frame) && frame.pixels[0].white == 0);
  Store restarted(memory);
  assert(restarted.load() == settings::Result::ok);
  Engine recovered(restarted);
  assert(recovered.tick(PEAK - 300, true, 500) == Result::ok);
  assert(recovered.active());
  assert(recovered.frame(500, frame));
  assert(frame.pixels[0].red > 0 && frame.pixels[0].white > 0);
  assert(recovered.tick(PEAK, true, 300500) == Result::ok);
  assert(recovered.frame(300500, frame));
  assert(frame.pixels[0].red == 0 && frame.pixels[0].white == 255);
  assert(recovered.tick(PEAK + 90, true, 390500) == Result::ok);
  assert(recovered.frame(390500, frame));
  assert(frame.pixels[0].white > 100 && frame.pixels[0].white < 140);
  assert(recovered.tick(PEAK + 120, true, 420500) == Result::ok);
  assert(!recovered.active());
  Store ended(memory);
  assert(ended.load() == settings::Result::ok);
  Engine next(ended);
  assert(next.tick(PEAK + 7 * 86400, true, 0) == Result::ok);
  assert(!next.active());
}

// REQ-010/036/054: exactly five minutes, dismissal and snooze power-loss behavior.
static void test_snooze_dismiss_and_button() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = configured();
  config.light.fade_in_minutes = 0;
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.tick(PEAK, true, 1000) == Result::ok);
  assert(engine.button(true, 2000) == Result::ok);
  assert(engine.status(2000).hold_reminder);
  assert(engine.button(false, 2100) == Result::ok);
  assert(engine.status(2100).phase == Phase::snoozed);
  assert(engine.button(true, 2200) == Result::ok);
  assert(engine.button(false, 2300) == Result::ok);
  assert(engine.status(2300).phase == Phase::snoozed);
  lighting::Frame frame = {};
  assert(engine.frame(2100, frame) && frame.pixels[0].white == 0);
  Store restarted(memory);
  assert(restarted.load() == settings::Result::ok);
  Engine no_resume(restarted);
  assert(no_resume.tick(PEAK + 10, true, 0) == Result::ok);
  assert(!no_resume.active());
  assert(engine.tick(PEAK + 300, true, 302099) == Result::ok);
  assert(engine.status(302099).phase == Phase::snoozed);
  assert(engine.tick(PEAK + 301, true, 302100) == Result::ok);
  assert(engine.status(302100).phase == Phase::ringing);
  assert(engine.button(true, 303000) == Result::ok);
  assert(engine.button(true, 304999) == Result::ok && engine.active());
  assert(engine.button(true, 305000) == Result::ok && !engine.active());
  assert(engine.button(false, 305100) == Result::ok);
}

// REQ-037/053/055: tie priority, preemption, no clock-set replay.
static void test_priority_clock_changes_and_invalid_time() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = configured();
  config.light.fade_in_minutes = 0;
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.set(1, config, MONDAY) == Result::ok);
  assert(engine.tick(PEAK, false, 0) == Result::ok && !engine.active());
  assert(engine.tick(PEAK, true, 1000) == Result::ok);
  assert(engine.status(1000).slot == 0);
  config.minute += 1;
  assert(engine.set(2, config, MONDAY) == Result::ok);
  assert(engine.tick(PEAK + 60, true, 61000) == Result::ok);
  assert(engine.status(61000).slot == 2);
  assert(engine.dismiss() == Result::ok);
  assert(engine.tick(PEAK + 61, true, 62000) == Result::ok && !engine.active());
  assert(engine.clockChanged(PEAK + 7 * 86400) == Result::ok);
  assert(engine.tick(PEAK + 7 * 86400, true, 63000) == Result::ok);
  assert(!engine.active());
}

static void test_storage_failure_and_manual_test() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  assert(engine.start(SLOT_COUNT, 0, 0) == Result::invalid_value);
  assert(engine.start(0, 0, UINT32_MAX - 1000) == Result::ok);
  assert(engine.tick(0, false, 0) == Result::ok && engine.active());
  assert(engine.reduce() == Result::ok);
  memory.fail = true;
  assert(engine.dismiss() == Result::storage && !engine.active());
  Store recovered(memory);
  assert(recovered.load() == settings::Result::ok);
  Engine next(recovered);
  assert(next.tick(PEAK, true, 0) == Result::ok && !next.active());
  memory.data[0][4] = 5;
  assert(recovered.load() == settings::Result::corrupt);
}

// REQ-009/033: running outputs retain their saved snapshot and can be halved.
static void test_output_snapshot_reduction_and_disable() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = configured();
  config.light.effect = lighting::Effect::solid;
  config.light.fade_in_minutes = 0;
  config.light.color = {200, 100, 40, 0};
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.tick(PEAK, true, 1000) == Result::ok);
  lighting::Frame frame = {};
  assert(engine.frame(1000, frame) && frame.pixels[0].red == 200);
  assert(engine.reduce() == Result::ok);
  assert(engine.frame(1000, frame));
  assert(frame.pixels[0].red == 100 && frame.pixels[0].green == 50);
  config.light.color = {0, 200, 0, 0};
  assert(engine.set(0, config, PEAK + 5) == Result::ok);
  assert(engine.frame(2000, frame) && frame.pixels[0].red == 100);
  Store restarted(memory);
  assert(restarted.load() == settings::Result::ok);
  Engine recovered(restarted);
  assert(recovered.tick(PEAK + 10, true, 0) == Result::ok);
  assert(recovered.frame(0, frame) && frame.pixels[0].red == 200);
  config.enabled = false;
  assert(recovered.set(0, config, PEAK + 10) == Result::ok);
  assert(!recovered.active());
}

static void test_simultaneous_ramp_starts_use_lowest_slot() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config first = configured();
  first.light.fade_in_minutes = 0;
  Config second = first;
  second.minute += 60;
  second.light.fade_in_minutes = 60;
  assert(engine.set(0, first, MONDAY) == Result::ok);
  assert(engine.set(1, second, MONDAY) == Result::ok);
  assert(engine.tick(PEAK, true, 0) == Result::ok);
  assert(engine.status(0).slot == 0);
  assert(engine.tick(PEAK + 10, true, 10000) == Result::ok);
  assert(engine.status(10000).slot == 0);
}

static void test_clock_correction_does_not_suppress_future_schedule() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = configured();
  config.light.fade_in_minutes = 0;
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.clockChanged(PEAK + 7 * 86400) == Result::ok);
  assert(engine.clockChanged(MONDAY) == Result::ok);
  assert(engine.tick(PEAK, true, 1000) == Result::ok);
  assert(engine.active());
}

static void test_manual_snooze_resume_does_not_consume_or_restart() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = configured(true);
  config.light.fade_in_minutes = 0;
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.start(0, MONDAY, 1000) == Result::ok);
  assert(engine.snooze(2000) == Result::ok);
  assert(engine.tick(MONDAY + 301, true, 302000) == Result::ok);
  assert(engine.status(302000).phase == Phase::ringing);
  assert(engine.get(0).enabled && store.bank().consumed[0] == 0);
  Store restarted(memory);
  assert(restarted.load() == settings::Result::ok);
  Engine recovered(restarted);
  assert(recovered.tick(MONDAY + 302, true, 0) == Result::ok);
  assert(!recovered.active() && recovered.get(0).enabled);
}

static void test_editing_consumed_one_shot_keeps_active_snapshot() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = configured(true);
  config.light.effect = lighting::Effect::solid;
  config.light.fade_in_minutes = 0;
  config.light.color = {200, 100, 40, 0};
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.tick(PEAK, true, 1000) == Result::ok);
  config = engine.get(0);
  assert(!config.enabled);
  config.light.color = {0, 200, 0, 0};
  assert(engine.set(0, config, PEAK + 1) == Result::ok);
  lighting::Frame frame = {};
  assert(engine.active() && engine.frame(2000, frame));
  assert(frame.pixels[0].red == 200 && frame.pixels[0].green == 100);
}

// REQ-009/030/049: migrate without surprise sound, align shared ramps,
// retain the active sound snapshot and recover its elapsed position.
static void test_audio_settings_migration_and_shared_ramps() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = configured();
  config.light.effect = lighting::Effect::solid;
  config.light.fade_in_minutes = 1;
  config.light.color = {0, 0, 0, 255};
  assert(engine.set(0, config, MONDAY) == Result::ok);
  for (uint8_t slot = 0; slot < 2; ++slot) if (memory.lengths[slot]) {
    memory.data[slot][4] = 1;
    settings::record::put32(memory.data[slot] + 252,
        settings::record::checksum(memory.data[slot], 252));
    memory.lengths[slot] = 256;
  }
  Store migrated(memory);
  assert(migrated.load() == settings::Result::ok);
  Engine next(migrated);
  assert(next.get(0).sound.path[0] == '\0');
  assert(next.get(0).light.fade_in_minutes == 1);
  config = next.get(0);
  strcpy(config.sound.path, "/morning.wav");
  config.sound.volume = 10;
  config.light.fade_in_minutes = 5;
  assert(next.set(0, config, MONDAY) == Result::ok);
  assert(next.tick(PEAK - 300, true, 1000) == Result::ok);
  lighting::Frame frame;
  assert(next.frame(1000, frame) && frame.pixels[0].white == 0);
  assert(next.frame(240999, frame) && frame.pixels[0].white == 203);
  assert(next.frame(271000, frame) && frame.pixels[0].white == 229);
  audio::Request request = next.audioRequest(271000);
  assert(request.timeline.peak_ms == 300000 && request.timeline.elapsed_ms == 270000);
  assert(audio::gainBasisPoints(request, 149999) == 0);
  assert(audio::gainBasisPoints(request, 150000) == 0);
  assert(next.frame(151000, frame) && frame.pixels[0].white == 127);
  assert(audio::gainBasisPoints(request, 225000) == 500);
  assert(next.frame(226000, frame) && frame.pixels[0].white == 191);
  assert(audio::gainBasisPoints(request, 270000) == 800);
  assert(next.frame(301000, frame) && frame.pixels[0].white == 255);
  assert(audio::gainBasisPoints(request, 300000) == 1000);
  config.sound.path[1] = 'X';
  assert(next.set(0, config, MONDAY) == Result::ok);
  assert(strcmp(next.audioRequest(301000).path, "/morning.wav") == 0);
  for (uint8_t slot = 0; slot < 2; ++slot) {
    if (memory.lengths[slot] != RECORD_SIZE) continue;
    memory.data[slot][4] = 2;
    memory.data[slot][353] = 60;
    memory.data[slot][897] = 60;
    settings::record::put32(memory.data[slot] + 1020,
        settings::record::checksum(memory.data[slot], 1020));
  }
  Store restarted(memory);
  assert(restarted.load() == settings::Result::ok);
  Engine recovered(restarted);
  assert(recovered.tick(PEAK - 30, true, 5000) == Result::ok);
  assert(recovered.audioRequest(5000).timeline.elapsed_ms == 270000);
  assert(strcmp(recovered.audioRequest(5000).path, "/morning.wav") == 0);
  assert(recovered.frame(5000, frame) && frame.pixels[0].white == 229);
  assert(recovered.audioRequest(5000).timeline.fade_in_ms == 150000);
  config.light.fade_in_minutes = 0;
  assert(recovered.set(0, config, MONDAY) == Result::ok);
  assert(recovered.start(0, MONDAY, 6000) == Result::ok);
  assert(recovered.frame(6000, frame) && frame.pixels[0].white == 255);
  assert(audio::gainBasisPoints(recovered.audioRequest(6000), 0) == 1000);
}

class AudioControl : public audio::Control {
public:
  bool busy = false;
  std::vector<audio::Request> requests;
  audio::Error submit(const audio::Request& request, uint32_t& id) override {
    if (busy) return audio::Error::busy;
    requests.push_back(request);
    id = static_cast<uint32_t>(requests.size());
    return audio::Error::none;
  }
  audio::Status status() const override { return {}; }
};

static void test_audio_bridge_retry_snooze_dismiss_and_half() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = defaults();
  strcpy(config.sound.path, "/sound.wav");
  config.light.fade_in_minutes = 5;
  assert(engine.set(0, config, MONDAY) == Result::ok);
  AudioBridge bridge;
  AudioControl control;
  control.busy = true;
  assert(engine.start(0, MONDAY, 1000) == Result::ok);
  assert(bridge.update(engine, control, 1000) == audio::Error::busy);
  control.busy = false;
  assert(bridge.update(engine, control, 2000) == audio::Error::none);
  assert(control.requests.back().timeline.elapsed_ms == 1000);
  assert(control.requests.back().timeline.peak_ms == 300000);
  assert(control.requests.back().timeline.fade_in_ms == 150000);
  assert(engine.reduce() == Result::ok);
  assert(bridge.update(engine, control, 2000) == audio::Error::none);
  assert(control.requests.back().action == audio::Action::reduce);
  assert(control.requests.back().reductions == 1);
  assert(engine.snooze(2000) == Result::ok);
  assert(bridge.update(engine, control, 2000) == audio::Error::none);
  assert(control.requests.back().action == audio::Action::stop);
  assert(engine.tick(MONDAY + 301, true, 302000) == Result::ok);
  assert(bridge.update(engine, control, 302000) == audio::Error::none);
  assert(control.requests.back().action == audio::Action::start);
  assert(control.requests.back().timeline.elapsed_ms == 1000);
  assert(control.requests.back().reductions == 1);
  assert(engine.dismiss() == Result::ok);
  assert(bridge.update(engine, control, 302001) == audio::Error::none);
  assert(control.requests.back().action == audio::Action::stop);
}

static void test_temporary_tap_restore_preserves_manual_dim_and_fade() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = defaults();
  config.light = {lighting::Effect::solid, {255, 0, 0, 0}, 255, 0, 1};
  config.hold_minutes = 1;
  strcpy(config.sound.path, "/sound.wav");
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.start(0, MONDAY, 1000) == Result::ok);
  AudioBridge bridge;
  AudioControl control;
  assert(bridge.update(engine, control, 1000) == audio::Error::none);
  assert(engine.reduce() == Result::ok);
  assert(engine.reduce(true) == Result::ok);
  assert(engine.reduce(true) == Result::ok);
  assert(engine.reductions() == 2);
  assert(bridge.update(engine, control, 61000) == audio::Error::none);
  assert(control.requests.back().action == audio::Action::reduce);
  assert(control.requests.back().reductions == 2);
  const size_t submitted = control.requests.size();
  assert(engine.reduce(true) == Result::ok);
  assert(bridge.update(engine, control, 62000) == audio::Error::none);
  assert(control.requests.size() == submitted);
  lighting::Frame reduced;
  assert(engine.frame(91000, reduced) && reduced.pixels[0].red == 31);
  const Status before = engine.status(91000);
  const uint32_t generation = engine.generation();
  engine.clearTapReduction();
  control.busy = true;
  assert(bridge.update(engine, control, 91000) == audio::Error::busy);
  control.busy = false;
  assert(bridge.update(engine, control, 92000) == audio::Error::none);
  assert(control.requests.back().action == audio::Action::reduce);
  assert(control.requests.back().reductions == 1);
  assert(engine.reductions() == 1 && engine.generation() == generation);
  lighting::Frame restored;
  assert(engine.frame(91000, restored) && restored.pixels[0].red == 63);
  assert(engine.status(91000).remaining_seconds == before.remaining_seconds);
  assert(engine.status(91000).phase == Phase::fading);
  assert(engine.activeConfig().light.brightness == 255);
  assert(engine.activeConfig().sound.volume == config.sound.volume);
  for (unsigned i = 0; i < 10; ++i) assert(engine.reduce(true) == Result::ok);
  assert(engine.reductions() == 2);
  engine.clearTapReduction();
  assert(engine.reductions() == 1);
  assert(engine.start(0, MONDAY, 93000) == Result::ok);
  assert(engine.reductions() == 0);
}

// REQ-030: light starts at 06:30, audio at 06:45, and both peak at 07:00.
static void test_req_030_thirty_minute_light_and_audio_ramps() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = configured();
  config.light.effect = lighting::Effect::solid;
  config.light.color = {0, 0, 0, 255};
  strcpy(config.sound.path, "/morning.wav");
  config.sound.volume = 10;
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.tick(PEAK - 1801, true, 0) == Result::ok && !engine.active());
  assert(engine.tick(PEAK - 1800, true, 1000) == Result::ok && engine.active());
  lighting::Frame frame;
  assert(engine.frame(1000, frame) && frame.pixels[0].white == 0);
  const audio::Request request = engine.audioRequest(1000);
  assert(request.timeline.peak_ms == 1800000 && request.timeline.fade_in_ms == 900000);
  assert(audio::gainBasisPoints(request, 0) == 0);
  assert(engine.frame(901000, frame) && frame.pixels[0].white == 127);
  assert(audio::gainBasisPoints(request, 900000) == 0);
  assert(audio::gainBasisPoints(request, 1350000) == 500);
  assert(engine.frame(1801000, frame) && frame.pixels[0].white == 255);
  assert(audio::gainBasisPoints(request, 1800000) == 1000);
}

static void test_req_009_saved_sound_volume_duration_and_fade_out() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config first = defaults();
  strcpy(first.sound.path, "/first.wav");
  first.sound.volume = 3;
  first.light.fade_in_minutes = 5;
  first.hold_minutes = 2;
  Config second = first;
  strcpy(second.sound.path, "/second.wav");
  second.sound.volume = 10;
  second.light.effect = lighting::Effect::solid;
  second.light.color = {0, 0, 0, 255};
  second.hold_minutes = 5;
  second.light.fade_out_minutes = 5;
  assert(engine.set(0, first, MONDAY) == Result::ok);
  assert(engine.set(1, second, MONDAY) == Result::ok);
  Store loaded(memory);
  assert(loaded.load() == settings::Result::ok);
  Engine next(loaded);
  assert(next.start(0, MONDAY, 1000) == Result::ok);
  assert(strcmp(next.audioRequest(1000).path, "/first.wav") == 0);
  assert(next.audioRequest(1000).volume == 3);
  assert(next.audioRequest(1000).timeline.end_ms == 420000);
  assert(next.get(0).light.effect == lighting::Effect::sunrise);
  assert(next.start(1, MONDAY, 1000) == Result::ok);
  audio::Request request = next.audioRequest(1000);
  assert(strcmp(request.path, "/second.wav") == 0 && request.volume == 10);
  assert(request.timeline.end_ms == 750000);
  lighting::Frame frame;
  assert(next.frame(751000, frame) && frame.pixels[0].white == 127);
  assert(audio::gainBasisPoints(request, 675000) == 500);
  assert(audio::gainBasisPoints(request, 750000) == 0);
  assert(next.frame(871000, frame) && frame.pixels[0].white == 25);
  assert(audio::gainBasisPoints(request, 870000) == 0);
}

static void test_manual_audio_stop_cancels_pending_alarm_start() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = defaults();
  strcpy(config.sound.path, "/sound.wav");
  assert(engine.set(0, config, MONDAY) == Result::ok);
  AudioBridge bridge;
  AudioControl control;
  assert(engine.start(0, MONDAY, 1000) == Result::ok);
  control.busy = true;
  assert(bridge.update(engine, control, 1000) == audio::Error::busy);
  control.busy = false;
  uint32_t id = 0;
  audio::Request stop;
  assert(control.submit(stop, id) == audio::Error::none);
  bridge.silence(engine, 1001);
  assert(bridge.update(engine, control, 1002) == audio::Error::none);
  assert(!bridge.pending() && control.requests.size() == 1);
  assert(engine.start(0, MONDAY, 2000) == Result::ok);
  assert(control.submit(stop, id) == audio::Error::none);
  bridge.silence(engine, 2001);
  assert(bridge.update(engine, control, 2002) == audio::Error::none);
  assert(control.requests.size() == 2);
  assert(engine.snooze(3000) == Result::ok);
  assert(bridge.update(engine, control, 3000) == audio::Error::none);
  assert(engine.tick(MONDAY + 301, true, 303000) == Result::ok);
  assert(bridge.update(engine, control, 303000) == audio::Error::none);
  assert(control.requests.back().action == audio::Action::start);
}

static void test_animated_alarm_keeps_moving_through_peak_and_fade_out() {
  for (lighting::Effect animation : {lighting::Effect::forest,
                                    lighting::Effect::beach,
                                    lighting::Effect::fire,
                                    lighting::Effect::nebula}) {
    AlarmMemory memory;
    Store store(memory);
    assert(store.load() == settings::Result::blank);
    Engine engine(store);
    Config config = defaults();
    config.light.effect = animation;
    config.light.fade_in_minutes = 20;
    config.light.fade_out_minutes = 10;
    config.hold_minutes = 0;
    assert(engine.set(0, config, MONDAY) == Result::ok);
    assert(engine.start(0, MONDAY, 0) == Result::ok);
    lighting::EffectConfig effect = {animation, config.light.color,
        config.light.brightness, 1200000};
    lighting::Frame peak;
    assert(engine.frame(1200000, peak));
    lighting::Frame later;
    assert(lighting::render(effect, 1205000, later));
    assert(memcmp(&peak, &later, sizeof(peak)) != 0);
    for (uint32_t elapsed : {1200000u, 1205000u, 1500000u, 1799000u}) {
      lighting::Frame expected;
      assert(lighting::render(effect, elapsed, expected));
      const uint32_t factor = (1800000u - elapsed) * 255u / 600000u;
      for (lighting::Color& color : expected.pixels) {
        color.red = static_cast<uint8_t>(color.red * factor / 255u);
        color.green = static_cast<uint8_t>(color.green * factor / 255u);
        color.blue = static_cast<uint8_t>(color.blue * factor / 255u);
        color.white = static_cast<uint8_t>(color.white * factor / 255u);
      }
      lighting::Frame actual;
      assert(engine.frame(elapsed, actual));
      assert(memcmp(&actual, &expected, sizeof(actual)) == 0);
    }
    lighting::Frame ended;
    lighting::Frame black = {};
    assert(engine.frame(1800000, ended));
    assert(memcmp(&ended, &black, sizeof(ended)) == 0);
  }
}

static void test_alarm_hold_and_audio_first_half_fade_out() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Engine engine(store);
  Config config = defaults();
  config.hold_minutes = 10;
  config.light.fade_out_minutes = 10;
  config.light.effect = lighting::Effect::solid;
  config.light.color = {0, 0, 0, 255};
  strcpy(config.sound.path, "/morning.wav");
  config.sound.volume = 10;
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.start(0, MONDAY, 0) == Result::ok);
  const audio::Request request = engine.audioRequest(0);
  assert(audio::validRequest(request));
  assert(request.timeline.end_ms == 2700000);
  assert(request.timeline.fade_out_ms == 300000);
  lighting::Frame frame;
  assert(engine.frame(900000, frame) && frame.pixels[0].white == 127);
  assert(audio::gainBasisPoints(request, 900000) == 0);
  for (uint32_t elapsed : {1800000u, 2100000u, 2399999u, 2400000u}) {
    assert(engine.frame(elapsed, frame) && frame.pixels[0].white == 255);
    assert(audio::gainBasisPoints(request, elapsed) == 1000);
  }
  assert(engine.status(2399999).phase == Phase::ringing);
  assert(engine.status(2400000).phase == Phase::fading);
  assert(engine.frame(2550000, frame) && frame.pixels[0].white == 191);
  assert(audio::gainBasisPoints(request, 2550000) == 500);
  assert(engine.frame(2700000, frame) && frame.pixels[0].white == 127);
  assert(audio::gainBasisPoints(request, 2700000) == 0);
  assert(engine.frame(2850000, frame) && frame.pixels[0].white == 63);
  assert(engine.frame(3000000, frame) && frame.pixels[0].white == 0);
  assert(engine.tick(MONDAY + 2999, false, 2999999) == Result::ok && engine.active());
  assert(engine.tick(MONDAY + 3000, false, 3000000) == Result::ok && !engine.active());

  config.light.fade_in_minutes = 1;
  config.light.fade_out_minutes = 0;
  config.hold_minutes = 0;
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.start(0, MONDAY, 0) == Result::ok);
  assert(audio::validRequest(engine.audioRequest(0)));
  assert(engine.tick(MONDAY + 60, false, 60000) == Result::ok && !engine.active());
  config.light.fade_in_minutes = 60;
  config.hold_minutes = 180;
  config.light.fade_out_minutes = 60;
  assert(valid(config));
  assert(engine.set(0, config, MONDAY) == Result::ok);
  assert(engine.start(0, MONDAY, 0) == Result::ok);
  assert(audio::validRequest(engine.audioRequest(0)));
}

static void test_legacy_alarm_run_migrates_hold_and_resume_snapshot() {
  AlarmMemory memory;
  Store store(memory);
  assert(store.load() == settings::Result::blank);
  Bank bank = store.bank();
  Config& config = bank.slots[0];
  config.hold_minutes = 10;
  config.light.fade_out_minutes = 10;
  strcpy(config.sound.path, "/morning.wav");
  bank.resume_slot = 0;
  bank.resume_peak = PEAK;
  bank.resume_config = config;
  assert(store.save(bank));
  for (uint8_t record = 0; record < 2; ++record) {
    if (!memory.lengths[record]) continue;
    uint8_t* data = memory.data[record];
    data[4] = 3;
    for (uint8_t slot = 0; slot < SLOT_COUNT; ++slot) {
      uint8_t* entry = data + 16 + slot * 16;
      settings::record::put16(entry + 6,
          settings::record::get16(entry + 6) + entry[14]);
    }
    uint8_t* resume = data + 124;
    settings::record::put16(resume + 6,
        settings::record::get16(resume + 6) + resume[14]);
    settings::record::put32(data + 1020, settings::record::checksum(data, 1020));
  }
  Store migrated(memory);
  assert(migrated.load() == settings::Result::ok);
  assert(migrated.bank().slots[0].hold_minutes == 10);
  assert(migrated.bank().resume_config.hold_minutes == 10);
  Engine recovered(migrated);
  assert(recovered.tick(PEAK + 600, true, 0) == Result::ok);
  assert(recovered.active() && recovered.status(0).phase == Phase::fading);
  assert(recovered.audioRequest(0).timeline.elapsed_ms == 2400000);
  assert(migrated.save(migrated.bank()));
  Store restarted(memory);
  assert(restarted.load() == settings::Result::ok);
  assert(restarted.bank().slots[0].hold_minutes == 10);
  assert(restarted.bank().resume_config.hold_minutes == 10);
  memory.data[0][4] = 5;
  assert(restarted.load() == settings::Result::corrupt);
}

int main() {
  test_temporary_tap_restore_preserves_manual_dim_and_fade();
  test_alarm_hold_and_audio_first_half_fade_out();
  test_legacy_alarm_run_migrates_hold_and_resume_snapshot();
  test_animated_alarm_keeps_moving_through_peak_and_fade_out();
  test_saved_slots_and_days();
  test_ramp_one_shot_and_recovery();
  test_snooze_dismiss_and_button();
  test_priority_clock_changes_and_invalid_time();
  test_storage_failure_and_manual_test();
  test_output_snapshot_reduction_and_disable();
  test_simultaneous_ramp_starts_use_lowest_slot();
  test_clock_correction_does_not_suppress_future_schedule();
  test_manual_snooze_resume_does_not_consume_or_restart();
  test_editing_consumed_one_shot_keeps_active_snapshot();
  test_audio_settings_migration_and_shared_ramps();
  test_audio_bridge_retry_snooze_dismiss_and_half();
  test_req_030_thirty_minute_light_and_audio_ramps();
  test_req_009_saved_sound_volume_duration_and_fade_out();
  test_manual_audio_stop_cancels_pending_alarm_start();
}
