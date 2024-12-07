find_package(Qt6 REQUIRED COMPONENTS Core DBus WaylandClient)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH} ${ECM_FIND_MODULE_DIR} ${CMAKE_SOURCE_DIR}/cmake ${CMAKE_BINARY_DIR}/includes)

find_package(Wayland REQUIRED)
find_package(QtWaylandScanner REQUIRED)

include(KDEInstallDirs)
include(ECMConfiguredInstall)

qt_standard_project_setup()

set(SRCS
        displays/configuration.cpp
        config/display.cpp
        config/utils.cpp
        dbus/DisplayAdaptorGen.cpp
        dbus/DisplayService.cpp
        displays/output-manager/WaylandOutputManager.cpp
        main.cpp
        displays/batch-system/DisplayConfigurationBatchSystem.cpp
        displays/batch-system/DisplayConfigurationBatchSystem.hpp
        displays/batch-system/DisplayConfigurationBatch.cpp
        displays/batch-system/DisplayConfigurationCalculation.cpp
        displays/batch-system/DisplayConfigurationBatchAction.cpp
)

add_library(WaylandProtocols_xml OBJECT)
set_property(TARGET WaylandProtocols_xml PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(WaylandProtocols_xml Qt6::Core Wayland::Client)
set_target_properties(WaylandProtocols_xml PROPERTIES LINKER_LANGUAGE C)

ecm_add_qtwayland_client_protocol(WaylandProtocols_xml
        PRIVATE_CODE
        PROTOCOL protocols/wlr-output-management-unstable-v1.xml
        BASENAME wlr-output-management-unstable-v1
)

add_executable(org.buddiesofbudgie.BudgieDaemonV2 ${SRCS} ${WaylandProtocols_xml})
target_include_directories(org.buddiesofbudgie.BudgieDaemonV2 PUBLIC ${CMAKE_CURRENT_BINARY_DIR} config dbus displays includes)
target_link_libraries(org.buddiesofbudgie.BudgieDaemonV2 PRIVATE Qt6::Core Qt6::DBus Wayland::Client WaylandProtocols_xml)
install(
        FILES dbus/org.buddiesofbudgie.BudgieDaemonX.conf
        DESTINATION ${CMAKE_INSTALL_DATADIR}/dbus-1/system.d
)

install(TARGETS org.buddiesofbudgie.BudgieDaemonV2 DESTINATION ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}/)
install(
        FILES data/labwc/autostart
        DESTINATION /etc/xdg/labwc
)

#ecm_install_configured_files(INPUT data/org.buddiesofbudgie.BudgieDaemonV2.desktop.in DESTINATION ${KDE_INSTALL_AUTOSTARTDIR})
#ecm_install_configured_files(
#        INPUT data/org.buddiesofbudgie.BudgieDaemonV2.service.in
#        DESTINATION ${KDE_INSTALL_SYSTEMDUSERUNITDIR}
#)