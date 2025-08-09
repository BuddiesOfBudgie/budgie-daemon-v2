#include "ConfigurationBatchSystem.hpp"
#include <output-manager/WaylandOutputManager.hpp>
#include <config/display.hpp>
#include <QSet>
#include <QRect>
#include <QStringList>
#include <QDebug>

namespace bd {
    ConfigurationBatchSystem::ConfigurationBatchSystem(QObject *parent) : QObject(parent),
        m_calculation_result(QSharedPointer<CalculationResult>()),
        m_actions(QList<QSharedPointer<ConfigurationAction>>()) {
    }

    ConfigurationBatchSystem& ConfigurationBatchSystem::instance() {
        static ConfigurationBatchSystem _instance(nullptr);
        return _instance;
    }

    void ConfigurationBatchSystem::addAction(QSharedPointer<ConfigurationAction> action) {

        // If the action is to turn off the head, remove any actions related to the head
        if (action->getActionType() == ConfigurationActionType::SetOnOff && !action->isOn()) {
            for (auto action : m_actions) {
                if (action->getSerial() == action->getSerial()) {
                    removeAction(action->getSerial(), action->getActionType());
                }
            }
        } else { // Remove any identical action for the action's serial
            removeAction(action->getSerial(), action->getActionType());
        }

        // If the action is to set the primary head, remove any other primary head actions
        if (action->getActionType() == ConfigurationActionType::SetPrimary) {
            for (auto action : m_actions) {
                if (action->getActionType() == ConfigurationActionType::SetPrimary) {
                    m_actions.removeOne(action);
                    break;
                }
            }
        }

        m_actions.append(action);
    }

    void ConfigurationBatchSystem::removeAction(QString serial, ConfigurationActionType action_type) {
        for (auto action : m_actions) {
            if (action->getSerial() == serial && action->getActionType() == action_type) {
                m_actions.removeOne(action);
                break;
            }
        }
    }

