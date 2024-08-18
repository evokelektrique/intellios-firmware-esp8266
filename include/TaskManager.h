#ifndef TASK_MANAGER_H
#define TASK_MANAGER_H

#include <Arduino.h>

#include <algorithm>
#include <any>
#include <memory>
#include <vector>

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

    bool evaluateCondition(const Condition& condition, const Device& device,
                           const Pin& pin);
    bool findDeviceAndPin(const String& deviceId, const String& pinMode,
                          Device& outDevice, Pin& outPin);
    void updatePreviousState(const Rule& rule, bool value);
    void performAction(const Action& action, unsigned long currentTime);
};

#endif  // TASK_MANAGER_H
