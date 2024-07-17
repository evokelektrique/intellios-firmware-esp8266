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
Task taskReconnectWiFi(
    5000, TASK_FOREVER, []() { wifiManager.reconnectWiFi(); }, &runner);

// Define the task to evaluate conditions
Task taskEvaluateConditions(
    100, TASK_FOREVER, []() { deviceManager.evaluateConditions(); }, &runner,
    true);

void setup() {
    Serial.begin(9600);
    Serial.println("Starting up...");

    if (!LittleFS.begin()) {
        Serial.println("LittleFS Mount Failed");
        return;
    }

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
}
