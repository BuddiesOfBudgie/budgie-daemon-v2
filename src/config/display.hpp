#pragma once

#include <QObject>

#include "format.hpp"
#include "utils.hpp"
#include "displays/batch-system/enums.hpp"

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

      QString                                  getName() const;
      bool                                     isPreferred() const;
      QStringList                              getOutputIdentifiers() const;
      QString                                  getPrimaryOutput() const;
      QList<DisplayGroupOutputConfig*>         getConfigs();
      std::optional<DisplayGroupOutputConfig*> getConfigForIdentifier(QString identifier);

      void                addConfig(DisplayGroupOutputConfig* config);
      void                setName(const QString& name);
      void                setOutputIdentifiers(const QStringList& identifiers);
      void                setPreferred(bool preferred);
      void                setPrimaryOutput(const QString& identifier);
      toml::ordered_value toToml();

    protected:
      QString                          m_name;
      bool                             m_preferred;
      QStringList                      m_output_identifiers;
      QString                          m_primary_output;
      QList<DisplayGroupOutputConfig*> m_configs;
  };

  class DisplayGroupOutputConfig : public QObject {
      Q_OBJECT

    public:
      DisplayGroupOutputConfig(QObject* parent = nullptr);

      bool               getAdaptiveSync() const;
      bool               getDisabled() const;
      int                getHeight() const;
      QString            getRelativeOutput() const;
      ConfigurationHorizontalAnchor getHorizontalAnchor() const;
      ConfigurationVerticalAnchor getVerticalAnchor() const;
      double             getRefresh() const;
      int                getRotation() const;
      double             getScale() const;
      QString            getIdentifier() const;
      int                getWidth() const;

      toml::ordered_value toToml();

      void setAdaptiveSync(bool adaptive_sync);
      void setDisabled(bool disabled);
      void setHeight(int height);
      void setIdentifier(const QString& identifier);
      void setRelativeOutput(const QString& relativeOutput);
      void setHorizontalAnchor(ConfigurationHorizontalAnchor horizontalAnchor);
      void setVerticalAnchor(ConfigurationVerticalAnchor verticalAnchor);
      void setRefresh(double refresh);
      void setRotation(int rotation);
      void setScale(double scale);
      void setWidth(int width);

    protected:
      QString            m_identifier;
      int                m_width;
      int                m_height;
      double             m_refresh;
      QString            m_relative_output;
      ConfigurationHorizontalAnchor m_horizontal_anchor;
      ConfigurationVerticalAnchor m_vertical_anchor;
      double             m_scale;
      int                m_rotation;
      bool               m_adaptive_sync;
      bool               m_disabled;
  };
}
