#include "display.hpp"

#include <QFile>
#include <QTextStream>
#include <filesystem>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "configuration.hpp"
#include "output-manager/WaylandOutputManager.hpp"
#include "utils.hpp"

namespace fs = std::filesystem;

namespace bd {
  DisplayConfig::DisplayConfig(QObject* parent)
      : QObject(parent), m_preferences({.automatic_attach_outputs_relative_position = DisplayRelativePosition::none}), m_groups({}) {}

  DisplayConfig& DisplayConfig::instance() {
    static DisplayConfig _instance(nullptr);
    return _instance;
  }

  void DisplayConfig::apply() {
    auto matchOption = getMatchingGroup();

    // Didn't find a match, so create a new group based on the orchestrator state
    // This in the background sets properties on our DisplayConfig class, so we just need to return at this point.
    if (!matchOption.has_value()) {
      std::cout << "No matching group found, creating new group" << std::endl;
      getActiveGroup();

      // Save the state to our config
      std::cout << "Saving state" << std::endl;
      saveState();

      return;
    }

    auto& orchestrator = bd::WaylandOrchestrator::instance();
    auto  manager      = orchestrator.getManager();

    auto group = matchOption.value();

    // Set the active group to the matched group
    m_activeGroup = group;

    auto heads = manager->getHeads();

    auto wlr_output_config = manager->configure();

    // TODO: Josh - Fix
    // zwlr_output_configuration_v1_add_listener(wlr_output_config, &config_listener, nullptr);

    bool should_apply = false;

    for (auto& head : heads) {
      if (head->getSerial() == nullptr) continue;
      for (const auto& qSerial : group->getOutputSerials()) {
        auto serial = qSerial.toStdString();
        if (head->getSerial() == qSerial) continue;
        std::cout << "Checking output " << serial << std::endl;

        auto config_option = bd::DisplayConfiguration::getDisplayOutputConfigurationForSerial(serial, group);
        if (!config_option.has_value()) continue;
        std::cout << "Got configuration for output " << serial << std::endl;
        const auto& config = config_option.value();
        should_apply       = true;

        if (config->getDisabled()) {
          std::cout << "Disabling output " << serial << std::endl;
          wlr_output_config->disable(head);
          continue;
        }

        // Enable the head and get the configuration head struct
        auto config_head = wlr_output_config->enable(head);

        auto width    = config->getWidth();
        auto height   = config->getHeight();
        auto refresh  = config->getRefresh();
        auto position = config->getPosition();

        auto mode_option = head->getModeForOutputHead(width, height, refresh);

        if (mode_option.has_value()) {  // Found an existing mode for the head
          auto mode = mode_option.value();
          std::cout << "Found mode for output " << serial << ": \n\t" << width << "x" << height << "@" << refresh << "\n\t"
                    << "Position: " << position.at(0) << ", " << position.at(1) << std::endl;
          config_head->setMode(mode);
        } else {
          std::cout << "Found no mode for output " << serial << ", applying custom: \n\t" << width << "x" << height << "@" << refresh << "\n\t"
                    << "Position: " << position.at(0) << ", " << position.at(1) << std::endl;
          config_head->setCustomMode(width, height, refresh);
        }

        config_head->setPosition(static_cast<int32_t>(position.at(0)), static_cast<int32_t>(position.at(1)));  // Apply related position to the head
        config_head->setScale(config->getScale());                                                             // Apply related scale to the head
        config_head->setTransform(config->getRotation());
        config_head->setAdaptiveSync(
            config->getAdaptiveSync() ? QtWayland::zwlr_output_head_v1::adaptive_sync_state_enabled
                                      : QtWayland::zwlr_output_head_v1::adaptive_sync_state_disabled);  // Apply related transform to the head
      }
    }

    if (should_apply) {
      std::cout << "Applying configuration" << std::endl;
      wlr_output_config->applySelf();
      wlr_output_config->release();
      wl_display_dispatch(bd::WaylandOrchestrator::instance().getDisplay());
    } else {
      std::cout << "No configuration to apply" << std::endl;
    }
  }

