#include "OutputService.hpp"
#include <QString>
#include <QDBusConnection>

namespace bd {
OutputService::OutputService(QSharedPointer<WaylandOutputMetaHead> output, QObject* parent)
    : QObject(parent), m_output(output) {
    QString objectPath = QString("/org/buddiesofbudgie/BudgieDaemonX/Displays/Outputs/%1").arg(output->getIdentifier());
    m_adaptor = new OutputAdaptor(this);
    QDBusConnection::sessionBus().registerObject(objectPath, this, QDBusConnection::ExportAdaptors);
}

OutputService::~OutputService() {}

uint OutputService::AdaptiveSync() const { return static_cast<uint>(m_output->getAdaptiveSync()); }

QString OutputService::Description() const { return m_output->getDescription(); }

bool OutputService::Enabled() const { return m_output->isEnabled(); }

QStringList OutputService::GetAvailableModes() {
    QStringList modePaths;
    for (const auto& mode : m_output->getModes()) {
        modePaths << QString("/org/buddiesofbudgie/BudgieDaemonX/Displays/Outputs/%1/Modes/%2")
                        .arg(m_output->getIdentifier())
                        .arg(mode->getId());
    }
    return modePaths;
}

QString OutputService::GetCurrentMode() {
    auto mode = m_output->getCurrentMode();
    if (mode) {
        return QString("/org/buddiesofbudgie/BudgieDaemonX/Displays/Outputs/%1/Modes/%2")
            .arg(m_output->getIdentifier())
            .arg(mode->getId());
    }
    return QString();
}

int OutputService::Height() const {
    auto mode = m_output->getCurrentMode();
    if (mode) return mode->getSize().value_or(QSize(0,0)).height();
    return 0;
}

int OutputService::HorizontalAnchor() const { return static_cast<int>(m_output->getHorizontalAnchor()); }

QString OutputService::Make() const { return m_output->getMake(); }

QString OutputService::MirrorOf() const { return QString(); /* TODO: implement if available */ }

QString OutputService::Model() const { return m_output->getModel(); }

QString OutputService::Name() const { return m_output->getName(); }

bool OutputService::Primary() const { return m_output->isPrimary(); }

double OutputService::RefreshRate() const {
    auto mode = m_output->getCurrentMode();
    if (mode) return mode->getRefresh().value_or(0.0);
    return 0.0;
}

QString OutputService::RelativeTo() const { return m_output->getRelativeOutput(); }

double OutputService::Scale() const { return m_output->getScale(); }

QString OutputService::Serial() const { return m_output->getIdentifier(); }

int OutputService::Transform() const { return m_output->getTransform(); }

int OutputService::VerticalAnchor() const { return static_cast<int>(m_output->getVerticalAnchor()); }

int OutputService::Width() const {
    auto mode = m_output->getCurrentMode();
    if (mode) return mode->getSize().value_or(QSize(0,0)).width();
    return 0;
}

int OutputService::X() const { return m_output->getPosition().x(); }

int OutputService::Y() const { return m_output->getPosition().y(); }

} // namespace bd
