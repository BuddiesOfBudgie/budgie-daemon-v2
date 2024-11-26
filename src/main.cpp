#include <sdbus-c++/sdbus-c++.h>

#include <QCoreApplication>
#include <iostream>

#include "config/display.hpp"
#include "dbus/server.hpp"
#include "displays/configuration.hpp"
#include "output-manager/WaylandOutputManager.hpp"

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  bd::DisplayConfig::create();
  auto& orchestrator = bd::WaylandOrchestrator::instance();

  app.connect(&orchestrator, &bd::WaylandOrchestrator::orchestratorInitFailed, [&app](const QString& error) {
    std::cerr << "Failed to initialize Wayland Orchestrator: " << error.toStdString() << std::endl;
    app.quit();
  });

  app.connect(&orchestrator, &bd::WaylandOrchestrator::ready, &app, [&app]() {
    std::cout << "Wayland Orchestrator ready" << std::endl;
    std::cout << "Starting dbus server now" << std::endl;
    auto              connection = sdbus::createSystemBusConnection("org.buddiesofbudgie.BudgieDaemonX");
    sdbus::ObjectPath objectPath {"/org/buddiesofbudgie/BudgieDaemonX"};
    DaemonServer      server {*connection, objectPath};

    connection->enterEventLoop();
  });

  orchestrator.init();

  wl_display_roundtrip(bd::WaylandOrchestrator::instance().getDisplay());

  //  TODO: Configuration initialization

  return app.exec();
}
