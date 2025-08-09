#include "OutputTargetState.hpp"

namespace bd {
    OutputTargetState::OutputTargetState(QString serial, QObject *parent) : QObject(parent),
        m_serial(serial), m_on(false), m_dimensions(QSize(0, 0)), m_refresh(0), m_horizontal_anchor(ConfigurationHorizontalAnchor::NoHorizontalAnchor),
        m_vertical_anchor(ConfigurationVerticalAnchor::NoVerticalAnchor), m_primary(false), m_position(QPoint(0, 0)), m_scale(1.0), m_transform(0), m_adaptive_sync(0) {
    }

    QString OutputTargetState::getSerial() const {
        return m_serial;
    }

    bool OutputTargetState::isOn() const {
        return m_on;
    }
    
    QSize OutputTargetState::getDimensions() const {
        return m_dimensions;
    }

    int OutputTargetState::getRefresh() const {
        return m_refresh;
    }
    
    ConfigurationHorizontalAnchor OutputTargetState::getHorizontalAnchor() const {
        return m_horizontal_anchor;
    }

    ConfigurationVerticalAnchor OutputTargetState::getVerticalAnchor() const {
        return m_vertical_anchor;
    }

    QPoint OutputTargetState::getPosition() const {
        return m_position;
    }

    bool OutputTargetState::isPrimary() const {
        return m_primary;
    }

    qreal OutputTargetState::getScale() const {
        return m_scale;
    }

    qint16 OutputTargetState::getTransform() const {
        return m_transform;
    }

    QSize OutputTargetState::getResultingDimensions() const {
        return m_resulting_dimensions;
    }

    uint32_t OutputTargetState::getAdaptiveSync() const {
        return m_adaptive_sync;
    }

    void OutputTargetState::setDefaultValues(QSharedPointer<WaylandOutputMetaHead> head) {
        if (head.isNull()) return;
        auto headData = head.data();
        m_on = headData->isEnabled();

        auto modePtr = headData->getCurrentMode();
        if (!modePtr.isNull()) {
            auto mode = modePtr.data();

            auto dimensionsOpt = mode->getSize();
            if (dimensionsOpt.has_value()) {
                m_dimensions = QSize(dimensionsOpt.value());
            }

            auto refreshOpt = mode->getRefresh();
            if (refreshOpt.has_value()) {
                m_refresh = refreshOpt.value();
            }

        }

        auto position = headData->getPosition();
        m_position = QPoint(position);

        auto scale = headData->getScale();
        m_scale = scale;

        auto transform = headData->getTransform();
        m_transform = transform;

        auto adaptiveSync = headData->getAdaptiveSync();
        m_adaptive_sync = static_cast<uint32_t>(adaptiveSync);

        m_horizontal_anchor = ConfigurationHorizontalAnchor::NoHorizontalAnchor;
        m_vertical_anchor = ConfigurationVerticalAnchor::NoVerticalAnchor;
        m_primary = false;
    }

    void OutputTargetState::setOn(bool on) {
        m_on = on;
    }

    void OutputTargetState::setDimensions(QSize dimensions) {
        m_dimensions = dimensions;
    }

    void OutputTargetState::setRefresh(int refresh) {
        m_refresh = refresh;
    }
    
    void OutputTargetState::setHorizontalAnchor(ConfigurationHorizontalAnchor horizontal_anchor) {
        m_horizontal_anchor = horizontal_anchor;
    }

    void OutputTargetState::setVerticalAnchor(ConfigurationVerticalAnchor vertical_anchor) {
        m_vertical_anchor = vertical_anchor;
    }

    void OutputTargetState::setPosition(QPoint position) {
        m_position = position;
    }

    void OutputTargetState::setPrimary(bool primary) {
        m_primary = primary;
    }

    void OutputTargetState::setScale(qreal scale) {
        m_scale = scale;
    }

    void OutputTargetState::setTransform(qint16 transform) {
        m_transform = transform;
    }

    void OutputTargetState::setAdaptiveSync(uint32_t adaptiveSync) {
        m_adaptive_sync = adaptiveSync;
    }

    void OutputTargetState::updateResultingDimensions() {
        m_resulting_dimensions = (m_scale == 1.0) ? m_dimensions : m_dimensions * m_scale;

        if (m_transform == 0 || m_transform == 180) {
            m_resulting_dimensions.setWidth(m_dimensions.width());
            m_resulting_dimensions.setHeight(m_dimensions.height());
        } else if (m_transform == 90 || m_transform == 270) {
            m_resulting_dimensions.setWidth(m_dimensions.height());
            m_resulting_dimensions.setHeight(m_dimensions.width());
        }
    }
}