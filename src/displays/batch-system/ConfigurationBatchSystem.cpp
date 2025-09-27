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
        if (m_calculation_result.isNull()) {
            calculate(); // Ensure it's calculated
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
            } else {
                qDebug() << "ConfigurationBatchSystem: Head" << serial << "has a corresponding OutputTargetState";
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
                    qDebug() << "Setting mode for output" << serial << "Dimensions:" << dimensions << "Refresh:" << refresh;
                    // Try to find an existing mode that matches
                    auto mode = head->getModeForOutputHead(dimensions.width(), dimensions.height(), refresh);
                    if (!mode.isNull()) {
                        qDebug() << "Found existing mode for output" << serial << "Setting mode";
                        configHead->setMode(mode.data());
                    } else {
                        qDebug() << "No existing mode found for output" << serial << "Setting custom mode";
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
                // After success, update primary in meta heads based on group's primary
                auto &orchestrator = bd::WaylandOrchestrator::instance();
                auto manager = orchestrator.getManager();
                auto heads = manager->getHeads();
                auto primary = activeGroup->getPrimaryOutput();
                if (!primary.isEmpty()) {
                    for (const auto &head : heads) {
                        if (head.isNull()) continue;
                        head->setPrimary(head->getIdentifier() == primary);
                    }
                }
                displayConfig.saveState();
                qDebug() << "Configuration saved to disk";
            } else {
                qWarning() << "Failed to get active group for saving configuration";
            }
            
            emit configurationApplied(true);
            config->release();
        });
        
        connect(config.data(), &WaylandOutputConfiguration::failed, this, [this, config]() {
            qWarning() << "Configuration application failed";
            emit configurationApplied(false);
            config->release();
        });
        
        connect(config.data(), &WaylandOutputConfiguration::cancelled, this, [this, config]() {
            qWarning() << "Configuration application was cancelled";
            emit configurationApplied(false);
            config->release();
        });

        // Apply the configuration
        qDebug() << "Applying configuration for" << outputStates.size() << "outputs";
        config->applySelf();
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
                        outputState->setRelative(action->getRelative());
                        outputState->setHorizontalAnchor(action->getHorizontalAnchor());
                        outputState->setVerticalAnchor(action->getVerticalAnchor());
                        break;
                    case ConfigurationActionType::SetMirrorOf:
                        outputState->setMirrorOf(action->getRelative());
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

        // Build anchor relationships
        auto anchorMap = QMap<QString, QString>(); // serial -> relative_serial
        auto unanchoredOutputs = QList<QString>();

        for (auto action : m_actions) {
            if (action->getActionType() == ConfigurationActionType::SetPositionAnchor) {
                anchorMap.insert(action->getSerial(), action->getRelative());
            }
        }

        for (auto serial : pendingOutputStates.keys()) {
            auto outputState = pendingOutputStates[serial];
            if (!outputState.isNull() && outputState->isOn() && !anchorMap.contains(serial)) {
                unanchoredOutputs.append(serial);
            }
        }

        // Build horizontal chain for positioning
        QList<QString> horizontalChain = buildHorizontalChain(pendingOutputStates, m_actions);
        qDebug() << "Horizontal output chain order:" << horizontalChain;

        // Position outputs in horizontal chain from left to right
        auto positionedOutputs = QSet<QString>();
        QPoint nextPosition(0, 0);
        for (const auto& serial : horizontalChain) {
            auto outputState = pendingOutputStates[serial];
            // If the output is enabled and not mirroring, position it in the chain
            if (!outputState.isNull() && outputState->isOn() && !outputState->isMirroring()) {
                outputState->setPosition(nextPosition);
                positionedOutputs.insert(serial);
                // Move next position to the right
                auto dimensions = outputState->getResultingDimensions();
                nextPosition.setX(nextPosition.x() + dimensions.width());
            }
        }

        // Position anchored outputs iteratively (vertical and other anchors)
        bool progressMade = true;
        while (progressMade && positionedOutputs.size() < pendingOutputStates.size()) {
            progressMade = false;
            for (auto serial : anchorMap.keys()) {
                if (positionedOutputs.contains(serial)) continue;
                auto relativeSerial = anchorMap[serial];
                if (!positionedOutputs.contains(relativeSerial)) continue;
                auto outputState = pendingOutputStates[serial];
                auto relativeState = pendingOutputStates[relativeSerial];
                if (outputState.isNull() || relativeState.isNull() || !outputState->isOn() || outputState->isMirroring()) continue;
                // Calculate position based on anchor (mirrored outputs will be handled separately)
                QPoint newPosition = calculateAnchoredPosition(outputState, relativeState);
                outputState->setPosition(newPosition);
                positionedOutputs.insert(serial);
                progressMade = true;
            }
        }
        // Position mirrored outputs
        for (auto serial : mirrorActions.keys()) {
            auto mirroredSerial = mirrorActions[serial];
            auto outputState = pendingOutputStates[serial];
            auto mirroredState = pendingOutputStates[mirroredSerial];
            
            if (!outputState.isNull() && !mirroredState.isNull() && outputState->isOn() && outputState->isMirroring()) {
                // Only inherit position if this output has no explicit anchoring and hasn't been positioned yet
                if (!positionedOutputs.contains(serial) && !anchorMap.contains(serial)) {
                    QPoint newPosition = calculateAnchoredPosition(outputState, mirroredState);
                    outputState->setPosition(newPosition);
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
            m_calculation_result->setOutputState(serial, outputState);
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
                // Default behavior: for mirrors, align left; otherwise place to the right
                newPosition.setX(outputState->isMirroring() ? relativePos.x() : relativePos.x() + relativeDimensions.width());
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
                // Default behavior: for mirrors, align top; otherwise keep same Y
                newPosition.setY(outputState->isMirroring() ? relativePos.y() : relativePos.y());
                break;
        }
        
        return newPosition;
    }

    void ConfigurationBatchSystem::reset() {
        m_calculation_result.clear(); // Clear the calculation result
        m_actions.clear(); // Clear the actions
    }
    
    QList<QSharedPointer<ConfigurationAction>> ConfigurationBatchSystem::getActions() const {
        return m_actions;
    }

    QSharedPointer<CalculationResult> ConfigurationBatchSystem::getCalculationResult() const {
        return m_calculation_result;
    }
}

