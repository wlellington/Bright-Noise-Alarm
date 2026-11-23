#pragma once

#include <SdFat.h>
#include <pico/mutex.h>

#include "Audio_Playback.hpp"
#include "File_Management.hpp"
#include "Serial_Command.hpp"

class SettingsStorageSD : public settings::Storage,
                          public serial_command::DirectoryReader {
public:
  SettingsStorageSD();
  class AudioFile : public audio::Source {
  public:
    explicit AudioFile(SettingsStorageSD& storage) : storage_(storage) {}
    audio::Error open(const char* path) override;
    uint64_t size() override;
    bool seek(uint32_t position) override;
    int read(uint8_t* data, size_t length) override;
    bool close() override;
    uint64_t read_us = 0;
    uint64_t wait_us = 0;
    uint32_t max_read_us = 0;
  private:
    SettingsStorageSD& storage_;
    FsFile file_;
  };
  bool begin();
  bool audioCatalog(char paths[][audio::PATH_CAPACITY], uint8_t capacity,
                    uint8_t& count, bool& more);
  audio::Error fixture(const audio::Request& request);
  audio::Error cleanup(const char* folder);
  audio::Error audioSpi(uint32_t mhz);
  uint32_t spiMHz();
  settings::ReadResult read(uint8_t slot, uint8_t* data, size_t capacity,
                            size_t* length) override;
  bool write(uint8_t slot, const uint8_t* data, size_t size) override;
  bool open(const char* path) override;
  serial_command::DirectoryResult next(char* name, size_t capacity,
                                       bool* is_directory,
                                       uint64_t* size) override;
  void close() override;
  bool openFile(const char* path) override;
  int readFile(uint8_t* data, size_t capacity) override;
  settings::ReadResult readRecord(const char* path, uint8_t version,
                                  uint8_t* data, size_t capacity, size_t* length);
  bool writeRecord(const char* path, const uint8_t* data, size_t size);

private:
  class Guard {
  public:
    explicit Guard(recursive_mutex_t& mutex) : mutex_(mutex) {
      recursive_mutex_enter_blocking(&mutex_);
    }
    ~Guard() { recursive_mutex_exit(&mutex_); }
    Guard(const Guard&) = delete;
    Guard& operator=(const Guard&) = delete;
  private:
    recursive_mutex_t& mutex_;
  };
  recursive_mutex_t mutex_;
  SdFs sd_;
  FsFile directory_;
  FsFile input_file_;
  bool ready_ = false;
  uint32_t spi_mhz_ = 10;
};

class AlarmStorageSD : public settings::Storage {
public:
  explicit AlarmStorageSD(SettingsStorageSD& storage) : storage_(storage) {}
  settings::ReadResult read(uint8_t slot, uint8_t* data, size_t capacity,
                            size_t* length) override;
  bool write(uint8_t slot, const uint8_t* data, size_t size) override;
private:
  SettingsStorageSD& storage_;
};

class SleepStorageSD : public settings::Storage {
public:
  explicit SleepStorageSD(SettingsStorageSD& storage) : storage_(storage) {}
  settings::ReadResult read(uint8_t slot, uint8_t* data, size_t capacity,
                            size_t* length) override;
  bool write(uint8_t slot, const uint8_t* data, size_t size) override;
private:
  SettingsStorageSD& storage_;
};
