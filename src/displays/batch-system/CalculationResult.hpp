#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QMap>
#include <QRect>
#include "OutputTargetState.hpp"

namespace bd {
    class CalculationResult : public QObject {
        Q_OBJECT

    public:
        CalculationResult(QObject *parent = nullptr);

        QSharedPointer<QRect> getGlobalSpace() const;
        QMap<QString, QSharedPointer<OutputTargetState>> getOutputStates() const;

        void setOutputState(QString serial, QSharedPointer<OutputTargetState> output_state);

    private:
        QSharedPointer<QRect> m_global_space;
        QMap<QString, QSharedPointer<OutputTargetState>> m_output_states;
    };
}