    void ConfigurationBatchSystem::apply() {
        // Ensure we have calculated the configuration first
        if (m_calculation_result.isNull()) {
            qWarning() << "No calculation result available. Call calculate() first.";
            return;
        }

        auto &orchestrator = bd::WaylandOrchestrator::instance();
        auto manager = orchestrator.getManager();
        
        if (manager.isNull()) {
            qWarning() << "WaylandOutputManager is not available";
            return;
        }

        // Create a new configuration
        auto config = manager->configure();
        if (config.isNull()) {
            qWarning() << "Failed to create WaylandOutputConfiguration";
            return;
        }

        // Get all output states from calculation result
        auto outputStates = m_calculation_result->getOutputStates();
        
        // Validate that all heads have corresponding output target states
        auto allHeads = manager->getHeads();
        for (auto const& headPtr : allHeads) {
            if (headPtr.isNull()) continue;
            auto head = headPtr.data();
            auto serial = head->getIdentifier();
            
            if (!outputStates.contains(serial)) {
                qWarning() << "ConfigurationBatchSystem error: Head" << serial 
                          << "does not have a corresponding OutputTargetState. This indicates a bug in the calculation logic.";
                return;
            }
        }

        // Process each output state
        for (auto serial : outputStates.keys()) {
            auto outputState = outputStates[serial];
            if (outputState.isNull()) continue;

            // Get the corresponding head
            auto head = manager->getOutputHead(serial);
            if (head.isNull()) {
                qWarning() << "Could not find head for serial:" << serial;
                continue;
            }

            if (outputState->isOn()) {
                // Enable the output and configure it
                auto configHead = config->enable(head.data());
                if (configHead.isNull()) {
                    qWarning() << "Failed to enable head for serial:" << serial;
                    continue;
                }

                // Set position
                auto position = outputState->getPosition();
                configHead->setPosition(position.x(), position.y());

                // Set scale
                auto scale = outputState->getScale();
                configHead->setScale(scale);

                // Set transform
                auto transform = outputState->getTransform();
                configHead->setTransform(transform);

                // Set adaptive sync
                auto adaptiveSync = outputState->getAdaptiveSync();
                configHead->setAdaptiveSync(adaptiveSync);

                // Set mode (dimensions and refresh rate)
                auto dimensions = outputState->getDimensions();
                auto refresh = outputState->getRefresh();
                
                if (!dimensions.isEmpty() && refresh > 0) {
                    // Try to find an existing mode that matches
                    auto mode = head->getModeForOutputHead(dimensions.width(), dimensions.height(), refresh);
                    if (!mode.isNull()) {
                        configHead->setMode(mode.data());
                    } else {
                        // Use custom mode if no existing mode matches
                        configHead->setCustomMode(dimensions.width(), dimensions.height(), refresh);
                    }
                }

                qDebug() << "Configured output" << serial << "- Position:" << position 
                         << "Scale:" << scale << "Transform:" << transform 
                         << "AdaptiveSync:" << adaptiveSync
                         << "Dimensions:" << dimensions << "Refresh:" << refresh;
            } else {
                // Disable the output
                config->disable(head.data());
                qDebug() << "Disabled output" << serial;
            }
        }

        // Connect to configuration result signals
        connect(config.data(), &WaylandOutputConfiguration::succeeded, this, [this, config]() {
            qDebug() << "Configuration applied successfully";
            
            // Update and save the configuration
            auto& displayConfig = bd::DisplayConfig::instance();
            
            // Force creation of a new active group from the current state and save it
            auto activeGroup = displayConfig.getActiveGroup();
            if (activeGroup) {
                displayConfig.saveState();
                qDebug() << "Configuration saved to disk";
            } else {
                qWarning() << "Failed to get active group for saving configuration";
            }
            
            emit configurationApplied(true);
            config->deleteLater();
        });
        
        connect(config.data(), &WaylandOutputConfiguration::failed, this, [this, config]() {
            qWarning() << "Configuration application failed";
            emit configurationApplied(false);
            config->deleteLater();
        });
        
        connect(config.data(), &WaylandOutputConfiguration::cancelled, this, [this, config]() {
            qWarning() << "Configuration application was cancelled";
            emit configurationApplied(false);
            config->deleteLater();
        });

        // Apply the configuration
        config->applySelf();
        qDebug() << "Applied configuration for" << outputStates.size() << "outputs";
    }

