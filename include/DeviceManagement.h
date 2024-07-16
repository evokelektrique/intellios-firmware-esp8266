#ifndef DEVICEMANAGEMENT_H
#define DEVICEMANAGEMENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include <functional>
#include <vector>

#include "FileUtils.h"
#include "TimeManagement.h"

// Structure to define a schedule
struct Schedule {
    int startHour;
    int startMinute;
    int endHour;
    int endMinute;
};

// StateEntry structure moved from previous examples
struct StateEntry {
    bool state;
    unsigned long timestamp;

    StateEntry() : state(false), timestamp(0) {}  // Default constructor
    StateEntry(bool s, unsigned long t) : state(s), timestamp(t) {}
};

// Structure to track the state of a component
struct ComponentState {
    bool currentState;
    bool scheduledState;
    bool manualOverride;
    bool previousSensorState;       // Track previous state of the sensor
    unsigned long lastStateChange;  // Timestamp of the last state change
    unsigned long lastScheduledStateChange;  // Timestamp of the last scheduled state change
    unsigned long lastManualOverride;  // Timestamp of the last manual override
    std::vector<StateEntry> stateHistory;     // History of states
    size_t historyIndex;                      // Index for the circular buffer
    static const size_t maxHistorySize = 10;  // Maximum size of state history
    int errorCode;            // Error code to indicate any issues
    float energyConsumption;  // Energy consumption or runtime (if applicable)

    ComponentState()
        : currentState(false),
          scheduledState(false),
          manualOverride(false),
          previousSensorState(false),
          lastStateChange(0),
          lastScheduledStateChange(0),
          lastManualOverride(0),
          historyIndex(0),
          errorCode(0),
          energyConsumption(0.0f) {
        stateHistory.resize(maxHistorySize);
    }

    void updateState(bool newState) {
        currentState = newState;
        lastStateChange = TimeManagement::getCurrentTimestamp();
        stateHistory[historyIndex] = StateEntry(newState, lastStateChange);
        historyIndex = (historyIndex + 1) % maxHistorySize;
    }

    void updateScheduledState(bool newScheduledState) {
        scheduledState = newScheduledState;
        lastScheduledStateChange = TimeManagement::getCurrentTimestamp();
    }

    void updateManualOverride(bool override) {
        manualOverride = override;
        lastManualOverride = TimeManagement::getCurrentTimestamp();
    }

    void setErrorCode(int code) { errorCode = code; }

    void updateEnergyConsumption(float consumption) {
        energyConsumption = consumption;
    }
};

// Structure to define a component configuration
struct ComponentConfig {
    String componentName;
    String componentType;
    int componentPin;
    String actionType;
    int actionPin;
    std::vector<String> behaviors;
    Schedule schedule;
    std::function<bool(int)> readDevice; // Function pointer to read device state
    std::function<void(int, bool)> performAction; // Function pointer to perform action
    ComponentState state; // Add state to each component
};

struct Device {
    std::vector<ComponentConfig> components;
};

class DeviceManager {
   public:
    DeviceManager();
    void loadConfig();
    void saveConfig(JsonDocument& doc);
    void configureDevices();
    void handleManualBehavior(const ComponentConfig& config, ComponentState& state);
    void handleScheduledBehavior(const ComponentConfig& config, ComponentState& state);
    void checkScheduler();
    void readSensorsAndHandleBehaviors();
    bool shouldHandleManualBehavior(const ComponentConfig& config, const ComponentState& state);
    void handleConfig(ESP8266WebServer* server);
    void handleControl(ESP8266WebServer* server);
    void handleGetDevices(ESP8266WebServer* server);
    void populateFunctionPointers();

   private:
    bool readDigitalSensor(int pin);
    bool readAnalogSensor(int pin);
    void controlDigitalActuator(int pin, bool state);
    void controlAnalogActuator(int pin, bool state);
    void toggleDigitalActuator(int pin);
    void pulseDigitalActuator(int pin, int duration);

    std::vector<Device> devices;
};

extern DeviceManager deviceManager;

#endif  // DEVICEMANAGEMENT_H
