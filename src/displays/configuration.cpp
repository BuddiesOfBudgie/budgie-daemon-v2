#include "configuration.hpp"

namespace bd::DisplayConfiguration {
  std::optional<DisplayGroupOutputConfig*> getDisplayOutputConfigurationForIdentifier(const QString& identifier, DisplayGroup* group) {
    std::optional<DisplayGroupOutputConfig*> config = std::nullopt;
    for (auto& output : group->getConfigs()) {
      if (output->getIdentifier() == identifier) {
        config = output;
        break;
      }
    }

    return config;
  }
}
