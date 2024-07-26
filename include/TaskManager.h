#ifndef INTELLIOS_TASK_MANAGER
#define INTELLIOS_TASK_MANAGER

#include <Arduino.h>
#include <ConfigManager.h>
#include <StateManager.h>

class TaskManager {
    public:
    TaskManager(ConfigManager& configManagerInstance,
                StateManager& stateManagerInstance)
        : configManager(configManagerInstance),
          stateManager(stateManagerInstance) {};
    void evaluateAndUpdate();

   private:
    ConfigManager& configManager;
    StateManager& stateManager;
};

#endif