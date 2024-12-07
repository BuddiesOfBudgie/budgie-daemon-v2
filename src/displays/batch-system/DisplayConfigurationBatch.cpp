#include "DisplayConfigurationBatchSystem.hpp"
#include <QtLogging>

namespace bd {
    DisplayConfigurationBatch::DisplayConfigurationBatch(QString *serial,
                                                         DisplayConfigurationBatchType batch_type, QObject *parent)
        : QObject(parent), m_serial(serial), m_batch_type(batch_type) {
    }

    void DisplayConfigurationBatch::addAction(DisplayConfigurationBatchAction *action) {
        if (m_batch_type == Disable) {
            qWarning("Cannot add actions to a disable batch");
            return;
        }
        m_actions.insert(action->actionType(), action);
    }

    QMap<DisplayConfigurationBatchActionType, DisplayConfigurationBatchAction *>
    DisplayConfigurationBatch::getActions() {
        return m_actions;
    }

    DisplayConfigurationBatchType DisplayConfigurationBatch::getBatchType() const {
        return m_batch_type;
    }

    QString *DisplayConfigurationBatch::getSerial() const {
        return m_serial;
    }
}
