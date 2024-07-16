#include <vector>

#include "Arduino.h"
#include "ArduinoJson.h"
#include "DeviceManagement.h"
#include "ESP8266WebServer.h"
#include "ESP8266WiFi.h"
#include "ESP8266mDNS.h"
#include "LittleFS.h"
#include "TaskDefinitions.h"
#include "TaskScheduler.h"
#include "WiFiManagement.h"

ESP8266WebServer server(80);  // Create a web server on port 80
DeviceManager deviceManager;
WiFiManager wifiManager;
Scheduler runner;  // Define the Scheduler

// Define the tasks and assign them to the scheduler
Task taskReadSensors(
    10, TASK_FOREVER, []() { deviceManager.readSensorsAndHandleBehaviors(); },
    &runner, true);

Task taskReconnectWiFi(
    5000, TASK_FOREVER, []() { wifiManager.reconnectWiFi(); }, &runner);

void setup() {
    Serial.begin(9600);
    Serial.println("Starting up...");

    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

    deviceManager.loadConfig();
    deviceManager.configureDevices();
    deviceManager.populateFunctionPointers();


    wifiManager.startAPMode();
    wifiManager.begin();

    MDNS.begin("myesp");
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
    server.on("/devices", HTTP_GET,
              []() { deviceManager.handleGetDevices(&server); });

    server.begin();
    Serial.println("HTTP server started");

    runner.startNow();  // Start the task scheduler
}

void loop() {
    runner.execute();  // Execute scheduled tasks
    MDNS.update();
    server.handleClient();
}
