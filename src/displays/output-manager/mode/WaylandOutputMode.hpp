#pragma once
#include <QObject>
#include <QSize>
#include <QVariant>

#include "enums.hpp"
#include "qwayland-wlr-output-management-unstable-v1.h"

namespace bd {
  class WaylandOutputMode : public QObject, QtWayland::zwlr_output_mode_v1 {
      Q_OBJECT

    public:
      WaylandOutputMode(QObject* parent, ::zwlr_output_mode_v1* mode);

      zwlr_output_mode_v1*   getBase();
      ::zwlr_output_mode_v1* getWlrMode();

    signals:
      void propertyChanged(WaylandOutputMetaModeProperty property, const QVariant& value);
      void modeFinished();

    protected:
      void zwlr_output_mode_v1_size(int32_t width, int32_t height) override;
      void zwlr_output_mode_v1_refresh(int32_t refresh) override;
      void zwlr_output_mode_v1_preferred() override;
      void zwlr_output_mode_v1_finished() override;

    private:
      ::zwlr_output_mode_v1* m_wlr_mode;
  };
}
