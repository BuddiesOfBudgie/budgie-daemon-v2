#pragma once

#include <QObject>
#include <QSharedPointer>
#include <QMap>
#include <QList>
#include "ConfigurationAction.hpp"
#include "CalculationResult.hpp"

namespace bd {
    class ConfigurationBatchSystem : public QObject {
    Q_OBJECT

    public:
        ConfigurationBatchSystem(QObject* parent = nullptr);
        static ConfigurationBatchSystem& instance();
        static ConfigurationBatchSystem* create() { return &instance(); }

        void addAction(QSharedPointer<ConfigurationAction> action);
        void removeAction(QString serial, ConfigurationActionType action_type);

        // Performs a calculation if necessary and applies them
        void apply();

        // Calculate potential resulting state from all actions
        // This does not apply the actions.
        void calculate();

        QSharedPointer<CalculationResult> getCalculationResult() const;

        // Clears any actions, resets any state
        void reset();

    signals:
        void configurationApplied(bool success);

    private:
        QSharedPointer<CalculationResult> m_calculation_result;
        QList<QSharedPointer<ConfigurationAction>> m_actions;

        // Helper method for calculating anchored positions
        QPoint calculateAnchoredPosition(QSharedPointer<OutputTargetState> outputState, QSharedPointer<OutputTargetState> relativeState);
    };
}