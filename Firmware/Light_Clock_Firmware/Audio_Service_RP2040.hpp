#pragma once

#include "Audio_Playback.hpp"

class SettingsStorageSD;

namespace audio {
void initControl();
void beginCore1(SettingsStorageSD& storage);
bool updateCore1();
Error submit(const Request& request, uint32_t& id);
Status snapshot();
}
