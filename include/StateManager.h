

#ifndef INTELLIOS_STATE_MANAGER
#define INTELLIOS_STATE_MANAGER

#include <ConfigManager.h>
#include <Arduino.h>
#include <map>

class StateManager {
   public:
    StateManager(ConfigManager& configManagerInstance)
        : configManager(configManagerInstance) {};
    void setState(const String& deviceId, const String& property, bool value);
    bool getState(const String& deviceId, const String& property) const;

    void setDeviceComponentState(const String& deviceComponentId,
                                 bool newState);
    bool getDeviceComponentState(const String& deviceComponentId);
    void toggleDeviceComponentState(const String& deviceComponentId);

   private:
    std::map<String, std::map<String, bool>> states;
    ConfigManager& configManager;
};

#endif