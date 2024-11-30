#include "configuration.hpp"

namespace bd::DisplayConfiguration {
  std::optional<DisplayGroupOutputConfig*> getDisplayOutputConfigurationForSerial(std::string& serial, DisplayGroup* group) {
    std::optional<DisplayGroupOutputConfig*> config = std::nullopt;
    for (auto& output : group->getConfigs()) {
      if (output->getSerial() == serial) {
        config = output;
        break;
      }
    }

    return config;
  }
}
