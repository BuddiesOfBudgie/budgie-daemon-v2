# SPDX-FileCopyrightText: Budgie Desktop Developers
#
# SPDX-License-Identifier: MPL-2.0

add_library(WaylandProtocols_xml OBJECT)
set_property(TARGET WaylandProtocols_xml PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(WaylandProtocols_xml PUBLIC Qt::Core Wayland::Client Plasma::KWaylandClient)
set_target_properties(WaylandProtocols_xml PROPERTIES LINKER_LANGUAGE C)

ecm_add_qtwayland_client_protocol(
  WaylandProtocols_xml PRIVATE_CODE PROTOCOL
  protocols/wlr-output-management-unstable-v1.xml BASENAME
  wlr-output-management-unstable-v1)

add_library(
  budgie-daemon-v2 STATIC
  config/display.cpp
  config/display.hpp
  config/format.hpp
  config/utils.cpp
  config/utils.hpp
  dbus/DisplayAdaptorGen.cpp
  dbus/DisplayAdaptorGen.h
  dbus/DisplaySchemaTypes.hpp
  dbus/DisplayService.cpp
  dbus/DisplayService.hpp
  displays/output-manager/head/enums.hpp
  displays/output-manager/head/WaylandOutputHead.cpp
  displays/output-manager/head/WaylandOutputHead.hpp
  displays/output-manager/head/WaylandOutputMetaHead.cpp
  displays/output-manager/head/WaylandOutputMetaHead.hpp
  displays/output-manager/mode/enums.hpp
  displays/output-manager/mode/WaylandOutputMetaMode.cpp
  displays/output-manager/mode/WaylandOutputMetaMode.hpp
  displays/output-manager/mode/WaylandOutputMode.cpp
  displays/output-manager/mode/WaylandOutputMode.hpp
  displays/output-manager/WaylandOutputManager.cpp
  displays/output-manager/WaylandOutputManager.hpp
  displays/configuration.cpp
  displays/configuration.hpp
  sys/SysInfo.cpp
  sys/SysInfo.hpp
)

add_executable(budgie-daemon-v2-app main.cpp)
target_include_directories(
  budgie-daemon-v2-app PRIVATE ${CMAKE_BINARY_DIR}
                               ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(budgie-daemon-v2-app PRIVATE budgie-daemon-v2)

target_include_directories(
  budgie-daemon-v2
  PRIVATE ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/includes
          ${CMAKE_CURRENT_SOURCE_DIR}/config ${CMAKE_CURRENT_SOURCE_DIR}/dbus
          ${CMAKE_CURRENT_SOURCE_DIR}/displays ${CMAKE_CURRENT_SOURCE_DIR}/sys)

target_link_libraries(
  budgie-daemon-v2 PUBLIC Qt::Core Qt::DBus Qt::WaylandClient Wayland::Client
                          WaylandProtocols_xml)

set_target_properties(
  budgie-daemon-v2-app PROPERTIES OUTPUT_NAME
                                  "org.buddiesofbudgie.BudgieDaemonV2")

install(TARGETS budgie-daemon-v2-app ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(FILES dbus/org.buddiesofbudgie.BudgieDaemonX.conf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d)

if(INSTALL_SERVICE_FILES)
    if(INSTALL_LABWC)
        install(FILES data/labwc/autostart DESTINATION /etc/xdg/labwc)
    else()
        ecm_install_configured_files(
      INPUT data/org.buddiesofbudgie.BudgieDaemonV2.desktop.in DESTINATION
      ${KDE_INSTALL_AUTOSTARTDIR})
        ecm_install_configured_files(
      INPUT data/org.buddiesofbudgie.BudgieDaemonV2.service.in DESTINATION
      ${KDE_INSTALL_SYSTEMDUSERUNITDIR})
    endif()
endif()
