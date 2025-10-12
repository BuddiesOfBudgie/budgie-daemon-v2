#include "utils.hpp"

#include <QtLogging>

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

bd::ConfigurationHorizontalAnchor bd::DisplayConfigurationUtils::getHorizontalAnchorFromString(const std::string& str) {
  if (str == "left") return bd::ConfigurationHorizontalAnchor::Left;
  if (str == "right") return bd::ConfigurationHorizontalAnchor::Right;
  if (str == "center") return bd::ConfigurationHorizontalAnchor::Center;
  return bd::ConfigurationHorizontalAnchor::NoHorizontalAnchor;
}

std::string bd::DisplayConfigurationUtils::getHorizontalAnchorString(bd::ConfigurationHorizontalAnchor anchor) {
  switch (anchor) {
    case bd::ConfigurationHorizontalAnchor::Left:
      return "left";
    case bd::ConfigurationHorizontalAnchor::Right:
      return "right";
    case bd::ConfigurationHorizontalAnchor::Center:
      return "center";
    default:
      return "none";
  }
}

bd::ConfigurationVerticalAnchor bd::DisplayConfigurationUtils::getVerticalAnchorFromString(const std::string& str) {
  if (str == "above") return bd::ConfigurationVerticalAnchor::Above;
  if (str == "top") return bd::ConfigurationVerticalAnchor::Top;
  if (str == "middle") return bd::ConfigurationVerticalAnchor::Middle;
  if (str == "bottom") return bd::ConfigurationVerticalAnchor::Bottom;
  if (str == "below") return bd::ConfigurationVerticalAnchor::Below;
  return bd::ConfigurationVerticalAnchor::NoVerticalAnchor;
}

std::string bd::DisplayConfigurationUtils::getVerticalAnchorString(bd::ConfigurationVerticalAnchor anchor) {
  switch (anchor) {
    case bd::ConfigurationVerticalAnchor::Above:
      return "above";
    case bd::ConfigurationVerticalAnchor::Top:
      return "top";
    case bd::ConfigurationVerticalAnchor::Middle:
      return "middle";
    case bd::ConfigurationVerticalAnchor::Bottom:
      return "bottom";
    case bd::ConfigurationVerticalAnchor::Below:
      return "below";
    default:
      return "none";
  }
}
