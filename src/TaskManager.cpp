#include "TaskManager.h"

void TaskManager::evaluateAndUpdate() {
    unsigned long currentTime = millis();

    // Iterate through each rule
    for (Rule& rule : configManager.rules) {
        bool conditionsMet = true;
        bool sensorValueChanged = false;
        bool sensorValue = false;

        // Check each condition for the current rule
        for (const auto& condition : rule.conditions) {
            bool conditionMet = false;

            // Find the device associated with the condition
            auto deviceIt = std::find_if(
                configManager.devices.begin(), configManager.devices.end(),
                [&](const Device& d) { return d.id == condition->device_id; });

            if (deviceIt == configManager.devices.end()) {
                conditionsMet = false;
                break;
            }

            Device& device = *deviceIt;

            // Find the pin associated with the condition
            auto pinIt =
                std::find_if(configManager.pins.begin(), configManager.pins.end(), [&](const Pin& p) {
                    return std::find(device.pins.begin(), device.pins.end(),
                                     p.id) != device.pins.end() &&
                           p.mode == "INPUT";
                });

            if (pinIt != configManager.pins.end()) {
                sensorValue = digitalRead(pinIt->gpio) == HIGH;

                if (condition->value.is<bool>()) {
                    // Handle boolean conditions
                    if (condition->operatorStr == "==") {
                        conditionMet =
                            (sensorValue == condition->value.as<bool>());
                    } else if (condition->operatorStr == "!=") {
                        conditionMet =
                            (sensorValue != condition->value.as<bool>());
                    }
                } else if (condition->value.is<int>()) {
                    // Handle integer conditions
                    int sensorValueInt = sensorValue ? 1 : 0;
                    int conditionValue = condition->value.as<int>();

                    if (condition->operatorStr == "==") {
                        conditionMet = (sensorValueInt == conditionValue);
                    } else if (condition->operatorStr == "!=") {
                        conditionMet = (sensorValueInt != conditionValue);
                    } else if (condition->operatorStr == ">") {
                        conditionMet = (sensorValueInt > conditionValue);
                    } else if (condition->operatorStr == "<") {
                        conditionMet = (sensorValueInt < conditionValue);
                    } else if (condition->operatorStr == ">=") {
                        conditionMet = (sensorValueInt >= conditionValue);
                    } else if (condition->operatorStr == "<=") {
                        conditionMet = (sensorValueInt <= conditionValue);
                    }
                }

                if (device.previousState != sensorValue) {
                    sensorValueChanged = true;
                }

                if (!conditionMet) {
                    conditionsMet = false;
                    break;
                }
            } else {
                conditionsMet = false;
                break;
            }
        }

        // Perform actions if all conditions are met and the sensor value
        // changed
        if (conditionsMet && sensorValueChanged) {
            for (const auto& action : rule.actions) {
                // Find the device associated with the action
                auto deviceIt = std::find_if(
                    configManager.devices.begin(), configManager.devices.end(),
                    [&](const Device& d) { return d.id == action->device_id; });

                if (deviceIt == configManager.devices.end()) {
                    continue;
                }

                Device& device = *deviceIt;

                // Check if the latency period has elapsed for the device
                if (currentTime - device.lastRunTime < device.latency) {
                    Serial.println("Ignored due to (device latency: " +
                                   String(device.latency) +
                                   ") and (device lastRunTime: " +
                                   String(device.lastRunTime) + ")");
                    continue;  // Skip this device and move on to the next one
                }

                // Find the pin associated with the action
                auto pinIt = std::find_if(
                    configManager.pins.begin(), configManager.pins.end(),
                    [&](const Pin& p) {
                        return std::find(device.pins.begin(), device.pins.end(),
                                         p.id) != device.pins.end() &&
                               p.mode == "OUTPUT";
                    });

                if (pinIt != configManager.pins.end()) {
                    if (action->action_type == "toggle") {
                        // Toggle the state only if the sensorValue has changed
                            // from false to true
                            if (sensorValue && !device.previousState) {
                            bool currentState = stateManager.getState(
                                action->device_id, action->property);
                            bool newState = !currentState;
                            stateManager.setState(action->device_id,
                                                  action->property, newState);
                            digitalWrite(pinIt->gpio, newState ? HIGH : LOW);
                        }
                    } else if (action->action_type == "set") {
                        // Set the state
                        bool newState = action->value.as<bool>();
                        stateManager.setState(action->device_id,
                                              action->property, newState);
                        digitalWrite(pinIt->gpio, newState ? HIGH : LOW);
                    } else {
                        // Handle other action types if needed
                        Serial.println("Unknown action type: " +
                                       action->action_type);
                    }

                    // Update lastRunTime for the device after performing the
                    // action
                    device.lastRunTime = currentTime;
                }
            }
        }

        // Update the device's previous state after evaluating all conditions
        for (auto& device : configManager.devices) {
            if (std::any_of(rule.conditions.begin(), rule.conditions.end(),
                            [&](const std::unique_ptr<Condition>& condition) {
                                return condition->device_id == device.id;
                            })) {
                device.previousState = sensorValue;
            }
        }
    }
}
