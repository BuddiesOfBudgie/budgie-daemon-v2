#include <optional>
#include <string>

#include "display.hpp"

namespace bd::DisplayConfiguration {
  std::optional<DisplayGroupOutputConfig*> getDisplayOutputConfigurationForSerial(std::string& serial, DisplayGroup* group);
}
