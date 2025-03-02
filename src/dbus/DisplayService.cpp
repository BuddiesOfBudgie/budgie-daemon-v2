#include "DisplayService.hpp"

#include "../displays/output-manager/WaylandOutputManager.hpp"
#include "display.hpp"

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
      auto modeInfo = QVector<QVariant> {mode->getWidth(), mode->getHeight(), mode->getRefresh()};
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
    auto output = WaylandOrchestrator::instance().getManager()->getOutputHead(identifier);
    if (!output.has_value()) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      QDBusMessage::createError(QDBusError::InvalidArgs, "Output does not exist");
    }

    auto output_head = output.value();
    auto mode        = output_head->getCurrentMode();

    auto list = QVariantList {};
    list.append(output_head->getName());
    list.append(output_head->getDescription());

    // These are all "out" parameters
    list.append(mode->getWidth());
    list.append(mode->getHeight());
    list.append(output_head->getX());
    list.append(output_head->getY());
    list.append(output_head->getScale());
    list.append(mode->getRefresh());
    list.append(mode->isPreferred());
    list.append(output_head->isEnabled());

    return list;
  }

  void DisplayService::SetCurrentMode(const QString &identifier, int width, int height, int refresh, bool preferred) {
    auto manager      = WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(identifier);

    if (!outputOption.has_value()) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      return;
    }

    auto output_config = manager->configure();
    auto output        = outputOption.value();

    if (!output->isEnabled()) {
      qWarning() << "Received request for output " << identifier << " which is not enabled";
      return;
    }

    auto configuration_head = output_config->enable(output);

    // TODO: Josh - add back
    // zwlr_output_configuration_v1_add_listener(wlr_output_config, &config_listener, NULL);

    auto refreshAsDouble =  refresh / 1000.0;
    auto modeOption = output->getModeForOutputHead(width, height, refreshAsDouble);

    if (modeOption.has_value()) {
      configuration_head->setMode(modeOption.value());
    } else {
      configuration_head->setCustomMode(width, height, refreshAsDouble);
    }

    auto heads = manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {identifier});

    output_config->applySelf();

    wl_display_dispatch(WaylandOrchestrator::instance().getDisplay());
    wl_display_dispatch_pending(WaylandOrchestrator::instance().getDisplay());

    auto& displayConfig       = DisplayConfig::instance();
    auto  configForIdentifier = displayConfig.getActiveGroup()->getConfigForIdentifier(identifier);

    if (configForIdentifier.has_value()) {
      configForIdentifier.value()->setWidth(width);
      configForIdentifier.value()->setHeight(height);
      configForIdentifier.value()->setRefresh(refreshAsDouble);
      displayConfig.saveState();
    }

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

    // TODO: Josh - Fix
    // zwlr_output_configuration_v1_add_listener(wlr_output_config, &config_listener, nullptr);

    if (enabled) {
      output_config->enable(output);
    } else {
      output_config->disable(output);
    }

    auto heads = manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {identifier});

    output_config->applySelf();
    output_config->release();

    auto& displayConfig       = DisplayConfig::instance();
    auto  configForIdentifier = displayConfig.getActiveGroup()->getConfigForIdentifier(identifier);

    if (configForIdentifier.has_value()) {
      configForIdentifier.value()->setDisabled(!enabled);
      displayConfig.saveState();
    }

    for (auto cleanup_head : heads) {
      delete cleanup_head;
    }

    wl_display_dispatch(WaylandOrchestrator::instance().getDisplay());
  }

  void DisplayService::SetOutputPosition(const QString& identifier, int x, int y) {
    auto manager      = WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(identifier);

    if (!outputOption.has_value()) {
      qWarning() << "Received request for output " << identifier << " which does not exist";
      return;
    }

    auto output = outputOption.value();

    qDebug() << "Found output " << identifier << "with name" << output->getName() << " at " << output->getX() << ", " << output->getY();

    if (!output->isEnabled()) {
      qWarning() << "Received request for output " << identifier << " which is not enabled";
      return;
    }

    auto output_config = manager->configure();

    // TODO: Josh - Fix
    auto configuration_head = output_config->enable(output);
    configuration_head->setPosition(x, y);

    // It is a protocol error to not specify everything else
    manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {identifier});

    output_config->applySelf();
    output_config->release();

    wl_display_dispatch(WaylandOrchestrator::instance().getDisplay());

    auto& displayConfig       = DisplayConfig::instance();
    auto  configForIdentifier = displayConfig.getActiveGroup()->getConfigForIdentifier(identifier);

    if (configForIdentifier.has_value()) {
      configForIdentifier.value()->setPosition({x, y});
      displayConfig.saveState();
    }
  }
}
