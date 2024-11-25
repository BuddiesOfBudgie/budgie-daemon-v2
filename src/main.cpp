#include <sdbus-c++/sdbus-c++.h>

#include <QCoreApplication>
#include <iostream>

#include "config/display.hpp"
#include "dbus/server.hpp"
#include "displays/configuration.hpp"

int main(int argc, char* argv[]) {
  QCoreApplication app(argc, argv);
  bd::DisplayConfig::create();

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
