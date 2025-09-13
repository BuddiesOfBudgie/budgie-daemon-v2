#pragma once

namespace bd {
    enum class ConfigurationActionType {
        SetAdaptiveSync,
        SetGamma,
        SetMirrorOf,
        SetMode,
        SetOnOff,
        SetPrimary,
        SetPositionAnchor,
        SetScale,
        SetTransform,
    };

    enum class ConfigurationVerticalAnchor {
        NoVerticalAnchor, // No vertical anchor set
        Above, // Bottom edge of serial is at the top edge of relative
        Top, // Top edge of serial is at the top edge of relative
        Middle, // Middle of serial is at the middle of relative
        Bottom, // Bottom edge of serial is at the bottom edge of relative
        Below // Top edge of serial is at the bottom edge of relative
    };

    enum class ConfigurationHorizontalAnchor {
        NoHorizontalAnchor, // No horizontal anchor set
        Left, // Left edge of serial is at the left edge of relative
        Right, // Right edge of serial is at the right edge of relative
        Center, // Center of serial is at the center of relative
    };
}