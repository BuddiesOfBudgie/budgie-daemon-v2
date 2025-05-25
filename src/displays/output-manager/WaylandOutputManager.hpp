#pragma once

#include <KWayland/Client/registry.h>
#include <wayland-client.h>
#include <wayland-util.h>

#include <QObject>
#include <optional>
#include <string>

#include "qwayland-wlr-output-management-unstable-v1.h"
#include "head/WaylandOutputMetaHead.hpp"

namespace bd {
  class WaylandOrchestrator;
  class WaylandOutputManager;
  class WaylandOutputMetaHead;
  class WaylandOutputConfiguration;
  class WaylandOutputConfigurationHead;
  class WaylandOutputMetaMode;

  class WaylandOrchestrator : public QObject {
      Q_OBJECT

    public:
      WaylandOrchestrator(QObject* parent);
      static WaylandOrchestrator& instance();
      static WaylandOrchestrator* create() { return &instance(); }

      void                        init();
      WaylandOutputManager*       getManager();
      wl_display*                 getDisplay();
      KWayland::Client::Registry* getRegistry();

      bool hasSerial();
      int  getSerial();

    signals:
      void ready();
      void done();
      void orchestratorInitFailed(QString error);

    public slots:
      void outputManagerDone();

    private:
      KWayland::Client::Registry* m_registry;
      wl_display*                 m_display;
      WaylandOutputManager*       m_manager;
      bool                        m_has_initted;
      bool                        m_has_serial;
      int                         m_serial;
  };

  class WaylandOutputManager : public QObject, QtWayland::zwlr_output_manager_v1 {
      Q_OBJECT

    public:
      WaylandOutputManager(QObject* parent, KWayland::Client::Registry* registry, uint32_t serial, uint32_t version);
      //      static WaylandOutputManager& instance();

      WaylandOutputConfiguration*            configure();
      QList<WaylandOutputMetaHead*>          getHeads();
      std::optional<WaylandOutputMetaHead*>  getOutputHead(const QString& str);
      QList<WaylandOutputConfigurationHead*> applyNoOpConfigurationForNonSpecifiedHeads(WaylandOutputConfiguration* config, const QStringList& identifiers);

      uint32_t getSerial();
      uint32_t getVersion();

    signals:
      void done();

    protected:
      void zwlr_output_manager_v1_head(zwlr_output_head_v1* head) override;
      void zwlr_output_manager_v1_finished() override;
      void zwlr_output_manager_v1_done(uint32_t serial) override;

    private:
      KWayland::Client::Registry*   m_registry;
      QList<WaylandOutputMetaHead*> m_heads;
      uint32_t                      m_serial;
      bool                          m_has_serial;
      uint32_t                      m_version;
  };

  class WaylandOutputConfiguration : public QObject, QtWayland::zwlr_output_configuration_v1 {
      Q_OBJECT

    public:
      WaylandOutputConfiguration(QObject* parent, ::zwlr_output_configuration_v1* config);

      void                            applySelf();
      std::optional<WaylandOutputConfigurationHead*> enable(WaylandOutputMetaHead* head);
      void                            disable(WaylandOutputMetaHead* head);
      void                            release();

    signals:
      void succeeded();
      void failed();
      void cancelled();

    protected:
      void zwlr_output_configuration_v1_succeeded() override;
      void zwlr_output_configuration_v1_failed() override;
      void zwlr_output_configuration_v1_cancelled() override;
  };

  class WaylandOutputConfigurationHead : public QObject, QtWayland::zwlr_output_configuration_head_v1 {
      Q_OBJECT

    public:
      WaylandOutputConfigurationHead(QObject* parent, WaylandOutputMetaHead* head, ::zwlr_output_configuration_head_v1* config_head);
      WaylandOutputMetaHead* getHead();
      void                   release();
      void                   setAdaptiveSync(uint32_t state);
      void                   setMode(WaylandOutputMetaMode* mode);
      void                   setCustomMode(int32_t width, int32_t height, double refresh);
      void                   setPosition(int32_t x, int32_t y);
      void                   setTransform(int32_t transform);
      void                   setScale(double scale);

    private:
      WaylandOutputMetaHead* m_head;
  };
}
