#pragma once

#include <stddef.h>
#include <stdint.h>

namespace settings {

namespace record {
uint32_t get32(const uint8_t* p);
void put32(uint8_t* p, uint32_t value);
uint16_t get16(const uint8_t* p);
void put16(uint8_t* p, uint16_t value);
uint32_t checksum(const uint8_t* data, size_t length);
}

struct DisplaySettings {
  bool use_12_hour;
  uint8_t brightness;
  bool dark_enabled;
  uint16_t dark_start;
  uint16_t dark_end;
  uint16_t wake_seconds = 30;
};

enum class ReadResult { ok, missing, error };
enum class Result { ok, blank, unavailable, corrupt, write_failed };

constexpr size_t LEGACY_RECORD_SIZE = 16;
constexpr size_t RECORD_SIZE = 24;

class Storage {
public:
  virtual ~Storage() {}
  virtual ReadResult read(uint8_t slot, uint8_t* data, size_t capacity,
                          size_t* length) = 0;
  virtual bool write(uint8_t slot, const uint8_t* data, size_t size) = 0;
};

class Store {
public:
  explicit Store(Storage& storage);
  Result load(DisplaySettings* settings);
  Result save(const DisplaySettings& settings);

private:
  Storage& storage_;
  uint32_t sequence_;
  int8_t active_slot_;
  bool ready_;
};

bool valid(const DisplaySettings& settings);
DisplaySettings defaults();

}  // namespace settings
