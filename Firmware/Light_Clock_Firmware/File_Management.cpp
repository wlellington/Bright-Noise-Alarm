#include "File_Management.hpp"

#include <string.h>

namespace settings {
namespace record {

uint32_t get32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) |
         (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) |
         (static_cast<uint32_t>(p[3]) << 24);
}

void put32(uint8_t* p, uint32_t value) {
  for (uint8_t i = 0; i < 4; ++i) {
    p[i] = static_cast<uint8_t>(value >> (8 * i));
  }
}

uint16_t get16(const uint8_t* p) {
  return static_cast<uint16_t>(p[0] | (static_cast<uint16_t>(p[1]) << 8));
}

void put16(uint8_t* p, uint16_t value) {
  p[0] = static_cast<uint8_t>(value);
  p[1] = static_cast<uint8_t>(value >> 8);
}

uint32_t checksum(const uint8_t* data, size_t length) {
  uint32_t crc = 0xffffffffu;
  for (size_t i = 0; i < length; ++i) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xedb88320u & (0u - (crc & 1u)));
    }
  }
  return ~crc;
}

}
namespace {
using namespace record;

void encode(const DisplaySettings& value, uint32_t sequence, uint8_t* record) {
  memset(record, 0, RECORD_SIZE);
  memcpy(record, "BNSA", 4);
  record[4] = 3;
  record[5] = value.use_12_hour ? 1 : 0;
  record[6] = value.brightness;
  record[7] = value.dark_enabled ? 1 : 0;
  put32(record + 8, sequence);
  put16(record + 12, value.dark_start);
  put16(record + 14, value.dark_end);
  put16(record + 16, value.wake_seconds);
  put32(record + 20, checksum(record, 20));
}

bool decode(const uint8_t* record, size_t length, DisplaySettings* value,
            uint32_t* sequence) {
  if (length < LEGACY_RECORD_SIZE || memcmp(record, "BNSA", 4) != 0 ||
      record[5] > 1) {
    return false;
  }
  *value = defaults();
  if (record[4] == 1) {
    if (length != LEGACY_RECORD_SIZE || record[7] != 0 ||
        get32(record + 12) != checksum(record, 12)) return false;
  } else if (record[4] == 2 || record[4] == 3) {
    if (length != RECORD_SIZE || record[7] > 1 ||
        (record[4] == 2 ? get32(record + 16) != 0
                        : get16(record + 18) != 0) ||
        get32(record + 20) != checksum(record, 20)) return false;
    value->dark_enabled = record[7] != 0;
    value->dark_start = get16(record + 12);
    value->dark_end = get16(record + 14);
    if (record[4] == 3) value->wake_seconds = get16(record + 16);
  } else {
    return false;
  }
  value->use_12_hour = record[5] != 0;
  value->brightness = record[6];
  *sequence = get32(record + 8);
  return valid(*value);
}

}  // namespace

bool valid(const DisplaySettings& value) {
  return value.brightness <= 15 && value.dark_start < 1440 &&
         value.dark_end < 1440 && value.wake_seconds >= 1 &&
         value.wake_seconds <= 300;
}

DisplaySettings defaults() {
  return {false, 3, false, 22 * 60, 7 * 60};
}

Store::Store(Storage& storage)
    : storage_(storage), sequence_(0), active_slot_(-1), ready_(false) {}

Result Store::load(DisplaySettings* value) {
  ready_ = false;
  active_slot_ = -1;
  uint8_t records[2][RECORD_SIZE] = {};
  DisplaySettings candidates[2] = {};
  uint32_t sequences[2] = {};
  bool good[2] = {};
  bool present = false;
  for (uint8_t slot = 0; slot < 2; ++slot) {
    size_t length = 0;
    const ReadResult result =
        storage_.read(slot, records[slot], RECORD_SIZE, &length);
    if (result == ReadResult::error) {
      return Result::unavailable;
    }
    if (result == ReadResult::ok) {
      present = true;
      // Do not overwrite a record from a newer firmware version.
      if (length >= 5 && memcmp(records[slot], "BNSA", 4) == 0 &&
          records[slot][4] != 1 && records[slot][4] != 2 &&
          records[slot][4] != 3) {
        return Result::corrupt;
      }
      good[slot] = decode(records[slot], length, &candidates[slot],
                          &sequences[slot]);
    }
  }
  if (!good[0] && !good[1]) {
    if (present) {
      return Result::corrupt;
    }
    sequence_ = 0;
    ready_ = true;
    return Result::blank;
  }
  active_slot_ = good[1] &&
                         (!good[0] ||
                          static_cast<int32_t>(sequences[1] - sequences[0]) > 0)
                     ? 1
                     : 0;
  sequence_ = sequences[active_slot_];
  *value = candidates[active_slot_];
  ready_ = true;
  return Result::ok;
}

Result Store::save(const DisplaySettings& value) {
  if (!ready_ || !valid(value)) {
    return Result::write_failed;
  }
  const uint8_t target = active_slot_ == 0 ? 1 : 0;
  uint8_t record[RECORD_SIZE];
  encode(value, sequence_ + 1, record);
  if (!storage_.write(target, record, RECORD_SIZE)) {
    ready_ = false;
    return Result::write_failed;
  }
  uint8_t verify[RECORD_SIZE];
  size_t length = 0;
  if (storage_.read(target, verify, RECORD_SIZE, &length) != ReadResult::ok ||
      length != RECORD_SIZE ||
      memcmp(record, verify, RECORD_SIZE) != 0) {
    ready_ = false;
    return Result::write_failed;
  }
  active_slot_ = target;
  ++sequence_;
  return Result::ok;
}

}  // namespace settings
