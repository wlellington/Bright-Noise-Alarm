#include "../Firmware/Light_Clock_Firmware/Audio_Playback.hpp"
#include "../Firmware/Light_Clock_Firmware/File_Management.hpp"

#include <assert.h>
#include <string.h>
#include <vector>

using namespace audio;

class MemorySource : public Source {
public:
  std::vector<uint8_t> data;
  size_t position = 0;
  size_t read_limit = 1024;
  size_t largest_read = 0;
  bool opened = false;
  bool fail_read = false;
  Error open_error = Error::none;
  Error open(const char*) override {
    position = 0;
    opened = open_error == Error::none;
    return open_error;
  }
  uint64_t size() override { return data.size(); }
  bool seek(uint32_t value) override { position = value; return value <= data.size(); }
  int read(uint8_t* output, size_t count) override {
    if (fail_read) return -1;
    if (count > largest_read) largest_read = count;
    if (count > read_limit) count = read_limit;
    if (count > data.size() - position) count = data.size() - position;
    memcpy(output, data.data() + position, count);
    position += count;
    return static_cast<int>(count);
  }
  bool close() override { opened = false; return true; }
};

class TestOutput : public Output {
public:
  bool running = false;
  bool fail_write = false;
  bool fail_drain = false;
  bool underflow = false;
  bool drained = false;
  uint32_t rate = 0;
  size_t capacity = BLOCK_FRAMES;
  std::vector<uint32_t> samples;
  bool begin(uint32_t value) override { rate = value; running = true; return true; }
  size_t availableFrames() override { return capacity; }
  size_t pendingFrames() override { return 0; }
  size_t write(const uint32_t* frames, size_t count) override {
    if (fail_write) return 0;
    samples.insert(samples.end(), frames, frames + count);
    return count;
  }
  bool underrun() override { const bool value = underflow; underflow = false; return value; }
  bool drain() override { drained = true; return !fail_drain; }
  bool end() override { running = false; return true; }
};

static std::vector<uint8_t> wav(uint8_t channels, uint8_t bits,
                                const std::vector<uint8_t>& samples,
                                bool metadata = false) {
  using namespace settings::record;
  std::vector<uint8_t> data(36, 0);
  memcpy(data.data(), "RIFF", 4);
  memcpy(data.data() + 8, "WAVEfmt ", 8);
  put32(data.data() + 16, 16);
  put16(data.data() + 20, 1);
  put16(data.data() + 22, channels);
  put32(data.data() + 24, 32000);
  put32(data.data() + 28, 32000 * channels * (bits / 8));
  put16(data.data() + 32, channels * (bits / 8));
  put16(data.data() + 34, bits);
  if (metadata) {
    const uint8_t chunk[] = {'J','U','N','K',3,0,0,0,1,2,3,0};
    data.insert(data.end(), chunk, chunk + sizeof(chunk));
  }
  const size_t offset = data.size();
  data.resize(offset + 8);
  memcpy(data.data() + offset, "data", 4);
  put32(data.data() + offset + 4, static_cast<uint32_t>(samples.size()));
  data.insert(data.end(), samples.begin(), samples.end());
  if (samples.size() & 1) data.push_back(0);
  put32(data.data() + 4, static_cast<uint32_t>(data.size() - 8));
  return data;
}

static Request start(uint8_t volume = 10) {
  Request request;
  request.action = Action::start;
  request.volume = volume;
  request.id = 1;
  strcpy(request.path, "/test.wav");
  return request;
}

