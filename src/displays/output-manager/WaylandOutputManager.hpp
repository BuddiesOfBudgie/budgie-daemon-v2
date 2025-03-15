#pragma once

#include <wayland-client.h>
#include <wayland-util.h>

#include <QObject>
#include <optional>
#include <string>

#include "qwayland-wlr-output-management-unstable-v1.h"

namespace bd {
  class WaylandOrchestrator;
  class WaylandOutputManager;
  class WaylandOutputHead;
  class WaylandOutputConfiguration;
  class WaylandOutputConfigurationHead;
  class WaylandOutputMode;

  class WaylandOrchestrator : public QObject {
      Q_OBJECT

    public:
      WaylandOrchestrator(QObject* parent);
      static WaylandOrchestrator& instance();
      static WaylandOrchestrator* create() { return &instance(); }

      void                  init();
      WaylandOutputManager* getManager();
      wl_display*           getDisplay();
      wl_registry*          getRegistry();

      bool hasSerial();
      int  getSerial();

      // wl_registry_listener public listeners
      void registryHandleGlobal(void *data, wl_registry *reg, uint32_t name, const char *interface);
      void registryHandleGlobalRemove(void* data, wl_registry* reg, uint32_t name);

    signals:
      void ready();
      void done();
      void orchestratorInitFailed(QString error);

    public slots:
      void outputManagerDone();

    private:
      wl_registry*          m_registry;
      wl_display*           m_display;
      WaylandOutputManager* m_manager;
      bool                  m_has_initted;
      bool                  m_has_serial;
      int                   m_serial;

      // wl_registry_listener static private handlers
      static void registryHandleGlobalStatic(void* data, wl_registry* reg, uint32_t name, const char* interface, uint32_t version);
      static void registryHandleGlobalRemoveStatic(void* data, wl_registry* reg, uint32_t name);
  };

  class WaylandOutputManager : public QObject, QtWayland::zwlr_output_manager_v1 {
      Q_OBJECT

    public:
      WaylandOutputManager(QObject* parent, wl_registry* registry, uint32_t serial, uint32_t version);
      //      static WaylandOutputManager& instance();

      WaylandOutputConfiguration*       configure();
      QList<WaylandOutputHead*>         getHeads();
      std::optional<WaylandOutputHead*> getOutputHead(const QString& str);
      QList<WaylandOutputConfigurationHead *> applyNoOpConfigurationForNonSpecifiedHeads(WaylandOutputConfiguration* config, const QStringList& identifiers);

      uint32_t getSerial();
      uint32_t getVersion();

    signals:
      void done();

    protected:
      void zwlr_output_manager_v1_head(zwlr_output_head_v1* head) override;
      void zwlr_output_manager_v1_finished() override;
      void zwlr_output_manager_v1_done(uint32_t serial) override;

    private:
      wl_registry*              m_registry;
      QList<WaylandOutputHead*> m_heads;
      uint32_t                  m_serial;
      bool                      m_has_serial;
      uint32_t                  m_version;
  };

  class WaylandOutputHead : public QObject, QtWayland::zwlr_output_head_v1 {
      Q_OBJECT

    public:
      WaylandOutputHead(QObject* parent, wl_registry* registry, ::zwlr_output_head_v1* wlr_head);
      ~WaylandOutputHead() override;

      bool isBuiltIn();
      QList<WaylandOutputMode*>                           getModes();
      WaylandOutputMode*                                  getCurrentMode();
      QString getMake();
      QString getModel();
      QString                                             getName();
      QString                                             getDescription();
      QString getIdentifier();
      int                                                 getX();
      int                                                 getY();
      int                                                 getTransform();
      double                                               getScale();
      ::zwlr_output_head_v1*                              getWlrHead();
      bool                                                isEnabled();
      adaptive_sync_state getAdaptiveSync();
      std::optional<WaylandOutputMode*>                   getModeForOutputHead(int width, int height, double refresh);
      void setX(int x);
      void setY(int y);

    signals:
      void headNoLongerAvailable();

    protected:
      void zwlr_output_head_v1_name(const QString& name) override;
      void zwlr_output_head_v1_description(const QString& description) override;
      void zwlr_output_head_v1_make(const QString &make) override;
      void zwlr_output_head_v1_model(const QString &model) override;
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
      wl_registry*              m_registry;
      ::zwlr_output_head_v1*    m_wlr_head;
      QString                   m_make;
      QString                   m_model;
      QString                   m_name;
      QString                   m_description;
      QString m_identifier;
      QList<WaylandOutputMode*> m_output_modes;
      QString                   m_serial;
      WaylandOutputMode*        m_current_mode;

      int   m_x;
      int   m_y;
      int   m_transform;
      double m_scale;

      bool                                                m_enabled;
      adaptive_sync_state m_adaptive_sync;
  };

  class WaylandOutputConfiguration : public QObject, QtWayland::zwlr_output_configuration_v1 {
      Q_OBJECT

    public:
      WaylandOutputConfiguration(QObject* parent, ::zwlr_output_configuration_v1* config);

      void                            applySelf();
      WaylandOutputConfigurationHead* enable(WaylandOutputHead* head);
      void                            disable(WaylandOutputHead* head);
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
      WaylandOutputConfigurationHead(QObject* parent, WaylandOutputHead* head, ::zwlr_output_configuration_head_v1* config_head);
      WaylandOutputHead* getHead();
      void               release();
      void               setAdaptiveSync(uint32_t state);
      void               setMode(WaylandOutputMode* mode);
      void               setCustomMode(int32_t width, int32_t height, double refresh);
      void               setPosition(int32_t x, int32_t y);
      void               setTransform(int32_t transform);
      void               setScale(double scale);

    private:
      WaylandOutputHead* m_head;
  };

  class WaylandOutputMode : public QObject, QtWayland::zwlr_output_mode_v1 {
      Q_OBJECT

    public:
      WaylandOutputMode(QObject* parent, WaylandOutputHead* head, ::zwlr_output_mode_v1* mode);

      zwlr_output_mode_v1* getBase();
      ::zwlr_output_mode_v1*          getWlrMode();
      uint32_t                        getId();
      int                             getWidth();
      int                             getHeight();
      double                          getRefresh();
      bool                            isPreferred();

    signals:
      void modeNoLongerAvailable();

    protected:
      void zwlr_output_mode_v1_size(int32_t width, int32_t height) override;
      void zwlr_output_mode_v1_refresh(int32_t refresh) override;
      void zwlr_output_mode_v1_preferred() override;
      void zwlr_output_mode_v1_finished() override;

    private:
      WaylandOutputHead*     m_head;
      ::zwlr_output_mode_v1* m_wlr_mode;
      uint32_t               m_id;
      int                    m_width;
      int                    m_height;
      double                 m_refresh;
      bool                   m_preferred;
  };
}
