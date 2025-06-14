#pragma once
#include <QObject>
#include <QVariant>

#include "enums.hpp"
#include "qwayland-wlr-output-management-unstable-v1.h"

namespace bd {
  class WaylandOutputHead : public QObject, QtWayland::zwlr_output_head_v1 {
      Q_OBJECT

    public:
      WaylandOutputHead(QObject* parent, ::zwlr_output_head_v1* wlr_head);

      QSharedPointer<::zwlr_output_head_v1> getWlrHead();

    signals:
      void propertyChanged(WaylandOutputMetaHeadProperty property, const QVariant& value);
      void headFinished();
      void modeAdded(::zwlr_output_mode_v1* mode);
      void modeChanged(::zwlr_output_mode_v1* mode);

    protected:
      void zwlr_output_head_v1_name(const QString& name) override;
      void zwlr_output_head_v1_description(const QString& description) override;
      void zwlr_output_head_v1_make(const QString& make) override;
      void zwlr_output_head_v1_model(const QString& model) override;
      void zwlr_output_head_v1_mode(::zwlr_output_mode_v1* mode) override;
      void zwlr_output_head_v1_enabled(int32_t enabled) override;
      void zwlr_output_head_v1_current_mode(::zwlr_output_mode_v1* mode) override;
      void zwlr_output_head_v1_position(int32_t x, int32_t y) override;
      void zwlr_output_head_v1_transform(int32_t transform) override;
      void zwlr_output_head_v1_scale(wl_fixed_t scale) override;
      void zwlr_output_head_v1_serial_number(const QString& serial) override;
      void zwlr_output_head_v1_adaptive_sync(uint32_t state) override;
      void zwlr_output_head_v1_finished() override;

    private:
      QSharedPointer<::zwlr_output_head_v1> m_wlr_head;
  };
}
