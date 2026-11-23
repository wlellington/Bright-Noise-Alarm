#include "Audio_Playback.hpp"

#include <string.h>
#include "File_Management.hpp"

namespace audio {
using settings::record::get16;
using settings::record::get32;

bool validRequest(const Request& request) {
  if (request.alarm && request.sleep) return false;
  if (request.action == Action::stop) return true;
  if (request.action == Action::reduce) return request.reductions <= 8;
  if (request.action == Action::cleanup) return validTestFolder(request.path);
  if (request.action == Action::spi)
    return request.rate == 10 || request.rate == 20;
  if (request.action == Action::fixture)
    return validTestFolder(request.path) && request.rate >= 8000 &&
        request.rate <= 96000 && request.channels >= 1 && request.channels <= 2 &&
        (request.bits == 8 || request.bits == 16 || request.bits == 24 ||
         request.bits == 32) && request.seconds >= 1 && request.seconds <= 10;
  if (request.volume > VOLUME_LIMIT) return false;
  if (request.action == Action::volume) return true;
  const Timeline& time = request.timeline;
  if (request.reductions > 8 || (time.enabled &&
      (time.peak_ms > 3600000u || time.end_ms == 0 ||
       time.end_ms < time.peak_ms ||
       time.end_ms > 18000000u || time.fade_in_ms > time.peak_ms ||
       time.fade_out_ms > time.end_ms - time.peak_ms))) return false;
  return request.action == Action::start && request.path[0] == '/' &&
      request.path[1] != '\0' &&
      memchr(request.path, '\0', PATH_CAPACITY) != nullptr;
}

bool validTestFolder(const char* path) {
  if (strncmp(path, "/BNTEST", 7) != 0) return false;
  for (uint8_t i = 7; i < 15; ++i) {
    if (!((path[i] >= '0' && path[i] <= '9') ||
          (path[i] >= 'a' && path[i] <= 'f'))) return false;
  }
  return path[15] == '\0';
}

uint16_t gainBasisPoints(const Request& request, uint32_t elapsed_ms) {
  uint32_t gain = request.volume * 100u;
  const Timeline& time = request.timeline;
  if (time.enabled) {
    const uint32_t start = time.peak_ms - time.fade_in_ms;
    if (elapsed_ms < start || elapsed_ms >= time.end_ms) return 0;
    if (elapsed_ms < time.peak_ms && time.fade_in_ms != 0)
      gain = static_cast<uint32_t>(uint64_t{gain} * (elapsed_ms - start) /
                                   time.fade_in_ms);
    if (time.fade_out_ms != 0 && elapsed_ms > time.end_ms - time.fade_out_ms)
      gain = static_cast<uint32_t>(uint64_t{gain} * (time.end_ms - elapsed_ms) /
                                   time.fade_out_ms);
  }
  return static_cast<uint16_t>(gain >> request.reductions);
}

const char* errorName(Error error) {
  switch (error) {
    case Error::none: return "NONE";
    case Error::invalid_value: return "INVALID_VALUE";
    case Error::unavailable: return "UNAVAILABLE";
    case Error::file: return "FILE";
    case Error::format: return "FORMAT";
    case Error::read: return "READ";
    case Error::output: return "OUTPUT";
    case Error::busy: return "BUSY";
  }
  return "INVALID_VALUE";
}

const char* stateName(State state) {
  switch (state) {
    case State::stopped: return "STOPPED";
    case State::waiting: return "WAITING";
    case State::playing: return "PLAYING";
    case State::error: return "ERROR";
  }
  return "ERROR";
}

Error Player::readExact(uint8_t* data, size_t count) {
  size_t offset = 0;
  while (offset < count) {
    const int read = source_.read(data + offset, count - offset);
    if (read <= 0 || static_cast<size_t>(read) > count - offset) return Error::read;
    offset += static_cast<size_t>(read);
  }
  return Error::none;
}

Error Player::header() {
  uint8_t bytes[24];
  Error error = readExact(bytes, 12);
  if (error != Error::none) return error;
  const uint64_t end = static_cast<uint64_t>(get32(bytes + 4)) + 8;
  if (memcmp(bytes, "RIFF", 4) || memcmp(bytes + 8, "WAVE", 4) ||
      end > source_.size() || end < 12 || end > UINT32_MAX) return Error::format;
  uint32_t offset = 12;
  bool format = false;
  // Bound malformed metadata work so control requests cannot be starved.
  for (uint8_t chunks = 0; chunks < 64 && offset + uint64_t{8} <= end; ++chunks) {
    if (!source_.seek(offset)) return Error::read;
    error = readExact(bytes, 8);
    if (error != Error::none) return error;
    const uint32_t length = get32(bytes + 4);
    const uint64_t next = offset + uint64_t{8} + length + (length & 1u);
    if (next > end) return Error::format;
    if (memcmp(bytes, "fmt ", 4) == 0) {
      if (format || length < 16) return Error::format;
      error = readExact(bytes + 8, 16);
      if (error != Error::none) return error;
      const uint16_t channels = get16(bytes + 10);
      const uint16_t bits = get16(bytes + 22);
      const uint32_t rate = get32(bytes + 12);
      if (get16(bytes + 8) != 1 || channels < 1 || channels > 2 ||
          (bits != 8 && bits != 16 && bits != 24 && bits != 32) ||
          rate < 8000 || rate > 96000 ||
          get16(bytes + 20) != channels * (bits / 8) ||
          get32(bytes + 16) != rate * channels * (bits / 8)) return Error::format;
      status_.rate = rate;
      status_.channels = static_cast<uint8_t>(channels);
      status_.bits = static_cast<uint8_t>(bits);
      format = true;
    } else if (memcmp(bytes, "data", 4) == 0) {
      const uint8_t alignment = status_.channels * (status_.bits / 8);
      if (!format || length == 0 || length % alignment != 0) return Error::format;
      remaining_ = length;
      data_bytes_ = length;
      data_offset_ = offset + 8;
      status_.total_frames = length / alignment;
      return source_.seek(offset + 8) ? Error::none : Error::read;
    }
    offset = static_cast<uint32_t>(next);
  }
  return Error::format;
}

Error Player::stop() {
  const bool ended = output_.end();
  const bool closed = source_.close();
  remaining_ = 0;
  status_.state = State::stopped;
  status_.gain_bps = 0;
  status_.block_peak = 0;
  status_.result = !ended ? Error::output : !closed ? Error::read : Error::none;
  if (status_.result != Error::none) status_.state = State::error;
  return status_.result;
}

Error Player::fail(Error error) {
  const Error cleanup = stop();
  status_.state = State::error;
  status_.result = cleanup == Error::none ? error : cleanup;
  return status_.result;
}

uint32_t Player::elapsed(uint32_t now) const {
  return playing_.timeline.elapsed_ms + (now - playing_.timeline.reference_ms);
}

Error Player::beginOutput(uint32_t now) {
  if (playing_.timeline.enabled) {
    const uint32_t start = playing_.timeline.peak_ms - playing_.timeline.fade_in_ms;
    const uint64_t frames = uint64_t{elapsed(now) - start} * status_.rate / 1000;
    const uint32_t offset = static_cast<uint32_t>(frames % status_.total_frames) *
        status_.channels * (status_.bits / 8);
    if (!source_.seek(data_offset_ + offset)) return fail(Error::read);
    remaining_ = data_bytes_ - offset;
  }
  if (!output_.begin(status_.rate)) return fail(Error::output);
  status_.state = State::playing;
  return Error::none;
}

Error Player::start(const Request& request, uint32_t now) {
  const Error stopped = stop();
  if (stopped != Error::none) return stopped;
  status_ = {};
  status_.volume = request.volume;
  status_.alarm = request.alarm;
  status_.sleep = request.sleep;
  playing_ = request;
  memcpy(status_.path, request.path, PATH_CAPACITY);
  if (request.timeline.enabled && elapsed(now) >= request.timeline.end_ms)
    return Error::none;
  Error error = source_.open(request.path);
  if (error != Error::none) return fail(error);
  error = header();
  if (error != Error::none) return fail(error);
  if (request.timeline.enabled &&
      elapsed(now) < request.timeline.peak_ms - request.timeline.fade_in_ms) {
    status_.state = State::waiting;
    return Error::none;
  }
  return beginOutput(now);
}

Error Player::command(const Request& request, uint32_t now) {
  if (!validRequest(request)) {
    status_.request = request.id;
    status_.result = Error::invalid_value;
    return status_.result;
  }
  Error result = Error::none;
  if (request.action == Action::start) result = start(request, now);
  else if (request.action == Action::stop) result = stop();
  else if (request.action == Action::volume || request.action == Action::reduce) {
    if (request.action == Action::volume) {
      status_.volume = request.volume;
      playing_.volume = request.volume;
    } else playing_.reductions = request.reductions;
    if (status_.state == State::error) result = status_.result;
  } else result = Error::unavailable;
  status_.request = request.id;
  status_.result = result;
  return result;
}

void Player::update(uint32_t now) {
  if (playing_.timeline.enabled &&
      (status_.state == State::waiting || status_.state == State::playing)) {
    if (elapsed(now) >= playing_.timeline.end_ms) {
      stop();
      return;
    }
    if (status_.state == State::waiting) {
      if (elapsed(now) < playing_.timeline.peak_ms - playing_.timeline.fade_in_ms)
        return;
      if (beginOutput(now) != Error::none) return;
    }
  }
  if (status_.state != State::playing) return;
  const size_t available = output_.availableFrames();
  if (available == 0) return;
  const uint8_t alignment = status_.channels * (status_.bits / 8);
  size_t count = remaining_ / alignment;
  if (count > BLOCK_FRAMES) count = BLOCK_FRAMES;
  if (count > available) count = available;
  const size_t bytes = count * alignment;
  const uint32_t presentation = playing_.timeline.enabled
      ? elapsed(now) + static_cast<uint32_t>(output_.pendingFrames() * 1000 /
                                             status_.rate) : 0;
  const uint16_t gain_start = gainBasisPoints(playing_, presentation);
  const uint16_t gain_end = gainBasisPoints(playing_, presentation +
      static_cast<uint32_t>(count * 1000 / status_.rate));
  status_.gain_bps = gain_end;
  const Error error = readExact(input_, bytes);
  if (error != Error::none) {
    fail(error);
    return;
  }
  status_.block_peak = 0;
  for (size_t frame = 0; frame < count; ++frame) {
    const int32_t gain = gain_start +
        (static_cast<int32_t>(gain_end) - gain_start) *
        static_cast<int32_t>(frame) / static_cast<int32_t>(count);
    int16_t channels[2] = {};
    for (uint8_t channel = 0; channel < status_.channels; ++channel) {
      const uint8_t* data = input_ + frame * alignment + channel * (status_.bits / 8);
      const uint16_t raw = status_.bits == 8 ? 0 :
          get16(data + status_.bits / 8 - 2);
      const int32_t sample = status_.bits == 8
          ? (static_cast<int32_t>(data[0]) - 128) * 256
          : raw < 32768 ? static_cast<int32_t>(raw) : static_cast<int32_t>(raw) - 65536;
      const int32_t scaled = sample * gain / 10000;
      channels[channel] = static_cast<int16_t>(scaled);
      const uint16_t peak = static_cast<uint16_t>(scaled < 0 ? -scaled : scaled);
      if (peak > status_.peak) status_.peak = peak;
      if (peak > status_.block_peak) status_.block_peak = peak;
    }
    if (status_.channels == 1) channels[1] = channels[0];
    samples_[frame] = (static_cast<uint32_t>(static_cast<uint16_t>(channels[0])) << 16) |
                     static_cast<uint16_t>(channels[1]);
  }
  if (status_.frames != 0 && output_.underrun()) ++status_.underruns;
  if (output_.write(samples_, count) != count) {
    fail(Error::output);
    return;
  }
  status_.frames += static_cast<uint32_t>(count);
  status_.bytes += static_cast<uint32_t>(bytes);
  remaining_ -= static_cast<uint32_t>(bytes);
  if (remaining_ == 0) {
    if (playing_.loop) {
      if (!source_.seek(data_offset_)) {
        fail(Error::read);
        return;
      }
      remaining_ = data_bytes_;
      return;
    }
    if (!output_.drain()) {
      fail(Error::output);
      return;
    }
    stop();
  }
}

}
