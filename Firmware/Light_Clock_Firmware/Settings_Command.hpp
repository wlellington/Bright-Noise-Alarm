#pragma once

#include "File_Management.hpp"

namespace settings_command {

enum class Result { success, invalid_value, storage_failed };

Result setDisplay(settings::Store& store, settings::DisplaySettings* current,
                  const settings::DisplaySettings& candidate);

}  // namespace settings_command