// REQ-017/059: bounded PCM streaming, metadata, exact sample cap and EOF drain.
static void test_pcm_streaming_and_volume_cap() {
  MemorySource source;
  std::vector<uint8_t> samples;
  for (size_t frame = 0; frame < 800; ++frame) {
    samples.insert(samples.end(), {0x00, 0x80, 0xff, 0x7f});
  }
  source.data = wav(2, 16, samples, true);
  source.read_limit = 3;
  TestOutput output;
  Player player(source, output);
  assert(player.command(start()) == Error::none);
  while (player.status().state == State::playing) player.update();
  assert(output.samples.size() == 800 && output.drained);
  assert(!output.running && !source.opened);
  for (uint32_t sample : output.samples) {
    assert(static_cast<int16_t>(sample >> 16) == -3276);
    assert(static_cast<int16_t>(sample) == 3276);
  }
  assert(player.status().peak == 3276 && player.status().frames == 800);
  assert(player.status().bytes == 3200 && source.largest_read <= 1024);
  assert(output.rate == 32000 && player.status().result == Error::none);
  assert(player.command(start(50)) == Error::none);
  while (player.status().state == State::playing) player.update();
  assert(player.status().peak == 16384);
  assert(player.command(start(51)) == Error::invalid_value && !output.running);
}

static void test_mono_eight_bit_and_live_mute() {
  MemorySource source;
  source.data = wav(1, 8, {0, 128, 255}, true);
  TestOutput output;
  output.capacity = 1;
  Player player(source, output);
  assert(player.command(start()) == Error::none);
  player.update();
  assert(static_cast<int16_t>(output.samples[0] >> 16) == -3276);
  assert(static_cast<int16_t>(output.samples[0]) == -3276);
  assert(player.command(start(51)) == Error::invalid_value);
  assert(player.status().volume == 10 && player.status().state == State::playing);
  Request mute;
  mute.action = Action::volume;
  mute.id = 2;
  assert(player.command(mute) == Error::none);
  player.update();
  player.update();
  assert(output.samples[1] == 0 && output.samples[2] == 0);
  assert(player.status().block_peak == 0);
  assert(player.status().request == 2 && player.status().state == State::stopped);
}

// REQ-019: reject malformed/unsupported WAV before enabling output; stop on read failure.
static void test_format_and_resource_failures() {
  MemorySource source;
  TestOutput output;
  Player player(source, output);
  source.open_error = Error::file;
  assert(player.command(start()) == Error::file && !output.running);
  source.open_error = Error::unavailable;
  assert(player.command(start()) == Error::unavailable && !output.running);
  source.open_error = Error::none;
  source.data = wav(1, 16, {0, 0});
  source.data[20] = 3;
  assert(player.command(start()) == Error::format && !output.running);
  source.data = wav(1, 16, {0, 0});
  source.data.pop_back();
  assert(player.command(start()) == Error::format && !output.running);
  source.data = wav(1, 16, {0, 0});
  assert(player.command(start()) == Error::none);
  source.fail_read = true;
  player.update();
  assert(player.status().state == State::error && player.status().result == Error::read);
  assert(!output.running && !source.opened);
}

static void test_backpressure_stop_and_output_failure() {
  MemorySource source;
  source.data = wav(2, 16, {0, 0, 0, 0});
  TestOutput output;
  output.capacity = 0;
  Player player(source, output);
  assert(player.command(start(5)) == Error::none);
  const size_t position = source.position;
  player.update();
  assert(source.position == position && player.status().bytes == 0);
  Request stop;
  assert(player.command(stop) == Error::none && !output.running);
  assert(player.command(start(5)) == Error::none);
  output.capacity = 1;
  output.fail_write = true;
  player.update();
  assert(player.status().result == Error::output && !output.running);
  output.fail_write = false;
  output.fail_drain = true;
  assert(player.command(start(5)) == Error::none);
  player.update();
  assert(player.status().result == Error::output && !output.running);
}

