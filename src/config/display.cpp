#include "display.hpp"

#include <QFile>
#include <QTextStream>
#include <QtDebug>
#include <string>
#include <vector>

#include "configuration.hpp"
#include "output-manager/WaylandOutputManager.hpp"
#include "displays/batch-system/ConfigurationBatchSystem.hpp"
#include "utils.hpp"

namespace bd {
  DisplayConfig::DisplayConfig(QObject* parent)
      : QObject(parent), m_preferences({.automatic_attach_outputs_relative_position = DisplayRelativePosition::none}), m_groups({}) {}

  DisplayConfig& DisplayConfig::instance() {
    static DisplayConfig _instance(nullptr);
    return _instance;
  }

  void DisplayConfig::apply() {
    // Get current system outputs
    auto& orchestrator = WaylandOrchestrator::instance();
    auto  manager      = orchestrator.getManager();
    auto  heads        = manager->getHeads();

    // Find a matching group for current system configuration
    auto matchOption = getMatchingGroup();

    // No matching group found - don't apply anything
    if (!matchOption.has_value()) {
      qInfo() << "No matching display group found for current system configuration";
      qInfo() << "Available outputs:" << [&heads]() {
        QStringList identifiers;
        for (const auto& head : heads) {
          if (head && !head->getIdentifier().isNull()) {
            identifiers.append(head->getIdentifier());
          }
        }
        return identifiers.join(", ");
      }();
      return;
    }

    auto group = matchOption.value();
    qInfo() << "Found matching display group:" << group->getName();

    // Set the active group to the matched group
    m_activeGroup = group;

    // Reset the batch system and prepare for new configuration
    auto& batchSystem = bd::ConfigurationBatchSystem::instance();
    batchSystem.reset();

    // Connect to batch system completion signals
    connect(&batchSystem, &ConfigurationBatchSystem::configurationApplied, this, [this, &batchSystem](bool success) {
      if (success) {
        qDebug() << "Display configuration applied successfully via batch system";
        emit applied();
      } else {
        qWarning() << "Display configuration failed via batch system";
        emit failed();
      }
      // Disconnect to avoid duplicate signals on subsequent uses
      disconnect(&batchSystem, &ConfigurationBatchSystem::configurationApplied, this, nullptr);
    }, Qt::SingleShotConnection);

    // Create actions for each output in the group
    for (const auto& serial : group->getOutputIdentifiers()) {
      auto config_option = group->getConfigForIdentifier(serial);
      if (!config_option.has_value()) {
        qWarning() << "No configuration found for output:" << serial;
        continue;
      }

      const auto& config = config_option.value();
      qDebug() << "Creating batch actions for output:" << serial;

      if (config->getDisabled()) {
        // Create action to disable this output
        auto offAction = ConfigurationAction::explicitOff(serial);
        batchSystem.addAction(offAction);
        qDebug() << "  - Disable output";
      } else {
        // Create action to enable this output
        auto onAction = ConfigurationAction::explicitOn(serial);
        batchSystem.addAction(onAction);

        // Set mode (dimensions and refresh)
        auto modeAction = ConfigurationAction::mode(serial, 
          QSize(config->getWidth(), config->getHeight()), 
          config->getRefresh());
        batchSystem.addAction(modeAction);

        // Set anchoring if specified; also update the meta head so defaults propagate
        auto relativeOutput = config->getRelativeOutput();
        if (!relativeOutput.isEmpty()) {
          auto horizontalAnchor = config->getHorizontalAnchor();
          auto verticalAnchor = config->getVerticalAnchor();
          auto anchorAction = ConfigurationAction::setPositionAnchor(serial, relativeOutput, 
            horizontalAnchor, verticalAnchor);
          batchSystem.addAction(anchorAction);
          qDebug() << "  - Set anchoring relative to:" << relativeOutput;
          // Update meta head anchoring
          auto head = manager->getOutputHead(serial);
          if (!head.isNull()) {
            head->setRelativeOutput(relativeOutput);
            head->setHorizontalAnchoring(horizontalAnchor);
            head->setVerticalAnchoring(verticalAnchor);
          }
        } else {
          // Clear meta head anchoring explicitly
          auto head = manager->getOutputHead(serial);
          if (!head.isNull()) {
            head->setRelativeOutput("");
            head->setHorizontalAnchoring(ConfigurationHorizontalAnchor::NoHorizontalAnchor);
            head->setVerticalAnchoring(ConfigurationVerticalAnchor::NoVerticalAnchor);
          }
          qDebug() << "  - No anchoring set";
        }

        // Set scale
        auto scaleAction = ConfigurationAction::scale(serial, config->getScale());
        batchSystem.addAction(scaleAction);

        // Set transform (rotation)
        auto transformAction = ConfigurationAction::transform(serial, 
          static_cast<qint16>(config->getRotation()));
        batchSystem.addAction(transformAction);

        // Set adaptive sync
        auto adaptiveSyncAction = ConfigurationAction::adaptiveSync(serial, 
          config->getAdaptiveSync() ? 1 : 0);
        batchSystem.addAction(adaptiveSyncAction);

        qDebug() << "  - Enable output with mode:" << config->getWidth() << "x" << config->getHeight() 
                 << "@" << config->getRefresh() << "Hz";
        qDebug() << "  - Scale:" << config->getScale();
        qDebug() << "  - Rotation:" << config->getRotation();
        qDebug() << "  - Adaptive Sync:" << config->getAdaptiveSync();
      }
    }

    // Set primary output if specified
    auto primaryOutput = group->getPrimaryOutput();
    if (!primaryOutput.isEmpty()) {
      qDebug() << "Primary output:" << primaryOutput;
      for (const auto& head : heads) {
        if (head.isNull()) continue;
        head->setPrimary(head->getIdentifier() == primaryOutput);
      }
    }

    // Calculate and apply the configuration
    qDebug() << "Calculating and applying display configuration via batch system";
    batchSystem.apply();
  }

