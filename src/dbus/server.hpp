#pragma once
#include <sdbus-c++/AdaptorInterfaces.h>
#include <sdbus-c++/IConnection.h>
#include <sdbus-c++/sdbus-c++.h>

#include <vector>

#include "daemon-server.hpp"

class DaemonServer : public sdbus::AdaptorInterfaces<org::buddiesofbudgie::BudgieDaemonX::Displays_adaptor> {
  public:
    DaemonServer(sdbus::IConnection& connection, sdbus::ObjectPath objectPath);
    ~DaemonServer();

    void GetAvailableOutputs(sdbus::Result<std::vector<std::string>>&& result) override;
    void GetCurrentOutputDetails(sdbus::Result<std::string, int32_t, int32_t, int32_t, int32_t, double, double, bool, bool>&& result, std::string serial)
        override;
    void GetAvailableModes(sdbus::Result<std::vector<sdbus::Struct<int32_t, int32_t, double>>>&& result, std::string serial) override;
    void SetCurrentMode(sdbus::Result<>&& result, std::string serial, int32_t width, int32_t height, double refresh, bool preferred) override;
    void SetOutputPosition(sdbus::Result<>&& result, std::string serial, int32_t x, int32_t y) override;
    void SetOutputEnabled(sdbus::Result<>&& result, std::string serial, bool enabled) override;
};
