#pragma once

#include <QObject>
#include <QSize>
#include <QSharedPointer>
#include "enums.hpp"

namespace bd {

    class ConfigurationAction : public QObject {
        Q_OBJECT

    public:
        static QSharedPointer<ConfigurationAction> explicitOn(const QString& serial, QObject *parent = nullptr);
        static QSharedPointer<ConfigurationAction> explicitOff(const QString& serial, QObject *parent = nullptr);

        static QSharedPointer<ConfigurationAction> mirrorOf(const QString& serial, QString relative, QObject *parent = nullptr);

        static QSharedPointer<ConfigurationAction> mode(const QString& serial, QSize dimensions, double refresh,
                                             QObject *parent = nullptr);

        static QSharedPointer<ConfigurationAction> setPositionAnchor(const QString& serial, QString relative, ConfigurationHorizontalAnchor horizontal,
                          ConfigurationVerticalAnchor vertical, QObject *parent = nullptr);

        static QSharedPointer<ConfigurationAction> scale(const QString& serial, qreal scale,  QObject *parent = nullptr);

        static QSharedPointer<ConfigurationAction> transform(const QString& serial, qint16 transform, QObject *parent = nullptr);

        static QSharedPointer<ConfigurationAction> adaptiveSync(const QString& serial, uint32_t adaptiveSync, QObject *parent = nullptr);

        ConfigurationActionType getActionType() const;
        QString getSerial() const;
        bool isOn() const;
        QString getRelative() const;
        QSize getDimensions() const;
        double getRefresh() const;
        ConfigurationHorizontalAnchor getHorizontalAnchor() const;
        ConfigurationVerticalAnchor getVerticalAnchor() const;
        qreal getScale() const;
        qint16 getTransform() const;
        uint32_t getAdaptiveSync() const;

    protected:
        explicit ConfigurationAction(ConfigurationActionType action_type, QString serial,
                                     QObject *parent = nullptr);

    private:
        ConfigurationActionType m_action_type;
        QString m_serial;

        // Explicit On/Off (otherwise uses whatever current state of WaylandOutputMetaHead is)
        bool m_on;

        // Shared by mirrorOf and setAnchorTo
        QString m_relative;

        // Mode
        QSize m_dimensions;
        double m_refresh;

        // Position Anchor
        ConfigurationHorizontalAnchor m_horizontal_anchor;
        ConfigurationVerticalAnchor m_vertical_anchor;

        // Scale
        qreal m_scale;

        // Transform
        qint16 m_transform;

        // Adaptive Sync
        uint32_t m_adaptive_sync;
    };

} // bd