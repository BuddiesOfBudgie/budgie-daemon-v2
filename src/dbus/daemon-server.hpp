
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_daemon_server_h__adaptor__H__
#define __sdbuscpp__dbus_daemon_server_h__adaptor__H__

#include <sdbus-c++/sdbus-c++.h>

#include <string>
#include <tuple>

namespace org {
  namespace buddiesofbudgie {
    namespace BudgieDaemonX {

      class Displays_adaptor {
        public:
          static constexpr const char* INTERFACE_NAME = "org.buddiesofbudgie.BudgieDaemonX.Displays";

        protected:
          Displays_adaptor(sdbus::IObject& object) : object_(&object) {
            object_->registerMethod("GetAvailableOutputs")
                .onInterface(INTERFACE_NAME)
                .withOutputParamNames("outputs")
                .implementedAs([this](sdbus::Result<std::vector<std::string>>&& result) { this->GetAvailableOutputs(std::move(result)); });
            object_->registerMethod("GetCurrentOutputDetails")
                .onInterface(INTERFACE_NAME)
                .withInputParamNames("serial")
                .withOutputParamNames("name", "width", "height", "x", "y", "scale", "refresh", "preferred", "enabled")
                .implementedAs([this](sdbus::Result<std::string, int32_t, int32_t, int32_t, int32_t, double, double, bool, bool>&& result, std::string serial) {
                  this->GetCurrentOutputDetails(std::move(result), std::move(serial));
                });
            object_->registerMethod("GetAvailableModes")
                .onInterface(INTERFACE_NAME)
                .withInputParamNames("serial")
                .withOutputParamNames("modes")
                .implementedAs([this](sdbus::Result<std::vector<sdbus::Struct<int32_t, int32_t, double>>>&& result, std::string serial) {
                  this->GetAvailableModes(std::move(result), std::move(serial));
                });
            object_->registerMethod("SetCurrentMode")
                .onInterface(INTERFACE_NAME)
                .withInputParamNames("serial", "width", "height", "refresh", "preferred")
                .implementedAs([this](sdbus::Result<>&& result, std::string serial, int32_t width, int32_t height, double refresh, bool preferred) {
                  this->SetCurrentMode(std::move(result), std::move(serial), std::move(width), std::move(height), std::move(refresh), std::move(preferred));
                });
            object_->registerMethod("SetOutputPosition")
                .onInterface(INTERFACE_NAME)
                .withInputParamNames("serial", "x", "y")
                .implementedAs([this](sdbus::Result<>&& result, std::string serial, int32_t x, int32_t y) {
                  this->SetOutputPosition(std::move(result), std::move(serial), std::move(x), std::move(y));
                });
            object_->registerMethod("SetOutputEnabled")
                .onInterface(INTERFACE_NAME)
                .withInputParamNames("serial", "enabled")
                .implementedAs([this](sdbus::Result<>&& result, std::string serial, bool enabled) {
                  this->SetOutputEnabled(std::move(result), std::move(serial), std::move(enabled));
                });
          }

          Displays_adaptor(const Displays_adaptor&)            = delete;
          Displays_adaptor& operator=(const Displays_adaptor&) = delete;
          Displays_adaptor(Displays_adaptor&&)                 = default;
          Displays_adaptor& operator=(Displays_adaptor&&)      = default;

          ~Displays_adaptor() = default;

        private:
          virtual void GetAvailableOutputs(sdbus::Result<std::vector<std::string>>&& result) = 0;
          virtual void GetCurrentOutputDetails(
              sdbus::Result<std::string, int32_t, int32_t, int32_t, int32_t, double, double, bool, bool>&& result,
              std::string                                                                                  serial)                                                                                                                  = 0;
          virtual void GetAvailableModes(sdbus::Result<std::vector<sdbus::Struct<int32_t, int32_t, double>>>&& result, std::string serial)         = 0;
          virtual void SetCurrentMode(sdbus::Result<>&& result, std::string serial, int32_t width, int32_t height, double refresh, bool preferred) = 0;
          virtual void SetOutputPosition(sdbus::Result<>&& result, std::string serial, int32_t x, int32_t y)                                       = 0;
          virtual void SetOutputEnabled(sdbus::Result<>&& result, std::string serial, bool enabled)                                                = 0;

        private:
          sdbus::IObject* object_;
      };

    }
  }
}  // namespaces

#endif
