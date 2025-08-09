#include "ConfigurationAction.hpp"

namespace bd {
    ConfigurationAction::ConfigurationAction(ConfigurationActionType action_type, QString serial, QObject *parent) : QObject(parent), m_action_type(action_type), m_serial(QString {serial}),
        m_on(false), m_dimensions(QSize()), m_refresh(0), m_horizontal_anchor(ConfigurationHorizontalAnchor::NoHorizontalAnchor),
        m_vertical_anchor(ConfigurationVerticalAnchor::NoVerticalAnchor), m_scale(1.0), m_transform(0), m_adaptive_sync(0) {
    }

    QSharedPointer<ConfigurationAction> ConfigurationAction::explicitOn(const QString& serial, QObject *parent) {
        auto action = QSharedPointer<ConfigurationAction>(new ConfigurationAction(ConfigurationActionType::SetOnOff, serial, parent));
        action->m_on = true;
        return action;
    }

    QSharedPointer<ConfigurationAction> ConfigurationAction::explicitOff(const QString& serial, QObject *parent) {
        return QSharedPointer<ConfigurationAction>(new ConfigurationAction(ConfigurationActionType::SetOnOff, serial, parent));
    }

    QSharedPointer<ConfigurationAction> ConfigurationAction::mirrorOf(const QString& serial, QString relative, QObject *parent) {
        auto action = QSharedPointer<ConfigurationAction>(new ConfigurationAction(ConfigurationActionType::SetMirrorOf, serial, parent));
        action->m_relative = QString { relative };
        return action;
    }

    QSharedPointer<ConfigurationAction> ConfigurationAction::mode(const QString& serial, QSize dimensions, int refresh, QObject *parent) {
        auto action = QSharedPointer<ConfigurationAction>(new ConfigurationAction(ConfigurationActionType::SetMode, serial, parent));
        action->m_dimensions = QSize {dimensions};
        action->m_refresh = refresh;
        return action;
    }

    QSharedPointer<ConfigurationAction> ConfigurationAction::setPositionAnchor(const QString& serial, QString relative, ConfigurationHorizontalAnchor horizontal,
                                                                                 ConfigurationVerticalAnchor vertical, QObject *parent) {
        auto action = QSharedPointer<ConfigurationAction>(new ConfigurationAction(ConfigurationActionType::SetPositionAnchor, serial, parent));
        action->m_relative = QString { relative };
        action->m_horizontal_anchor = horizontal;
        action->m_vertical_anchor = vertical;
        return action;
    }

    QSharedPointer<ConfigurationAction> ConfigurationAction::scale(const QString& serial, qreal scale, QObject *parent) {
        auto action = QSharedPointer<ConfigurationAction>(new ConfigurationAction(ConfigurationActionType::SetScale, serial, parent));
        action->m_scale = scale;
        return action;
    }

    QSharedPointer<ConfigurationAction> ConfigurationAction::transform(const QString& serial, qint16 transform, QObject *parent) {
        auto action = QSharedPointer<ConfigurationAction>(new ConfigurationAction(ConfigurationActionType::SetTransform, serial, parent));
        action->m_transform = transform;
        return action;
    }

    QSharedPointer<ConfigurationAction> ConfigurationAction::adaptiveSync(const QString& serial, uint32_t adaptiveSync, QObject *parent) {
        auto action = QSharedPointer<ConfigurationAction>(new ConfigurationAction(ConfigurationActionType::SetAdaptiveSync, serial, parent));
        action->m_adaptive_sync = adaptiveSync;
        return action;
    }

    ConfigurationActionType ConfigurationAction::getActionType() const {
        return m_action_type;
    }

    QString ConfigurationAction::getSerial() const {
        return m_serial;
    }

    bool ConfigurationAction::isOn() const {
        return m_on;
    }

    QString ConfigurationAction::getRelative() const {
        return m_relative;
    }

    QSize ConfigurationAction::getDimensions() const {
        return m_dimensions;
    }

    int ConfigurationAction::getRefresh() const {
        return m_refresh;
    }

    ConfigurationHorizontalAnchor ConfigurationAction::getHorizontalAnchor() const {
        return m_horizontal_anchor;
    }

    ConfigurationVerticalAnchor ConfigurationAction::getVerticalAnchor() const {
        return m_vertical_anchor;
    }

    qreal ConfigurationAction::getScale() const {
        return m_scale;
    }

    qint16 ConfigurationAction::getTransform() const {
        return m_transform;
    }

    uint32_t ConfigurationAction::getAdaptiveSync() const {
        return m_adaptive_sync;
    }
}
