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

  app.connect(&bd::WaylandOrchestrator::instance(), &bd::WaylandOrchestrator::orchestratorInitFailed, [&app](const QString& error) {
    std::cerr << "Failed to initialize Wayland Orchestrator: " << error.toStdString() << std::endl;
    app.quit();
  });

  app.connect(
      &bd::WaylandOrchestrator::instance(), &bd::WaylandOrchestrator::orchestratorReady, [&app]() { std::cout << "Wayland Orchestrator ready" << std::endl; });

  wl_display_roundtrip(bd::WaylandOrchestrator::instance().getDisplay());
  wl_display_dispatch(bd::WaylandOrchestrator::instance().getDisplay());
  
  //  TODO: Configuration initialization
  //  TODO: WaylandOutputManager setup
  //  TODO: WaylandOutputManager, Configuration, DBus Server hookup
  //  TODO: WaylandOutputManager init

  std::cout << "Gonna start creating dbus server now" << std::endl;
  auto              connection = sdbus::createSystemBusConnection("org.buddiesofbudgie.BudgieDaemonX");
  sdbus::ObjectPath objectPath {"/org/buddiesofbudgie/BudgieDaemonX"};
  DaemonServer      server {*connection, objectPath};

  connection->enterEventLoop();

  return app.exec();
}
