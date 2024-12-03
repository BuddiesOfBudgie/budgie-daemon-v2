#include <optional>
#include <string>

#include "display.hpp"

namespace bd::DisplayConfiguration {
  std::optional<DisplayGroupOutputConfig*> getDisplayOutputConfigurationForSerial(const QString& serial, DisplayGroup* group);
}
