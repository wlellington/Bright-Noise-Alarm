#include "Settings_Storage_SD.hpp"

#include <string.h>
#include <stdio.h>

#include "Defines.hpp"

namespace {
const char* const paths[] = {"BNSET0.DAT", "BNSET1.DAT"};
}

SettingsStorageSD::SettingsStorageSD() {
  recursive_mutex_init(&mutex_);
}

bool SettingsStorageSD::begin() {
  Guard guard(mutex_);
  ready_ = sd_.begin(SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(10)));
  return ready_;
}

uint32_t SettingsStorageSD::spiMHz() {
  Guard guard(mutex_);
  return spi_mhz_;
}

audio::Error SettingsStorageSD::audioSpi(uint32_t mhz) {
  Guard guard(mutex_);
  if (directory_ || input_file_) return audio::Error::busy;
  ready_ = sd_.begin(SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(mhz)));
  spi_mhz_ = mhz;
  if (ready_) return audio::Error::none;
  ready_ = sd_.begin(SdSpiConfig(SD_CS_PIN, SHARED_SPI, SD_SCK_MHZ(10)));
  spi_mhz_ = 10;
  return ready_ ? audio::Error::read : audio::Error::unavailable;
}

bool SettingsStorageSD::audioCatalog(char paths[][audio::PATH_CAPACITY],
                                    uint8_t capacity, uint8_t& count, bool& more) {
  FsFile root;
  count = 0;
  more = false;
  {
    Guard guard(mutex_);
    if (!ready_ || !root.open("/", O_RDONLY)) return false;
  }
  bool success = true;
  for (uint16_t scanned = 0; scanned < 128; ++scanned) {
    Guard guard(mutex_);
    FsFile entry;
    if (!entry.openNext(&root, O_RDONLY)) {
      success = !root.getError() && !sd_.sdErrorCode();
      break;
    }
    char name[256] = {};
    const size_t length = entry.getName(name, sizeof(name));
    if (!entry.isDir() && length >= 4 &&
        name[length - 4] == '.' && (name[length - 3] == 'w' || name[length - 3] == 'W') &&
        (name[length - 2] == 'a' || name[length - 2] == 'A') &&
        (name[length - 1] == 'v' || name[length - 1] == 'V')) {
      if (count < capacity && length + 1 < audio::PATH_CAPACITY) {
        paths[count][0] = '/';
        memcpy(paths[count++] + 1, name, length + 1);
      } else more = true;
    }
    if (!entry.close()) { success = false; break; }
    if (scanned == 127) more = true;
  }
  Guard guard(mutex_);
  return root.close() && success;
}

audio::Error SettingsStorageSD::fixture(const audio::Request& request) {
  Guard guard(mutex_);
  if (!ready_) return audio::Error::unavailable;
  if (!audio::validRequest(request)) return audio::Error::invalid_value;
  if (sd_.exists(request.path)) return audio::Error::busy;
  if (!sd_.mkdir(request.path)) return audio::Error::file;
  char path[audio::PATH_CAPACITY];
  snprintf(path, sizeof(path), "%s/OWNER.DAT", request.path);
  FsFile owner;
  if (!owner.open(path, O_WRONLY | O_CREAT | O_EXCL)) return audio::Error::file;
  const bool owned = owner.write(request.path, 15) == 15 && owner.sync();
  const bool owner_closed = owner.close();
  if (!owned || !owner_closed) return audio::Error::read;
  snprintf(path, sizeof(path), "%s/TEST.WAV", request.path);
  FsFile file;
  if (!file.open(path, O_WRONLY | O_CREAT | O_EXCL)) return audio::Error::file;
  uint8_t header[44] = {};
  using namespace settings::record;
  const uint32_t frames = request.rate * request.seconds;
  const uint8_t alignment = request.channels * (request.bits / 8);
  const uint32_t bytes = frames * alignment;
  memcpy(header, "RIFF", 4);
  put32(header + 4, bytes + 36 + (bytes & 1u));
  memcpy(header + 8, "WAVEfmt ", 8);
  put32(header + 16, 16);
  put16(header + 20, 1);
  put16(header + 22, request.channels);
  put32(header + 24, request.rate);
  put32(header + 28, request.rate * alignment);
  put16(header + 32, alignment);
  put16(header + 34, request.bits);
  memcpy(header + 36, "data", 4);
  put32(header + 40, bytes);
  bool success = file.write(header, sizeof(header)) == sizeof(header);
  const int16_t wave[16] = {0, 6270, 11585, 15137, 16384, 15137, 11585, 6270,
      0, -6270, -11585, -15137, -16384, -15137, -11585, -6270};
  uint32_t phase = 0, fraction = 0;
  uint8_t data[audio::BLOCK_FRAMES * 8];
  for (uint32_t first = 0; success && first < frames; first += audio::BLOCK_FRAMES) {
    const uint32_t count = frames - first < audio::BLOCK_FRAMES
        ? frames - first : audio::BLOCK_FRAMES;
    for (uint32_t frame = 0; frame < count; ++frame) {
      const int32_t sample = wave[phase];
      const uint32_t raw = request.bits == 8 ? sample / 256 + 128 :
          static_cast<uint32_t>(sample * (1 << (request.bits - 16)));
      for (uint8_t channel = 0; channel < request.channels; ++channel)
        for (uint8_t byte = 0; byte < request.bits / 8; ++byte)
          data[frame * alignment + channel * (request.bits / 8) + byte] =
              static_cast<uint8_t>(raw >> (byte * 8));
      fraction += 7040;
      if (fraction >= request.rate) {
        fraction -= request.rate;
        phase = (phase + 1) % 16;
      }
    }
    success = file.write(data, count * alignment) == count * alignment;
  }
  if (success && (bytes & 1u)) {
    const uint8_t padding = 0;
    success = file.write(&padding, 1) == 1;
  }
  success = file.sync() && success;
  const bool closed = file.close();
  return success && closed ? audio::Error::none : audio::Error::read;
}

