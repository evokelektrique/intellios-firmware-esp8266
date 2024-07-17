#ifndef DEVICEMANAGEMENT_H
#define DEVICEMANAGEMENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include "ConditionManager.h"
#include "FileUtils.h"

struct Component {
    String name;
    std::vector<int> inputPins;      // Input pins
    std::vector<int> outputPins;     // Output pins
    std::vector<String> conditions;  // DSL conditions for this component
};

class DeviceManager {
   public:
    DeviceManager();
    void loadConfig();
    void saveConfig();
    void configureComponents();
    void handleConfig(ESP8266WebServer* server);
    void handleControl(ESP8266WebServer* server);
    void handleGetComponents(ESP8266WebServer* server);
    void evaluateConditions();
    int readComponentState(const String& componentName,
                           int pin);  // Function to read the state of a
                                      // specific pin of a component

   private:
    ConditionManager conditionManager;
    std::vector<Component> components;
    void configureComponentConditions();
    void setupPins();  // Function to set up pins
    Component* getComponentByName(
        const String& name);  // Helper function to get a component by name
};

extern DeviceManager deviceManager;

#endif  // DEVICEMANAGEMENT_H
