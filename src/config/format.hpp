#pragma once

#include <array>
#include <string>
#include <vector>

struct DisplayConfig;
struct DisplayGlobalPreferences;
struct DisplayGrouping;
struct DisplayGroupOutputConfig;

enum DisplayRelativePosition {
  none = 0,
  left,
  right,
  above,
  below,
};

struct DisplayGlobalPreferences {
    DisplayRelativePosition automatic_attach_outputs_relative_position;
};

struct DisplayGrouping {
    std::string                           name;
    bool                                  preferred;
    std::vector<std::string>              output_serials;
    std::string                           primary_output;
    std::vector<DisplayGroupOutputConfig> configs;
};

struct DisplayGroupOutputConfig {
    std::string        serial;
    int                width;
    int                height;
    int                refresh;
    std::array<int, 2> position;
    double             scale;
    int                rotation;
    bool               adaptive_sync;
    bool               disabled;
};
