#include "DisplayService.hpp"

#include "display.hpp"
#include "displays/output-manager/WaylandOutputManager.hpp"

namespace bd {
  DisplayService::DisplayService(QObject* parent) : QObject(parent) {
    m_adaptor = new DisplaysAdaptor(this);
  }

  OutputModesList DisplayService::GetAvailableModes(const QString& identifier) {
    auto output = WaylandOrchestrator::instance().getManager()->getOutputHead(identifier);
    if (!output.has_value()) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      return {};
    }

    auto modes = OutputModesList {};
    for (const auto& mode : output.value()->getModes()) {
      auto modeSizeOpt    = mode->getSize();
      auto modeRefreshOpt = mode->getRefresh();
      if (!modeSizeOpt.has_value() || !modeRefreshOpt.has_value()) {
        qWarning() << "Mode for output " << identifier << " is invalid, skipping";
        continue;
      }
      auto modeSize = modeSizeOpt.value();

      if (!modeSize.isValid()) {
        qWarning() << "Mode size for output " << identifier << " is invalid, skipping";
        continue;
      }

      auto modeRefresh = modeRefreshOpt.value();
      auto modeInfo    = QVector<QVariant> {modeSize.width(), modeSize.height(), modeRefresh};
      modes.append(modeInfo);
    }

