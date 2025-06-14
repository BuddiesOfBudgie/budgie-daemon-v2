#include "WaylandOutputMetaHead.hpp"

#include <QCryptographicHash>
#include <QtAlgorithms>
#include <optional>

#include "SysInfo.hpp"
#include "displays/output-manager/mode/WaylandOutputMetaMode.hpp"
#include "output-manager/head/WaylandOutputHead.hpp"

namespace bd {
    WaylandOutputMetaHead::WaylandOutputMetaHead(QObject *parent, KWayland::Client::Registry *registry,
                                                 ::zwlr_output_head_v1 *wlr_head)
            : QObject(parent),
              m_registry(registry),
              m_current_mode(nullptr),
              m_head(nullptr),
              m_position(QPoint{0, 0}),
              m_transform(0),
              m_scale(1.0),
              m_is_available(false),
              m_enabled(false),
              m_adaptive_sync() {
        setHead(wlr_head);
    }

    WaylandOutputMetaHead::~WaylandOutputMetaHead() {
        for (auto &mode: m_output_modes) {
            if (!mode) continue;
            auto mode_ptr = mode.data();
            mode_ptr->deleteLater();
        }
        m_output_modes.clear();
    }

    // Getters

    QtWayland::zwlr_output_head_v1::adaptive_sync_state WaylandOutputMetaHead::getAdaptiveSync() {
        return m_adaptive_sync;
    }

    QSharedPointer<WaylandOutputMetaMode> WaylandOutputMetaHead::getCurrentMode() {
        return m_current_mode;
    }

    QString WaylandOutputMetaHead::getDescription() {
        return m_description;
    }

    QSharedPointer<WaylandOutputHead> WaylandOutputMetaHead::getHead() {
        return m_head;
    }

    QString WaylandOutputMetaHead::getIdentifier() {
        // Have a valid serial, use that as the identifier
        if (!m_serial.isNull() && !m_serial.isEmpty()) {
            qDebug() << "Using serial as identifier:" << m_serial;
            return m_serial;
        }

        // Already generated an identifier
        if (!m_identifier.isNull() && !m_identifier.isEmpty()) { return m_identifier; }

        // Default to unique name being machine ID + name
        auto unique_name = QString{SysInfo::instance().getMachineId() + "_" + m_name};

        if (!m_make.isNull() && !m_model.isNull() && !m_make.isEmpty() && !m_model.isEmpty()) {
            unique_name = QString{m_make + " " + m_model + " (" + m_name + ")"};
        }

        qDebug() << "Generated unique name:" << unique_name;

        auto hash = QCryptographicHash::hash(unique_name.toUtf8(), QCryptographicHash::Md5);

        m_identifier = QString{hash.toHex()};

        qDebug() << "Generated identifier:" << m_identifier;

        return m_identifier;
    }

    QSharedPointer<WaylandOutputMetaMode> WaylandOutputMetaHead::getModeForOutputHead(int width, int height, double refresh) {
        // debug here
        for (const auto& mode_ptr: m_output_modes) {
            if (!mode_ptr) continue;

            auto mode = mode_ptr.data();
            auto modeSizeOpt = mode->getSize();
            auto modeRefreshOpt = mode->getRefresh();
            if (!modeSizeOpt.has_value() || !modeRefreshOpt.has_value()) continue;
            if (!modeSizeOpt.value().isValid()) continue;
            auto modeSize = modeSizeOpt.value();
            auto modeRefresh = modeRefreshOpt.value();

            if (modeSize.width() == width && modeSize.height() == height && qFuzzyCompare(modeRefresh, refresh)) { return mode_ptr; }
        }

        return nullptr;
    }

    QList<QSharedPointer<WaylandOutputMetaMode>> WaylandOutputMetaHead::getModes() {
        return m_output_modes;
    }

    QString WaylandOutputMetaHead::getMake() {
        return m_make;
    }

    QString WaylandOutputMetaHead::getModel() {
        return m_model;
    }

    QString WaylandOutputMetaHead::getName() {
        return m_name;
    }

    QPoint WaylandOutputMetaHead::getPosition() {
        return m_position;
    }

    double WaylandOutputMetaHead::getScale() {
        return m_scale;
    }

    int WaylandOutputMetaHead::getTransform() {
        return m_transform;
    }

    std::optional<QSharedPointer<::zwlr_output_head_v1>> WaylandOutputMetaHead::getWlrHead() {
        if (m_head == nullptr) return std::nullopt;
        auto head = m_head.data()->getWlrHead();
        if (head == nullptr) return std::nullopt;
        return std::make_optional(head);
    }

    bool WaylandOutputMetaHead::isAvailable() {
        return m_is_available;
    }

    bool WaylandOutputMetaHead::isBuiltIn() {
        // Generate identifier if necessary
        getIdentifier();
        // Return if identifier exists
        return !m_identifier.isNull() && !m_identifier.isEmpty();
    }

    bool WaylandOutputMetaHead::isEnabled() {
        return m_enabled;
    }

    // Setters

    void WaylandOutputMetaHead::setHead(::zwlr_output_head_v1 *wlr_head) {
        auto head = new WaylandOutputHead(this, wlr_head);
        m_head = QSharedPointer<WaylandOutputHead>(head);
        m_is_available = true;
        connect(head, &WaylandOutputHead::headFinished, this, &WaylandOutputMetaHead::headDisconnected);
        connect(head, &WaylandOutputHead::modeAdded, this, &WaylandOutputMetaHead::addMode);
        connect(head, &WaylandOutputHead::modeChanged, this, &WaylandOutputMetaHead::currentModeChanged);
        connect(head, &WaylandOutputHead::propertyChanged, this, &WaylandOutputMetaHead::setProperty);
    }

