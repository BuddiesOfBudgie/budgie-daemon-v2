#include "utils.hpp"

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
    if (!home) {
      std::cerr << "HOME environment variable not set" << std::endl;
      std::exit(1);
    }
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

void bd::DisplayConfigurationUtils::tomlToDisplayGrouping(const toml::value& v, DisplayGrouping& dg) {
  dg.name           = toml::find<std::string>(v, "name");
  dg.output_serials = toml::find_or<std::vector<std::string>>(v, "output_serials", {});
  dg.primary_output = toml::find<std::string>(v, "primary_output");
  dg.preferred      = toml::find_or<bool>(v, "preferred", false);

  auto outputs = toml::find_or<std::vector<toml::value>>(v, "output", {});
  if (outputs.empty()) return;

  for (const toml::value& output : outputs) {
    DisplayGroupOutputConfig dgo;
    dgo.serial        = toml::find<std::string>(output, "serial");
    dgo.width         = toml::find<int>(output, "width");
    dgo.height        = toml::find<int>(output, "height");
    dgo.refresh       = toml::find<int>(output, "refresh");
    dgo.position      = toml::find<std::array<int, 2>>(output, "position");
    dgo.scale         = toml::find_or<float>(output, "scale", 1.0);
    dgo.rotation      = toml::find_or<int>(output, "rotation", 0);
    dgo.adaptive_sync = toml::find_or<bool>(output, "adaptive_sync", false);
    dgo.disabled      = toml::find_or<bool>(output, "disabled", false);

    dg.configs.push_back(dgo);
  }
}