  DisplayGroup* DisplayConfig::createDisplayGroupForState() {
    auto& orchestrator = WaylandOrchestrator::instance();
    auto  manager      = orchestrator.getManager();
    auto  heads        = manager->getHeads();

    QStringList names_of_active_outputs;
    std::transform(heads.begin(), heads.end(), std::back_inserter(names_of_active_outputs), [](auto head) {
      if (head->getIdentifier() == nullptr) return QString {};
      return head->getIdentifier();
    });

    if (names_of_active_outputs.isEmpty()) {
      qWarning() << "No active outputs found, cannot create display group for state.";
      return nullptr;
    }

    auto defaultDisplayGroupForState = new DisplayGroup();
    defaultDisplayGroupForState->setName(names_of_active_outputs.join(", ").append(" (Auto Generated)"));
    defaultDisplayGroupForState->setOutputIdentifiers(names_of_active_outputs);
    defaultDisplayGroupForState->setPreferred(true);
    defaultDisplayGroupForState->setPrimaryOutput(names_of_active_outputs.first());

    for (const auto& head : heads) {
      if (head->getIdentifier() == nullptr) continue;
      auto head_mode_ptr = head->getCurrentMode();

      if (!head_mode_ptr) {
        qWarning() << "Head " << head->getIdentifier() << " has no current mode, skipping.";
        continue;
      }

      auto head_mode = head_mode_ptr.get();

      auto mode_size_opt    = head_mode->getSize();
      auto mode_refresh_opt = head_mode->getRefresh();
      if (!mode_size_opt.has_value() || !mode_refresh_opt.has_value()) {
        qWarning() << "Head " << head->getIdentifier() << " has no size or refresh value set, skipping.";
        continue;
      }
      if (!mode_size_opt.value().isValid() || !mode_refresh_opt.has_value()) continue;

      auto mode_size    = mode_size_opt.value();
      auto mode_refresh = mode_refresh_opt.value();
      auto head_pos     = head->getPosition();

      auto config = new DisplayGroupOutputConfig();
      config->setIdentifier(head->getIdentifier());
      config->setWidth(mode_size.width());
      config->setHeight(mode_size.height());
      config->setRefresh(mode_refresh);
      
      // Use meta-head anchoring if present so config can persist it
      config->setRelativeOutput(head->getRelativeOutput());
      config->setHorizontalAnchor(head->getHorizontalAnchor());
      config->setVerticalAnchor(head->getVerticalAnchor());
      
      config->setScale(head->getScale());
      config->setRotation(head->getTransform());
      config->setAdaptiveSync(head->getAdaptiveSync() != 0);
      config->setDisabled(!head->isEnabled());
      defaultDisplayGroupForState->addConfig(config);
      // config->deleteLater();
    }

    return defaultDisplayGroupForState;
  }

