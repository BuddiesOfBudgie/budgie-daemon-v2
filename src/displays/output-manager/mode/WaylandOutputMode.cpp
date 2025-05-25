#include "WaylandOutputMode.hpp"

namespace bd {
  WaylandOutputMode::WaylandOutputMode(QObject* parent, ::zwlr_output_mode_v1* mode) : QObject(parent), zwlr_output_mode_v1(mode), m_wlr_mode(mode) {}

  void WaylandOutputMode::checkIfDone() {
    if (m_emitted_size && m_emitted_refresh && m_emitted_preferred) { emit done(); }
  }

  QtWayland::zwlr_output_mode_v1* WaylandOutputMode::getBase() {
    return this;
  }

  zwlr_output_mode_v1* WaylandOutputMode::getWlrMode() {
    return m_wlr_mode;
  }

  void WaylandOutputMode::zwlr_output_mode_v1_size(int32_t width, int32_t height) {
    emit propertyChanged(WaylandOutputMetaModeProperty::Size, QVariant { QSize(width, height) });
    m_emitted_size = true;
    checkIfDone();
  }

  void WaylandOutputMode::zwlr_output_mode_v1_refresh(int32_t refresh) {
    auto val = QVariant::fromValue(refresh / 1000.0);
    emit propertyChanged(WaylandOutputMetaModeProperty::Refresh, val);
    m_emitted_refresh = true;
    checkIfDone();
  }

  void WaylandOutputMode::zwlr_output_mode_v1_preferred() {
    emit propertyChanged(WaylandOutputMetaModeProperty::Preferred, QVariant::fromValue(true));
    m_emitted_preferred = true;
    checkIfDone();
  }

  void WaylandOutputMode::zwlr_output_mode_v1_finished() {
    emit modeFinished();
  }
}
