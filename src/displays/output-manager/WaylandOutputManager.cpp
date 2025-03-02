#include "WaylandOutputManager.hpp"

#include <QtDebug>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <qcryptographichash.h>

#include "SysInfo.hpp"

namespace bd {
  WaylandOrchestrator::WaylandOrchestrator(QObject* parent) : QObject(parent), m_registry(nullptr), m_display(nullptr),
                                                              m_manager(nullptr),
                                                              m_has_serial(false),
                                                              m_serial(0), m_has_initted(false) {
  }

  WaylandOrchestrator& WaylandOrchestrator::instance() {
    static WaylandOrchestrator _instance(nullptr);
    return _instance;
  }

  void WaylandOrchestrator::init() {
    m_display = wl_display_connect(nullptr);
    if (m_display == nullptr) {
      emit orchestratorInitFailed(QString("Failed to connect to the Wayland display"));
      return;
    }

    m_registry = wl_display_get_registry(m_display);
    if (m_registry == nullptr) {
      wl_display_disconnect(m_display);
      emit orchestratorInitFailed(QString("Failed to get the Wayland registry"));
      return;
    }

    wl_registry_listener listener = {
        .global        = registryHandleGlobalStatic,
        .global_remove = nullptr,
    };

    wl_registry_add_listener(m_registry, &listener, this);

    if (wl_display_roundtrip(m_display) < 0) {
      emit orchestratorInitFailed(QString("Failed to perform roundtrip on Wayland display"));
      return;
    }

    if (m_manager == nullptr) {
      emit orchestratorInitFailed(QString("Failed to get output manager"));
      return;
    }

    wl_display_dispatch(m_display);
  }

  WaylandOutputManager* WaylandOrchestrator::getManager() {
    return m_manager;
  }

  wl_display* WaylandOrchestrator::getDisplay() {
    return m_display;
  }

  wl_registry* WaylandOrchestrator::getRegistry() {
    return m_registry;
  }

  bool WaylandOrchestrator::hasSerial() {
    return m_has_serial;
  }

  int WaylandOrchestrator::getSerial() {
    return m_serial;
  }

  void WaylandOrchestrator::outputManagerDone() {
    if (!m_has_initted) emit ready(); // Haven't done our first init, emit that we are ready
    m_has_initted = true;
    emit done();
  }

  void WaylandOrchestrator::registryHandleGlobal(void *data, wl_registry *reg, uint32_t name, const char *interface) {
    Q_UNUSED(data);
    if (std::strcmp(interface, QtWayland::zwlr_output_manager_v1::interface()->name) == 0) {
      this->m_manager = new WaylandOutputManager(nullptr, reg, name, QtWayland::zwlr_output_manager_v1::interface()->version);

      connect(this->m_manager, &WaylandOutputManager::done, this, &WaylandOrchestrator::outputManagerDone);
    }
  }

  void WaylandOrchestrator::registryHandleGlobalStatic(void *data, wl_registry *reg, uint32_t name, const char *interface, [[maybe_unused]] uint32_t version) {
    static_cast<WaylandOrchestrator*>(data)->registryHandleGlobal(data, reg, name, interface);
  }

  WaylandOutputManager::WaylandOutputManager(QObject* parent, wl_registry* registry, uint32_t serial, uint32_t version)
      : QObject(parent),
        zwlr_output_manager_v1(registry, serial, static_cast<int>(version)),
        m_registry(registry),
        m_serial(serial),
        m_has_serial(true),
        m_version(version) {}

  // Overridden methods from QtWayland::zwlr_output_manager_v1
  void WaylandOutputManager::zwlr_output_manager_v1_head(zwlr_output_head_v1* wlr_head) {
    auto head = new WaylandOutputHead(nullptr, m_registry, wlr_head);
    this->m_heads.append(head);
    connect(head, &WaylandOutputHead::headNoLongerAvailable, this, [this, head]() { this->m_heads.removeOne(head); });
  }

  void WaylandOutputManager::zwlr_output_manager_v1_done(uint32_t serial) {
    this->m_serial     = serial;
    this->m_has_serial = true;

    emit done();
  }

