#pragma once
#include <QMap>
#include <QObject>
#include <QSharedPointer>

#include "OutputModeService.hpp"
#include "OutputService.hpp"

namespace bd {
  class DisplayObjectManager : public QObject {
      Q_OBJECT
    public:
      static DisplayObjectManager& instance();

    public slots:
      void onOutputManagerReady();

    private:
      explicit DisplayObjectManager(QObject* parent = nullptr);
      Q_DISABLE_COPY(DisplayObjectManager)

      QMap<QString, OutputService*>     m_outputServices;
      QMap<QString, OutputModeService*> m_modeServices;
  };
}
