#include "../Firmware/Light_Clock_Firmware/File_Management.hpp"

#include <assert.h>
#include <string.h>

class MemoryStorage : public settings::Storage {
public:
  uint8_t bytes[2][settings::RECORD_SIZE] = {};
  bool present[2] = {};
  size_t lengths[2] = {};
  bool read_error = false;
  bool partial_write = false;

  settings::ReadResult read(uint8_t slot, uint8_t* data, size_t capacity,
                            size_t* length) override {
    if (read_error) return settings::ReadResult::error;
    if (!present[slot]) return settings::ReadResult::missing;
    assert(lengths[slot] <= capacity);
    *length = lengths[slot];
    memcpy(data, bytes[slot], *length);
    return settings::ReadResult::ok;
  }
  bool write(uint8_t slot, const uint8_t* data, size_t size) override {
    present[slot] = true;
    lengths[slot] = partial_write ? size / 2 : size;
    memcpy(bytes[slot], data, lengths[slot]);
    return !partial_write;
  }
};

// REQ-018: changes survive restart and an interrupted replacement recovers.
static void test_req_018_roundtrip_and_interrupted_write() {
  MemoryStorage storage;
  settings::Store store(storage);
  settings::DisplaySettings value = settings::defaults();
  assert(store.load(&value) == settings::Result::blank);
  assert(store.save(value) == settings::Result::ok);
  value.use_12_hour = true;
  assert(store.save(value) == settings::Result::ok);
  storage.partial_write = true;
  value.brightness = 8;
  assert(store.save(value) == settings::Result::write_failed);
  assert(store.save(value) == settings::Result::write_failed);

  settings::Store restarted(storage);
  settings::DisplaySettings restored = {};
  assert(restarted.load(&restored) == settings::Result::ok);
  assert(restored.use_12_hour && restored.brightness == 3);
  storage.partial_write = false;
  assert(restarted.save(value) == settings::Result::ok);
  settings::Store again(storage);
  assert(again.load(&restored) == settings::Result::ok);
  assert(restored.use_12_hour && restored.brightness == 8);
}

// REQ-019: blank, corrupt and unreadable stores remain distinct.
static void test_req_019_storage_errors_do_not_initialize_corrupt_data() {
  MemoryStorage storage;
  settings::Store store(storage);
  settings::DisplaySettings value = settings::defaults();
  assert(store.load(&value) == settings::Result::blank);
  settings::DisplaySettings invalid = value;
  invalid.brightness = 16;
  assert(store.save(invalid) == settings::Result::write_failed);
  assert(!storage.present[0]);
  storage.present[0] = true;
  assert(store.load(&value) == settings::Result::corrupt);
  assert(store.save(value) == settings::Result::write_failed);
  storage.read_error = true;
  assert(store.load(&value) == settings::Result::unavailable);
}

static void test_recovery_and_future_version() {
  MemoryStorage storage;
  settings::Store store(storage);
  settings::DisplaySettings value = settings::defaults();
  assert(store.load(&value) == settings::Result::blank);
  assert(store.save(value) == settings::Result::ok);
  value.brightness = 5;
  assert(store.save(value) == settings::Result::ok);
  storage.bytes[1][20] ^= 1;
  settings::Store recovered(storage);
  assert(recovered.load(&value) == settings::Result::ok);
  assert(value.brightness == 3);
  storage.bytes[1][4] = 4;
  assert(recovered.load(&value) == settings::Result::corrupt);
  assert(recovered.save(value) == settings::Result::write_failed);
}

static void test_req_018_version_one_migrates_without_losing_settings() {
  const uint8_t legacy[settings::LEGACY_RECORD_SIZE] = {
      0x42, 0x4e, 0x53, 0x41, 0x01, 0x01, 0x07, 0x00,
      0x7b, 0x00, 0x00, 0x00, 0xe9, 0xeb, 0x9f, 0x73};
  MemoryStorage storage;
  memcpy(storage.bytes[0], legacy, sizeof(legacy));
  storage.present[0] = true;
  storage.lengths[0] = sizeof(legacy);
  settings::Store store(storage);
  settings::DisplaySettings value = {};
  assert(store.load(&value) == settings::Result::ok);
  assert(value.use_12_hour && value.brightness == 7);
  assert(!value.dark_enabled && value.dark_start == 1320 &&
         value.dark_end == 420);
  value.dark_enabled = true;
  value.dark_start = 23 * 60 + 15;
  assert(store.save(value) == settings::Result::ok);
  assert(storage.lengths[0] == settings::LEGACY_RECORD_SIZE);
  assert(memcmp(storage.bytes[0], legacy, sizeof(legacy)) == 0);
  settings::Store restarted(storage);
  assert(restarted.load(&value) == settings::Result::ok);
  assert(value.use_12_hour && value.brightness == 7);
  assert(value.dark_enabled && value.dark_start == 1395);
}

static void test_wake_duration_migrates_v2_and_survives_restart() {
  MemoryStorage storage;
  settings::Store store(storage);
  settings::DisplaySettings value = settings::defaults();
  assert(store.load(&value) == settings::Result::blank);
  value.dark_enabled = true;
  value.brightness = 7;
  assert(store.save(value) == settings::Result::ok);
  storage.bytes[0][4] = 2;
  settings::record::put32(storage.bytes[0] + 16, 0);
  settings::record::put32(storage.bytes[0] + 20,
      settings::record::checksum(storage.bytes[0], 20));
  settings::Store migrated(storage);
  assert(migrated.load(&value) == settings::Result::ok);
  assert(value.wake_seconds == 30 && value.dark_enabled && value.brightness == 7);
  value.wake_seconds = 5;
  assert(migrated.save(value) == settings::Result::ok);
  settings::Store restarted(storage);
  assert(restarted.load(&value) == settings::Result::ok);
  assert(value.wake_seconds == 5 && value.dark_start == 1320);
  value.wake_seconds = 0;
  assert(restarted.save(value) == settings::Result::write_failed);
  value.wake_seconds = 301;
  assert(restarted.save(value) == settings::Result::write_failed);
}

int main() {
  test_wake_duration_migrates_v2_and_survives_restart();
  test_req_018_roundtrip_and_interrupted_write();
  test_req_019_storage_errors_do_not_initialize_corrupt_data();
  test_recovery_and_future_version();
  test_req_018_version_one_migrates_without_losing_settings();
}
