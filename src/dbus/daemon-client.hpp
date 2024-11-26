
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_daemon_client_h__proxy__H__
#define __sdbuscpp__dbus_daemon_client_h__proxy__H__

#include <sdbus-c++/sdbus-c++.h>

#include <string>
#include <tuple>

namespace org {
  namespace buddiesofbudgie {
    namespace BudgieDaemonX {

      class Displays_proxy {
        public:
          static constexpr const char* INTERFACE_NAME = "org.buddiesofbudgie.BudgieDaemonX.Displays";

        protected:
          Displays_proxy(sdbus::IProxy& proxy) : proxy_(&proxy) {}

          Displays_proxy(const Displays_proxy&)            = delete;
          Displays_proxy& operator=(const Displays_proxy&) = delete;
          Displays_proxy(Displays_proxy&&)                 = default;
          Displays_proxy& operator=(Displays_proxy&&)      = default;

          ~Displays_proxy() = default;

          virtual void onGetAvailableOutputsReply(const std::vector<std::string>& outputs, const sdbus::Error* error) = 0;
          virtual void onGetCurrentOutputDetailsReply(
              const std::string&  name,
              const int32_t&      width,
              const int32_t&      height,
              const int32_t&      x,
              const int32_t&      y,
              const double&       scale,
              const double&       refresh,
              const bool&         preferred,
              const bool&         enabled,
              const sdbus::Error* error)                                                                                                      = 0;
          virtual void onGetAvailableModesReply(const std::vector<sdbus::Struct<int32_t, int32_t, double>>& modes, const sdbus::Error* error) = 0;
          virtual void onSetCurrentModeReply(const sdbus::Error* error)                                                                       = 0;
          virtual void onSetOutputPositionReply(const sdbus::Error* error)                                                                    = 0;
          virtual void onSetOutputEnabledReply(const sdbus::Error* error)                                                                     = 0;

        public:
          sdbus::PendingAsyncCall GetAvailableOutputs() {
            return proxy_->callMethodAsync("GetAvailableOutputs")
                .onInterface(INTERFACE_NAME)
                .uponReplyInvoke(
                    [this](const sdbus::Error* error, const std::vector<std::string>& outputs) { this->onGetAvailableOutputsReply(outputs, error); });
          }

          sdbus::PendingAsyncCall GetCurrentOutputDetails(const std::string& serial) {
            return proxy_->callMethodAsync("GetCurrentOutputDetails")
                .onInterface(INTERFACE_NAME)
                .withArguments(serial)
                .uponReplyInvoke([this](
                                     const sdbus::Error* error, const std::string& name, const int32_t& width, const int32_t& height, const int32_t& x,
                                     const int32_t& y, const double& scale, const double& refresh, const bool& preferred, const bool& enabled) {
                  this->onGetCurrentOutputDetailsReply(name, width, height, x, y, scale, refresh, preferred, enabled, error);
                });
          }

          sdbus::PendingAsyncCall GetAvailableModes(const std::string& serial) {
            return proxy_->callMethodAsync("GetAvailableModes")
                .onInterface(INTERFACE_NAME)
                .withArguments(serial)
                .uponReplyInvoke([this](const sdbus::Error* error, const std::vector<sdbus::Struct<int32_t, int32_t, double>>& modes) {
                  this->onGetAvailableModesReply(modes, error);
                });
          }

          sdbus::PendingAsyncCall
          SetCurrentMode(const std::string& serial, const int32_t& width, const int32_t& height, const double& refresh, const bool& preferred) {
            return proxy_->callMethodAsync("SetCurrentMode")
                .onInterface(INTERFACE_NAME)
                .withArguments(serial, width, height, refresh, preferred)
                .uponReplyInvoke([this](const sdbus::Error* error) { this->onSetCurrentModeReply(error); });
          }

          sdbus::PendingAsyncCall SetOutputPosition(const std::string& serial, const int32_t& x, const int32_t& y) {
            return proxy_->callMethodAsync("SetOutputPosition")
                .onInterface(INTERFACE_NAME)
                .withArguments(serial, x, y)
                .uponReplyInvoke([this](const sdbus::Error* error) { this->onSetOutputPositionReply(error); });
          }

          sdbus::PendingAsyncCall SetOutputEnabled(const std::string& serial, const bool& enabled) {
            return proxy_->callMethodAsync("SetOutputEnabled")
                .onInterface(INTERFACE_NAME)
                .withArguments(serial, enabled)
                .uponReplyInvoke([this](const sdbus::Error* error) { this->onSetOutputEnabledReply(error); });
          }

        private:
          sdbus::IProxy* proxy_;
      };

    }
  }
}  // namespaces

#endif