QList<QString> bd::ConfigurationBatchSystem::buildHorizontalChain(const QMap<QString, QSharedPointer<OutputTargetState>>& pendingOutputStates, const QList<QSharedPointer<ConfigurationAction>>& actions) const {
    // Map: serial -> relative_serial (for Right anchors only, not Above/Below)
    QMap<QString, QString> rightOfMap;
    // Reverse map: relative_serial -> serial
    QMap<QString, QString> referencedByMap;
    QSet<QString> allSerials;
    QSet<QString> allRelatives;

    for (const auto& action : actions) {
        if (action->getActionType() == ConfigurationActionType::SetPositionAnchor &&
            action->getHorizontalAnchor() == ConfigurationHorizontalAnchor::Right &&
            action->getVerticalAnchor() != ConfigurationVerticalAnchor::Above &&
            action->getVerticalAnchor() != ConfigurationVerticalAnchor::Below) {
            rightOfMap.insert(action->getSerial(), action->getRelative());
            referencedByMap.insert(action->getRelative(), action->getSerial());
            allSerials.insert(action->getSerial());
            allRelatives.insert(action->getRelative());
        }
    }
    // All outputs
    for (const auto& serial : pendingOutputStates.keys()) {
        allSerials.insert(serial);
    }

    // Find the leftmost output: one that is referenced as a relative, but is not a serial in rightOfMap
    // (i.e., appears as a relative, but not as a serial)
    QString leftmost;
    for (const auto& rel : allRelatives) {
        if (!rightOfMap.contains(rel)) {
            leftmost = rel;
            break;
        }
    }
    // If not found, fallback: pick any output not a serial in rightOfMap
    if (leftmost.isEmpty()) {
        for (const auto& serial : pendingOutputStates.keys()) {
            if (!rightOfMap.contains(serial)) {
                leftmost = serial;
                break;
            }
        }
    }

    QList<QString> chain;
    QSet<QString> visited;
    // Walk the chain from leftmost to rightmost
    QString current = leftmost;
    while (!current.isEmpty() && !visited.contains(current)) {
        chain.append(current);
        visited.insert(current);
        // Find who is right of current
        if (referencedByMap.contains(current)) {
            current = referencedByMap[current];
        } else {
            break;
        }
    }

    // Append unanchored outputs (not in chain)
    for (const auto& serial : pendingOutputStates.keys()) {
        if (!visited.contains(serial)) {
            chain.append(serial);
        }
    }
    return chain;
}