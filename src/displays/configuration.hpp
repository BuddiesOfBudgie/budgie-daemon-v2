#include <optional>
#include <string>

#include "config/display.hpp"

namespace bd::DisplayConfiguration {
  std::optional<DisplayGroupOutputConfig*> getDisplayOutputConfigurationForIdentifier(const QString& identifier, DisplayGroup* group);
}
