#include "WaylandOutputHead.hpp"

#include <QPoint>

namespace bd {
  WaylandOutputHead::WaylandOutputHead(QObject* parent, ::zwlr_output_head_v1* wlr_head)
      : QObject(parent), zwlr_output_head_v1(wlr_head), m_wlr_head(std::make_shared<::zwlr_output_head_v1>(wlr_head)) {}

  std::shared_ptr<::zwlr_output_head_v1> WaylandOutputHead::getWlrHead() {
    return m_wlr_head;
  }

  void WaylandOutputHead::zwlr_output_head_v1_name(const QString& name) {
    qDebug() << "Head name changed to: " << name;
    emit propertyChanged(WaylandOutputMetaHeadProperty::Name, QVariant {name});
  }

  void WaylandOutputHead::zwlr_output_head_v1_description(const QString& description) {
    qDebug() << "Head description changed to: " << description;
    emit propertyChanged(WaylandOutputMetaHeadProperty::Description, QVariant {description});
  }

  void WaylandOutputHead::zwlr_output_head_v1_make(const QString& make) {
    qDebug() << "Head make changed to: " << make;
    emit propertyChanged(WaylandOutputMetaHeadProperty::Make, QVariant {make});
  }

  void WaylandOutputHead::zwlr_output_head_v1_model(const QString& model) {
    qDebug() << "Head model changed to: " << model;
    emit propertyChanged(WaylandOutputMetaHeadProperty::Model, QVariant {model});
  }

  void WaylandOutputHead::zwlr_output_head_v1_mode(::zwlr_output_mode_v1* mode) {
    qDebug() << "Head mode changed to: " << mode;
    emit modeAdded(mode);
  }

  void WaylandOutputHead::zwlr_output_head_v1_enabled(int32_t enabled) {
    qDebug() << "Head enabled state changed to: " << enabled;
    emit propertyChanged(WaylandOutputMetaHeadProperty::Enabled, QVariant {enabled});
  }

  void WaylandOutputHead::zwlr_output_head_v1_current_mode(::zwlr_output_mode_v1* mode) {
    emit modeChanged(mode);
  }

  void WaylandOutputHead::zwlr_output_head_v1_finished() {
    emit headFinished();
  }

  void WaylandOutputHead::zwlr_output_head_v1_position(int32_t x, int32_t y) {
    qDebug() << "Head position changed to: " << x << ", " << y;
    emit propertyChanged(WaylandOutputMetaHeadProperty::Position, QVariant {QPoint(x, y)});
  }

  void WaylandOutputHead::zwlr_output_head_v1_transform(int32_t transform) {
    qDebug() << "Head transform changed to: " << transform;
    emit propertyChanged(WaylandOutputMetaHeadProperty::Transform, QVariant {transform});
  }

  void WaylandOutputHead::zwlr_output_head_v1_scale(wl_fixed_t scale) {
    qDebug() << "Head scale changed to: " << wl_fixed_to_double(scale);
    emit propertyChanged(WaylandOutputMetaHeadProperty::Scale, QVariant {wl_fixed_to_double(scale)});
  }

  void WaylandOutputHead::zwlr_output_head_v1_serial_number(const QString& serial) {
    qDebug() << "Head serial number changed to: " << serial;
    emit propertyChanged(WaylandOutputMetaHeadProperty::SerialNumber, QVariant {serial});
  }

  void WaylandOutputHead::zwlr_output_head_v1_adaptive_sync(uint32_t state) {
    qDebug() << "Head adaptive sync state changed to: " << state;
    emit propertyChanged(WaylandOutputMetaHeadProperty::AdaptiveSync, QVariant {state});
  }
}
