#include "SysInfo.hpp"

#include <QSysInfo>

namespace bd {

  SysInfo::SysInfo(QObject* parent) : QObject(parent) {
    m_machine_id = QString {QSysInfo::machineUniqueId()};
  }

  SysInfo& SysInfo::instance() {
    static SysInfo _instance(nullptr);
    return _instance;
  }

  QString SysInfo::getMachineId() {
    return m_machine_id;
  }
}  // bd
