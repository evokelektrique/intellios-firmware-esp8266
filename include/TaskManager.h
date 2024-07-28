#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>
#include <vector>
#include <memory>
#include <algorithm>
#include <any>

#include "ConfigManager.h"
#include "StateManager.h"

class TaskManager {
public:
    TaskManager(ConfigManager& configManager, StateManager& stateManager)
        : configManager(configManager), stateManager(stateManager) {}

    void evaluateAndUpdate();

private:
    ConfigManager& configManager;
    StateManager& stateManager;

    bool evaluateCondition(const Condition& condition, const Device& device, const Pin& pin);
    bool findDeviceAndPin(const String& deviceId, const String& pinMode, Device& outDevice, Pin& outPin);
    void updatePreviousState(const Rule& rule, bool sensorValue);
    void performAction(const Action& action, bool sensorValue, unsigned long currentTime);
};

#endif // TASK_MANAGER_H
