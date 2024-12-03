#include "utils.hpp"

#include <QtLogging>
#include <iostream>

namespace fs = std::filesystem;

void bd::ConfigUtils::ensureConfigPathExists(const fs::path& p) {
  auto dir = p.parent_path();
  if (!fs::exists(dir)) { fs::create_directories(dir); }
}

fs::path bd::ConfigUtils::getConfigPath(const std::string& config_name) {
  const char* xdg_config_home = std::getenv("XDG_CONFIG_HOME");
  fs::path    path {};
  if (xdg_config_home) path /= xdg_config_home;
  if (xdg_config_home == nullptr) {
    const char* home = std::getenv("HOME");
    if (!home) { qFatal("HOME environment variable not set"); }
    path /= home;
    path /= ".config";
  }

  path /= "budgie-desktop";
  path /= config_name;
  return path;
}

DisplayRelativePosition bd::DisplayConfigurationUtils::getDisplayRelativePositionFromString(std::string_view& str) {
  if (str == "left") {
    return DisplayRelativePosition::left;
  } else if (str == "right") {
    return DisplayRelativePosition::right;
  } else if (str == "above") {
    return DisplayRelativePosition::above;
  } else if (str == "below") {
    return DisplayRelativePosition::below;
  } else {
    return DisplayRelativePosition::none;
  }
}

std::string bd::DisplayConfigurationUtils::getDisplayRelativePositionString(DisplayRelativePosition pos) {
  switch (pos) {
    case DisplayRelativePosition::left:
      return "left";
    case DisplayRelativePosition::right:
      return "right";
    case DisplayRelativePosition::above:
      return "above";
    case DisplayRelativePosition::below:
      return "below";
    default:
      return "none";
  }
}