  // applyNoOpConfigurationForNonSpecifiedHeads is a bit of a funky function, but effectively it applies a configuration that does nothing for every output
  // excluding the ones we are wanting to change (specified by the serial). This is to ensure we don't create protocol errors when performing output
  // configurations, as it is a protocol error to not specify everything else.
  QList<WaylandOutputConfigurationHead *> WaylandOutputManager::applyNoOpConfigurationForNonSpecifiedHeads(WaylandOutputConfiguration* config, const QStringList& serials) {
    auto configHeads = QList<WaylandOutputConfigurationHead*>{};

    for (const auto o : this->m_heads) {
      // Skip the output for the serial we are changing
      if (serials.contains(o->getIdentifier())) { continue; }

      if (o->isEnabled()) {
        auto head = config->enable(o);
        configHeads.append(head);
      } else {
        config->disable(o);
      }
    }

    return configHeads;
  }

  WaylandOutputConfiguration* WaylandOutputManager::configure() {
    auto wlr_output_configuration = this->create_configuration(this->m_serial);
    auto config                   = new WaylandOutputConfiguration(nullptr, wlr_output_configuration);
    connect(config, &WaylandOutputConfiguration::cancelled, this, [this, config]() {
      config->deleteLater();
    });
    connect(config, &WaylandOutputConfiguration::succeeded, this, [this, config]() {
      qInfo() << "Configuration succeeded";
      config->deleteLater();
    });
    connect(config, &WaylandOutputConfiguration::failed, this, [this, config]() {
      qInfo() << "Configuration failed";
      config->deleteLater();
    });
    return config;
  }

  QList<WaylandOutputHead*> WaylandOutputManager::getHeads() {
    return this->m_heads;
  }

  std::optional<WaylandOutputHead*> WaylandOutputManager::getOutputHead(const QString& str) {
    std::optional<WaylandOutputHead*> output_head = std::nullopt;

    for (auto head : this->m_heads) {
      if (head->getIdentifier() == str) {
        output_head = head;
        break;
      }
    }

    return output_head;
  }

  uint32_t WaylandOutputManager::getSerial() {
    return this->m_serial;
  }

  uint32_t WaylandOutputManager::getVersion() {
    return this->m_version;
  }

  // Output Head

  WaylandOutputHead::WaylandOutputHead(QObject* parent, wl_registry* registry, ::zwlr_output_head_v1* wlr_head)
    : QObject(parent), zwlr_output_head_v1(wlr_head), m_registry(registry), m_wlr_head(wlr_head),
      m_current_mode(nullptr), m_x(0),
      m_y(0),
      m_transform(0), m_scale(1.0),
      m_enabled(false),
      m_adaptive_sync() {
  }

  bool WaylandOutputHead::isBuiltIn() {
    // Generate identifier if necessary
    getIdentifier();
    // Return if identifier exists
    return !this->m_identifier.isNull() && !this->m_identifier.isEmpty();
  }

  QList<WaylandOutputMode*> WaylandOutputHead::getModes() {
    return this->m_output_modes;
  }

  WaylandOutputMode* WaylandOutputHead::getCurrentMode() {
    return this->m_current_mode;
  }

  QString WaylandOutputHead::getName() {
    return this->m_name;
  }

  QString WaylandOutputHead::getDescription() {
    return this->m_description;
  }

  QString WaylandOutputHead::getIdentifier() {
    // Have a valid serial, use that as the identifier
    if (!this->m_serial.isNull() && !this->m_serial.isEmpty()) {
      qDebug() << "Using serial as identifier:" << this->m_serial;
      return this->m_serial;
    }

    // Already generated an identifier
    if (!this->m_identifier.isNull() && !this->m_identifier.isEmpty()) {
      return this->m_identifier;
    }

    // Default to unique name being machine ID + name
    auto unique_name = QString { SysInfo::instance().getMachineId() + "_" +  this->m_name };

    if (!this->m_make.isNull() && !this->m_model.isNull() && !this->m_make.isEmpty() && !this->m_model.isEmpty()) {
      unique_name = QString {  this->m_make + " " + this->m_model + " (" + this->m_name + ")" };
    }

    qDebug() << "Generated unique name:" << unique_name;

    auto hash = QCryptographicHash::hash(unique_name.toUtf8(), QCryptographicHash::Md5);

    m_identifier = QString { hash.toHex() };

    qDebug() << "Generated identifier:" << m_identifier;

    return this->m_identifier;
  }

