#include "display.hpp"

#include <iostream>
#include <map>

#include "configuration.hpp"
#include "output-manager/WaylandOutputManager.hpp"
#include "utils.hpp"

namespace bd {
  DisplayConfig::DisplayConfig(QObject* parent)
      : QObject(parent), preferences({.automatic_attach_outputs_relative_position = DisplayRelativePosition::none}), groups({}) {}

  DisplayConfig& DisplayConfig::instance() {
    static DisplayConfig _instance(nullptr);
    return _instance;
  }

  void DisplayConfig::apply() {
    if (this->groups.empty()) return;
    auto matchOption = getMatchingGroup();
    if (!matchOption.has_value()) return;

    auto& orchestrator = bd::WaylandOrchestrator::instance();
    auto  manager      = orchestrator.getManager();

    auto group = matchOption.value();
    auto heads = manager->getHeads();

    if (group.output_serials.size() != heads.size()) {
      std::cout << "Grouping does not match the number of outputs" << std::endl;
      return;
    }
    std::cout << "Serial in apply: " << manager->getSerial() << std::endl;
    auto wlr_output_config = manager->configure();

    // TODO: Josh - Fix
    // zwlr_output_configuration_v1_add_listener(wlr_output_config, &config_listener, nullptr);

    bool should_apply = false;

    for (auto& head : heads) {
      if (head->getSerial() == nullptr) continue;
      for (auto serial : group.output_serials) {
        auto qSerial = QString {serial.c_str()};
        if (head->getSerial() == qSerial) continue;
        std::cout << "Checking output " << serial << std::endl;

        auto config_option = bd::DisplayConfiguration::getDisplayOutputConfigurationForSerial(serial, &group);
        if (!config_option.has_value()) continue;
        std::cout << "Got configuration for output " << serial << std::endl;
        auto config  = config_option.value();
        should_apply = true;

        if (config.disabled) {
          std::cout << "Disabling output " << serial << std::endl;
          wlr_output_config->disable(head);
          continue;
        }

        // Enable the head and get the configuration head struct
        auto config_head = wlr_output_config->enable(head);

        auto mode_option = head->getModeForOutputHead(config.width, config.height, config.refresh);
        if (mode_option.has_value()) {  // Found an existing mode for the head
          auto mode = mode_option.value();
          std::cout << "Found mode for output " << serial << ": \n\t" << mode->getWidth() << "x" << mode->getHeight() << "@" << mode->getRefresh() << "\n\t"
                    << "Position: " << config.position.at(0) << ", " << config.position.at(1) << std::endl;
          config_head->set_mode(mode->getWlrMode());
        } else {
          std::cout << "Found no custom mode for output " << serial << ", applying custom: \n\t" << config.width << "x" << config.height << "@"
                    << config.refresh << "\n\t"
                    << "Position: " << config.position.at(0) << ", " << config.position.at(1) << std::endl;
          config_head->set_custom_mode(config.width, config.height, config.refresh);
        }

        config_head->set_position(
            static_cast<int32_t>(config.position.at(0)), static_cast<int32_t>(config.position.at(1)));  // Apply related position to the head
        config_head->set_scale(wl_fixed_from_double(config.scale));                                     // Apply related scale to the head
        config_head->set_transform(config.rotation);
        config_head->set_adaptive_sync(
            config.adaptive_sync ? QtWayland::zwlr_output_head_v1::adaptive_sync_state_enabled
                                 : QtWayland::zwlr_output_head_v1::adaptive_sync_state_disabled);  // Apply related transform to the head
        // TODO: Josh - Maybe have to do something here
        // zwlr_output_configuration_head_v1_destroy(config_head);
      }
    }

    if (should_apply) {
      std::cout << "Applying configuration" << std::endl;
      // TODO: Josh - Debug segfault in config_head setup before trying bits below
      //      wlr_output_config->applySelf();
      //      wlr_output_config->release();
      //
      //      wl_display_dispatch_pending(bd::WaylandOrchestrator::instance().getDisplay());
    } else {
      std::cout << "No configuration to apply" << std::endl;
    }
  }

