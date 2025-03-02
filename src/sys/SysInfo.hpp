#pragma once
#include <QObject>

namespace bd {

  class SysInfo : public QObject {
      Q_OBJECT

    public:
      SysInfo(QObject* parent);
      static SysInfo& instance();
      static SysInfo* create() { return &instance(); }

      QString getMachineId();

    private:
      QString m_machine_id;
  };

}  // bd