  int WaylandOutputHead::getX() {
    return this->m_x;
  }

  int WaylandOutputHead::getY() {
    return this->m_y;
  }

  bool WaylandOutputHead::isEnabled() {
    return this->m_enabled;
  }

  QtWayland::zwlr_output_head_v1::adaptive_sync_state WaylandOutputHead::getAdaptiveSync() {
    return this->m_adaptive_sync;
  }

  int WaylandOutputHead::getTransform() {
    return this->m_transform;
  }

  double WaylandOutputHead::getScale() {
    return this->m_scale;
  }

  ::zwlr_output_head_v1* WaylandOutputHead::getWlrHead() {
    return this->m_wlr_head;
  }

  std::optional<WaylandOutputMode*> WaylandOutputHead::getModeForOutputHead(int width, int height, double refresh) {
    std::optional<WaylandOutputMode*> output_mode = std::nullopt;

    for (auto mode : this->m_output_modes) {
      if (mode->getWidth() == width && mode->getHeight() == height && mode->getRefresh() == refresh) {
        output_mode = mode;
        break;
      }
    }

    return output_mode;
  }

  void WaylandOutputHead::setX(int x) {
    this->m_x = x;
  }

  void WaylandOutputHead::setY(int y) {
    this->m_y = y;
  }

  void WaylandOutputHead::zwlr_output_head_v1_name(const QString& name) {
    this->m_name = QString {name};
  }

  void WaylandOutputHead::zwlr_output_head_v1_description(const QString& description) {
    this->m_description = QString { description };
  }

  void WaylandOutputHead::zwlr_output_head_v1_make(const QString& make) {
    this->m_make = QString { make };
  }

  void WaylandOutputHead::zwlr_output_head_v1_model(const QString& model) {
    this->m_model = QString { model };
  }

  void WaylandOutputHead::zwlr_output_head_v1_mode(::zwlr_output_mode_v1* mode) {
    auto output_mode = new WaylandOutputMode(nullptr, this, mode);
    this->m_output_modes.append(output_mode);
    connect(output_mode, &WaylandOutputMode::modeNoLongerAvailable, this, [this, output_mode] { this->m_output_modes.removeOne(output_mode); });
  }

  void WaylandOutputHead::zwlr_output_head_v1_enabled(int32_t enabled) {
    this->m_enabled = enabled;
  }

  void WaylandOutputHead::zwlr_output_head_v1_current_mode(::zwlr_output_mode_v1* mode) {
    for (auto output_mode : this->m_output_modes) {
      if (output_mode->getWlrMode() == mode) {
        qDebug() << "Setting current mode to" << output_mode->getWidth() << "x" << output_mode->getHeight() << "@" << output_mode->getRefresh();
        this->m_current_mode = output_mode;
        break;
      }
    }
  }

  void WaylandOutputHead::zwlr_output_head_v1_finished() {
    qDebug() << "Output head finished, emitting headNoLongerAvailable: " << this->getIdentifier() << " with description: " << this->getDescription();
    emit headNoLongerAvailable();
  }

  void WaylandOutputHead::zwlr_output_head_v1_position(int32_t x, int32_t y) {
    this->m_x = x;
    this->m_y = y;
  }

  void WaylandOutputHead::zwlr_output_head_v1_transform(int32_t transform) {
    this->m_transform = transform;
  }

  void WaylandOutputHead::zwlr_output_head_v1_scale(wl_fixed_t scale) {
    this->m_scale = wl_fixed_to_double(scale);
  }

  void WaylandOutputHead::zwlr_output_head_v1_serial_number(const QString& serial) {
    this->m_serial = serial;
  }

  void WaylandOutputHead::zwlr_output_head_v1_adaptive_sync(uint32_t state) {
    this->m_adaptive_sync = static_cast<adaptive_sync_state>(state);
  }

  // Output Mode Configuration

  WaylandOutputConfiguration::WaylandOutputConfiguration(QObject* parent, ::zwlr_output_configuration_v1* config)
      : QObject(parent), zwlr_output_configuration_v1(config) {}

