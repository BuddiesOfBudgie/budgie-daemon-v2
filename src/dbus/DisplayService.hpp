#pragma once
#include "DisplayAdaptorGen.h"

#define DISPLAY_SERVICE_NAME "org.buddiesofbudgie.BudgieDaemonX.Displays"
#define DISPLAY_SERVICE_PATH "/org/buddiesofbudgie/BudgieDaemonX/Displays"

namespace bd {
  class DisplayService : public QObject {
      Q_OBJECT

    public:
      explicit DisplayService(QObject* parent = nullptr);
      DisplaysAdaptor* GetAdaptor();

    public slots:
      QVariantList GetAvailableModes(const QString& serial);
      QStringList  GetAvailableOutputs();
      QString
           GetCurrentOutputDetails(const QString& serial, int& width, int& height, int& x, int& y, double& scale, int& refresh, bool& preferred, bool& enabled);
      void SetCurrentMode(const QString& serial, int width, int height, double refresh, bool preferred);
      void SetOutputEnabled(const QString& serial, bool enabled);
      void SetOutputPosition(const QString& serial, int x, int y);

    private:
      DisplaysAdaptor* m_adaptor;
  };
}
