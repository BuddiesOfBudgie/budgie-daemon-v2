#pragma once
#include "OutputModeAdaptorGen.h"
#include <QSharedPointer>
#include "displays/output-manager/mode/WaylandOutputMetaMode.hpp"

namespace bd {
class OutputModeService : public QObject {
    Q_OBJECT
    Q_PROPERTY(int Width READ Width)
    Q_PROPERTY(int Height READ Height)
    Q_PROPERTY(double RefreshRate READ RefreshRate)
    Q_PROPERTY(bool Preferred READ Preferred)
    Q_PROPERTY(bool Current READ Current)
public:
    OutputModeService(QSharedPointer<WaylandOutputMetaMode> mode, const QString& outputId, QObject* parent = nullptr);
    ~OutputModeService();

    // Property getters
    int Width() const;
    int Height() const;
    double RefreshRate() const;
    bool Preferred() const;
    bool Current() const;

    // D-Bus methods
    Q_INVOKABLE void Activate();
    Q_INVOKABLE QVariantMap GetModeInfo();

private:
    QSharedPointer<WaylandOutputMetaMode> m_mode;
    OutputModeAdaptor* m_adaptor;
    QString m_outputId;
    bool isCurrentMode() const;
};
}