  WaylandOutputConfigurationHead* WaylandOutputConfiguration::enable(WaylandOutputHead* head) {
    auto config_head = this->enable_head(head->getWlrHead());
    return new WaylandOutputConfigurationHead(nullptr, head, config_head);
  }

  void WaylandOutputConfiguration::applySelf() {
    this->apply();
  }

  void WaylandOutputConfiguration::release() {
    this->destroy();
  }

  void WaylandOutputConfiguration::disable(WaylandOutputHead* head) {
    this->disable_head(head->getWlrHead());
  }

  void WaylandOutputConfiguration::zwlr_output_configuration_v1_succeeded() {
    qInfo() << "Configuration succeeded";
    emit succeeded();
  }

  void WaylandOutputConfiguration::zwlr_output_configuration_v1_failed() {
    qCritical() << "Configuration failed";
    emit failed();
  }

  void WaylandOutputConfiguration::zwlr_output_configuration_v1_cancelled() {
    qWarning() << "Configuration cancelled";
    emit cancelled();
  }

  // Output Configuration Head

  WaylandOutputConfigurationHead::WaylandOutputConfigurationHead(QObject* parent, WaylandOutputHead* head, ::zwlr_output_configuration_head_v1* wlr_head)
      : QObject(parent), zwlr_output_configuration_head_v1(wlr_head), m_head(head){}

  WaylandOutputHead* WaylandOutputConfigurationHead::getHead() {
    return this->m_head;
  }

  void WaylandOutputConfigurationHead::release() {
    // TODO: change from being a no-op for now
  }

  void WaylandOutputConfigurationHead::setAdaptiveSync(uint32_t state) {
    this->set_adaptive_sync(state);
  }

  void WaylandOutputConfigurationHead::setMode(WaylandOutputMode* mode) {
    this->set_mode(mode->getWlrMode());
  }

  void WaylandOutputConfigurationHead::setCustomMode(signed int width, signed int height, double refresh) {
    this->set_custom_mode(width, height, static_cast<int32_t>(refresh * 1000));
  }

  void WaylandOutputConfigurationHead::setPosition(int32_t x, int32_t y) {
    this->set_position(x, y);
  }

  void WaylandOutputConfigurationHead::setScale(double scale) {
    this->set_scale(wl_fixed_from_double(scale));
  }

  void WaylandOutputConfigurationHead::setTransform(int32_t transform) {
    this->set_transform(transform);
  }

  // Output Mode Handlers

  WaylandOutputMode::WaylandOutputMode(QObject* parent, WaylandOutputHead* head, ::zwlr_output_mode_v1* mode)
    : QObject(parent), zwlr_output_mode_v1(mode), m_head(head), m_wlr_mode(mode), m_id(0) {
  }

  QtWayland::zwlr_output_mode_v1* WaylandOutputMode::getBase() {
    return this;
  }

  zwlr_output_mode_v1* WaylandOutputMode::getWlrMode() {
    return this->m_wlr_mode;
  }

  uint32_t WaylandOutputMode::getId() {
    return this->m_id;
  }

  int WaylandOutputMode::getWidth() {
    return this->m_width;
  }

  int WaylandOutputMode::getHeight() {
    return this->m_height;
  }

  double WaylandOutputMode::getRefresh() {
    return this->m_refresh;
  }

  bool WaylandOutputMode::isPreferred() {
    return this->m_preferred;
  }

  void WaylandOutputMode::zwlr_output_mode_v1_size(int32_t width, int32_t height) {
    this->m_width  = width;
    this->m_height = height;
  }

  void WaylandOutputMode::zwlr_output_mode_v1_refresh(int32_t refresh) {
    this->m_refresh = refresh / 1000.0;
  }

  void WaylandOutputMode::zwlr_output_mode_v1_preferred() {
    this->m_preferred = true;
  }

  void WaylandOutputMode::zwlr_output_mode_v1_finished() {
    qInfo() << "WaylandOutputMode::zwlr_output_mode_v1_finished (no longer available):" << this->getWidth() << "x" << this->getHeight() << "@" << this->getRefresh();
    emit modeNoLongerAvailable();
  }
}
