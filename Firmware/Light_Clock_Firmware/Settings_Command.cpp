#include "Settings_Command.hpp"

namespace settings_command {

Result setDisplay(settings::Store& store, settings::DisplaySettings* current,
                  const settings::DisplaySettings& candidate) {
  if (!settings::valid(candidate)) {
    return Result::invalid_value;
  }
  if (store.save(candidate) != settings::Result::ok) {
    return Result::storage_failed;
  }
  *current = candidate;
  return Result::success;
}

}  // namespace settings_command