// REQ-030/049: delayed start, continuous loops, elapsed recovery and smooth gain.
static void test_alarm_timeline_loop_and_recovery() {
  MemorySource source;
  source.data = wav(1, 16, {0xff, 0x7f, 0, 0x80});
  TestOutput output;
  output.capacity = 1;
  Player player(source, output);
  Request request = start();
  request.alarm = true;
  request.loop = true;
  request.timeline = {true, 100, 0, 2000, 3000, 1000, 1000};
  assert(player.command(request, 100) == Error::none);
  assert(player.status().state == State::waiting && !output.running);
  player.update(1099);
  assert(!output.running);
  player.update(1100);
  assert(output.running && player.status().gain_bps == 0);
  player.update(1600);
  assert(player.status().gain_bps == 500 && player.status().block_peak == 1638);
  assert(!output.drained && player.status().state == State::playing);
  player.update(2100);
  assert(player.status().gain_bps == 1000 && player.status().block_peak == 3276);
  Request half;
  half.action = Action::reduce;
  half.reductions = 1;
  assert(player.command(half, 2100) == Error::none);
  player.update(2100);
  assert(player.status().gain_bps == 500);
  player.update(3100);
  assert(!output.running && player.status().state == State::stopped);
  request.timeline.elapsed_ms = 1501;
  assert(player.command(request, 100) == Error::none);
  // 501 ms into the sound is frame 16032, modulo the two-frame fixture.
  assert(source.position == 44);
  player.update(100);
  assert(player.status().gain_bps == 501 && player.status().block_peak == 1641);
  source.fail_read = true;
  player.update(100);
  assert(player.status().state == State::error);
  assert(player.command(half, 100) == Error::read);
  assert(player.status().result == Error::read);
}

static void test_wide_pcm_and_supported_rates() {
  for (const uint8_t bits : {24, 32}) {
    MemorySource source;
    std::vector<uint8_t> pcm(bits / 8 * 2, 0);
    pcm[bits / 8 - 1] = 0x80;
    for (size_t byte = bits / 8; byte < pcm.size(); ++byte) pcm[byte] = 0xff;
    pcm.back() = 0x7f;
    source.data = wav(1, bits, pcm);
    settings::record::put32(source.data.data() + 24, 96000);
    settings::record::put32(source.data.data() + 28, 96000 * bits / 8);
    TestOutput output;
    Player player(source, output);
    assert(player.command(start()) == Error::none);
    player.update();
    assert(output.samples[0] == 0xf334f334u);
    assert(output.samples[1] == 0x0ccc0cccu);
    settings::record::put32(source.data.data() + 24, 96001);
    assert(player.command(start()) == Error::format);
    settings::record::put32(source.data.data() + 24, 7999);
    assert(player.command(start()) == Error::format);
  }
}

static void test_live_volume_preserves_owned_timeline_and_end() {
  MemorySource source;
  source.data = wav(1, 16, {0xff, 0x7f, 0xff, 0x7f});
  TestOutput output;
  output.capacity = 1;
  Player player(source, output);
  Request request = start(5);
  request.sleep = true;
  request.loop = true;
  request.timeline = {true, 100, 0, 2000, 3000, 1000, 1000};
  assert(player.command(request, 100) == Error::none);
  assert(player.status().state == State::waiting);
  Request volume;
  volume.action = Action::volume;
  volume.volume = 20;
  const size_t position = source.position;
  assert(player.command(volume, 500) == Error::none);
  assert(source.position == position && player.status().state == State::waiting);
  assert(player.status().sleep && player.status().volume == 20);
  player.update(1100);
  player.update(1600);
  assert(player.status().gain_bps == 1000);
  player.update(2100);
  assert(player.status().gain_bps == 2000 && player.status().block_peak == 6553);
  player.update(2600);
  assert(player.status().gain_bps == 1000);
  volume.volume = 10;
  assert(player.command(volume, 2600) == Error::none);
  player.update(2600);
  assert(player.status().gain_bps == 500 && player.status().sleep);
  player.update(3100);
  assert(player.status().state == State::stopped);
}

int main() {
  test_live_volume_preserves_owned_timeline_and_end();
  test_pcm_streaming_and_volume_cap();
  test_mono_eight_bit_and_live_mute();
  test_format_and_resource_failures();
  test_backpressure_stop_and_output_failure();
  test_alarm_timeline_loop_and_recovery();
  test_wide_pcm_and_supported_rates();
}
