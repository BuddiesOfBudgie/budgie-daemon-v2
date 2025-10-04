#include "OutputModeService.hpp"
#include <QString>
#include <QDBusConnection>
#include "displays/output-manager/WaylandOutputManager.hpp"
#include "displays/output-manager/head/WaylandOutputMetaHead.hpp"
#include "displays/output-manager/WaylandOutputManager.hpp"
#include "displays/output-manager/mode/WaylandOutputMetaMode.hpp"
#include "displays/output-manager/WaylandOutputManager.hpp"
#include "displays/output-manager/head/WaylandOutputMetaHead.hpp"
#include "displays/output-manager/mode/WaylandOutputMetaMode.hpp"
#include "displays/output-manager/WaylandOutputManager.hpp"
#include "displays/output-manager/head/WaylandOutputMetaHead.hpp"
#include "displays/output-manager/mode/WaylandOutputMetaMode.hpp"

namespace bd {
OutputModeService::OutputModeService(QSharedPointer<WaylandOutputMetaMode> mode, const QString& outputId, QObject* parent)
    : QObject(parent), m_mode(mode), m_outputId(outputId) {
    QString objectPath = QString("/org/buddiesofbudgie/BudgieDaemonX/Displays/Outputs/%1/Modes/%2").arg(outputId).arg(mode->getId());
    m_adaptor = new OutputModeAdaptor(this);
    QDBusConnection::sessionBus().registerObject(objectPath, this, QDBusConnection::ExportAdaptors);
}

OutputModeService::~OutputModeService() {}

int OutputModeService::Width() const {
    auto size = m_mode->getSize();
    if (size) return size->width();
    return 0;
}
int OutputModeService::Height() const {
    auto size = m_mode->getSize();
    if (size) return size->height();
    return 0;
}
qulonglong OutputModeService::RefreshRate() const {
    auto refresh = m_mode->getRefresh();
    if (refresh) return static_cast<qulonglong>(refresh.value());
    return 0;
}
bool OutputModeService::Preferred() const {
    auto preferred = m_mode->isPreferred();
    if (preferred) return preferred.value();
    return false;
}
bool OutputModeService::Current() const {
    return isCurrentMode();
}
bool OutputModeService::isCurrentMode() const {
    auto head = qobject_cast<WaylandOutputMetaHead*>(m_mode->parent());
    if (!head) return false;
    auto currentMode = head->getCurrentMode();
    if (!currentMode) return false;
    // Compare pointer identity
    return currentMode.data() == m_mode.data();
}

QVariantMap OutputModeService::GetModeInfo() {
    QVariantMap info;
    auto size = m_mode->getSize();
    auto refresh = m_mode->getRefresh();
    auto preferred = m_mode->isPreferred();
    info["Width"] = size ? size->width() : 0;
    info["Height"] = size ? size->height() : 0;
    info["RefreshRate"] = refresh ? static_cast<qulonglong>(refresh.value()) : static_cast<qulonglong>(0);
    info["Preferred"] = preferred ? preferred.value() : false;
    info["Current"] = Current();
    return info;
}

} // namespace bd
