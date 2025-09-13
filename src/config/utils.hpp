#pragma once

#include <filesystem>
#include <string>
#include <toml.hpp>

#include "format.hpp"
#include "displays/batch-system/enums.hpp"

namespace bd::ConfigUtils {
  void                  ensureConfigPathExists(const std::filesystem::path& p);
  std::filesystem::path getConfigPath(const std::string& config_name);
}

namespace bd::DisplayConfigurationUtils {
  DisplayRelativePosition getDisplayRelativePositionFromString(std::string_view& str);
  std::string             getDisplayRelativePositionString(DisplayRelativePosition pos);
  
  // Anchoring conversion functions
  ConfigurationHorizontalAnchor getHorizontalAnchorFromString(const std::string& str);
  std::string                   getHorizontalAnchorString(ConfigurationHorizontalAnchor anchor);
  ConfigurationVerticalAnchor   getVerticalAnchorFromString(const std::string& str);
  std::string                   getVerticalAnchorString(ConfigurationVerticalAnchor anchor);
}