    return modes;
  }

  QStringList DisplayService::GetAvailableOutputs() {
    auto outputs = QStringList {};
    for (const auto& output : WaylandOrchestrator::instance().getManager()->getHeads()) { outputs.append(output->getIdentifier()); }

    return outputs;
  }

  DisplaysAdaptor* DisplayService::GetAdaptor() {
    return m_adaptor;
  }

  OutputDetailsList DisplayService::GetOutputDetails(const QString& identifier) {
    auto outputOption = WaylandOrchestrator::instance().getManager()->getOutputHead(identifier);
    if (!outputOption.has_value()) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      // Create a QDBusMessage for the error, but note this doesn't send it.
      // The method will return an empty list, signaling success to D-Bus.
      QDBusMessage::createError(QDBusError::InternalError, "Output " + identifier + " does not exist");
      return {};
    }

    auto output_head = outputOption.value();
    // Assuming getCurrentMode() returns a pointer to the mode, which can be nullptr
    auto current_mode_opt = output_head->getCurrentMode();
    if (!current_mode_opt.has_value()) {
      qWarning() << "Output " << identifier << " has no current mode set.";
      // Create a QDBusMessage for the error, but note this doesn't send it.
      QDBusMessage::createError(QDBusError::InternalError, "Output " + identifier + " has no current mode");
      return {};
    }

    auto current_mode = current_mode_opt.value();

    auto list = QVariantList {};
    list.append(output_head->getName());
    list.append(output_head->getDescription());

    // Assuming current_mode->getSize() returns std::optional<QSize>
    // and current_mode->getRefresh() returns std::optional<double>
    auto size    = current_mode->getSize().value_or(QSize(0, 0));
    auto refresh = current_mode->getRefresh().value_or(0.0);
    auto pos     = output_head->getPosition();

    // These are all "out" parameters
    list.append(size.width());   // Corrected: QSize uses .width()
    list.append(size.height());  // Corrected: QSize uses .height()
    list.append(pos.x());
    list.append(pos.y());
    list.append(output_head->getScale());
    list.append(refresh);
    list.append(output_head->isAvailable());
    list.append(current_mode->isPreferred().value_or(false));  // Use current_mode
    list.append(output_head->isEnabled());

    return list;
  }

  void DisplayService::SetCurrentMode(const QString& identifier, int width, int height, int refresh, bool preferred) {
    auto manager = WaylandOrchestrator::instance().getManager();
    auto output  = manager->getOutputHead(identifier);

    if (!output) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      return;
    }

    auto output_config = manager->configure();

    if (!output->isEnabled()) {
      qWarning() << "Received request for output " << identifier << " which is not enabled";
      return;
    }

    auto configuration_head = output_config->enable(output.get());

    if (!configuration_head) {
      qWarning() << "Failed to enable output " << identifier << ", wlr_head is not available";
      QDBusMessage::createError(QDBusError::InternalError, "Failed to enable output " + identifier + ", wlr_head is not available");
      return;
    }

    auto refreshAsDouble = refresh / 1000.0;
    auto mode            = output->getModeForOutputHead(width, height, refreshAsDouble);

    if (mode) {
      configuration_head->setMode(mode.get());
    } else {
      configuration_head->setCustomMode(width, height, refreshAsDouble);
    }

    auto heads = manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config.get(), QStringList {identifier});

    output_config->applySelf();

    auto display = WaylandOrchestrator::instance().getDisplay();
    if (display) wl_display_dispatch(display.get());

    auto& displayConfig       = DisplayConfig::instance();
    auto  configForIdentifier = displayConfig.getActiveGroup()->getConfigForIdentifier(identifier);

    if (configForIdentifier.has_value()) {
      configForIdentifier.value()->setWidth(width);
      configForIdentifier.value()->setHeight(height);
      configForIdentifier.value()->setRefresh(refreshAsDouble);
      displayConfig.saveState();
    }

    heads.clear();
    output_config->release();
  }

  void DisplayService::SetOutputEnabled(const QString& identifier, bool enabled) {
    auto manager      = WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(identifier);

    if (!outputOption.has_value()) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      return;
    }

    auto output_config = manager->configure();
    auto output        = outputOption.value();

    if (enabled) {
      output_config->enable(output);
    } else {
      output_config->disable(output);
    }

    auto heads = manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {identifier});

    try {
      output_config->applySelf();
      output_config->release();
    } catch (const std::exception& e) { qWarning() << "Failed to apply configuration: " << e.what(); }

    if (WaylandOrchestrator::instance().getDisplay()) {
      try {
        wl_display_roundtrip(WaylandOrchestrator::instance().getDisplay());
      } catch (const std::exception& e) { qWarning() << "Failed to dispatch display changes: " << e.what(); }
    } else {
      qWarning() << "Wayland display is not initialized.";
    }

    auto& displayConfig       = DisplayConfig::instance();
    auto  configForIdentifier = displayConfig.getActiveGroup()->getConfigForIdentifier(identifier);

    if (configForIdentifier.has_value()) {
      configForIdentifier.value()->setDisabled(!enabled);
      displayConfig.saveState();
    }

    for (auto cleanup_head : heads) { delete cleanup_head; }
  }

  void DisplayService::SetOutputPosition(const QString& identifier, int x, int y) {
    auto manager      = WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(identifier);

    if (!outputOption.has_value()) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      return;
    }

    auto output          = outputOption.value();
    auto output_position = output->getPosition();

    qDebug() << "Found output " << identifier << "with name" << output->getName() << " at " << output_position.x() << ", " << output_position.y();

    if (!output->isEnabled()) {
      qWarning() << "Received request for output " << identifier << " which is not enabled";
      return;
    }

    auto output_config = manager->configure();

    // TODO: Josh - Fix
    auto configuration_head_opt = output_config->enable(output);

    if (!configuration_head_opt.has_value()) {
      qWarning() << "Failed to enable output " << identifier << ", wlr_head is not available";
      QDBusMessage::createError(QDBusError::InternalError, "Failed to enable output " + identifier + ", wlr_head is not available");
      return;
    }

    auto configuration_head = configuration_head_opt.value();
    configuration_head->setPosition(x, y);

    // It is a protocol error to not specify everything else
    auto heads = manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {identifier});

    output_config->applySelf();

    wl_display_dispatch(WaylandOrchestrator::instance().getDisplay());

    auto& displayConfig       = DisplayConfig::instance();
    auto  configForIdentifier = displayConfig.getActiveGroup()->getConfigForIdentifier(identifier);

    if (configForIdentifier.has_value()) {
      configForIdentifier.value()->setPosition({x, y});
      displayConfig.saveState();
    }

    for (auto cleanup_head : heads) { delete cleanup_head; }

    output_config->release();
  }
}
