#include <optional>
#include <string>

#include "display.hpp"

namespace bd::DisplayConfiguration {
  std::optional<DisplayGroupOutputConfig*> getDisplayOutputConfigurationForIdentifier(const QString& identifier, DisplayGroup* group);
}
