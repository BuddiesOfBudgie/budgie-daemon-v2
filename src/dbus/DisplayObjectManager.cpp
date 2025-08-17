#include "DisplayObjectManager.hpp"
#include "displays/output-manager/WaylandOutputManager.hpp"
#include "displays/output-manager/head/WaylandOutputMetaHead.hpp"
#include "displays/output-manager/mode/WaylandOutputMetaMode.hpp"

namespace bd {

DisplayObjectManager& DisplayObjectManager::instance() {
    static DisplayObjectManager mgr;
    return mgr;
}

DisplayObjectManager::DisplayObjectManager(QObject* parent)
    : QObject(parent) {}

void DisplayObjectManager::onOutputManagerReady() {
    qInfo() << "Wayland Orchestrator ready";
    qInfo() << "Starting Display DBus Service now (outputs/modes)";
    auto manager = WaylandOrchestrator::instance().getManager();
    if (!manager) return;
    for (const auto& output : manager->getHeads()) {
        if (!output) continue;
        QString outputId = output->getIdentifier();
        if (m_outputServices.contains(outputId)) continue;
        auto* outputService = new OutputService(output, this);
        m_outputServices[outputId] = outputService;
        for (const auto& mode : output->getModes()) {
            if (!mode) continue;
            QString modeKey = outputId + ":" + QString::number(mode->getId());
            if (m_modeServices.contains(modeKey)) continue;
            auto* modeService = new OutputModeService(mode, outputId, this);
            m_modeServices[modeKey] = modeService;
        }
    }
}

} // namespace bd
