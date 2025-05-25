#pragma once
#include "DisplayAdaptorGen.h"
#include "DisplaySchemaTypes.hpp"

#define DISPLAY_SERVICE_NAME "org.buddiesofbudgie.BudgieDaemonX.Displays"
#define DISPLAY_SERVICE_PATH "/org/buddiesofbudgie/BudgieDaemonX/Displays"

namespace bd {
  class DisplayService : public QObject {
      Q_OBJECT

    public:
      explicit DisplayService(QObject* parent = nullptr);
      DisplaysAdaptor* GetAdaptor();

    public slots:
      OutputModesList   GetAvailableModes(const QString& identifier);
      QStringList       GetAvailableOutputs();
      OutputDetailsList GetOutputDetails(const QString& identifier);
      void              SetCurrentMode(const QString& identifier, int width, int height, int refresh, bool preferred);
      void              SetOutputEnabled(const QString& identifier, bool enabled);
      void              SetOutputPosition(const QString& identifier, int x, int y);

    private:
      DisplaysAdaptor* m_adaptor;
  };
}
