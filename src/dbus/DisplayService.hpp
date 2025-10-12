#pragma once
#include "generated/DisplayAdaptorGen.h"

#define DISPLAY_SERVICE_NAME "org.buddiesofbudgie.BudgieDaemon.Displays"
#define DISPLAY_SERVICE_PATH "/org/buddiesofbudgie/BudgieDaemon/Displays"

namespace bd {
  class DisplayService : public QObject {
      Q_OBJECT

    public:
      explicit DisplayService(QObject* parent = nullptr);
      static DisplayService& instance();
      static DisplayService* create() { return &instance(); }
      DisplaysAdaptor*       GetAdaptor();

    public slots:
      QStringList GetAvailableOutputs();
      QString     GetPrimaryOutput();
      QVariantMap GetPrimaryOutputRect();

    private:
      DisplaysAdaptor* m_adaptor;
  };
}