audio::Error SettingsStorageSD::cleanup(const char* folder) {
  Guard guard(mutex_);
  if (!ready_) return audio::Error::unavailable;
  if (!audio::validTestFolder(folder)) return audio::Error::invalid_value;
  char path[audio::PATH_CAPACITY], marker[15];
  snprintf(path, sizeof(path), "%s/OWNER.DAT", folder);
  FsFile owner;
  if (!owner.open(path, O_RDONLY)) return audio::Error::file;
  const bool owned = owner.fileSize() == 15 && owner.read(marker, 15) == 15 &&
      memcmp(marker, folder, 15) == 0;
  const bool closed = owner.close();
  if (!owned || !closed) return audio::Error::file;
  FsFile root;
  if (!root.open(folder, O_RDONLY)) return audio::Error::file;
  bool clean = true;
  FsFile entry;
  while (entry.openNext(&root, O_RDONLY)) {
    char name[32] = {};
    const size_t length = entry.getName(name, sizeof(name));
    clean = clean && !entry.isDir() && length > 0 &&
        (strcmp(name, "OWNER.DAT") == 0 || strcmp(name, "TEST.WAV") == 0);
    if (!entry.close()) clean = false;
    if (!clean) break;
  }
  clean = !root.getError() && !sd_.sdErrorCode() && clean;
  const bool root_closed = root.close();
  if (!clean || !root_closed) return audio::Error::file;
  snprintf(path, sizeof(path), "%s/TEST.WAV", folder);
  if (sd_.exists(path) && !sd_.remove(path)) return audio::Error::read;
  snprintf(path, sizeof(path), "%s/OWNER.DAT", folder);
  if (!sd_.remove(path) || !sd_.rmdir(folder)) return audio::Error::read;
  return audio::Error::none;
}

bool SettingsStorageSD::open(const char* path) {
  Guard guard(mutex_);
  close();
  if (!ready_ || !directory_.open(path, O_RDONLY)) {
    return false;
  }
  if (!directory_.isDir()) {
    directory_.close();
    return false;
  }
  return true;
}

serial_command::DirectoryResult SettingsStorageSD::next(
    char* name, size_t capacity, bool* is_directory, uint64_t* size) {
  Guard guard(mutex_);
  FsFile entry;
  if (!entry.openNext(&directory_, O_RDONLY)) {
    return directory_.getError() || sd_.sdErrorCode()
               ? serial_command::DirectoryResult::error
               : serial_command::DirectoryResult::end;
  }
  const size_t length = entry.getName(name, capacity);
  *is_directory = entry.isDir();
  *size = entry.fileSize();
  const bool closed = entry.close();
  return closed && length > 0 && length < capacity - 1
             ? serial_command::DirectoryResult::entry
             : serial_command::DirectoryResult::error;
}

void SettingsStorageSD::close() {
  Guard guard(mutex_);
  if (directory_) {
    directory_.close();
  }
  if (input_file_) {
    input_file_.close();
  }
}

bool SettingsStorageSD::openFile(const char* path) {
  Guard guard(mutex_);
  close();
  if (!ready_ || !input_file_.open(path, O_RDONLY)) {
    return false;
  }
  if (input_file_.isDir()) {
    input_file_.close();
    return false;
  }
  return true;
}

int SettingsStorageSD::readFile(uint8_t* data, size_t capacity) {
  Guard guard(mutex_);
  return input_file_.read(data, capacity);
}

settings::ReadResult SettingsStorageSD::read(uint8_t slot, uint8_t* data,
                                             size_t capacity, size_t* length) {
  if (slot >= 2) return settings::ReadResult::error;
  return readRecord(paths[slot], 2, data, capacity, length);
}

