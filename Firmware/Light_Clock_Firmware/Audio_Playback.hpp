#pragma once

#include <stddef.h>
#include <stdint.h>

namespace audio {

constexpr uint8_t VOLUME_LIMIT = 50;
constexpr size_t PATH_CAPACITY = 96;
constexpr size_t BLOCK_FRAMES = 256;

enum class Error { none, invalid_value, unavailable, file, format, read, output, busy };
enum class State { stopped, waiting, playing, error };
enum class Action { start, stop, volume, reduce, fixture, cleanup, spi };

struct Timeline {
  bool enabled = false;
  uint32_t reference_ms = 0;
  uint32_t elapsed_ms = 0;
  uint32_t peak_ms = 0;
  uint32_t end_ms = 0;
  uint32_t fade_in_ms = 0;
  uint32_t fade_out_ms = 0;
};

struct Request {
  Action action = Action::stop;
  uint32_t id = 0;
  uint8_t volume = 0;
  char path[PATH_CAPACITY] = {};
  Timeline timeline;
  bool alarm = false;
  bool sleep = false;
  bool loop = false;
  uint8_t reductions = 0;
  uint32_t rate = 0;
  uint8_t bits = 0;
  uint8_t channels = 0;
  uint8_t seconds = 0;
};

struct Status {
  State state = State::stopped;
  Error result = Error::none;
  uint32_t request = 0;
  uint32_t frames = 0;
  uint32_t total_frames = 0;
  uint64_t bytes = 0;
  uint32_t underruns = 0;
  uint32_t rate = 0;
  uint16_t peak = 0;
  uint16_t block_peak = 0;
  uint16_t gain_bps = 0;
  uint8_t channels = 0;
  uint8_t bits = 0;
  uint8_t volume = 0;
  uint8_t core = 0;
  uint8_t irq_core = 255;
  char path[PATH_CAPACITY] = {};
  bool alarm = false;
  bool sleep = false;
  uint64_t read_us = 0;
  uint64_t wait_us = 0;
  uint32_t max_read_us = 0;
  uint32_t spi_mhz = 10;
};

class Source {
public:
  virtual ~Source() {}
  virtual Error open(const char* path) = 0;
  virtual uint64_t size() = 0;
  virtual bool seek(uint32_t position) = 0;
  virtual int read(uint8_t* data, size_t length) = 0;
  virtual bool close() = 0;
};

class Output {
public:
  virtual ~Output() {}
  virtual bool begin(uint32_t rate) = 0;
  virtual size_t availableFrames() = 0;
  virtual size_t write(const uint32_t* frames, size_t count) = 0;
  virtual bool underrun() = 0;
  virtual bool drain() = 0;
  virtual bool end() = 0;
  virtual size_t pendingFrames() = 0;
};

class Control {
public:
  virtual ~Control() {}
  virtual Error submit(const Request& request, uint32_t& id) = 0;
  virtual Status status() const = 0;
};

bool validRequest(const Request& request);
bool validTestFolder(const char* path);
uint16_t gainBasisPoints(const Request& request, uint32_t elapsed_ms);
const char* errorName(Error error);
const char* stateName(State state);

class Player {
public:
  Player(Source& source, Output& output) : source_(source), output_(output) {}
  Error command(const Request& request, uint32_t now = 0);
  void update(uint32_t now = 0);
  Status status() const { return status_; }

private:
  Error start(const Request& request, uint32_t now);
  Error beginOutput(uint32_t now);
  uint32_t elapsed(uint32_t now) const;
  Error stop();
  Error header();
  Error readExact(uint8_t* data, size_t count);
  Error fail(Error error);
  Source& source_;
  Output& output_;
  Status status_;
  uint32_t remaining_ = 0;
  uint32_t data_offset_ = 0;
  uint32_t data_bytes_ = 0;
  Request playing_;
  uint8_t input_[BLOCK_FRAMES * 8] = {};
  uint32_t samples_[BLOCK_FRAMES] = {};
};

}
