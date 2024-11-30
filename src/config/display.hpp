#pragma once

#include <QObject>

#include "format.hpp"
#include "utils.hpp"

namespace bd {
  class DisplayConfig;
  class DisplayGroup;
  class DisplayGroupOutputConfig;

  class DisplayConfig : public QObject {
      Q_OBJECT

    public:
      DisplayConfig(QObject* parent);
      static DisplayConfig& instance();
      static DisplayConfig* create() { return &instance(); }

      void                         debugOutput();
      DisplayGroup*                getActiveGroup();
      std::optional<DisplayGroup*> getMatchingGroup();
      void                         parseConfig();
      void                         saveState();
      std::string                  serialize();

    signals:
      void cancelled();
      void failed();
      void applied();

    public slots:
      void apply();

    protected:
      DisplayGroup*            createDisplayGroupForState();
      DisplayGroup*            m_activeGroup;
      DisplayGlobalPreferences m_preferences;
      QList<DisplayGroup*>     m_groups;
  };

  class DisplayGroup : public QObject {
      Q_OBJECT

    public:
      DisplayGroup(QObject* parent = nullptr);
      DisplayGroup(const toml::value& v, QObject* parent = nullptr);

      QString                          getName() const;
      bool                             isPreferred() const;
      QStringList                      getOutputSerials() const;
      QString                          getPrimaryOutput() const;
      QList<DisplayGroupOutputConfig*> getConfigs();

      void                addConfig(DisplayGroupOutputConfig* config);
      void                setName(const QString& name);
      void                setOutputSerials(const QStringList& serials);
      void                setPreferred(bool preferred);
      void                setPrimaryOutput(const QString& serial);
      toml::ordered_value toToml();

    protected:
      QString                          m_name;
      bool                             m_preferred;
      QStringList                      m_output_serials;
      QString                          m_primary_output;
      QList<DisplayGroupOutputConfig*> m_configs;
  };

  class DisplayGroupOutputConfig : public QObject {
      Q_OBJECT

    public:
      DisplayGroupOutputConfig(QObject* parent = nullptr);

      QString            getSerial() const;
      int                getWidth() const;
      int                getHeight() const;
      int                getRefresh() const;
      std::array<int, 2> getPosition() const;
      double             getScale() const;
      int                getRotation() const;
      bool               getAdaptiveSync() const;
      bool               getDisabled() const;

      toml::ordered_value toToml();

      void setSerial(const QString& serial);
      void setWidth(int width);
      void setHeight(int height);
      void setRefresh(int refresh);
      void setPosition(const std::array<int, 2>& position);
      void setScale(double scale);
      void setRotation(int rotation);
      void setAdaptiveSync(bool adaptive_sync);
      void setDisabled(bool disabled);

    protected:
      QString            m_serial;
      int                m_width;
      int                m_height;
      int                m_refresh;
      std::array<int, 2> m_position;
      double             m_scale;
      int                m_rotation;
      bool               m_adaptive_sync;
      bool               m_disabled;
  };
}
