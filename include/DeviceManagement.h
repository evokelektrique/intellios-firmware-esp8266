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
    std::vector<int> inputPins;
    std::vector<int> outputPins;
    std::vector<JsonObject> conditions;
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
    int readComponentState(const String& componentName, int pin);

   private:
    ConditionManager conditionManager;
    std::vector<Component> components;
    void configureComponentConditions();
    void setupPins();
    Component* getComponentByName(const String& name);
};

extern DeviceManager deviceManager;

#endif  // DEVICEMANAGEMENT_H
