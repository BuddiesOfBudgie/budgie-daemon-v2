#include "DisplayService.hpp"

#include "../displays/output-manager/WaylandOutputManager.hpp"
#include "display.hpp"

namespace bd {
  DisplayService::DisplayService(QObject* parent) : QObject(parent) {
    m_adaptor = new DisplaysAdaptor(this);
  }

  QVariantList DisplayService::GetAvailableModes(const QString& serial) {
    auto output = WaylandOrchestrator::instance().getManager()->getOutputHead(serial);
    if (!output.has_value()) {
      qWarning() << "Received request for output " << serial << " which does not exist";
      return {};
    }

    auto modes = QVariantList {};
    for (const auto& mode : output.value()->getModes()) { modes.append({mode->getWidth(), mode->getHeight(), mode->getRefresh()}); }

    return modes;
  }

  QStringList DisplayService::GetAvailableOutputs() {
    auto outputs = QStringList {};
    for (const auto& output : WaylandOrchestrator::instance().getManager()->getHeads()) { outputs.append(output->getSerial()); }

    return outputs;
  }

  DisplaysAdaptor* DisplayService::GetAdaptor() {
    return m_adaptor;
  }

  QVariantList DisplayService::GetCurrentOutputDetails(const QString& serial) {
    auto output = WaylandOrchestrator::instance().getManager()->getOutputHead(serial);
    if (!output.has_value()) {
      qWarning() << "Received request for output " << serial << " which does not exist";
      QDBusMessage::createError(QDBusError::InvalidArgs, "Output does not exist");
    }

    auto output_head = output.value();
    auto mode        = output_head->getCurrentMode();

    auto list = QVariantList {};
    list.append(output_head->getName());

    // These are all "out" parameters
    list.append(mode->getWidth());
    list.append(mode->getHeight());
    list.append(output_head->getX());
    list.append(output_head->getY());
    list.append(static_cast<double>(output_head->getScale()));
    list.append(mode->getRefresh());
    list.append(mode->isPreferred());
    list.append(output_head->isEnabled());

    return list;
  }

  void DisplayService::SetCurrentMode(const QString& serial, int width, int height, double refresh, bool preferred) {
    auto manager      = bd::WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(serial);

    if (!outputOption.has_value()) {
      qWarning() << "Received request for output " << serial << " which does not exist";
      return;
    }

    auto output_config = manager->configure();
    auto output        = outputOption.value();

    if (!output->isEnabled()) {
      qWarning() << "Received request for output " << serial << " which is not enabled";
      return;
    }

    auto configuration_head = output_config->enable(output);

    // TODO: Josh - add back
    // zwlr_output_configuration_v1_add_listener(wlr_output_config, &config_listener, NULL);

    auto modeOption = output->getModeForOutputHead(width, height, refresh);

    if (modeOption.has_value()) {
      configuration_head->setMode(modeOption.value());
    } else {
      configuration_head->setCustomMode(width, height, refresh);
    }

    manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {serial});

    output_config->applySelf();
    output_config->release();

    wl_display_dispatch(bd::WaylandOrchestrator::instance().getDisplay());

    auto& displayConfig   = bd::DisplayConfig::instance();
    auto  configForSerial = displayConfig.getActiveGroup()->getConfigForSerial(serial);

    if (configForSerial.has_value()) {
      configForSerial.value()->setWidth(width);
      configForSerial.value()->setHeight(height);
      configForSerial.value()->setRefresh(refresh);
      displayConfig.saveState();
    }
  }

  void DisplayService::SetOutputEnabled(const QString& serial, bool enabled) {
    auto manager      = bd::WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(serial);

    if (!outputOption.has_value()) {
      qWarning() << "Received request for output " << serial << " which does not exist";
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

    manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {serial});

    output_config->applySelf();
    output_config->release();

    auto& displayConfig   = bd::DisplayConfig::instance();
    auto  configForSerial = displayConfig.getActiveGroup()->getConfigForSerial(serial);

    if (configForSerial.has_value()) {
      configForSerial.value()->setDisabled(!enabled);
      displayConfig.saveState();
    }

    wl_display_dispatch(bd::WaylandOrchestrator::instance().getDisplay());
  }

  void DisplayService::SetOutputPosition(const QString& serial, int x, int y) {
    auto manager      = bd::WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(serial);

    if (!outputOption.has_value()) {
      qWarning() << "Received request for output " << serial << " which does not exist";
      return;
    }

    auto output = outputOption.value();

    qInfo() << "Found output " << serial;

    if (!output->isEnabled()) {
      qWarning() << "Received request for output " << serial << " which is not enabled";
      return;
    }

    auto output_config = manager->configure();

    // TODO: Josh - Fix
    auto configuration_head = output_config->enable(output);
    configuration_head->setPosition(x, y);

    // It is a protocol error to not specify everything else
    manager->applyNoOpConfigurationForNonSpecifiedHeads(output_config, QStringList {serial});

    output_config->applySelf();
    output_config->release();

    wl_display_dispatch(bd::WaylandOrchestrator::instance().getDisplay());

    auto& displayConfig   = bd::DisplayConfig::instance();
    auto  configForSerial = displayConfig.getActiveGroup()->getConfigForSerial(serial);

    if (configForSerial.has_value()) {
      configForSerial.value()->setPosition({x, y});
      displayConfig.saveState();
    }
  }
}
