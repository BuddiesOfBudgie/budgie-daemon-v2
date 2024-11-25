#include <optional>
#include <string>

#include "format.hpp"

namespace bd::DisplayConfiguration {
  std::optional<DisplayGroupOutputConfig> getDisplayOutputConfigurationForSerial(std::string& serial, DisplayGrouping* group);
}
