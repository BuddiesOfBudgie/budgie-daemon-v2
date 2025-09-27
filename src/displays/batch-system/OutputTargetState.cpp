#include "OutputTargetState.hpp"
#include "utils.hpp"

namespace bd {
    OutputTargetState::OutputTargetState(QString serial, QObject *parent) : QObject(parent),
        m_serial(serial), m_on(false), m_dimensions(QSize(0, 0)), m_refresh(0.0), m_relative(""), m_horizontal_anchor(ConfigurationHorizontalAnchor::NoHorizontalAnchor),
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

    double OutputTargetState::getRefresh() const {
        return m_refresh;
    }

    QString OutputTargetState::getRelative() const {
        return m_relative;
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
        qDebug() << "OutputTargetState::setDefaultValues" << m_serial;
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

            qDebug() << "dimensions" << m_dimensions << "refresh" << m_refresh;
        }

        auto position = headData->getPosition();
        m_position = QPoint(position);
        qDebug() << "position" << m_position;
        auto scale = headData->getScale();
        m_scale = scale;

        auto transform = headData->getTransform();
        m_transform = transform;

        auto adaptiveSync = headData->getAdaptiveSync();
        m_adaptive_sync = static_cast<uint32_t>(adaptiveSync);

        // Default anchoring from meta head if present (user or config provided)
        m_relative = headData->getRelativeOutput();
        m_horizontal_anchor = headData->getHorizontalAnchor();
        m_vertical_anchor = headData->getVerticalAnchor();
        m_primary = false;

        qDebug() << "horizontalAnchor" << bd::DisplayConfigurationUtils::getHorizontalAnchorString(m_horizontal_anchor) << "\n" << "verticalAnchor" << bd::DisplayConfigurationUtils::getVerticalAnchorString(m_vertical_anchor) << "\n" << "primary" << m_primary;
    }

    void OutputTargetState::setOn(bool on) {
        qDebug() << "OutputTargetState::setOn" << m_serial << on;
        m_on = on;
    }

    void OutputTargetState::setDimensions(QSize dimensions) {
        qDebug() << "OutputTargetState::setDimensions" << m_serial << dimensions;
        m_dimensions = dimensions;
    }

    void OutputTargetState::setRefresh(double refresh) {
        qDebug() << "OutputTargetState::setRefresh" << m_serial << refresh;
        m_refresh = refresh;
    }

    void OutputTargetState::setRelative(const QString& relative) {
        qDebug() << "OutputTargetState::setRelative" << m_serial << relative;
        m_relative = relative;
    }
    
    void OutputTargetState::setHorizontalAnchor(ConfigurationHorizontalAnchor horizontal_anchor) {
        qDebug() << "OutputTargetState::setHorizontalAnchor" << m_serial << bd::DisplayConfigurationUtils::getHorizontalAnchorString(horizontal_anchor);
        m_horizontal_anchor = horizontal_anchor;
    }

    void OutputTargetState::setVerticalAnchor(ConfigurationVerticalAnchor vertical_anchor) {
        qDebug() << "OutputTargetState::setVerticalAnchor" << m_serial << bd::DisplayConfigurationUtils::getVerticalAnchorString(vertical_anchor);
        m_vertical_anchor = vertical_anchor;
    }

    void OutputTargetState::setPosition(QPoint position) {
        qDebug() << "OutputTargetState::setPosition" << m_serial << position;
        m_position = position;
    }

    void OutputTargetState::setPrimary(bool primary) {
        qDebug() << "OutputTargetState::setPrimary" << m_serial << primary;
        m_primary = primary;
    }

    void OutputTargetState::setScale(qreal scale) {
        qDebug() << "OutputTargetState::setScale" << m_serial << scale;
        m_scale = scale;
    }

    void OutputTargetState::setTransform(qint16 transform) {
        qDebug() << "OutputTargetState::setTransform" << m_serial << transform;
        m_transform = transform;
    }

    void OutputTargetState::setAdaptiveSync(uint32_t adaptiveSync) {
        qDebug() << "OutputTargetState::setAdaptiveSync" << m_serial << adaptiveSync;
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