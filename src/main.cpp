#include <QCoreApplication>
#include <iostream>
#include <QDBusConnection>

#include "config/display.hpp"
#include "dbus/BatchSystemService.hpp"
#include "dbus/DisplayService.hpp"
#include "displays/configuration.hpp"
#include "displays/output-manager/WaylandOutputManager.hpp"
#include "dbus/DisplayObjectManager.hpp"

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
  bd::BatchSystemService batchSystemService;

  app.connect(&orchestrator, &bd::WaylandOrchestrator::ready, &bd::DisplayObjectManager::instance(), &bd::DisplayObjectManager::onOutputManagerReady);

  orchestrator.init();

  wl_display_roundtrip(bd::WaylandOrchestrator::instance().getDisplay());

  return app.exec();
}
