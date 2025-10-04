#include "WaylandOutputManager.hpp"

#include <KWayland/Client/registry.h>
#include <qcryptographichash.h>

#include <cstdint>
#include <cstring>

namespace bd {
  WaylandOrchestrator::WaylandOrchestrator(QObject* parent)
      : QObject(parent), m_registry(nullptr), m_display(nullptr), m_manager(nullptr), m_has_serial(false), m_serial(0), m_has_initted(false) {}

  WaylandOrchestrator& WaylandOrchestrator::instance() {
    static WaylandOrchestrator _instance(nullptr);
    return _instance;
  }

  void WaylandOrchestrator::init() {
    auto display = wl_display_connect(nullptr);
    if (display == nullptr) {
      emit orchestratorInitFailed(QString("Failed to connect to the Wayland display"));
      return;
    }

    m_display = display;

    m_registry = new KWayland::Client::Registry();
    m_registry->create(m_display);  // Create using our existing display connection

    if (!m_registry->isValid()) {
      wl_display_disconnect(m_display);
      m_registry->release();
      emit orchestratorInitFailed(QString("Failed to create our KWayland registry and manage it"));
      return;
    }

    connect(m_registry, &KWayland::Client::Registry::interfaceAnnounced, this, [this](const QByteArray& interface, quint32 name, quint32 version) {
      if (std::strcmp(interface, QtWayland::zwlr_output_manager_v1::interface()->name) == 0) {
        auto manager = new WaylandOutputManager(nullptr, m_registry, name, QtWayland::zwlr_output_manager_v1::interface()->version);
        connect(manager, &WaylandOutputManager::done, this, &WaylandOrchestrator::outputManagerDone);
        m_manager = QSharedPointer<WaylandOutputManager>(manager);
      }
    });

    m_registry->setup();

    if (wl_display_roundtrip(m_display) < 0) {
      emit orchestratorInitFailed(QString("Failed to perform roundtrip on Wayland display"));
      return;
    }

    wl_display_dispatch(m_display);
  }

