#include <QCoreApplication>
#include <iostream>

#include "config/display.hpp"
#include "dbus/DisplayService.hpp"
#include "displays/configuration.hpp"
#include "output-manager/WaylandOutputManager.hpp"

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  qSetMessagePattern("[%{type}] %{if-debug}[%{file}:%{line} %{function}]%{endif}%{message}");
  if (!QDBusConnection::sessionBus().isConnected()) {
    qCritical() << "Cannot connect to the session bus";
    return EXIT_FAILURE;
  }
  qDBusRegisterMetaType<OutputModesList>();
  qDBusRegisterMetaType<OutputDetailsList>();

  bd::DisplayConfig::instance().parseConfig();
  bd::DisplayConfig::instance().debugOutput();
  auto& orchestrator = bd::WaylandOrchestrator::instance();

  app.connect(&orchestrator, &bd::WaylandOrchestrator::orchestratorInitFailed, [](const QString& error) {
    qFatal() << "Failed to initialize Wayland Orchestrator: " << error;
  });

  app.connect(&orchestrator, &bd::WaylandOrchestrator::ready, &bd::DisplayConfig::instance(), &bd::DisplayConfig::apply);

  bd::DisplayService displayService;

  app.connect(&orchestrator, &bd::WaylandOrchestrator::ready, &app, [&displayService]() {
    qInfo() << "Wayland Orchestrator ready";
    qInfo() << "Starting Display DBus Service now";
    if (!QDBusConnection::sessionBus().registerObject(DISPLAY_SERVICE_PATH, displayService.GetAdaptor(), QDBusConnection::ExportAllSlots)) {
      qCritical() << "Failed to register DBus object at path" << DISPLAY_SERVICE_PATH;
      return;
    }
    if (!QDBusConnection::sessionBus().registerService(DISPLAY_SERVICE_NAME)) {
      qCritical() << "Failed to register DBus service" << DISPLAY_SERVICE_NAME;
      return;
    }
  });

  orchestrator.init();

  wl_display_roundtrip(bd::WaylandOrchestrator::instance().getDisplay());

  return app.exec();
}