  DisplayGroup* DisplayConfig::createDisplayGroupForState() {
    auto& orchestrator = bd::WaylandOrchestrator::instance();
    auto  manager      = orchestrator.getManager();
    auto  heads        = manager->getHeads();

    QStringList names_of_active_outputs;
    std::transform(heads.begin(), heads.end(), std::back_inserter(names_of_active_outputs), [](auto head) {
      if (head->getSerial() == nullptr) return QString {};
      return head->getSerial();
    });

    auto defaultDisplayGroupForState = new DisplayGroup();
    defaultDisplayGroupForState->setName(names_of_active_outputs.join(", ").append(" (Auto Generated)"));
    defaultDisplayGroupForState->setOutputSerials(names_of_active_outputs);
    defaultDisplayGroupForState->setPreferred(true);
    defaultDisplayGroupForState->setPrimaryOutput(names_of_active_outputs.first());

    for (const auto& head : heads) {
      if (head->getSerial() == nullptr) continue;
      auto head_mode = head->getCurrentMode();
      auto config    = new DisplayGroupOutputConfig();
      config->setSerial(head->getSerial());
      config->setWidth(head_mode->getWidth());
      config->setHeight(head_mode->getHeight());
      config->setRefresh(head_mode->getRefresh());
      config->setPosition({head->getX(), head->getY()});
      config->setScale(head->getScale());
      config->setRotation(head->getTransform());
      config->setAdaptiveSync(head->getAdaptiveSync());
      defaultDisplayGroupForState->addConfig(config);
    }

    return defaultDisplayGroupForState;
  }

  void DisplayConfig::debugOutput() {
    for (const auto& group : this->m_groups) {
      std::cout << "Group: " << group->getName().toStdString() << std::endl;
      std::cout << "Primary Output: " << group->getPrimaryOutput().toStdString() << std::endl;
      std::cout << "Output Serials: " << std::endl;

      for (auto config : group->getConfigs()) {
        std::cout << "  Serial: " << config->getSerial().toStdString() << std::endl;
        std::cout << "    Width: " << config->getWidth() << std::endl;
        std::cout << "    Height: " << config->getHeight() << std::endl;
        std::cout << "    Refresh: " << config->getRefresh() << std::endl;
        std::cout << "    Position: " << config->getPosition()[0] << ", " << config->getPosition()[1] << std::endl;
        std::cout << "    Scale: " << config->getScale() << std::endl;
        std::cout << "    Rotation: " << config->getRotation() << std::endl;
        std::cout << "    Adaptive Sync: " << config->getAdaptiveSync() << std::endl;
      }
    }
  }

  DisplayGroup* DisplayConfig::getActiveGroup() {
    if (this->m_activeGroup == nullptr) {
      auto groupFromState = createDisplayGroupForState();
      m_activeGroup       = groupFromState;
      m_groups.append(this->m_activeGroup);
    }

    return this->m_activeGroup;
  }

