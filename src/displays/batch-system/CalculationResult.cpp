#include "CalculationResult.hpp"

namespace bd {
    CalculationResult::CalculationResult(QObject *parent) : QObject(parent),
        m_global_space(QSharedPointer<QRect>(new QRect(0, 0, 0, 0))),
        m_output_states(QMap<QString, QSharedPointer<OutputTargetState>>()) {
    }

    QSharedPointer<QRect> CalculationResult::getGlobalSpace() const {
        return m_global_space;
    }

    QMap<QString, QSharedPointer<OutputTargetState>> CalculationResult::getOutputStates() const {
        return m_output_states;
    }

    void CalculationResult::setOutputState(QString serial, QSharedPointer<OutputTargetState> output_state) {
        m_output_states.insert(serial, output_state);
    }
}