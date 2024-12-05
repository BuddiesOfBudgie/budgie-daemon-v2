#include "DisplayConfigurationBatchSystem.hpp"

namespace bd {
    // Display Configuration Batch System

    DisplayConfigurationBatchSystem::DisplayConfigurationBatchSystem(QObject *parent) : QObject(parent), m_batches({}),
        m_calculation({}) {
    }

    void DisplayConfigurationBatchSystem::addBatch(DisplayConfigurationBatch *batch) {
        m_batches.append(batch);
        // Clear our calculation as it may be invalid now
        m_calculation = {};
    }

    void DisplayConfigurationBatchSystem::apply() {
        if (!m_calculation->has_value()) createCalculation();
        // TODO: Implement
    }

    DisplayConfigurationCalculation *DisplayConfigurationBatchSystem::createCalculation() {
    }

    void DisplayConfigurationBatchSystem::reset() {
        m_batches.clear();
        m_calculation = {};
    }

    void DisplayConfigurationBatchSystem::test() {
        if (!m_calculation->has_value()) createCalculation();
        // TODO: Implement
    }

    // DisplayConfigurationCalculation

    DisplayConfigurationCalculation::DisplayConfigurationCalculation(QObject *parent) : QObject(parent),
        m_global_output_size({}), m_actions({}), m_output_rects({}) {
    }

    void DisplayConfigurationCalculation::addAction(DisplayConfigurationBatchAction *action) {
        // TODO: Implement
    }
}