  std::optional<DisplayGroup*> DisplayConfig::getMatchingGroup() {
    auto                         manager        = bd::WaylandOrchestrator::instance().getManager();
    auto                         heads          = manager->getHeads();
    auto                         heads_size     = heads.size();
    std::optional<DisplayGroup*> matching_group = std::nullopt;

    auto matching_groups = std::vector<DisplayGroup*> {};
    for (auto group : this->m_groups) {
      if (group->getOutputSerials().size() != heads_size) { continue; }

      bool match = true;
      for (const auto& qSerial : group->getOutputSerials()) {
        bool found = false;
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
      if (group->isPreferred()) {
        matching_group = group;
        break;
      }
    }

    return matching_group;
  }

  void DisplayConfig::parseConfig() {
    auto config_location = bd::ConfigUtils::getConfigPath("display-config.toml");

    try {
      std::cout << "Reading display config from " << config_location << std::endl;
      bd::ConfigUtils::ensureConfigPathExists(config_location);

      auto data = toml::parse(config_location);

      if (data.contains("preferences")) {
        auto position = data.at("preferences").at("automatic_attach_outputs_relative_position");
        if (position.is_string()) {
          auto pos                                                       = std::string_view {position.as_string()};
          this->m_preferences.automatic_attach_outputs_relative_position = bd::DisplayConfigurationUtils::getDisplayRelativePositionFromString(pos);
        }
      }

      for (const auto& group : toml::find<std::vector<toml::value>>(data, "group")) { this->m_groups.append(new DisplayGroup(group)); }
    } catch (const std::exception& e) {
      if (std::basic_string(e.what()).contains("error opening file")) return;
      std::cerr << "Error parsing display-config.toml: " << e.what() << std::endl;
    }
  }

  void DisplayConfig::saveState() {
    toml::ordered_value config(toml::ordered_table {});
    config.as_table_fmt().fmt = toml::table_format::multiline;

    toml::ordered_value preferences_table(toml::ordered_table {});
    preferences_table["automatic_attach_outputs_relative_position"] =
        bd::DisplayConfigurationUtils::getDisplayRelativePositionString(this->m_preferences.automatic_attach_outputs_relative_position);

    // Create our toml table for each group
    toml::ordered_value groups(toml::ordered_array {});
    groups.as_array_fmt().fmt = toml::array_format::array_of_tables;

    for (const auto& group : this->m_groups) { groups.push_back(group->toToml()); }

    config["preferences"] = preferences_table;
    config.as_table().emplace_back("group", groups);

    auto serialized_config = toml::format(config);
    auto config_location   = bd::ConfigUtils::getConfigPath("display-config.toml");
    auto config_file       = QFile(config_location);

    if (config_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream stream(&config_file);
      stream << serialized_config.c_str();
      config_file.close();
    } else {
      std::cerr << "Failed to open display-config.toml for writing" << std::endl;
    }
  }

  // DisplayGroup
  DisplayGroup::DisplayGroup(QObject* parent) : QObject(parent), m_name(""), m_preferred(false), m_output_serials({}), m_primary_output(""), m_configs({}) {}

  DisplayGroup::DisplayGroup(const toml::value& v, QObject* parent) : QObject(parent) {
    QStringList output_serials;
    for (const auto& serial : toml::find_or<std::vector<std::string>>(v, "output_serials", {})) { output_serials.append(QString::fromStdString(serial)); }

    m_name           = QString::fromStdString(toml::find<std::string>(v, "name"));
    m_output_serials = output_serials;
    m_primary_output = QString::fromStdString(toml::find<std::string>(v, "primary_output"));
    m_preferred      = toml::find_or<bool>(v, "preferred", false);

    auto outputs = toml::find_or<std::vector<toml::value>>(v, "output", {});
    if (outputs.empty()) return;

    for (const toml::value& output : outputs) {
      auto dgo = new DisplayGroupOutputConfig();
      dgo->setSerial(QString::fromStdString(toml::find<std::string>(output, "serial")));
      dgo->setWidth(toml::find<int>(output, "width"));
      dgo->setHeight(toml::find<int>(output, "height"));
      dgo->setRefresh(toml::find<int>(output, "refresh"));
      dgo->setPosition(toml::find<std::array<int, 2>>(output, "position"));
      dgo->setScale(toml::find_or<float>(output, "scale", 1.0));
      dgo->setRotation(toml::find_or<int>(output, "rotation", 0));
      dgo->setAdaptiveSync(toml::find_or<bool>(output, "adaptive_sync", false));
      dgo->setDisabled(toml::find_or<bool>(output, "disabled", false));

      m_configs.append(dgo);
    }
  }

  QString DisplayGroup::getName() const {
    return this->m_name;
  }

  bool DisplayGroup::isPreferred() const {
    return this->m_preferred;
  }

  QStringList DisplayGroup::getOutputSerials() const {
    return this->m_output_serials;
  }

  QString DisplayGroup::getPrimaryOutput() const {
    return this->m_primary_output;
  }

  QList<DisplayGroupOutputConfig*> DisplayGroup::getConfigs() {
    return this->m_configs;
  }

  std::optional<DisplayGroupOutputConfig*> DisplayGroup::getConfigForSerial(QString serial) {
    std::optional<DisplayGroupOutputConfig*> config = std::nullopt;
    for (auto& output : this->m_configs) {
      if (output->getSerial() == serial) {
        config = output;
        break;
      }
    }

    return config;
  }

  void DisplayGroup::addConfig(DisplayGroupOutputConfig* config) {
    this->m_configs.append(config);
  }

  void DisplayGroup::setName(const QString& name) {
    this->m_name = name;
  }

  void DisplayGroup::setOutputSerials(const QStringList& serials) {
    this->m_output_serials = serials;
  }

  void DisplayGroup::setPreferred(bool preferred) {
    this->m_preferred = preferred;
  }

  void DisplayGroup::setPrimaryOutput(const QString& serial) {
    this->m_primary_output = serial;
  }

  toml::ordered_value DisplayGroup::toToml() {
    std::vector<std::string> output_serials;
    for (const auto& serial : m_output_serials) { output_serials.push_back(serial.toStdString()); }

    toml::ordered_value group_table(toml::ordered_table {});
    group_table.as_table_fmt().fmt = toml::table_format::multiline;

    group_table["name"]           = this->m_name.toStdString();
    group_table["preferred"]      = this->m_preferred;
    group_table["output_serials"] = output_serials;
    group_table["primary_output"] = this->m_primary_output.toStdString();

    toml::ordered_value outputs(toml::ordered_array {});
    outputs.as_array_fmt().fmt = toml::array_format::array_of_tables;
    for (auto config : this->m_configs) { outputs.push_back(config->toToml()); }

    group_table.as_table().emplace_back("output", outputs);

    return group_table;
  }

  // DisplayGroupOutputConfig

  DisplayGroupOutputConfig::DisplayGroupOutputConfig(QObject* parent) : QObject(parent) {}

  bool DisplayGroupOutputConfig::getAdaptiveSync() const {
    return this->m_adaptive_sync;
  }

  bool DisplayGroupOutputConfig::getDisabled() const {
    return this->m_disabled;
  }

  int DisplayGroupOutputConfig::getHeight() const {
    return this->m_height;
  }

  QString DisplayGroupOutputConfig::getSerial() const {
    return this->m_serial;
  }

  std::array<int, 2> DisplayGroupOutputConfig::getPosition() const {
    return this->m_position;
  }

  int DisplayGroupOutputConfig::getRefresh() const {
    return this->m_refresh;
  }

  int DisplayGroupOutputConfig::getRotation() const {
    return this->m_rotation;
  }

  double DisplayGroupOutputConfig::getScale() const {
    return this->m_scale;
  }

  int DisplayGroupOutputConfig::getWidth() const {
    return this->m_width;
  }

  void DisplayGroupOutputConfig::setAdaptiveSync(bool adaptive_sync) {
    this->m_adaptive_sync = adaptive_sync;
  }

  void DisplayGroupOutputConfig::setDisabled(bool disabled) {
    this->m_disabled = disabled;
  }

  void DisplayGroupOutputConfig::setHeight(int height) {
    this->m_height = height;
  }

  void DisplayGroupOutputConfig::setPosition(const std::array<int, 2>& position) {
    this->m_position = position;
  }

  void DisplayGroupOutputConfig::setRefresh(int refresh) {
    this->m_refresh = refresh;
  }

  void DisplayGroupOutputConfig::setRotation(int rotation) {
    this->m_rotation = rotation;
  }

  void DisplayGroupOutputConfig::setScale(double scale) {
    this->m_scale = scale;
  }

  void DisplayGroupOutputConfig::setSerial(const QString& serial) {
    this->m_serial = serial;
  }

  void DisplayGroupOutputConfig::setWidth(int width) {
    this->m_width = width;
  }

  toml::ordered_value DisplayGroupOutputConfig::toToml() {
    toml::ordered_value config_table(toml::ordered_table {});
    config_table.as_table_fmt().fmt = toml::table_format::multiline;

    config_table["serial"]        = this->m_serial.toStdString();
    config_table["width"]         = this->m_width;
    config_table["height"]        = this->m_height;
    config_table["refresh"]       = this->m_refresh;
    config_table["position"]      = this->m_position;
    config_table["scale"]         = this->m_scale;
    config_table["rotation"]      = this->m_rotation;
    config_table["adaptive_sync"] = this->m_adaptive_sync;
    config_table["disabled"]      = this->m_disabled;

    return config_table;
  }
}
