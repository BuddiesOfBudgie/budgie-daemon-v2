#pragma once
#include <KWayland/Client/registry.h>

#include <QObject>
#include <QPoint>
#include <optional>

#include "enums.hpp"
#include "displays/output-manager/head/WaylandOutputHead.hpp"
#include "displays/output-manager/mode/WaylandOutputMetaMode.hpp"

namespace bd {

    class WaylandOutputMetaHead : public QObject {
    Q_OBJECT

    public:
        WaylandOutputMetaHead(QObject *parent, KWayland::Client::Registry *registry, ::zwlr_output_head_v1 *wlr_head);

        ~WaylandOutputMetaHead() override;

        QtWayland::zwlr_output_head_v1::adaptive_sync_state getAdaptiveSync();

        std::optional<WaylandOutputMetaMode *> getCurrentMode();

        QString getDescription();

        std::optional<WaylandOutputHead *> getHead();

        QString getIdentifier();

        QList<WaylandOutputMetaMode *> getModes();

        QString getMake();

        QString getModel();

        std::optional<WaylandOutputMetaMode *> getModeForOutputHead(int width, int height, double refresh);

        QString getName();

        QPoint getPosition();

        double getScale();

        int getTransform();

        std::optional<::zwlr_output_head_v1 *> getWlrHead();

        bool isAvailable();

        bool isBuiltIn();

        bool isEnabled();

        void setHead(::zwlr_output_head_v1 *head);

        void setPosition(QPoint *position);

    signals:

        void headNoLongerAvailable();

        void propertyChanged(WaylandOutputMetaHeadProperty property, const QVariant &value);

    public slots:

        void addMode(::zwlr_output_mode_v1 *mode);

        void currentModeChanged(::zwlr_output_mode_v1 *mode);

        void headDisconnected();

        void setProperty(WaylandOutputMetaHeadProperty property, const QVariant &value);

    private:
        KWayland::Client::Registry *m_registry;
        std::optional<WaylandOutputHead *> m_head;
        QString m_make;
        QString m_model;
        QString m_name;
        QString m_description;
        QString m_identifier;
        QList<WaylandOutputMetaMode *> m_output_modes;
        QString m_serial;
        WaylandOutputMetaMode *m_current_mode;

        QPoint m_position;
        int m_transform;
        double m_scale;

        bool m_is_available;
        bool m_enabled;
        QtWayland::zwlr_output_head_v1::adaptive_sync_state m_adaptive_sync;
    };
}