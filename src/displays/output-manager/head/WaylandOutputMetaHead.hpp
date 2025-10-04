#pragma once

#include <KWayland/Client/registry.h>

#include <QObject>
#include <QPoint>
#include <optional>

#include "displays/output-manager/head/WaylandOutputHead.hpp"
#include "displays/output-manager/mode/WaylandOutputMetaMode.hpp"
#include "enums.hpp"
#include "displays/batch-system/enums.hpp"

namespace bd {

    class WaylandOutputMetaHead : public QObject {
    Q_OBJECT

    public:
        WaylandOutputMetaHead(QObject *parent, KWayland::Client::Registry *registry);

        ~WaylandOutputMetaHead() override;

        QtWayland::zwlr_output_head_v1::adaptive_sync_state getAdaptiveSync();

        QSharedPointer<WaylandOutputMetaMode> getCurrentMode();

        QString getDescription();

        QSharedPointer<WaylandOutputHead> getHead();

        ConfigurationHorizontalAnchor getHorizontalAnchor();

        QString getIdentifier();

        QString getMake();

        QString getModel();

        QSharedPointer<WaylandOutputMetaMode> getModeForOutputHead(int width, int height, qulonglong refresh);

        QList<QSharedPointer<WaylandOutputMetaMode>> getModes();

        QString getName();

        QPoint getPosition();

        QString getRelativeOutput();

        double getScale();

        int getTransform();

        ConfigurationVerticalAnchor getVerticalAnchor();

        std::optional<::zwlr_output_head_v1*> getWlrHead();

        bool isAvailable();
        bool isBuiltIn();
        bool isEnabled();
        bool isPrimary();

        void setHead(::zwlr_output_head_v1 *head);

        void setHorizontalAnchoring(ConfigurationHorizontalAnchor horizontal);

        void setPosition(QPoint position);

        void setRelativeOutput(const QString &relative);
        void setPrimary(bool primary);

        void setVerticalAnchoring(ConfigurationVerticalAnchor vertical);

        void unsetModes();

    signals:

        void headAvailable();

        void headNoLongerAvailable();

        void propertyChanged(WaylandOutputMetaHeadProperty property, const QVariant &value);

    public slots:

        QSharedPointer<WaylandOutputMetaMode> addMode(::zwlr_output_mode_v1 *mode);

        void currentModeChanged(::zwlr_output_mode_v1 *mode);

        void headDisconnected();

        void setProperty(WaylandOutputMetaHeadProperty property, const QVariant &value);

    private:
        KWayland::Client::Registry *m_registry;
        QSharedPointer<WaylandOutputHead> m_head;
        QString m_make;
        QString m_model;
        QString m_name;
        QString m_description;
        QString m_identifier;
        QList<QSharedPointer<WaylandOutputMetaMode>> m_output_modes;
        QString m_serial;
        QSharedPointer<WaylandOutputMetaMode> m_current_mode;

        QPoint m_position;
        qint16 m_transform;
        qreal m_scale;

        bool m_is_available;
        bool m_enabled;
        QtWayland::zwlr_output_head_v1::adaptive_sync_state m_adaptive_sync;

        // Non-protocol metadata persisted via config
        QString m_relative_output;
        ConfigurationHorizontalAnchor m_horizontal_anchor;
        ConfigurationVerticalAnchor m_vertical_anchor;
        bool m_primary;
    };
}
