#include <Arduino.h>
#include <ArduinoJson.h>
#include <DeviceManagement.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <LittleFS.h>
#include <ScriptManager.h>
#include <TaskDefinitions.h>
#include <TaskScheduler.h>
#include <WiFiManagement.h>

#include <vector>

ESP8266WebServer server(80);  // Create a web server on port 80
DeviceManager deviceManager;
WiFiManager wifiManager;
Scheduler runner;  // Define the Scheduler
ScriptManager scriptManager;

// Define the tasks and assign them to the scheduler
Task taskReconnectWiFi(
    5000, TASK_FOREVER, []() { wifiManager.reconnectWiFi(); }, &runner);

// Define the task to evaluate conditions
Task taskEvaluateScripts(
    10, TASK_FOREVER, []() { scriptManager.evaluateScripts(); }, &runner, true);

void saveConfig() {
    StaticJsonDocument<1024> doc;  // Adjust the size as needed
    JsonArray componentsArray = doc.createNestedArray("components");

    JsonObject componentObj = componentsArray.createNestedObject();
    componentObj["name"] = "ToggleComponent";

    JsonArray inputPins = componentObj.createNestedArray("inputPins");
    inputPins.add(4);  // Assuming GPIO4 is D2

    JsonArray outputPins = componentObj.createNestedArray("outputPins");
    outputPins.add(5);  // Assuming GPIO5 is D1

    JsonArray conditions = componentObj.createNestedArray("conditions");
    conditions.add(
        "IF sensor 'readDigital' pin 'D2' operator '=' value 'HIGH' "
        "check_previous THEN action 'toggle' pin 'D1'");

    String jsonString;
    serializeJson(doc, jsonString);

    File configFile = LittleFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return;
    }
    configFile.print(jsonString);
    configFile.close();
    Serial.println("Config saved to /config.json");
}

void setup() {
    Serial.begin(9600);
    Serial.println("Starting up...");

    // DSLParser parser;

    // String dsl1 =
    //     "IF sensor 'readDigital' pin 'D2' operator '=' value 'HIGH' "
    //     "check_previous THEN action 'toggle' pin 'D1'";
    // String dsl2 =
    //     "IF sensor 'readDigital' pin 'D2' operator '=' value 'HIGH' THEN "
    //     "action 'toggle' pin 'D1' WAIT 5000";
    // String dsl3 =
    //     "IF sensor 'readDigital' pin 'D2' operator '=' value 'HIGH' THEN "
    //     "action 'toggle' pin 'D1' SCHEDULE start '08:00' end '20:00' days "
    //     "'Mon,Wed,Fri'";

    // parser.parse(dsl1);
    // parser.parse(dsl2);
    // parser.parse(dsl3);

    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    // Save configuration to /config.json
    saveConfig();

    // Load configuration from /config.json
    deviceManager.loadConfig();

    deviceManager.configureComponents();

    wifiManager.startAPMode();
    wifiManager.begin();

    if (MDNS.begin("myesp")) {
        Serial.println("MDNS responder started");
    }

    Serial.println("Address: http://myesp.local");

    // Wifi Manager Routes
    server.on("/", HTTP_GET, []() { wifiManager.handleRoot(&server); });
    server.on("/scan", HTTP_GET, []() { wifiManager.handleScan(&server); });
    server.on("/connect", HTTP_POST,
              []() { wifiManager.handleConnect(&server); });
    server.on("/status", HTTP_GET, []() { wifiManager.handleStatus(&server); });

    // Device Manager Routes
    server.on("/config", HTTP_POST,
              []() { deviceManager.handleConfig(&server); });
    server.on("/control", HTTP_POST,
              []() { deviceManager.handleControl(&server); });
    server.on("/components", HTTP_GET,
              []() { deviceManager.handleGetComponents(&server); });

    server.begin();
    Serial.println("HTTP server started");

    runner.startNow();  // Start the task scheduler
}

void loop() {
    runner.execute();  // Execute scheduled tasks
    MDNS.update();
    server.handleClient();
    // delay(100);  // Add a small delay to avoid flooding the serial output
}
