#include "server.hpp"

#include <iostream>
#include <string>

#include "../displays/output-manager/WaylandOutputManager.hpp"

DaemonServer::DaemonServer(sdbus::IConnection& connection, sdbus::ObjectPath objectPath) : AdaptorInterfaces(connection, std::move(objectPath)) {
  registerAdaptor();
}

DaemonServer::~DaemonServer() {
  unregisterAdaptor();
}

void DaemonServer::GetAvailableOutputs(sdbus::Result<std::vector<std::string>>&& result) {
  std::thread([methodResult = std::move(result)]() {
    auto outputs = std::vector<std::string> {};
    for (const auto& output : bd::WaylandOrchestrator::instance().getManager()->getHeads()) { outputs.push_back(output->getSerial().toStdString()); }

    methodResult.returnResults(outputs);
  }).detach();
}

void DaemonServer::GetCurrentOutputDetails(
    sdbus::Result<std::string, int32_t, int32_t, int32_t, int32_t, double, double, bool, bool>&& result,
    std::string                                                                                  serial) {
  std::thread([methodResult = std::move(result), serial]() {
    auto outputOptional = bd::WaylandOrchestrator::instance().getManager()->getOutputHead(QString {serial.c_str()});

    if (!outputOptional.has_value()) {
      std::cerr << "Received request for output " << serial << " which does not exist" << std::endl;
      methodResult.returnError(sdbus::Error {"org.buddiesofbudgie.BudgieDaemonX.Error.OutputMissing"});
      return;
    }

    auto output = outputOptional.value();

    methodResult.returnResults(
        output->getName().toStdString(), output->getCurrentMode()->getWidth(), output->getCurrentMode()->getHeight(), output->getX(), output->getY(),
        1,  // TODO Josh: Implement scale factor
        output->getCurrentMode()->getRefresh(), output->getCurrentMode()->isPreferred(), output->isEnabled());
  }).detach();
}

void DaemonServer::GetAvailableModes(sdbus::Result<std::vector<sdbus::Struct<int32_t, int32_t, double>>>&& result, std::string serial) {
  std::thread([methodResult = std::move(result), serial]() {
    auto outputOptional = bd::WaylandOrchestrator::instance().getManager()->getOutputHead(QString {serial.c_str()});

    if (!outputOptional.has_value()) {
      std::cerr << "Received request for output " << serial << " which does not exist" << std::endl;
      methodResult.returnError(sdbus::Error {"org.buddiesofbudgie.BudgieDaemonX.Error.OutputMissing"});
      return;
    }

    auto output = outputOptional.value();

    auto modes = std::vector<sdbus::Struct<int32_t, int32_t, double>> {};
    for (const auto& mode : output->getModes()) { modes.push_back({mode->getWidth(), mode->getHeight(), mode->getRefresh()}); }

    methodResult.returnResults(modes);
  }).detach();
}

static void config_handle_succeeded(void* data, struct zwlr_output_configuration_v1* config) {
  std::cout << "Configuration successfully applied" << std::endl;
  zwlr_output_configuration_v1_destroy(config);
}

static void config_handle_failed(void* data, struct zwlr_output_configuration_v1* config) {
  std::cout << "Configuration failed to apply" << std::endl;
  zwlr_output_configuration_v1_destroy(config);
}

static void config_handle_cancelled(void* data, struct zwlr_output_configuration_v1* config) {
  std::cout << "Configuration was cancelled" << std::endl;
  zwlr_output_configuration_v1_destroy(config);
}

static const struct zwlr_output_configuration_v1_listener config_listener = {
    .succeeded = config_handle_succeeded,
    .failed    = config_handle_failed,
    .cancelled = config_handle_cancelled,
};

