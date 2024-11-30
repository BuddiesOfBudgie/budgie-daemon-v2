#pragma once

#include <QList>
#include <QString>
#include <array>
#include <string>
#include <vector>

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
