#include "DisplayService.hpp"

#include "display.hpp"
#include "displays/output-manager/WaylandOutputManager.hpp"

namespace bd {
  DisplayService::DisplayService(QObject* parent) : QObject(parent) {
    m_adaptor = new DisplaysAdaptor(this);
  }

  OutputModesList DisplayService::GetAvailableModes(const QString& identifier) {
    auto output = WaylandOrchestrator::instance().getManager()->getOutputHead(identifier);
    if (!output) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      return {};
    }

    auto modes = OutputModesList {};
    for (const auto& mode : output.get()->getModes()) {
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
    if (!outputOption) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      // Create a QDBusMessage for the error, but note this doesn't send it.
      // The method will return an empty list, signaling success to D-Bus.
      QDBusMessage::createError(QDBusError::InternalError, "Output " + identifier + " does not exist");
      return {};
    }

    auto output_head = outputOption.get();

    auto size         = QSize(0, 0);
    auto refresh      = 0.0;
    auto is_preferred = false;

    auto current_mode_opt = output_head->getCurrentMode();

    if (current_mode_opt) {  // If we have a mode
      auto current_mode = current_mode_opt.get();
      size              = current_mode->getSize().value_or(QSize(0, 0));
      refresh           = current_mode->getRefresh().value_or(0.0);

      // TODO: Fix later so we have it as display being preferred not mode
      is_preferred = current_mode->isPreferred().value_or(false);
    }

    auto list = QVariantList {};
    list.append(output_head->getName());
    list.append(output_head->getDescription());

    auto pos = output_head->getPosition();

    // These are all "out" parameters
    list.append(size.width());
    list.append(size.height());
    list.append(pos.x());
    list.append(pos.y());
    list.append(output_head->getScale());
    list.append(refresh);
    list.append(output_head->isAvailable());
    list.append(is_preferred);
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

    auto configuration_head = output_config->enable(output.data());

    if (!configuration_head) {
      qWarning() << "Failed to enable output " << identifier << ", wlr_head is not available";
      QDBusMessage::createError(QDBusError::InternalError, "Failed to enable output " + identifier + ", wlr_head is not available");
      return;
    }

    auto refreshAsDouble = refresh / 1000.0;
    auto mode            = output->getModeForOutputHead(width, height, refreshAsDouble);

    if (mode) {
      configuration_head->setMode(mode.data());
    } else {
      configuration_head->setCustomMode(width, height, refreshAsDouble);
    }

    auto heads = manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config.get(), QStringList {identifier});

    output_config->applySelf();

    auto display = WaylandOrchestrator::instance().getDisplay();
    if (display) wl_display_dispatch(display);

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

    if (!outputOption) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      return;
    }

    auto output_config_ptr = manager->configure();

    if (!output_config_ptr) {
      qWarning() << "Failed to create output configuration for " << identifier;
      return;
    }
    auto output_config = output_config_ptr.get();
    auto output        = outputOption.get();

    if (enabled) {
        qDebug() << "Enabling output " << identifier;
      auto config_head_ptr = output_config->enable(output);
      if (!config_head_ptr.isNull()) {
          qDebug() << "Have output configuration head ptr for " << identifier;
        auto config_head = config_head_ptr.get();

        // If we have a current mode, set it as preferred
        auto current_mode_ptr = output->getCurrentMode();
//        if (!current_mode_ptr) current_mode_ptr = output->getModes().last();

        if (current_mode_ptr.isNull()) {
          qWarning() << "No current mode found for output " << identifier;
          QDBusMessage::createError(QDBusError::InternalError, "No current mode found for output " + identifier);
          return;
        }

        auto current_mode = current_mode_ptr.data();

        auto size    = current_mode->getSize().value_or(QSize(0, 0));
        auto refresh = current_mode->getRefresh().value_or(0.0);

        config_head->setCustomMode(size.width(), size.height(), refresh);

        // Set position and scale
        auto position = output->getPosition();
        config_head->setPosition(position.x(), position.y());
        config_head->setScale(output->getScale());
        config_head->setTransform(output->getTransform());
        config_head->setAdaptiveSync(output->getAdaptiveSync() ? 1 : 0);
        qDebug() << "Enabling output" << identifier << "with mode" << size.width() << "x" << size.height() << "@" << refresh
                 << ", position:" << position.x() << ", " << position.y() << ", scale:" << output->getScale()
                 << ", transform:" << output->getTransform() << ", adaptive sync:" << output->getAdaptiveSync();
      } else {
        qWarning() << "Failed to enable output " << identifier << ", wlr_head is not available";
        QDBusMessage::createError(QDBusError::InternalError, "Failed to enable output " + identifier + ", wlr_head is not available");
        return;
      }
    } else {
      //            output->unsetModes(); // Unset all the references to WaylandOutputMode so we don't get wayland client segfaults when re-enabling the output
      output_config->disable(output);
    }

    auto heads = manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {identifier});

    try {
      output_config->applySelf();
    } catch (const std::exception& e) { qWarning() << "Failed to apply configuration: " << e.what(); }

    auto display_ptr = WaylandOrchestrator::instance().getDisplay();

    if (display_ptr) {
      try {
        wl_display_dispatch(display_ptr);
      } catch (const std::exception& e) { qWarning() << "Failed to dispatch display changes: " << e.what(); }
    } else {
      qWarning() << "Wayland display is not initialized.";
    }

    auto& displayConfig       = DisplayConfig::instance();
    auto  configForIdentifier = displayConfig.getActiveGroup()->getConfigForIdentifier(identifier);

    if (configForIdentifier.has_value()) {
//      configForIdentifier.value()->setDisabled(!enabled);
      //            displayConfig.saveState();
    }

    heads.clear();
    output_config->release();
  }

  void DisplayService::SetOutputPosition(const QString& identifier, int x, int y) {
    auto manager      = WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(identifier);

    if (!outputOption) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      return;
    }

    auto output          = outputOption.get();
    auto output_position = output->getPosition();

    qDebug() << "Found output " << identifier << "with name" << output->getName() << " at " << output_position.x() << ", " << output_position.y();

    if (!output->isEnabled()) {
      qWarning() << "Received request for output " << identifier << " which is not enabled";
      return;
    }

    auto output_config_ptr = manager->configure();

    if (!output_config_ptr) {
      qWarning() << "Failed to create output configuration for " << identifier;
      return;
    }

    auto output_config = output_config_ptr.get();

    auto configuration_head_opt = output_config->enable(output);

    if (!configuration_head_opt) {
      qWarning() << "Failed to enable output " << identifier << ", wlr_head is not available";
      QDBusMessage::createError(QDBusError::InternalError, "Failed to enable output " + identifier + ", wlr_head is not available");
      return;
    }

    auto configuration_head = configuration_head_opt.get();
    configuration_head->setPosition(x, y);

    // It is a protocol error to not specify everything else
    auto heads = manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {identifier});

    output_config->applySelf();

    auto display_ptr = WaylandOrchestrator::instance().getDisplay();

    if (display_ptr == nullptr) {
      qWarning() << "Failed to get wl_display during configuration of " << identifier;
      return;
    }

    wl_display_dispatch(display_ptr);

    auto& displayConfig       = DisplayConfig::instance();
    auto  configForIdentifier = displayConfig.getActiveGroup()->getConfigForIdentifier(identifier);

    if (configForIdentifier.has_value()) {
      configForIdentifier.value()->setPosition({x, y});
      displayConfig.saveState();
    }

    heads.clear();

    output_config->release();
  }
}