void DaemonServer::SetCurrentMode(sdbus::Result<>&& result, std::string serial, int32_t width, int32_t height, double refresh, bool preferred) {
  std::thread([methodResult = std::move(result), serial, width, height, refresh, preferred]() {
    auto qSerial = QString {serial.c_str()};

    auto manager      = bd::WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(qSerial);

    if (!outputOption.has_value()) {
      std::cerr << "Received request for output " << serial << " which does not exist" << std::endl;
      methodResult.returnError(sdbus::Error {"org.buddiesofbudgie.BudgieDaemonX.Error.OutputMissing"});
      return;
    }

    auto config = manager->configure();
    auto output = outputOption.value();

    if (!output->isEnabled()) {
      std::cerr << "Received request for output " << serial << " which is not enabled" << std::endl;
      methodResult.returnError(sdbus::Error {"org.buddiesofbudgie.BudgieDaemonX.Error.OutputDisabled"});
      return;
    }

    auto configuration_head = config->enable(output);

    // TODO: Josh - add back
    // zwlr_output_configuration_v1_add_listener(wlr_output_config, &config_listener, NULL);

    auto modeOption = output->getModeForOutputHead(width, height, refresh);

    if (modeOption.has_value()) {
      configuration_head->setMode(modeOption.value());
    } else {
      configuration_head->setCustomMode(width, height, refresh);
    }

    manager->applyNoOpConfigurationForNonSpecifiedHeads(config, QStringList {qSerial});

    config->applySelf();
    config->release();

    wl_display_dispatch(bd::WaylandOrchestrator::instance().getDisplay());
    methodResult.returnResults();
  }).detach();
}

void DaemonServer::SetOutputPosition(sdbus::Result<>&& result, std::string serial, int32_t x, int32_t y) {
  std::thread([methodResult = std::move(result), serial, x, y]() {
    auto qSerial = QString {serial.c_str()};

    auto manager      = bd::WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(qSerial);

    if (!outputOption.has_value()) {
      std::cerr << "Received request for output " << serial << " which does not exist" << std::endl;
      methodResult.returnError(sdbus::Error {"org.buddiesofbudgie.BudgieDaemonX.Error.OutputMissing"});
      return;
    }

    auto output = outputOption.value();

    std::cout << "Found output " << serial << std::endl;

    if (!output->isEnabled()) {
      std::cerr << "Received request for output " << serial << " which is not enabled" << std::endl;
      methodResult.returnError(sdbus::Error {"org.buddiesofbudgie.BudgieDaemonX.Error.OutputDisabled"});
      return;
    }

    auto config = manager->configure();

    // TODO: Josh - Fix
    auto configuration_head = config->enable(output);
    configuration_head->setPosition(x, y);

    // It is a protocol error to not specify everything else
    manager->applyNoOpConfigurationForNonSpecifiedHeads(config, QStringList {qSerial});

    config->applySelf();
    config->release();

    wl_display_dispatch(bd::WaylandOrchestrator::instance().getDisplay());
    methodResult.returnResults();
  }).detach();
}

void DaemonServer::SetOutputEnabled(sdbus::Result<>&& result, std::string serial, bool enabled) {
  std::thread([methodResult = std::move(result), serial, enabled]() {
    auto qSerial = QString {serial.c_str()};

    auto manager      = bd::WaylandOrchestrator::instance().getManager();
    auto outputOption = manager->getOutputHead(qSerial);

    if (!outputOption.has_value()) {
      std::cerr << "Received request for output " << serial << " which does not exist" << std::endl;
      methodResult.returnError(sdbus::Error {"org.buddiesofbudgie.BudgieDaemonX.Error.OutputMissing"});
      return;
    }

    auto config = manager->configure();
    auto output = outputOption.value();

    // TODO: Josh - Fix
    // zwlr_output_configuration_v1_add_listener(wlr_output_config, &config_listener, nullptr);

    if (enabled) {
      config->enable(output);
    } else {
      config->disable(output);
    }

    manager->applyNoOpConfigurationForNonSpecifiedHeads(config, QStringList {qSerial});

    config->applySelf();
    config->release();

    wl_display_dispatch(bd::WaylandOrchestrator::instance().getDisplay());
    methodResult.returnResults();
  }).detach();
}