    void WaylandOutputMetaHead::setPosition(QPoint position) {
        if (position.isNull()) {
            qWarning() << "Invalid position provided, not setting position on head: " << getIdentifier();
            return;
        }

        if (m_position == position) {
            qWarning() << "Position is already set to" << position.x() << position.y() << ", not changing it.";
            return;
        }

        qDebug() << "Setting position on head" << getIdentifier() << "to" << m_position.x() << m_position.y();
        m_position.setX(position.x());
        m_position.setY(position.y());
        emit propertyChanged(WaylandOutputMetaHeadProperty::Position, QVariant{m_position});
    }

    // Slots

    void WaylandOutputMetaHead::addMode(::zwlr_output_mode_v1 *mode) {
        auto output_mode = new WaylandOutputMetaMode(this, mode);

        connect(output_mode, &WaylandOutputMetaMode::done, this, [this, output_mode]() {
            // Check if this already exists
            for (const auto &mode_ptr: m_output_modes) {
                if (!mode_ptr) continue;
                auto mode = mode_ptr.data();
                // Already exists, set the wlr_mode of the existing mode and delete this newly created meta mode
                if (mode->isSameAs(output_mode)) {
                    qDebug() << "Found an output mode that matches one we already have, deleting the new one.";
                    auto wlr_mode = output_mode->getWlrMode();
                    if (wlr_mode != nullptr) mode->setMode(wlr_mode);
                    output_mode->deleteLater();
                    return;
                }
            }

            // Doesn't already exist, add it
            qDebug() << "Adding new output mode to head: " << getIdentifier() << " with size: "
                     << output_mode->getSize().value_or(QSize(0, 0))
                     << " and refresh: " << output_mode->getRefresh().value_or(0.0);
            auto mode = QSharedPointer<WaylandOutputMetaMode>(output_mode);
            m_output_modes.append(mode);
        });
    }

    void WaylandOutputMetaHead::currentModeChanged(::zwlr_output_mode_v1 *mode) {
        for (const auto &output_mode_ptr: m_output_modes) {
            if (!output_mode_ptr) continue;
            auto output_mode = output_mode_ptr.data();
            if (!output_mode->getWlrMode()) continue;
            if (output_mode->getWlrMode() == mode) {
                auto outputModeSizeOpt = output_mode->getSize();
                if (!outputModeSizeOpt.has_value()) return;
                if (!outputModeSizeOpt.value().isValid()) return;
                auto outputModeSize = outputModeSizeOpt.value();
                qDebug() << "Setting current mode to" << outputModeSize.width() << "x" << outputModeSize.height() << "@"
                         << output_mode->getRefresh();
                m_current_mode = output_mode_ptr; // Set m_current_mode to same QSharedPointer as iterated output mode
                return;
            }
        }
    }

    void WaylandOutputMetaHead::headDisconnected() {
        qDebug() << "Head disconnected for output: " << getIdentifier();
        m_head = nullptr;
        m_is_available = false;
        emit headNoLongerAvailable();
    }

    void WaylandOutputMetaHead::setProperty(WaylandOutputMetaHeadProperty property, const QVariant &value) {
        bool changed = true;

        switch (property) {
            case WaylandOutputMetaHeadProperty::AdaptiveSync:
                m_adaptive_sync = static_cast<QtWayland::zwlr_output_head_v1::adaptive_sync_state>(value.toInt());
                qDebug() << "Setting adaptive sync on head" << getIdentifier() << "to" << m_adaptive_sync;
                break;
            case WaylandOutputMetaHeadProperty::Description:
                m_description = value.toString();
                qDebug() << "Output head finished, emitting headNoLongerAvailable: " << getIdentifier()
                         << " with description: " << m_description;
                break;
            case WaylandOutputMetaHeadProperty::Enabled:
                m_enabled = value.toBool();
                qInfo() << "Setting enabled state on head" << getIdentifier() << "to" << m_enabled;
                break;
            case WaylandOutputMetaHeadProperty::Make:
                m_make = value.toString();
                break;
            case WaylandOutputMetaHeadProperty::Model:
                m_model = value.toString();
                break;
            case WaylandOutputMetaHeadProperty::Name:
                m_name = value.toString();
                break;
            case WaylandOutputMetaHeadProperty::Position:
                m_position = value.toPoint();
                qDebug() << "Setting position on head" << getIdentifier() << "to" << m_position.x() << m_position.y();
                break;
            case WaylandOutputMetaHeadProperty::Scale:
                m_scale = value.toDouble();
                qDebug() << "Setting scale on head" << getIdentifier() << "to" << m_scale;
                break;
            case WaylandOutputMetaHeadProperty::SerialNumber:
                m_serial = value.toString();
                qDebug() << "Setting serial number on head" << getIdentifier() << "to" << m_serial;
                break;
            case WaylandOutputMetaHeadProperty::Transform:
                m_transform = value.toInt();
                qDebug() << "Setting transform on head" << getIdentifier() << "to" << m_transform;
                break;
            default:
                qWarning() << "Unknown property" << property << "for output head";
                changed = false;
                break;
        }

        if (changed) { emit propertyChanged(property, value); }
    }
}
