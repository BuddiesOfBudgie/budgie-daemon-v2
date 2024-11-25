#include "configuration.hpp"

namespace bd::DisplayConfiguration {
  std::optional<DisplayGroupOutputConfig> getDisplayOutputConfigurationForSerial(std::string& serial, DisplayGrouping* group) {
    std::optional<DisplayGroupOutputConfig> config = std::nullopt;
    for (auto& output : group->configs) {
      if (output.serial == serial) {
        config = output;
        break;
      }
    }

    return config;
  }
}
