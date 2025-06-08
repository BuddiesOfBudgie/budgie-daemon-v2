#include "WaylandOutputMetaMode.hpp"

namespace bd {
  WaylandOutputMetaMode::WaylandOutputMetaMode(QObject* parent, ::zwlr_output_mode_v1* wlr_mode)
      : QObject(parent), m_id(0), m_refresh(0.0), m_preferred(std::nullopt), m_size(QSize {0, 0}), m_is_available(std::nullopt) {
    setMode(wlr_mode);
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

  ::zwlr_output_mode_v1* WaylandOutputMetaMode::getWlrMode() {
    return m_mode->getWlrMode();
  }

  std::optional<bool> WaylandOutputMetaMode::isAvailable() {
    return m_is_available;
  }

  std::optional<bool> WaylandOutputMetaMode::isPreferred() {
    return m_preferred;
  }

  bool WaylandOutputMetaMode::isSameAs(WaylandOutputMetaMode* mode) {
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
    if (same) { qDebug() << "Mode is same as ours, with refresh:" << r_refresh.value() << "and size:" << r_size.value(); }

    return same;
  }

  // Setters

  void WaylandOutputMetaMode::setMode(::zwlr_output_mode_v1* wlr_mode) {
    auto mode = new WaylandOutputMode(this, wlr_mode);
    connect(mode, &WaylandOutputMode::modeFinished, this, &WaylandOutputMetaMode::modeDisconnected);
    connect(mode, &WaylandOutputMode::propertyChanged, this, &WaylandOutputMetaMode::setProperty);

    m_mode = std::make_shared<WaylandOutputMode>(mode);
  }

  // Slots

  void WaylandOutputMetaMode::modeDisconnected() {
    qDebug() << "Mode disconnected for output: " << getId();
    m_mode         = nullptr;
    m_is_available = false;
    emit modeNoLongerAvailable();
  }

  void WaylandOutputMetaMode::setProperty(WaylandOutputMetaModeProperty property, const QVariant& value) {
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
      auto size    = getSize();

      if (refresh && size) {
        m_is_available = true;
        emit done();
      }
    }
  }
}
