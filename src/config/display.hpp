#pragma once

#include <QObject>

#include "format.hpp"
#include "utils.hpp"

namespace bd {
  class DisplayConfig : public QObject {
      Q_OBJECT

    public:
      DisplayConfig(QObject* parent);
      static DisplayConfig& instance();
      static DisplayConfig* create() { return &instance(); }

      void                           debugOutput();
      std::optional<DisplayGrouping> getMatchingGroup();
      void                           parseConfig();
      void                           saveManagerState();
      std::string                    serialize();

    signals:
      void cancelled();
      void failed();
      void applied();

    public slots:
      void apply();

    protected:
      DisplayGlobalPreferences     preferences;
      std::vector<DisplayGrouping> groups;
  };
}
