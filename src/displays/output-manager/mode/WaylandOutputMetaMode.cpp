#include <QPointer>
#include "WaylandOutputMetaMode.hpp"

namespace bd {
    WaylandOutputMetaMode::WaylandOutputMetaMode(QObject *parent, ::zwlr_output_mode_v1 *wlr_mode)
            : QObject(parent), m_id(0), m_mode(QSharedPointer<WaylandOutputMode>(nullptr)), m_refresh(0.0), m_preferred(std::nullopt), m_size(QSize{0, 0}),
              m_is_available(std::nullopt) {
        setMode(wlr_mode);
    }

    WaylandOutputMetaMode::~WaylandOutputMetaMode() {
        unsetMode(); // Unset the mode to ensure no references are held
        m_preferred = std::nullopt; // Clear preferred state
    }

    uint32_t WaylandOutputMetaMode::getId() {
        return m_id;
    }

    std::optional<double> WaylandOutputMetaMode::getRefresh() {
        if (m_refresh == 0.0) return std::nullopt;
        return std::make_optional(m_refresh);
    }

    std::optional<QSize> WaylandOutputMetaMode::getSize() {
        if (m_size.isEmpty() || m_size.isNull()) return std::nullopt;
        return std::make_optional(m_size);
    }

    std::optional<const ::zwlr_output_mode_v1*> WaylandOutputMetaMode::getWlrMode() {
        if (!m_mode || m_mode.isNull()) return std::nullopt;
        return m_mode->getWlrMode();
    }

    std::optional<bool> WaylandOutputMetaMode::isAvailable() {
        return m_is_available;
    }

    std::optional<bool> WaylandOutputMetaMode::isPreferred() {
        return m_preferred;
    }

    bool WaylandOutputMetaMode::isSameAs(WaylandOutputMetaMode *mode) {
        if (mode == nullptr) { return false; }

        auto r_refresh = mode->getRefresh();
        if (!r_refresh) { return false; }

        auto r_size = mode->getSize();
        if (!r_size) { return false; }

        auto refresh = getRefresh();
        if (!refresh) { return false; }

        auto size = getSize();
        if (!size) { return false; }

        auto same = r_refresh.value() == refresh.value() && r_size.value() == size.value();
        if (same) {
            qDebug() << "Mode is same as ours, with refresh:" << r_refresh.value() << "and size:" << r_size.value();
        }

        return same;
    }

    // Setters

    void WaylandOutputMetaMode::setMode(::zwlr_output_mode_v1 *wlr_mode) {
        if (wlr_mode == nullptr) {
            qWarning() << "Received null wlr_mode, doing nothing.";
            return;
        }
        unsetMode(); // Unset any existing mode

        qDebug() << "Setting new mode with Wayland object:" << (void*)wlr_mode;
        auto mode = new WaylandOutputMode(wlr_mode);
        m_mode = QSharedPointer<WaylandOutputMode>(mode);
//        connect(mode, &WaylandOutputMode::modeFinished, this, &WaylandOutputMetaMode::modeDisconnected);

        connect(mode, &WaylandOutputMode::propertyChanged,
                this, &WaylandOutputMetaMode::setProperty);
    }

    void WaylandOutputMetaMode::unsetMode() {
        if (!m_mode.isNull()) {
            m_mode.clear();
        }
        m_is_available = std::make_optional<bool>(false);
    }

    // Slots

    void WaylandOutputMetaMode::modeDisconnected() {
        unsetMode();
        emit modeNoLongerAvailable();
        m_is_available = std::make_optional<bool>(false);
    }

    void WaylandOutputMetaMode::setPreferred(bool preferred) {
        m_preferred = std::make_optional<bool>(preferred);
        emit propertyChanged(WaylandOutputMetaModeProperty::Preferred, QVariant::fromValue(preferred));
    }

    void WaylandOutputMetaMode::setProperty(WaylandOutputMetaModeProperty property, const QVariant &value) {
        bool changed = true;
        switch (property) {
            case WaylandOutputMetaModeProperty::Preferred:
                m_preferred = std::make_optional<bool>(value.toBool());
                break;
            case WaylandOutputMetaModeProperty::Refresh:
                m_refresh = value.toDouble();
                break;
            case WaylandOutputMetaModeProperty::Size:
                m_size = value.toSize();
                break;
            default:
                changed = false;
                break;
        }

        if (changed) {
            emit propertyChanged(property, value);

            auto refresh = getRefresh();
            auto size = getSize();

            if (!refresh.has_value()) return;
            if (!size.has_value()) return;
            if (!size.value().isValid()) return;

            m_is_available = true;
            emit done();
        }
    }
}