    void ConfigurationBatchSystem::calculate() {
        m_calculation_result = QSharedPointer<CalculationResult>(new CalculationResult(this));

        // Create our output target states for each serial first
        auto &orchestrator = bd::WaylandOrchestrator::instance();
        auto manager = orchestrator.getManager();

        auto pendingOutputStates = QMap<QString, QSharedPointer<OutputTargetState>>();

        for (auto const& headPtr : manager->getHeads()) {
            // Skip null heads
            if (headPtr.isNull()) continue;
            auto head = headPtr.data();

            auto outputState = new OutputTargetState(head->getIdentifier());
            outputState->setDefaultValues(headPtr); // Set some default values for the head
            
            auto outputStatePtr = QSharedPointer<OutputTargetState>(outputState);
            pendingOutputStates.insert(head->getIdentifier(), outputStatePtr);
        }

        auto actionsBySerial = QMap<QString, QList<QSharedPointer<ConfigurationAction>>>();
        // Group actions by serial
        for (auto action : m_actions) {
            if (actionsBySerial.contains(action->getSerial())) {
                actionsBySerial[action->getSerial()].append(action);
            } else {
                actionsBySerial.insert(action->getSerial(), QList<QSharedPointer<ConfigurationAction>>{action});
            }
        }

        // Apply all configuration actions to output states
        for (auto serial : actionsBySerial.keys()) {
            auto actions = actionsBySerial[serial];
            auto outputState = pendingOutputStates[serial];
            if (outputState.isNull()) continue;

            for (auto action : actions) {
                switch (action->getActionType()) {
                    case ConfigurationActionType::SetOnOff:
                        outputState->setOn(action->isOn());
                        break;
                    case ConfigurationActionType::SetMode:
                        outputState->setDimensions(action->getDimensions());
                        outputState->setRefresh(action->getRefresh());
                        break;
                    case ConfigurationActionType::SetScale:
                        outputState->setScale(action->getScale());
                        break;
                    case ConfigurationActionType::SetTransform:
                        outputState->setTransform(action->getTransform());
                        break;
                    case ConfigurationActionType::SetAdaptiveSync:
                        outputState->setAdaptiveSync(action->getAdaptiveSync());
                        break;
                    case ConfigurationActionType::SetPrimary:
                        outputState->setPrimary(true);
                        break;
                    case ConfigurationActionType::SetPositionAnchor:
                        outputState->setHorizontalAnchor(action->getHorizontalAnchor());
                        outputState->setVerticalAnchor(action->getVerticalAnchor());
                        break;
                    case ConfigurationActionType::SetMirrorOf:
                        // Mirroring will be handled during position calculation
                        break;
                    default:
                        break;
                }
            }
        }

        // Update resulting dimensions for all outputs
        for (auto outputState : pendingOutputStates.values()) {
            if (!outputState.isNull()) {
                outputState->updateResultingDimensions();
            }
        }

        // Identify mirroring relationships - mirrored outputs only share position, not configuration
        auto mirrorActions = QMap<QString, QString>(); // serial -> mirrored_serial
        for (auto action : m_actions) {
            if (action->getActionType() == ConfigurationActionType::SetMirrorOf) {
                mirrorActions.insert(action->getSerial(), action->getRelative());
            }
        }

        // Calculate positions based on anchoring
        auto positionedOutputs = QSet<QString>();
        auto anchorMap = QMap<QString, QString>(); // serial -> relative_serial
        
        // Build anchor relationships
        for (auto action : m_actions) {
            if (action->getActionType() == ConfigurationActionType::SetPositionAnchor) {
                anchorMap.insert(action->getSerial(), action->getRelative());
            }
        }

        // Find outputs without anchors (these will be positioned first)
        auto unanchoredOutputs = QList<QString>();
        for (auto serial : pendingOutputStates.keys()) {
            auto outputState = pendingOutputStates[serial];
            if (!outputState.isNull() && outputState->isOn() && !anchorMap.contains(serial)) {
                unanchoredOutputs.append(serial);
            }
        }

        // Position unanchored outputs starting from origin
        QPoint nextPosition(0, 0);
        for (auto serial : unanchoredOutputs) {
            auto outputState = pendingOutputStates[serial];
            if (!outputState.isNull()) {
                outputState->setPosition(nextPosition);
                positionedOutputs.insert(serial);
                
                // Move next position to the right
                auto dimensions = outputState->getResultingDimensions();
                nextPosition.setX(nextPosition.x() + dimensions.width());
            }
        }

        // Position anchored outputs iteratively
        bool progressMade = true;
        while (progressMade && positionedOutputs.size() < pendingOutputStates.size()) {
            progressMade = false;
            
            for (auto serial : anchorMap.keys()) {
                if (positionedOutputs.contains(serial)) continue;
                
                auto relativeSerial = anchorMap[serial];
                if (!positionedOutputs.contains(relativeSerial)) continue;
                
                auto outputState = pendingOutputStates[serial];
                auto relativeState = pendingOutputStates[relativeSerial];
                
                if (outputState.isNull() || relativeState.isNull() || !outputState->isOn()) continue;
                
                // Calculate position based on anchor (mirrored outputs will be handled separately)
                QPoint newPosition = calculateAnchoredPosition(outputState, relativeState);
                outputState->setPosition(newPosition);
                positionedOutputs.insert(serial);
                progressMade = true;
            }
        }

        // Handle mirrored outputs positioning - only share position if no explicit anchoring
        for (auto serial : mirrorActions.keys()) {
            auto mirroredSerial = mirrorActions[serial];
            auto outputState = pendingOutputStates[serial];
            auto mirroredState = pendingOutputStates[mirroredSerial];
            
            if (!outputState.isNull() && !mirroredState.isNull() && outputState->isOn()) {
                // Only inherit position if this output has no explicit anchoring and hasn't been positioned yet
                if (!positionedOutputs.contains(serial) && !anchorMap.contains(serial)) {
                    outputState->setPosition(mirroredState->getPosition());
                    positionedOutputs.insert(serial);
                }
            }
        }

        // Calculate global bounding rectangle
        QRect globalRect;
        bool firstOutput = true;
        
        for (auto outputState : pendingOutputStates.values()) {
            if (!outputState.isNull() && outputState->isOn()) {
                auto position = outputState->getPosition();
                auto dimensions = outputState->getResultingDimensions();
                QRect outputRect(position, dimensions);
                
                if (firstOutput) {
                    globalRect = outputRect;
                    firstOutput = false;
                } else {
                    globalRect = globalRect.united(outputRect);
                }
            }
        }

        // Store results
        m_calculation_result->getOutputStates().clear();
        for (auto serial : pendingOutputStates.keys()) {
            auto outputState = pendingOutputStates[serial];
            if (!outputState.isNull() && outputState->isOn()) {
                m_calculation_result->setOutputState(serial, outputState);
            }
        }

        // Update global space
        if (!globalRect.isEmpty()) {
            auto globalSpace = m_calculation_result->getGlobalSpace();
            *globalSpace = globalRect;
        }
    }

