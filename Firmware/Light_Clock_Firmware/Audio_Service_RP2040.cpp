#include "Audio_Service_RP2040.hpp"

#include <I2S.h>
#include <atomic>
#include <string.h>
#include <pico/mutex.h>
#include <pico/util/queue.h>

#include "Settings_Storage_SD.hpp"

namespace audio {
namespace {
queue_t commands;
mutex_t status_mutex;
Status published;
Player* player = nullptr;
SettingsStorageSD* storage_device = nullptr;
SettingsStorageSD::AudioFile* audio_source = nullptr;
Status diagnostic;
bool diagnostic_active = false;
uint32_t next_request = 0;

class I2SOutput : public Output {
public:
  bool begin(uint32_t rate) override {
    if (!i2s_.setBCLK(PIN_I2S_BIT_CLOCK) ||
        !i2s_.setDATA(PIN_I2S_DATA) ||
        !i2s_.setBitsPerSample(16) ||
        !i2s_.setBuffers(16, BLOCK_FRAMES) ||
        !i2s_.setFrequency(static_cast<int>(rate))) return false;
    irq_seen_.store(false);
    wrong_irq_core_.store(false);
    i2s_.onTransmit(transmitted, this);
    running_ = i2s_.begin();
    if (!running_) i2s_.end();
    if (running_) {
      capacity_ = availableFrames();
      partial_ = 0;
      rate_ = rate;
    }
    return running_;
  }
  size_t availableFrames() override {
    return static_cast<size_t>(i2s_.availableForWrite()) / sizeof(uint32_t);
  }
  size_t pendingFrames() override {
    return running_ ? capacity_ -
        static_cast<size_t>(i2s_.availableForWrite()) / sizeof(uint32_t) : 0;
  }
  size_t write(const uint32_t* frames, size_t count) override {
    if (wrong_irq_core_.load()) return 0;
    const size_t written = i2s_.write(reinterpret_cast<const uint8_t*>(frames),
                                    count * sizeof(uint32_t)) / sizeof(uint32_t);
    partial_ = (partial_ + written) % BLOCK_FRAMES;
    return written;
  }
  bool underrun() override { return i2s_.getUnderflow(); }
  bool drain() override {
    // I2S::flush does not commit a partial block or wait for both DMA tails.
    if (partial_ != 0) {
      const uint32_t zeros[BLOCK_FRAMES] = {};
      const size_t padding = BLOCK_FRAMES - partial_;
      if (write(zeros, padding) != padding) return false;
    }
    const uint32_t started = millis();
    const uint32_t timeout = (capacity_ + 2 * BLOCK_FRAMES) * 1000 / rate_ + 100;
    while (availableFrames() < capacity_) {
      if (millis() - started >= timeout) return false;
      delay(1);
    }
    delay(1);
    return true;
  }
  bool end() override {
    if (!running_) return true;
    const bool ended = i2s_.end();
    running_ = false;
    return ended;
  }
  uint8_t irqCore() const {
    return wrong_irq_core_.load() ? 0 : irq_seen_.load() ? 1 : 255;
  }
private:
  static void __not_in_flash_func(transmitted)(void* context) {
    I2SOutput* output = static_cast<I2SOutput*>(context);
    if (get_core_num() != 1) output->wrong_irq_core_.store(true);
    output->irq_seen_.store(true);
  }
  std::atomic<bool> irq_seen_{false};
  std::atomic<bool> wrong_irq_core_{false};
  I2S i2s_{OUTPUT};
  bool running_ = false;
  size_t partial_ = 0;
  size_t capacity_ = 0;
  uint32_t rate_ = 0;
};
I2SOutput* device = nullptr;

void publish() {
  Status status = diagnostic_active ? diagnostic : player->status();
  status.core = static_cast<uint8_t>(get_core_num());
  status.irq_core = device->irqCore();
  status.read_us = audio_source->read_us;
  status.wait_us = audio_source->wait_us;
  status.max_read_us = audio_source->max_read_us;
  status.spi_mhz = storage_device->spiMHz();
  mutex_enter_blocking(&status_mutex);
  published = status;
  mutex_exit(&status_mutex);
}
}

void initControl() {
  queue_init(&commands, sizeof(Request), 8);
  mutex_init(&status_mutex);
}

void beginCore1(SettingsStorageSD& storage) {
  static SettingsStorageSD::AudioFile source(storage);
  static I2SOutput output;
  static Player instance(source, output);
  player = &instance;
  device = &output;
  storage_device = &storage;
  audio_source = &source;
  publish();
}

bool updateCore1() {
  const uint32_t frames = player->status().frames;
  Request request;
  if (queue_try_remove(&commands, &request)) {
    diagnostic_active = request.action == Action::fixture ||
        request.action == Action::cleanup || request.action == Action::spi;
    if (diagnostic_active) {
      Request stop;
      Error error = player->command(stop, millis());
      if (error == Error::none) {
        error = request.action == Action::fixture ? storage_device->fixture(request) :
            request.action == Action::cleanup ? storage_device->cleanup(request.path) :
            storage_device->audioSpi(request.rate);
      }
      diagnostic = {};
      diagnostic.request = request.id;
      diagnostic.result = error;
      diagnostic.state = error == Error::none ? State::stopped : State::error;
      memcpy(diagnostic.path, request.path, PATH_CAPACITY);
    } else player->command(request, millis());
  }
  player->update(millis());
  publish();
  return player->status().frames != frames;
}

Error submit(const Request& request, uint32_t& id) {
  if (!validRequest(request)) return Error::invalid_value;
  if (snapshot().core != 1) return Error::unavailable;
  Request queued = request;
  queued.id = next_request + 1;
  if (!queue_try_add(&commands, &queued)) return Error::busy;
  next_request = queued.id;
  id = queued.id;
  return Error::none;
}

Status snapshot() {
  mutex_enter_blocking(&status_mutex);
  const Status status = published;
  mutex_exit(&status_mutex);
  return status;
}

}
