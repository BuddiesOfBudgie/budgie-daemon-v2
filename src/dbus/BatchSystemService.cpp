#include "BatchSystemService.hpp"
#include <QDBusConnection>
#include "displays/batch-system/ConfigurationAction.hpp"
#include "displays/batch-system/ConfigurationBatchSystem.hpp"
#include "displays/batch-system/enums.hpp"
#include "displays/batch-system/CalculationResult.hpp"

namespace bd {
BatchSystemService::BatchSystemService(QObject* parent)
    : QObject(parent) {
    m_adaptor = new BatchSystemAdaptor(this);
    connect(&ConfigurationBatchSystem::instance(), &ConfigurationBatchSystem::configurationApplied,
            this, &BatchSystemService::ConfigurationApplied);
}

BatchSystemService& BatchSystemService::instance() {
    static BatchSystemService _instance(nullptr);
    return _instance;
}

BatchSystemAdaptor* BatchSystemService::GetAdaptor() {
    return m_adaptor;
}

void BatchSystemService::ResetConfiguration() {
    ConfigurationBatchSystem::instance().reset();
}

void BatchSystemService::SetOutputEnabled(const QString& serial, bool enabled) {
    auto action = enabled ? ConfigurationAction::explicitOn(serial) : ConfigurationAction::explicitOff(serial);
    ConfigurationBatchSystem::instance().addAction(action);
}

void BatchSystemService::SetOutputMode(const QString& serial, int width, int height, double refreshRate) {
    auto action = ConfigurationAction::mode(serial, QSize(width, height), refreshRate);
    ConfigurationBatchSystem::instance().addAction(action);
}

void BatchSystemService::SetOutputPositionAnchor(const QString& serial, const QString& relativeSerial, int horizontalAnchor, int verticalAnchor) {
    auto hAnchor = static_cast<ConfigurationHorizontalAnchor>(horizontalAnchor);
    auto vAnchor = static_cast<ConfigurationVerticalAnchor>(verticalAnchor);
    auto action = ConfigurationAction::setPositionAnchor(serial, relativeSerial, hAnchor, vAnchor);
    ConfigurationBatchSystem::instance().addAction(action);
}

void BatchSystemService::SetOutputScale(const QString& serial, double scale) {
    auto action = ConfigurationAction::scale(serial, scale);
    ConfigurationBatchSystem::instance().addAction(action);
}

void BatchSystemService::SetOutputTransform(const QString& serial, int transform) {
    auto action = ConfigurationAction::transform(serial, static_cast<qint16>(transform));
    ConfigurationBatchSystem::instance().addAction(action);
}

void BatchSystemService::SetOutputAdaptiveSync(const QString& serial, uint adaptiveSync) {
    auto action = ConfigurationAction::adaptiveSync(serial, static_cast<uint32_t>(adaptiveSync));
    ConfigurationBatchSystem::instance().addAction(action);
}

void BatchSystemService::SetOutputPrimary(const QString& serial) {
    // TODO: Implement primary output action if supported by ConfigurationAction
}

void BatchSystemService::SetOutputMirrorOf(const QString& serial, const QString& mirrorSerial) {
    auto action = ConfigurationAction::mirrorOf(serial, mirrorSerial);
    ConfigurationBatchSystem::instance().addAction(action);
}

QVariantMap BatchSystemService::CalculateConfiguration() {
    ConfigurationBatchSystem::instance().calculate();
    auto result = ConfigurationBatchSystem::instance().getCalculationResult();
    if (result) {
        return result->toVariantMap();
    }
    return QVariantMap{};
}

bool BatchSystemService::ApplyConfiguration() {
    ConfigurationBatchSystem::instance().apply();
    // The result will be emitted via ConfigurationApplied signal
    return true;
}

QVariantList BatchSystemService::GetActions() {
    QVariantList result;
    auto actions = ConfigurationBatchSystem::instance().getActions();
    for (const auto& action : actions) {
        QVariantMap map;
        map["type"] = static_cast<int>(action->getActionType());
        map["serial"] = action->getSerial();
        switch (action->getActionType()) {
            case ConfigurationActionType::SetOnOff:
                map["on"] = action->isOn();
                break;
            case ConfigurationActionType::SetMode:
                map["dimensions"] = QVariant::fromValue(action->getDimensions());
                map["refresh"] = action->getRefresh();
                break;
            case ConfigurationActionType::SetPositionAnchor:
                map["relative"] = action->getRelative();
                map["horizontalAnchor"] = static_cast<int>(action->getHorizontalAnchor());
                map["verticalAnchor"] = static_cast<int>(action->getVerticalAnchor());
                break;
            case ConfigurationActionType::SetScale:
                map["scale"] = action->getScale();
                break;
            case ConfigurationActionType::SetTransform:
                map["transform"] = action->getTransform();
                break;
            case ConfigurationActionType::SetAdaptiveSync:
                map["adaptiveSync"] = action->getAdaptiveSync();
                break;
            case ConfigurationActionType::SetPrimary:
                // No extra fields
                break;
            case ConfigurationActionType::SetMirrorOf:
                map["relative"] = action->getRelative();
                break;
            default:
                break;
        }
        result << map;
    }
    return result;
}

} // namespace bd
