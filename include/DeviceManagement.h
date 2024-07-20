#ifndef DEVICEMANAGEMENT_H
#define DEVICEMANAGEMENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <LittleFS.h>

#include "ScriptManager.h"
#include "FileUtils.h"

struct Component {
    String name;
    std::vector<int> inputPins;      // Input pins
    std::vector<int> outputPins;     // Output pins
    std::vector<String> scripts;  // DSL scripts for this component
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
    int readComponentState(const String& componentName,
                           int pin);  // Function to read the state of a
                                      // specific pin of a component
    ScriptManager& getScriptManager();

   private:
    ScriptManager scriptManager;
    std::vector<Component> components;
    void configureComponentScripts();
    void setupPins();  // Function to set up pins
    Component* getComponentByName(
        const String& name);  // Helper function to get a component by name
};

extern DeviceManager deviceManager;

#endif  // DEVICEMANAGEMENT_H
