#include "DisplayObjectManager.hpp"

#include "BatchSystemService.hpp"
#include "DisplayService.hpp"
#include "displays/output-manager/WaylandOutputManager.hpp"
#include "displays/output-manager/head/WaylandOutputMetaHead.hpp"
#include "displays/output-manager/mode/WaylandOutputMetaMode.hpp"

namespace bd {

  DisplayObjectManager& DisplayObjectManager::instance() {
    static DisplayObjectManager mgr;
    return mgr;
  }

  DisplayObjectManager::DisplayObjectManager(QObject* parent) : QObject(parent) {}

  void DisplayObjectManager::onOutputManagerReady() {
    qInfo() << "Wayland Orchestrator ready";
    qInfo() << "Starting Display DBus Service now (outputs/modes)";
    auto manager = WaylandOrchestrator::instance().getManager();
    if (!manager) return;

    if (!QDBusConnection::sessionBus().registerService("org.buddiesofbudgie.BudgieDaemon")) {
      qCritical() << "Failed to acquire DBus service name org.buddiesofbudgie.BudgieDaemon";
    }

    for (const auto& output : manager->getHeads()) {
      if (!output) continue;
      QString outputId = output->getIdentifier();
      if (m_outputServices.contains(outputId)) continue;
      auto* outputService        = new OutputService(output, this);
      m_outputServices[outputId] = outputService;
      for (const auto& mode : output->getModes()) {
        if (!mode) continue;
        QString modeKey = outputId + ":" + mode->getId();
        if (m_modeServices.contains(modeKey)) continue;
        auto* modeService       = new OutputModeService(mode, outputId, this);
        m_modeServices[modeKey] = modeService;
      }
    }

    if (!QDBusConnection::sessionBus().registerObject(DISPLAY_SERVICE_PATH, DisplayService::instance().GetAdaptor(), QDBusConnection::ExportAllContents)) {
      qCritical() << "Failed to register DBus object at path" << DISPLAY_SERVICE_PATH;
    }

    if (!QDBusConnection::sessionBus().registerObject(
            BATCH_SYSTEM_SERVICE_PATH, BatchSystemService::instance().GetAdaptor(), QDBusConnection::ExportAllContents)) {
      qCritical() << "Failed to register DBus object at path" << BATCH_SYSTEM_SERVICE_PATH;
    }
  }

}  // namespace bd