  void DisplayConfig::debugOutput() {
    for (const auto& group : this->groups) {
      std::cout << "Group: " << group.name << std::endl;
      std::cout << "Primary Output: " << group.primary_output << std::endl;
      std::cout << "Output Serials: " << std::endl;

      for (const auto& config : group.configs) {
        std::cout << "  Serial: " << config.serial << std::endl;
        std::cout << "    Width: " << config.width << std::endl;
        std::cout << "    Height: " << config.height << std::endl;
        std::cout << "    Refresh: " << config.refresh << std::endl;
        std::cout << "    Position: " << config.position[0] << ", " << config.position[1] << std::endl;
        std::cout << "    Scale: " << config.scale << std::endl;
        std::cout << "    Rotation: " << config.rotation << std::endl;
        std::cout << "    Adaptive Sync: " << config.adaptive_sync << std::endl;
      }
    }
  }

  std::optional<DisplayGrouping> DisplayConfig::getMatchingGroup() {
    auto                           manager        = bd::WaylandOrchestrator::instance().getManager();
    auto                           heads          = manager->getHeads();
    auto                           serials_size   = heads.size();
    std::optional<DisplayGrouping> matching_group = std::nullopt;

    auto matching_groups = std::vector<DisplayGrouping> {};
    for (const auto& group : this->groups) {
      if (group.output_serials.size() != serials_size) { continue; }

      bool match = true;
      for (const auto& serial : group.output_serials) {
        auto qSerial = QString {serial.c_str()};
        bool found   = false;
        for (const auto& head : heads) {
          if (head->getSerial() == nullptr) continue;
          if (head->getSerial() == qSerial) {
            found = true;
            break;
          }
        }

        if (!found) {
          match = false;
          break;
        }
      }

      if (!match) continue;
      matching_groups.push_back(group);
    }

    if (matching_groups.empty()) return matching_group;

    for (const auto& group : matching_groups) {
      if (group.preferred) {
        matching_group = group;
        break;
      }
    }

    return matching_group;
  }

  void DisplayConfig::parseConfig() {
    try {
      auto config_location = bd::ConfigUtils::getConfigPath("display-config.toml");
      std::cout << "Reading display config from " << config_location << std::endl;
      bd::ConfigUtils::ensureConfigPathExists(config_location);
      auto data = toml::parse(config_location);

      if (data.contains("preferences")) {
        auto position = data.at("preferences").at("automatic_attach_outputs_relative_position");
        if (position.is_string()) {
          auto pos                                                     = std::string_view {position.as_string()};
          this->preferences.automatic_attach_outputs_relative_position = bd::DisplayConfigurationUtils::getDisplayRelativePositionFromString(pos);
        }
      }

      for (const auto& group : toml::find<std::vector<toml::value>>(data, "group")) {
        DisplayGrouping dg;
        bd::DisplayConfigurationUtils::tomlToDisplayGrouping(group, dg);
        this->groups.push_back(dg);
      }
    } catch (const std::exception& e) { std::cerr << "Error parsing display-config.toml: " << e.what() << std::endl; }
  }

  void DisplayConfig::saveManagerState() {
    std::map<std::string, DisplayGroupOutputConfig> configOutputGroup;
    for (const auto& head : bd::WaylandOrchestrator::instance().getManager()->getHeads()) {
      DisplayGroupOutputConfig dgo = {
          .serial   = head->getSerial().toStdString(),
          .width    = head->getCurrentMode()->getWidth(),
          .height   = head->getCurrentMode()->getHeight(),
          .refresh  = head->getCurrentMode()->getRefresh(),
          .position = {head->getX(), head->getY()}};

      configOutputGroup[dgo.serial] = dgo;
    }

    // TODO: Actually finish this
  }

  std::string DisplayConfig::serialize() {
    toml::ordered_value output(toml::ordered_array {});
    output.as_array_fmt().fmt = toml::array_format::array_of_tables;
    for (const auto& group : this->groups) {
      toml::ordered_value group_table(toml::ordered_table {});
      group_table["name"]            = group.name;
      group_table["preferred"]       = group.preferred;
      group_table["output_serials"]  = group.output_serials;
      group_table["primary_output"]  = group.primary_output;
      group_table.as_table_fmt().fmt = toml::table_format::multiline;

      toml::ordered_array configs;
      for (auto& config : group.configs) {
        toml::ordered_value config_table(toml::ordered_table {});
        config_table["serial"]          = config.serial;
        config_table["width"]           = config.width;
        config_table["height"]          = config.height;
        config_table["refresh"]         = config.refresh;
        config_table["position"]        = config.position;
        config_table.as_table_fmt().fmt = toml::table_format::multiline;
        configs.push_back(config_table);
      }

      group_table["output"] = configs;
      output.push_back(group_table);
    }

    return toml::format("group", output);
  }
}