    QPoint ConfigurationBatchSystem::calculateAnchoredPosition(QSharedPointer<OutputTargetState> outputState, QSharedPointer<OutputTargetState> relativeState) {
        if (outputState.isNull() || relativeState.isNull()) return QPoint(0, 0);
        
        auto relativePos = relativeState->getPosition();
        auto relativeDimensions = relativeState->getResultingDimensions();
        auto outputDimensions = outputState->getResultingDimensions();
        
        QPoint newPosition = relativePos;
        
        // Calculate horizontal position
        switch (outputState->getHorizontalAnchor()) {
            case ConfigurationHorizontalAnchor::Left:
                // Left edge of output aligns with left edge of relative
                newPosition.setX(relativePos.x());
                break;
            case ConfigurationHorizontalAnchor::Right:
                // Right edge of output aligns with right edge of relative
                newPosition.setX(relativePos.x() + relativeDimensions.width() - outputDimensions.width());
                break;
            case ConfigurationHorizontalAnchor::Center:
                // Center of output aligns with center of relative
                newPosition.setX(relativePos.x() + (relativeDimensions.width() - outputDimensions.width()) / 2);
                break;
            default:
                // Default to right of relative output
                newPosition.setX(relativePos.x() + relativeDimensions.width());
                break;
        }
        
        // Calculate vertical position
        switch (outputState->getVerticalAnchor()) {
            case ConfigurationVerticalAnchor::Above:
                // Bottom edge of output is at top edge of relative
                newPosition.setY(relativePos.y() - outputDimensions.height());
                break;
            case ConfigurationVerticalAnchor::Top:
                // Top edge of output aligns with top edge of relative
                newPosition.setY(relativePos.y());
                break;
            case ConfigurationVerticalAnchor::Middle:
                // Middle of output aligns with middle of relative
                newPosition.setY(relativePos.y() + (relativeDimensions.height() - outputDimensions.height()) / 2);
                break;
            case ConfigurationVerticalAnchor::Bottom:
                // Bottom edge of output aligns with bottom edge of relative
                newPosition.setY(relativePos.y() + relativeDimensions.height() - outputDimensions.height());
                break;
            case ConfigurationVerticalAnchor::Below:
                // Top edge of output is at bottom edge of relative
                newPosition.setY(relativePos.y() + relativeDimensions.height());
                break;
            default:
                // Default to same vertical position as relative
                newPosition.setY(relativePos.y());
                break;
        }
        
        return newPosition;
    }

    void ConfigurationBatchSystem::reset() {
        m_calculation_result.clear(); // Clear the calculation result
        m_actions.clear(); // Clear the actions
    }
    
}