settings::ReadResult SettingsStorageSD::readRecord(
    const char* path, uint8_t version, uint8_t* data, size_t capacity,
    size_t* length) {
  Guard guard(mutex_);
  *length = 0;
  if (!ready_) return settings::ReadResult::error;
  if (!sd_.exists(path)) {
    return sd_.sdErrorCode() ? settings::ReadResult::error
                             : settings::ReadResult::missing;
  }
  FsFile file = sd_.open(path, O_RDONLY);
  if (!file) {
    return settings::ReadResult::error;
  }
  if (file.fileSize() > capacity) {
    // Read the header so newer versions are never overwritten.
    const int count = file.read(data, capacity);
    if (count < 0) {
      file.close();
      return settings::ReadResult::error;
    }
    *length = static_cast<size_t>(count);
    if (*length == capacity && capacity >= 5 && data[4] <= version) {
      *length = 0;
    }
    return file.close() ? settings::ReadResult::ok
                        : settings::ReadResult::error;
  }
  const size_t size = static_cast<size_t>(file.fileSize());
  const int count = file.read(data, size);
  const bool closed = file.close();
  *length = count < 0 ? 0 : static_cast<size_t>(count);
  return count == static_cast<int>(size) && closed
             ? settings::ReadResult::ok
             : settings::ReadResult::error;
}

bool SettingsStorageSD::write(uint8_t slot, const uint8_t* data, size_t size) {
  return slot < 2 && writeRecord(paths[slot], data, size);
}

bool SettingsStorageSD::writeRecord(const char* path, const uint8_t* data,
                                    size_t size) {
  Guard guard(mutex_);
  if (!ready_) return false;
  FsFile file = sd_.open(path, O_WRONLY | O_CREAT | O_TRUNC);
  if (!file) {
    return false;
  }
  const bool written = file.write(data, size) == size;
  const bool synced = file.sync();
  const bool closed = file.close();
  return written && synced && closed;
}

audio::Error SettingsStorageSD::AudioFile::open(const char* path) {
  Guard guard(storage_.mutex_);
  read_us = 0;
  wait_us = 0;
  max_read_us = 0;
  if (!storage_.ready_) return audio::Error::unavailable;
  if (!storage_.sd_.exists(path)) {
    return storage_.sd_.sdErrorCode() ? audio::Error::read : audio::Error::file;
  }
  file_ = storage_.sd_.open(path, O_RDONLY);
  if (!file_) return audio::Error::read;
  if (!file_.isFile()) {
    return file_.close() ? audio::Error::file : audio::Error::read;
  }
  return audio::Error::none;
}

uint64_t SettingsStorageSD::AudioFile::size() {
  Guard guard(storage_.mutex_);
  return file_.fileSize();
}

bool SettingsStorageSD::AudioFile::seek(uint32_t position) {
  Guard guard(storage_.mutex_);
  return file_.seekSet(position);
}

int SettingsStorageSD::AudioFile::read(uint8_t* data, size_t length) {
  const uint32_t started = micros();
  Guard guard(storage_.mutex_);
  const uint32_t acquired = micros();
  wait_us += static_cast<uint32_t>(acquired - started);
  const int result = file_.read(data, length);
  const uint32_t duration = micros() - acquired;
  read_us += duration;
  if (duration > max_read_us) max_read_us = duration;
  return result;
}

bool SettingsStorageSD::AudioFile::close() {
  Guard guard(storage_.mutex_);
  return !file_ || file_.close();
}

settings::ReadResult AlarmStorageSD::read(uint8_t slot, uint8_t* data,
                                          size_t capacity, size_t* length) {
  const char* const paths[] = {"BNAL0.DAT", "BNAL1.DAT"};
  return slot < 2 ? storage_.readRecord(paths[slot], 3, data, capacity, length)
                  : settings::ReadResult::error;
}

bool AlarmStorageSD::write(uint8_t slot, const uint8_t* data, size_t size) {
  const char* const paths[] = {"BNAL0.DAT", "BNAL1.DAT"};
  return slot < 2 && storage_.writeRecord(paths[slot], data, size);
}

settings::ReadResult SleepStorageSD::read(uint8_t slot, uint8_t* data,
                                         size_t capacity, size_t* length) {
  const char* const paths[] = {"BNSL0.DAT", "BNSL1.DAT"};
  return slot < 2 ? storage_.readRecord(paths[slot], 1, data, capacity, length)
                  : settings::ReadResult::error;
}

bool SleepStorageSD::write(uint8_t slot, const uint8_t* data, size_t size) {
  const char* const paths[] = {"BNSL0.DAT", "BNSL1.DAT"};
  return slot < 2 && storage_.writeRecord(paths[slot], data, size);
}
