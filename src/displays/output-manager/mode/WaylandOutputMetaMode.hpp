#pragma once
#include <QObject>
#include <QSize>
#include <QVariant>
#include <optional>

#include "WaylandOutputMode.hpp"

namespace bd {
  class WaylandOutputMetaMode : public QObject {
      Q_OBJECT

    public:
      WaylandOutputMetaMode(QObject* parent, ::zwlr_output_mode_v1* wlr_mode);

      uint32_t               getId();
      std::optional<double>  getRefresh();
      std::optional<QSize>   getSize();
      ::zwlr_output_mode_v1* getWlrMode();
      std::optional<bool>    isAvailable();
      std::optional<bool>    isPreferred();
      bool                   isSameAs(WaylandOutputMetaMode* mode);
      void                   setMode(::zwlr_output_mode_v1* wlr_mode);

    signals:
      void done();
      void modeNoLongerAvailable();
      void propertyChanged(WaylandOutputMetaModeProperty property, const QVariant& value);

    public slots:
      void modeDisconnected();
      void setProperty(WaylandOutputMetaModeProperty property, const QVariant& value);

    private:
      std::shared_ptr<WaylandOutputMode> m_mode;
      uint32_t                           m_id;
      QSize                              m_size;
      double                             m_refresh;
      std::optional<bool>                m_preferred;
      std::optional<bool>                m_is_available;
  };
}
