#include "WaylandOutputMode.hpp"

namespace bd {
    WaylandOutputMode::WaylandOutputMode(::zwlr_output_mode_v1* mode) : zwlr_output_mode_v1(mode) {}

    std::optional<::zwlr_output_mode_v1*> WaylandOutputMode::getWlrMode() {
        if (isInitialized() && object()) {
            return std::make_optional(object());
        }
        return std::nullopt;
    }

  void WaylandOutputMode::zwlr_output_mode_v1_size(int32_t width, int32_t height) {
    emit propertyChanged(WaylandOutputMetaModeProperty::Size, QVariant {QSize(width, height)});
  }

  void WaylandOutputMode::zwlr_output_mode_v1_refresh(int32_t refresh) {
    auto val = QVariant::fromValue(refresh);
    emit propertyChanged(WaylandOutputMetaModeProperty::Refresh, val);
  }

  void WaylandOutputMode::zwlr_output_mode_v1_preferred() {
    emit propertyChanged(WaylandOutputMetaModeProperty::Preferred, QVariant::fromValue(true));
  }

//  void WaylandOutputMode::zwlr_output_mode_v1_finished() {
//    emit modeFinished();
//  }
}
