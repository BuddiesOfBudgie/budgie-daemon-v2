#include "WaylandOutputManager.hpp"

#include <KWayland/Client/registry.h>
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

    m_registry = new KWayland::Client::Registry();
    m_registry->create(m_display); // Create using our existing display connection

    if (!m_registry->isValid()) {
      wl_display_disconnect(m_display);
      m_registry->release();
      emit orchestratorInitFailed(QString("Failed to create our KWayland registry and manage it"));
      return;
    }

    connect(m_registry, &KWayland::Client::Registry::interfaceAnnounced, this, [this](QByteArray interface, quint32 name, quint32 version) {
      if (std::strcmp(interface, QtWayland::zwlr_output_manager_v1::interface()->name) == 0) {
        m_manager = new WaylandOutputManager(nullptr, m_registry, name, QtWayland::zwlr_output_manager_v1::interface()->version);

        connect(m_manager, &WaylandOutputManager::done, this, &WaylandOrchestrator::outputManagerDone);
      }
    });

    m_registry->setup();

    if (wl_display_roundtrip(m_display) < 0) {
      emit orchestratorInitFailed(QString("Failed to perform roundtrip on Wayland display"));
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

  KWayland::Client::Registry* WaylandOrchestrator::getRegistry() {
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

  WaylandOutputManager::WaylandOutputManager(QObject* parent, KWayland::Client::Registry* registry, uint32_t serial, uint32_t version)
      : QObject(parent),
        zwlr_output_manager_v1(registry->registry(), serial, static_cast<int>(version)),
        m_registry(registry),
        m_serial(serial),
        m_has_serial(true),
        m_version(version) {}

  // Overridden methods from QtWayland::zwlr_output_manager_v1
  void WaylandOutputManager::zwlr_output_manager_v1_head(zwlr_output_head_v1* wlr_head) {
    auto head = new WaylandOutputHead(nullptr, m_registry, wlr_head);
    qInfo() << "WaylandOutputManager::zwlr_output_manager_v1_head with id:" << head->getIdentifier() << ", description:" << head->getDescription();
    m_heads.append(head);
    connect(head, &WaylandOutputHead::headNoLongerAvailable, this, [this, head]() {
      qInfo() << "WaylandOutputManager::zwlr_output_manager_v1_head headNoLongerAvailable:" << head->getIdentifier() << ", description:" << head->getDescription();
      m_heads.removeOne(head);
      delete head;
    });
  }

  void WaylandOutputManager::zwlr_output_manager_v1_finished() {
    qInfo() << "WaylandOutputManager::zwlr_output_manager_v1_finished";
  }

  void WaylandOutputManager::zwlr_output_manager_v1_done(uint32_t serial) {
    m_serial     = serial;
    m_has_serial = true;

    emit done();
  }

  // applyNoOpConfigurationForNonSpecifiedHeads is a bit of a funky function, but effectively it applies a configuration that does nothing for every output
  // excluding the ones we are wanting to change (specified by the serial). This is to ensure we don't create protocol errors when performing output
  // configurations, as it is a protocol error to not specify everything else.
  QList<WaylandOutputConfigurationHead *> WaylandOutputManager::applyNoOpConfigurationForNonSpecifiedHeads(WaylandOutputConfiguration* config, const QStringList& serials) {
    auto configHeads = QList<WaylandOutputConfigurationHead*>{};
    qDebug() << "Applying no-op configuration for non-specified heads. Ignoring:" << serials.join(", ");

    for (const auto o : m_heads) {
      qDebug() << "Checking head " << o->getIdentifier() << ": " << o->getDescription();
      // Skip the output for the serial we are changing
      if (serials.contains(o->getIdentifier())) {
        qDebug() << "Skipping head " << o->getIdentifier();
        continue;
      }

      if (o->isEnabled()) {
        qDebug() << "Ensuring head " << o->getIdentifier() << " is enabled";
        auto head = config->enable(o);
        configHeads.append(head);
      } else {
        qDebug() << "Ensuring head " << o->getIdentifier() << " is disabled";
        config->disable(o);
      }
    }

    return configHeads;
  }

  WaylandOutputConfiguration* WaylandOutputManager::configure() {
    auto wlr_output_configuration = create_configuration(m_serial);
    auto config                   = new WaylandOutputConfiguration(nullptr, wlr_output_configuration);
    connect(config, &WaylandOutputConfiguration::cancelled, this, [this, config]() {
      qDebug() << "Configuration cancelled";
      config->deleteLater();
    });
    connect(config, &WaylandOutputConfiguration::succeeded, this, [this, config]() {
      qDebug() << "Configuration succeeded";
      config->deleteLater();
    });
    connect(config, &WaylandOutputConfiguration::failed, this, [this, config]() {
      qDebug() << "Configuration failed";
      config->deleteLater();
    });
    return config;
  }

  QList<WaylandOutputHead*> WaylandOutputManager::getHeads() {
    return m_heads;
  }

  std::optional<WaylandOutputHead*> WaylandOutputManager::getOutputHead(const QString& str) {
    std::optional<WaylandOutputHead*> output_head = std::nullopt;

    for (auto head : m_heads) {
      if (head->getIdentifier() == str) {
        output_head = head;
        break;
      }
    }

    return output_head;
  }

  uint32_t WaylandOutputManager::getSerial() {
    return m_serial;
  }

  uint32_t WaylandOutputManager::getVersion() {
    return m_version;
  }

  // Output Head

  WaylandOutputHead::WaylandOutputHead(QObject* parent, KWayland::Client::Registry* registry, ::zwlr_output_head_v1* wlr_head)
    : QObject(parent), zwlr_output_head_v1(wlr_head), m_registry(registry), m_wlr_head(wlr_head),
      m_current_mode(nullptr), m_x(0),
      m_y(0),
      m_transform(0), m_scale(1.0),
      m_enabled(false),
      m_adaptive_sync() {
  }

  WaylandOutputHead::~WaylandOutputHead() {
    qDeleteAll(m_output_modes);
    m_output_modes.clear();
  }

  bool WaylandOutputHead::isBuiltIn() {
    // Generate identifier if necessary
    getIdentifier();
    // Return if identifier exists
    return !m_identifier.isNull() && !m_identifier.isEmpty();
  }

  QList<WaylandOutputMode*> WaylandOutputHead::getModes() {
    return m_output_modes;
  }

  WaylandOutputMode* WaylandOutputHead::getCurrentMode() {
    return m_current_mode;
  }

  QString WaylandOutputHead::getName() {
    return m_name;
  }

  QString WaylandOutputHead::getDescription() {
    return m_description;
  }

  QString WaylandOutputHead::getIdentifier() {
    // Have a valid serial, use that as the identifier
    if (!m_serial.isNull() && !m_serial.isEmpty()) {
      qDebug() << "Using serial as identifier:" << m_serial;
      return m_serial;
    }

    // Already generated an identifier
    if (!m_identifier.isNull() && !m_identifier.isEmpty()) {
      return m_identifier;
    }

    // Default to unique name being machine ID + name
    auto unique_name = QString { SysInfo::instance().getMachineId() + "_" +  m_name };

    if (!m_make.isNull() && !m_model.isNull() && !m_make.isEmpty() && !m_model.isEmpty()) {
      unique_name = QString {  m_make + " " + m_model + " (" + m_name + ")" };
    }

    qDebug() << "Generated unique name:" << unique_name;

    auto hash = QCryptographicHash::hash(unique_name.toUtf8(), QCryptographicHash::Md5);

    m_identifier = QString { hash.toHex() };

    qDebug() << "Generated identifier:" << m_identifier;

    return m_identifier;
  }

  int WaylandOutputHead::getX() {
    return m_x;
  }

  int WaylandOutputHead::getY() {
    return m_y;
  }

  bool WaylandOutputHead::isEnabled() {
    return m_enabled;
  }

  QtWayland::zwlr_output_head_v1::adaptive_sync_state WaylandOutputHead::getAdaptiveSync() {
    return m_adaptive_sync;
  }

  int WaylandOutputHead::getTransform() {
    return m_transform;
  }

  double WaylandOutputHead::getScale() {
    return m_scale;
  }

  ::zwlr_output_head_v1* WaylandOutputHead::getWlrHead() {
    return m_wlr_head;
  }

  std::optional<WaylandOutputMode*> WaylandOutputHead::getModeForOutputHead(int width, int height, double refresh) {
    std::optional<WaylandOutputMode*> output_mode = std::nullopt;

    for (auto mode : m_output_modes) {
      if (mode->getWidth() == width && mode->getHeight() == height && mode->getRefresh() == refresh) {
        output_mode = mode;
        break;
      }
    }

    return output_mode;
  }

  void WaylandOutputHead::setX(int x) {
    m_x = x;
  }

  void WaylandOutputHead::setY(int y) {
    m_y = y;
  }

  void WaylandOutputHead::zwlr_output_head_v1_name(const QString& name) {
    m_name = QString {name};
  }

  void WaylandOutputHead::zwlr_output_head_v1_description(const QString& description) {
    m_description = QString { description };
  }

  void WaylandOutputHead::zwlr_output_head_v1_make(const QString& make) {
    m_make = QString { make };
  }

  void WaylandOutputHead::zwlr_output_head_v1_model(const QString& model) {
    m_model = QString { model };
  }

  void WaylandOutputHead::zwlr_output_head_v1_mode(::zwlr_output_mode_v1* mode) {
    auto output_mode = new WaylandOutputMode(nullptr, this, mode);
    m_output_modes.append(output_mode);
    connect(output_mode, &WaylandOutputMode::modeNoLongerAvailable, this, [this, output_mode] {
      m_output_modes.removeOne(output_mode);
      delete output_mode;
    });
  }

  void WaylandOutputHead::zwlr_output_head_v1_enabled(int32_t enabled) {
    qInfo() << "Setting enabled state on head" << getIdentifier() << "to" << enabled;
    m_enabled = enabled;
  }

  void WaylandOutputHead::zwlr_output_head_v1_current_mode(::zwlr_output_mode_v1* mode) {
    for (auto output_mode : m_output_modes) {
      if (output_mode->getWlrMode() == mode) {
        qDebug() << "Setting current mode to" << output_mode->getWidth() << "x" << output_mode->getHeight() << "@" << output_mode->getRefresh();
        m_current_mode = output_mode;
        return;
      }
    }
  }

  void WaylandOutputHead::zwlr_output_head_v1_finished() {
    qDebug() << "Output head finished, emitting headNoLongerAvailable: " << getIdentifier() << " with description: " << getDescription();
    emit headNoLongerAvailable();
  }

  void WaylandOutputHead::zwlr_output_head_v1_position(int32_t x, int32_t y) {
    m_x = x;
    m_y = y;
  }

  void WaylandOutputHead::zwlr_output_head_v1_transform(int32_t transform) {
    m_transform = transform;
  }

  void WaylandOutputHead::zwlr_output_head_v1_scale(wl_fixed_t scale) {
    m_scale = wl_fixed_to_double(scale);
  }

  void WaylandOutputHead::zwlr_output_head_v1_serial_number(const QString& serial) {
    m_serial = serial;
  }

  void WaylandOutputHead::zwlr_output_head_v1_adaptive_sync(uint32_t state) {
    m_adaptive_sync = static_cast<adaptive_sync_state>(state);
  }

  // Output Mode Configuration

  WaylandOutputConfiguration::WaylandOutputConfiguration(QObject* parent, ::zwlr_output_configuration_v1* config)
      : QObject(parent), zwlr_output_configuration_v1(config) {}

  WaylandOutputConfigurationHead* WaylandOutputConfiguration::enable(WaylandOutputHead* head) {
    auto config_head = enable_head(head->getWlrHead());
    return new WaylandOutputConfigurationHead(nullptr, head, config_head);
  }

  void WaylandOutputConfiguration::applySelf() {
    apply();
  }

  void WaylandOutputConfiguration::release() {
    destroy();
  }

  void WaylandOutputConfiguration::disable(WaylandOutputHead* head) {
    disable_head(head->getWlrHead());
  }

  void WaylandOutputConfiguration::zwlr_output_configuration_v1_succeeded() {
    emit succeeded();
  }

  void WaylandOutputConfiguration::zwlr_output_configuration_v1_failed() {
    emit failed();
  }

  void WaylandOutputConfiguration::zwlr_output_configuration_v1_cancelled() {
    emit cancelled();
  }

  // Output Configuration Head

  WaylandOutputConfigurationHead::WaylandOutputConfigurationHead(QObject* parent, WaylandOutputHead* head, ::zwlr_output_configuration_head_v1* wlr_head)
      : QObject(parent), zwlr_output_configuration_head_v1(wlr_head), m_head(head){}

  WaylandOutputHead* WaylandOutputConfigurationHead::getHead() {
    return m_head;
  }

  void WaylandOutputConfigurationHead::release() {
    // TODO: change from being a no-op for now
  }

  void WaylandOutputConfigurationHead::setAdaptiveSync(uint32_t state) {
    set_adaptive_sync(state);
  }

  void WaylandOutputConfigurationHead::setMode(WaylandOutputMode* mode) {
    set_mode(mode->getWlrMode());
  }

  void WaylandOutputConfigurationHead::setCustomMode(signed int width, signed int height, double refresh) {
    set_custom_mode(width, height, static_cast<int32_t>(refresh * 1000));
  }

  void WaylandOutputConfigurationHead::setPosition(int32_t x, int32_t y) {
    set_position(x, y);
  }

  void WaylandOutputConfigurationHead::setScale(double scale) {
    set_scale(wl_fixed_from_double(scale));
  }

  void WaylandOutputConfigurationHead::setTransform(int32_t transform) {
    set_transform(transform);
  }

  // Output Mode Handlers

  WaylandOutputMode::WaylandOutputMode(QObject* parent, WaylandOutputHead* head, ::zwlr_output_mode_v1* mode)
    : QObject(parent), zwlr_output_mode_v1(mode), m_head(head), m_wlr_mode(mode), m_id(0) {
  }

  QtWayland::zwlr_output_mode_v1* WaylandOutputMode::getBase() {
    return this;
  }

  zwlr_output_mode_v1* WaylandOutputMode::getWlrMode() {
    return m_wlr_mode;
  }

  uint32_t WaylandOutputMode::getId() {
    return m_id;
  }

  int WaylandOutputMode::getWidth() {
    return m_width;
  }

  int WaylandOutputMode::getHeight() {
    return m_height;
  }

  double WaylandOutputMode::getRefresh() {
    return m_refresh;
  }

  bool WaylandOutputMode::isPreferred() {
    return m_preferred;
  }

  void WaylandOutputMode::zwlr_output_mode_v1_size(int32_t width, int32_t height) {
    m_width  = width;
    m_height = height;
  }

  void WaylandOutputMode::zwlr_output_mode_v1_refresh(int32_t refresh) {
    m_refresh = refresh / 1000.0;
  }

  void WaylandOutputMode::zwlr_output_mode_v1_preferred() {
    m_preferred = true;
  }

  void WaylandOutputMode::zwlr_output_mode_v1_finished() {
    qDebug() << "WaylandOutputMode::zwlr_output_mode_v1_finished (no longer available):" << getWidth() << "x" << getHeight() << "@" << getRefresh();
    emit modeNoLongerAvailable();
  }
}
