#pragma once

#include <QObject>
#include <QSize>
#include <QPoint>
#include <QRect>
#include <QSharedPointer>
#include <output-manager/head/WaylandOutputMetaHead.hpp>

#include "enums.hpp"

namespace bd {
    class OutputTargetState : public QObject {
        Q_OBJECT

    public:
        OutputTargetState(QString serial, QObject *parent = nullptr);

        QString getSerial() const;
        bool isOn() const;
        QSize getDimensions() const;
        int getRefresh() const;
        ConfigurationHorizontalAnchor getHorizontalAnchor() const;
        ConfigurationVerticalAnchor getVerticalAnchor() const;
        QPoint getPosition() const;
        bool isPrimary() const;
        qreal getScale() const;
        qint16 getTransform() const;
        QSize getResultingDimensions() const;
        uint32_t getAdaptiveSync() const;

        void setDefaultValues(QSharedPointer<WaylandOutputMetaHead> head);

        void setOn(bool on);
        void setDimensions(QSize dimensions);
        void setRefresh(int refresh);
        void setHorizontalAnchor(ConfigurationHorizontalAnchor horizontal_anchor);
        void setVerticalAnchor(ConfigurationVerticalAnchor vertical_anchor);
        void setPosition(QPoint position);
        void setPrimary(bool primary);
        void setScale(qreal scale);
        void setTransform(qint16 transform);
        void setAdaptiveSync(uint32_t adaptiveSync);

        void updateResultingDimensions();

    private:
        QString m_serial;
        bool m_on;
        QSize m_dimensions;
        QSize m_resulting_dimensions;
        int m_refresh;
        ConfigurationHorizontalAnchor m_horizontal_anchor;
        ConfigurationVerticalAnchor m_vertical_anchor;
        bool m_primary;
        QPoint m_position;
        qreal m_scale;
        qint16 m_transform;
        uint32_t m_adaptive_sync;
    };
}