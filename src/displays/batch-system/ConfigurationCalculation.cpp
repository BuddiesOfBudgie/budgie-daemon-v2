#include "DisplayConfigurationBatchSystem.hpp"
#include <QtLogging>
#include <algorithm>
#include <qrect.h>
#include <output-manager/WaylandOutputManager.hpp>

namespace bd {
    DisplayConfigurationCalculation::DisplayConfigurationCalculation(QObject *parent) : QObject(parent),
        m_global_space({}), m_actions({}), m_batches({}), m_output_rects({}) {
    }

    void DisplayConfigurationCalculation::addBatch(DisplayConfigurationBatch *batch) {
        m_batches.insert(batch->getSerial(), batch);
    }

    void DisplayConfigurationCalculation::calculate() {
        auto &orchestrator = bd::WaylandOrchestrator::instance();
        auto manager = orchestrator.getManager();
        // First filter duplicate actions
        QList<DisplayConfigurationBatchAction *> reversed_actions = m_buffered_actions;
        std::ranges::reverse(reversed_actions);
        QList<DisplayConfigurationBatchAction *> filtered_actions{};

        for (auto batch: m_batches) {
            if (batch->getBatchType() == Disable) continue;
            auto serial = batch->getSerial();
            auto headOption = manager->getOutputHead(*serial);

            // Head doesn't exist at point of compute
            if (!headOption.has_value()) continue;
            auto head = headOption.value();

            auto actions = batch->getActions();
            auto output_rect = QRect{};
            auto dimensions = std::optional<QSize>{};
            auto refresh = std::optional<int>{};

            // Attempt to get by mode action
            if (actions.contains(SetMode)) {
                auto setModeAction = actions.value(SetMode);
                auto dimensionsOption = setModeAction->getDimensions();
                if (dimensionsOption.has_value()) {
                    dimensions = QSize{
                        dimensionsOption.value()->width(), dimensionsOption.value()->height()
                    };
                }

                auto refreshOption = setModeAction->getRefresh();
                if (refreshOption.has_value()) refresh = refreshOption.value();
            }

            // Didn't get a value from mode action, attempt to fetch from WaylandOutputManager
            if (!dimensions.has_value() || !refresh.has_value()) {
                const auto currentMode = head->getCurrentMode();
                dimensions = QSize{currentMode->getWidth(), currentMode->getHeight()};
                refresh = currentMode->getRefresh();
            }

            auto setModeAction = actions.value()
        }
    }
}