  void DisplayConfig::debugOutput() {
    for (const auto& group : this->m_groups) {
      qDebug() << "Group: " << group->getName();
      qDebug() << "Primary Output: " << group->getPrimaryOutput();
      qDebug() << "Output Serials: ";

      for (auto config : group->getConfigs()) {
        qDebug() << "  Serial: " << config->getIdentifier();
        qDebug() << "    Width: " << config->getWidth();
        qDebug() << "    Height: " << config->getHeight();
        qDebug() << "    Refresh: " << config->getRefresh();
        qDebug() << "    Relative Output: " << config->getRelativeOutput();
        qDebug() << "    Horizontal Anchor: " << static_cast<int>(config->getHorizontalAnchor());
        qDebug() << "    Vertical Anchor: " << static_cast<int>(config->getVerticalAnchor());
        qDebug() << "    Scale: " << config->getScale();
        qDebug() << "    Rotation: " << config->getRotation();
        qDebug() << "    Adaptive Sync: " << config->getAdaptiveSync();
        qDebug() << "    Disabled: " << config->getDisabled();
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
    auto                         manager        = WaylandOrchestrator::instance().getManager();
    auto                         heads          = manager->getHeads();
    auto                         heads_size     = heads.size();
    std::optional<DisplayGroup*> matching_group = std::nullopt;

    auto matching_groups = std::vector<DisplayGroup*> {};
    for (auto group : this->m_groups) {
      if (group->getOutputIdentifiers().size() != heads_size) { continue; }

      bool match = true;
      for (const auto& qIdentifier : group->getOutputIdentifiers()) {
        bool found = false;
        for (const auto& head : heads) {
          if (head->getIdentifier() == nullptr) continue;
          if (head->getIdentifier() == qIdentifier) {
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
    auto config_location = ConfigUtils::getConfigPath("display-config-v2.toml");

    try {
      qDebug() << "Reading display config from " << QString {config_location.c_str()};
      ConfigUtils::ensureConfigPathExists(config_location);

      auto data = toml::parse(config_location);

      if (data.contains("preferences")) {
        auto position = data.at("preferences").at("automatic_attach_outputs_relative_position");
        if (position.is_string()) {
          auto pos                                                       = std::string_view {position.as_string()};
          this->m_preferences.automatic_attach_outputs_relative_position = DisplayConfigurationUtils::getDisplayRelativePositionFromString(pos);
        }
      }

      for (const auto& group : toml::find<std::vector<toml::value>>(data, "group")) { this->m_groups.append(new DisplayGroup(group)); }
    } catch (const std::exception& e) {
      if (QString(e.what()).contains("error opening file")) return;
      qWarning() << "Error parsing display-config-v2.toml: " << e.what();
    }
  }

  void DisplayConfig::saveState() {
    toml::ordered_value config(toml::ordered_table {});
    config.as_table_fmt().fmt = toml::table_format::multiline;

    toml::ordered_value preferences_table(toml::ordered_table {});
    preferences_table["automatic_attach_outputs_relative_position"] =
        DisplayConfigurationUtils::getDisplayRelativePositionString(this->m_preferences.automatic_attach_outputs_relative_position);

    // Create our toml table for each group
    toml::ordered_value groups(toml::ordered_array {});
    groups.as_array_fmt().fmt = toml::array_format::array_of_tables;

    for (const auto& group : this->m_groups) { groups.push_back(group->toToml()); }

    config["preferences"] = preferences_table;
    config.as_table().emplace_back("group", groups);

    auto serialized_config = toml::format(config);
    auto config_location   = ConfigUtils::getConfigPath("display-config-v2.toml");
    auto config_file       = QFile(config_location);

    if (config_file.open(QIODevice::WriteOnly | QIODevice::Text)) {
      QTextStream stream(&config_file);
      stream << serialized_config.c_str();
      config_file.close();
    } else {
      qWarning() << "Failed to open display-config-v2.toml for writing";
    }
  }

  // DisplayGroup
  DisplayGroup::DisplayGroup(QObject* parent)
      : QObject(parent), m_name(""), m_preferred(false), m_output_identifiers({}), m_primary_output(""), m_configs({}) {}

  DisplayGroup::DisplayGroup(const toml::value& v, QObject* parent) : QObject(parent) {
    QStringList output_identifiers;
    for (const auto& serial : toml::find_or<std::vector<std::string>>(v, "identifiers", {})) { output_identifiers.append(QString::fromStdString(serial)); }

    m_name               = QString::fromStdString(toml::find<std::string>(v, "name"));
    m_output_identifiers = output_identifiers;
    m_primary_output     = QString::fromStdString(toml::find<std::string>(v, "primary_output"));
    m_preferred          = toml::find_or<bool>(v, "preferred", false);

    auto outputs = toml::find_or<std::vector<toml::value>>(v, "output", {});
    if (outputs.empty()) return;

    for (const toml::value& output : outputs) {
      auto dgo = new DisplayGroupOutputConfig();
      dgo->setIdentifier(QString::fromStdString(toml::find<std::string>(output, "identifier")));
      dgo->setWidth(toml::find<int>(output, "width"));
      dgo->setHeight(toml::find<int>(output, "height"));
      dgo->setRefresh(toml::find<double>(output, "refresh"));
      
      // Parse anchoring information (new format)
      auto relativeOutput = QString::fromStdString(toml::find_or<std::string>(output, "relative_output", ""));
      if (!relativeOutput.isEmpty()) dgo->setRelativeOutput(relativeOutput);
      
      dgo->setHorizontalAnchor(DisplayConfigurationUtils::getHorizontalAnchorFromString(toml::find_or<std::string>(output, "horizontal_anchor", "none")));
      dgo->setVerticalAnchor(DisplayConfigurationUtils::getVerticalAnchorFromString(toml::find_or<std::string>(output, "vertical_anchor", "none")));
      
      dgo->setScale(toml::find_or<double>(output, "scale", 1.0));
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

  QStringList DisplayGroup::getOutputIdentifiers() const {
    return this->m_output_identifiers;
  }

  QString DisplayGroup::getPrimaryOutput() const {
    return this->m_primary_output;
  }

  QList<DisplayGroupOutputConfig*> DisplayGroup::getConfigs() {
    return this->m_configs;
  }

  std::optional<DisplayGroupOutputConfig*> DisplayGroup::getConfigForIdentifier(QString identifier) {
    std::optional<DisplayGroupOutputConfig*> config = std::nullopt;
    for (auto& output : this->m_configs) {
      if (output->getIdentifier() == identifier) {
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

  void DisplayGroup::setOutputIdentifiers(const QStringList& identifiers) {
    this->m_output_identifiers = identifiers;
  }

  void DisplayGroup::setPreferred(bool preferred) {
    this->m_preferred = preferred;
  }

  void DisplayGroup::setPrimaryOutput(const QString& identifier) {
    this->m_primary_output = identifier;
  }

  toml::ordered_value DisplayGroup::toToml() {
    std::vector<std::string> output_identifiers;
    for (const auto& identifier : m_output_identifiers) { output_identifiers.push_back(identifier.toStdString()); }

    toml::ordered_value group_table(toml::ordered_table {});
    group_table.as_table_fmt().fmt = toml::table_format::multiline;

    group_table["name"]           = this->m_name.toStdString();
    group_table["preferred"]      = this->m_preferred;
    group_table["identifiers"]    = output_identifiers;
    group_table["primary_output"] = this->m_primary_output.toStdString();

    toml::ordered_value outputs(toml::ordered_array {});
    outputs.as_array_fmt().fmt = toml::array_format::array_of_tables;
    for (auto config : this->m_configs) { outputs.push_back(config->toToml()); }

    group_table.as_table().emplace_back("output", outputs);

    return group_table;
  }

  // DisplayGroupOutputConfig

  DisplayGroupOutputConfig::DisplayGroupOutputConfig(QObject* parent) : QObject(parent),
    m_width(0), m_height(0), m_refresh(0.0), m_relative_output(""),
    m_horizontal_anchor(ConfigurationHorizontalAnchor::NoHorizontalAnchor),
    m_vertical_anchor(ConfigurationVerticalAnchor::NoVerticalAnchor),
    m_scale(1.0), m_rotation(0), m_adaptive_sync(false), m_disabled(false) {}

  bool DisplayGroupOutputConfig::getAdaptiveSync() const {
    return this->m_adaptive_sync;
  }

  bool DisplayGroupOutputConfig::getDisabled() const {
    return this->m_disabled;
  }

  int DisplayGroupOutputConfig::getHeight() const {
    return this->m_height;
  }

  QString DisplayGroupOutputConfig::getIdentifier() const {
    return this->m_identifier;
  }

  QString DisplayGroupOutputConfig::getRelativeOutput() const {
    return this->m_relative_output;
  }

  ConfigurationHorizontalAnchor DisplayGroupOutputConfig::getHorizontalAnchor() const {
    return this->m_horizontal_anchor;
  }

  ConfigurationVerticalAnchor DisplayGroupOutputConfig::getVerticalAnchor() const {
    return this->m_vertical_anchor;
  }

  double DisplayGroupOutputConfig::getRefresh() const {
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

  void DisplayGroupOutputConfig::setIdentifier(const QString& identifier) {
    this->m_identifier = identifier;
  }

  void DisplayGroupOutputConfig::setRelativeOutput(const QString& relativeOutput) {
    this->m_relative_output = relativeOutput;
  }

  void DisplayGroupOutputConfig::setHorizontalAnchor(ConfigurationHorizontalAnchor horizontalAnchor) {
    this->m_horizontal_anchor = horizontalAnchor;
  }

  void DisplayGroupOutputConfig::setVerticalAnchor(ConfigurationVerticalAnchor verticalAnchor) {
    this->m_vertical_anchor = verticalAnchor;
  }

  void DisplayGroupOutputConfig::setRefresh(double refresh) {
    this->m_refresh = refresh;
  }

  void DisplayGroupOutputConfig::setRotation(int rotation) {
    this->m_rotation = rotation;
  }

  void DisplayGroupOutputConfig::setScale(double scale) {
    this->m_scale = scale;
  }

  void DisplayGroupOutputConfig::setWidth(int width) {
    this->m_width = width;
  }

  toml::ordered_value DisplayGroupOutputConfig::toToml() {
    toml::ordered_value config_table(toml::ordered_table {});
    config_table.as_table_fmt().fmt = toml::table_format::multiline;

    config_table["identifier"]    = this->m_identifier.toStdString();
    config_table["width"]         = this->m_width;
    config_table["height"]        = this->m_height;
    config_table["refresh"]       = this->m_refresh;
    
    // Serialize anchoring information (new format)
    if (!this->m_relative_output.isEmpty()) {
      config_table["relative_output"] = this->m_relative_output.toStdString();
    }
    config_table["horizontal_anchor"] = DisplayConfigurationUtils::getHorizontalAnchorString(this->m_horizontal_anchor);
    config_table["vertical_anchor"] = DisplayConfigurationUtils::getVerticalAnchorString(this->m_vertical_anchor);
    
    config_table["scale"]         = this->m_scale;
    config_table["rotation"]      = this->m_rotation;
    config_table["adaptive_sync"] = this->m_adaptive_sync;
    config_table["disabled"]      = this->m_disabled;

    return config_table;
  }
}
