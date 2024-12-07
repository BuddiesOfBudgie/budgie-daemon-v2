#include "DisplayConfigurationBatchSystem.hpp"

namespace bd {
    // Display Configuration Batch System

    DisplayConfigurationBatchSystem::DisplayConfigurationBatchSystem(QObject *parent) : QObject(parent),
        m_calculation({}) {
    }

    void DisplayConfigurationBatchSystem::addBatch(DisplayConfigurationBatch *batch) {
        m_calculation->addBatch(batch);
    }

    void DisplayConfigurationBatchSystem::apply() {
        // TODO: Implement
    }

    void DisplayConfigurationBatchSystem::outputRemoved(QString *serial) {
        m_calculation->removeBatch(serial);
    }

    void DisplayConfigurationBatchSystem::reset() {
        m_calculation->reset();
    }

    void DisplayConfigurationBatchSystem::test() {
        // TODO: Implement
    }
}
