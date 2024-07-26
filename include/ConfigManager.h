#ifndef INTELLIOS_CONFIG_MANAGER
#define INTELLIOS_CONFIG_MANAGER

#include <map>
#include <memory>

#include "Arduino.h"
#include "ArduinoJson.h"
#include "FileUtils.h"

struct Pin {
    String id;
    String label;
    String type;
    int gpio;
    String mode;
};

struct Device {
    String id;
    String label;
    std::vector<String> pins;
    std::map<String, JsonVariant> state;
    unsigned long latency;
    unsigned long lastRunTime;
    bool previousState;
};

struct DeviceComponent {
    String id;
    String device_id;
    String property;
    String type;
};

struct Condition {
    String device_id;
    String property;
    String operatorStr;
    JsonVariant value;
};

struct Action {
    String device_id;
    String property;
    String action_type;
    JsonVariant value;
};

struct Rule {
    String id;
    String label;
    std::vector<std::unique_ptr<Condition>> conditions;
    std::vector<std::unique_ptr<Action>> actions;
};

class ConfigManager {
   public:
    ConfigManager();
    bool save(const char* path, const char* data);
    void populateFromFile(const char* path);
    std::vector<Pin> pins;
    std::vector<Device> devices;
    std::vector<Rule> rules;
    std::vector<DeviceComponent> deviceComponents;

   private:
    void setupPins(JsonArray& pinsArray);
    void setupDevices(JsonArray& devicesArray);
    void setupDeviceComponents(JsonArray& deviceComponentsArray);
    void setupRules(JsonArray& rulesArray);
};

#endif