  QSharedPointer<WaylandOutputManager> WaylandOrchestrator::getManager() {
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
    if (!m_has_initted) emit ready();  // Haven't done our first init, emit that we are ready
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
    auto head = new WaylandOutputMetaHead(nullptr, m_registry);
    qInfo() << "WaylandOutputManager::zwlr_output_manager_v1_head with id:" << head->getIdentifier() << ", description:" << head->getDescription();

    connect(head, &WaylandOutputMetaHead::headAvailable, this, [this, head]() {
        qDebug() << "Head available for output: " << head->getIdentifier();
        bool headAlreadyExists = false;
        for (const auto& existingHead : m_heads) {
            qDebug() << "Checking existing head: " << existingHead->getIdentifier();
          if (existingHead->getIdentifier() == head->getIdentifier()) {
            qDebug() << "Head already exists for output: " << head->getIdentifier();
            headAlreadyExists = true;
            existingHead->setHead(head->getWlrHead().value());
            break;
          }
        }

        if (!headAlreadyExists) {
            qDebug() << "Adding new head for output: " << head->getIdentifier();
            m_heads.append(QSharedPointer<WaylandOutputMetaHead>(head));
        }
    });

    head->setHead(wlr_head);
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
  QList<QSharedPointer<WaylandOutputConfigurationHead>> WaylandOutputManager::applyNoOpConfigurationForNonSpecifiedHeads(
      WaylandOutputConfiguration* config,
      const QStringList&          serials) {
    auto configHeads = QList<QSharedPointer<WaylandOutputConfigurationHead>> {};
    qDebug() << "Applying no-op configuration for non-specified heads. Ignoring:" << serials.join(", ");

    for (const auto& o : m_heads) {
      qDebug() << "Checking head " << o->getIdentifier() << ": " << o->getDescription();
      // Skip the output for the serial we are changing
      if (serials.contains(o->getIdentifier())) {
        qDebug() << "Skipping head " << o->getIdentifier();
        continue;
      }

      if (o->isEnabled()) {
        qDebug() << "Ensuring head " << o->getIdentifier() << " is enabled";
        auto head = config->enable(o.data());
        if (!head) {
          qWarning() << "Failed to enable head " << o->getIdentifier() << ", wlr_head is not available";
          continue;
        }
        configHeads.append(head);
      } else {
        qDebug() << "Ensuring head " << o->getIdentifier() << " is disabled";
        config->disable(o.data());
      }
    }

    return configHeads;
  }

  QSharedPointer<WaylandOutputConfiguration> WaylandOutputManager::configure() {
    auto wlr_output_configuration = create_configuration(m_serial);
    auto config                   = new WaylandOutputConfiguration(nullptr, wlr_output_configuration);
    connect(config, &WaylandOutputConfiguration::cancelled, this, [this, config]() {
      qDebug() << "Configuration cancelled";
      // config->deleteLater();
    });
    connect(config, &WaylandOutputConfiguration::succeeded, this, [this, config]() {
      qDebug() << "Configuration succeeded";
      // config->deleteLater();
    });
    connect(config, &WaylandOutputConfiguration::failed, this, [this, config]() {
      qDebug() << "Configuration failed";
      // config->deleteLater();
    });
    return QSharedPointer<WaylandOutputConfiguration>(config);
  }

  QList<QSharedPointer<WaylandOutputMetaHead>> WaylandOutputManager::getHeads() {
    return m_heads;
  }

  QSharedPointer<WaylandOutputMetaHead> WaylandOutputManager::getOutputHead(const QString& str) {
    for (auto head : m_heads) {
      if (head->getIdentifier() == str) { return head; }
    }

    return nullptr;
  }

  uint32_t WaylandOutputManager::getSerial() {
    return m_serial;
  }

  uint32_t WaylandOutputManager::getVersion() {
    return m_version;
  }

  // Output Mode Configuration

  WaylandOutputConfiguration::WaylandOutputConfiguration(QObject* parent, ::zwlr_output_configuration_v1* config)
      : QObject(parent), zwlr_output_configuration_v1(config) {}

  QSharedPointer<WaylandOutputConfigurationHead> WaylandOutputConfiguration::enable(WaylandOutputMetaHead* head) {
    auto wlrHeadOpt = head->getWlrHead();
    if (!wlrHeadOpt.has_value()) {
      qWarning() << "Tried to enable head, but wlr_head is not available";
      return nullptr;
    }
    auto zwlr_config_head = enable_head(wlrHeadOpt.value());
    auto config_head      = new WaylandOutputConfigurationHead(nullptr, head, zwlr_config_head);
    return QSharedPointer<WaylandOutputConfigurationHead>(config_head);
  }

  void WaylandOutputConfiguration::applySelf() {
    apply();
    wl_display_roundtrip(bd::WaylandOrchestrator::instance().getDisplay());
  }

  void WaylandOutputConfiguration::release() {
    destroy();
  }

  void WaylandOutputConfiguration::disable(WaylandOutputMetaHead* head) {
    auto wlrHeadOpt = head->getWlrHead();
    if (!wlrHeadOpt.has_value()) {
      qWarning() << "Tried to disable head, but wlr_head is not available";
      return;
    }
    disable_head(wlrHeadOpt.value());
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

  WaylandOutputConfigurationHead::WaylandOutputConfigurationHead(QObject* parent, WaylandOutputMetaHead* head, ::zwlr_output_configuration_head_v1* wlr_head)
      : QObject(parent), zwlr_output_configuration_head_v1(wlr_head), m_head(head) {}

  WaylandOutputMetaHead* WaylandOutputConfigurationHead::getHead() {
    return m_head;
  }

  void WaylandOutputConfigurationHead::release() {
    // TODO: change from being a no-op for now
  }

  void WaylandOutputConfigurationHead::setAdaptiveSync(uint32_t state) {
    set_adaptive_sync(state);
  }

  void WaylandOutputConfigurationHead::setMode(WaylandOutputMetaMode* mode) {
    auto wlrModeOpt = mode->getWlrMode();
    if (wlrModeOpt == nullptr || (wlrModeOpt != nullptr && !wlrModeOpt.has_value())) {
      qWarning() << "Tried to set mode on configuration head, but mode is not available";
      return;
    }
    set_mode(const_cast<::zwlr_output_mode_v1*>(wlrModeOpt.value()));
  }

  void WaylandOutputConfigurationHead::setCustomMode(signed int width, signed int height, qulonglong refresh) {
    set_custom_mode(width, height, static_cast<int32_t>(refresh));
  }

  void WaylandOutputConfigurationHead::setPosition(int32_t x, int32_t y) {
    set_position(x, y);
  }

  void WaylandOutputConfigurationHead::setScale(double scale) {
    set_scale(wl_fixed_from_double(scale));
  }

  void WaylandOutputConfigurationHead::setTransform(quint8 transform) {
    set_transform(static_cast<int32_t>(transform));
  }
}
