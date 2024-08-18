#include "TaskManager.h"

bool TaskManager::evaluateCondition(const Condition& condition,
                                    const Device& device, const Pin& pin) {
    bool value = digitalRead(pin.gpio) == HIGH;
    if (condition.operatorStr == "==") {
        return (value == std::any_cast<bool>(condition.value));
    } else if (condition.operatorStr == "!=") {
        return (value != std::any_cast<bool>(condition.value));
    }
    return false;
}

bool TaskManager::findDeviceAndPin(const String& deviceId,
                                   const String& pinMode, Device& outDevice,
                                   Pin& outPin) {
    auto deviceIt =
        std::find_if(configManager.devices.begin(), configManager.devices.end(),
                     [&](const Device& d) { return d.id == deviceId; });

    if (deviceIt == configManager.devices.end()) {
        return false;
    }

    outDevice = *deviceIt;

    for (const auto& pinId : outDevice.pins) {
        auto pinIt = std::find_if(
            configManager.pins.begin(), configManager.pins.end(),
            [&](const Pin& p) { return p.id == pinId && p.mode == pinMode; });

        if (pinIt != configManager.pins.end()) {
            outPin = *pinIt;
            return true;
        }
    }

    return false;
}

void TaskManager::updatePreviousState(const Rule& rule, bool value) {
    for (auto& device : configManager.devices) {
        if (std::any_of(rule.conditions.begin(), rule.conditions.end(),
                        [&](const std::unique_ptr<Condition>& condition) {
                            return condition->device_id == device.id;
                        })) {
            device.previousState = value;
        }
    }
}

void TaskManager::performAction(const Action& action,
                                unsigned long currentTime) {
    Device device;
    Pin pin;

    if (!findDeviceAndPin(action.device_id, "OUTPUT", device, pin)) {
        return;
    }

    if (currentTime - device.lastRunTime < device.latency) {
        return;
    }

    bool newState;

    if (action.action_type == "toggle") {
        bool currentState =
            stateManager.getState(action.device_id, action.property);
        newState = !currentState;
    } else if (action.action_type == "set") {
        newState = std::any_cast<bool>(action.value);
    }

    stateManager.setState(action.device_id, action.property, newState, pin);
    device.lastRunTime = currentTime;
}

void TaskManager::evaluateAndUpdate() {
    unsigned long currentTime = millis();

    for (Rule& rule : configManager.rules) {
        bool conditionsMet = true;
        bool value = false;

        for (const auto& condition : rule.conditions) {
            Device device;
            Pin pin;

            if (!findDeviceAndPin(condition->device_id, "INPUT", device, pin)) {
                conditionsMet = false;
                break;
            }

            value = digitalRead(pin.gpio) == HIGH;
            if (!evaluateCondition(*condition, device, pin)) {
                conditionsMet = false;
                break;
            }

            if (rule.triggerOnChange && (device.previousState == value)) {
                conditionsMet = false;
                break;
            }
        }

        if (conditionsMet) {
            for (const auto& action : rule.actions) {
                performAction(*action, currentTime);
            }
        }

        updatePreviousState(rule, value);
    }
}
