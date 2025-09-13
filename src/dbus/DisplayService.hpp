#pragma once
#include "generated/DisplayAdaptorGen.h"

#define DISPLAY_SERVICE_NAME "org.buddiesofbudgie.BudgieDaemonX.Displays"
#define DISPLAY_SERVICE_PATH "/org/buddiesofbudgie/BudgieDaemonX/Displays"

namespace bd {
  class DisplayService : public QObject {
      Q_OBJECT

    public:
      explicit DisplayService(QObject* parent = nullptr);
      static DisplayService& instance();
      static DisplayService* create() { return &instance(); }
      DisplaysAdaptor* GetAdaptor();

    public slots:
      QStringList GetAvailableOutputs();

    private:
      DisplaysAdaptor* m_adaptor;
  };
}
