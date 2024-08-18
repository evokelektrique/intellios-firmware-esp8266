#include <Arduino.h>
#include <ArduinoJson.h>
#include <ConfigManager.h>
#include <DeviceInfoManager.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <StateManager.h>
#include <TaskDefinitions.h>
#include <TaskManager.h>
#include <TaskScheduler.h>
#include <WiFiManagement.h>

DeviceInfoManager deviceInfoManager;
ConfigManager configManager;
StateManager stateManager(configManager);
TaskManager taskManager(configManager, stateManager);
Scheduler runner;
WiFiManager wifiManager(deviceInfoManager);
ESP8266WebServer server(80);

// Tasks
Task taskReconnectWiFi(
    5000, TASK_FOREVER, []() { wifiManager.reconnectWiFi(); }, &runner);

Task evaluateAndUpdateTask(
    1, TASK_FOREVER, []() { taskManager.evaluateAndUpdate(); }, &runner, true);

// bool ledState = false;
// unsigned long previousMillis = 0;
// const long interval = 2000;  // 2 seconds

void setup() {
    Serial.begin(9600);

    if (!LittleFS.begin()) {
        Serial.println("Could not initialize LittleFS");
        return;
    }

    // Setup device information
    Serial.println("CHIP ID: " + deviceInfoManager.deviceInfo.chipId);

    // Save and load json config
    server.on("/config/update", HTTP_PUT, []() {
        if (!server.hasArg("config")) {
            server.send(400, "text/plain", "Bad Request");
            return;
        }

        String jsonConfig = server.arg("config");

        // Cast String to char*
        const char* jsonConfigChar = jsonConfig.c_str();

        const char* configPath = "/config.json";

        // Save the updated configuration
        if (configManager.save(configPath, jsonConfigChar)) {
            configManager.populateFromFile(configPath);
            server.send(200, "application/json",
                        "{\"status\":\"Config updated successfully\"}");
        } else {
            server.send(500, "application/json",
                        "{\"status\":\"Failed to save config\"}");
        }
    });

    // Start the task scheduler
    runner.startNow();

    // Wifi Manager Routes
    server.on("/wifi/", HTTP_GET, []() { wifiManager.handleRoot(&server); });

    // Scan nearby WiFi connections
    server.on("/wifi/scan", HTTP_GET,
              []() { wifiManager.handleScan(&server); });

    // Connect to a WiFi router
    server.on("/wifi/connect", HTTP_POST,
              []() { wifiManager.handleConnect(&server); });

    // Get current AP/STA connection status
    server.on("/wifi/status", HTTP_GET,
              []() { wifiManager.handleStatus(&server); });

    // Change or Update current AP password
    server.on("/wifi/update_ap_password", HTTP_GET, []() {
        wifiManager.handleUpdateAccessPointCredentials(&server);
    });

    // State manager - Setter
    server.on("/state/set", HTTP_POST, []() {
        if (server.hasArg("deviceComponentId") && server.hasArg("newState")) {

            String deviceComponentId = server.arg("deviceComponentId");
            String newState = server.arg("newState");

            bool state = (newState == "true" || newState == "1");

            JsonDocument doc;
            doc["deviceComponentId"] = deviceComponentId;
            doc["state"] = state;

            stateManager.setDeviceComponentState(deviceComponentId, state);

            String response;
            serializeJson(doc, response);

            server.send(200, "application/json", response);
        } else {
            server.send(400, "text/plain", "Bad Request");
        }
    });

    // State manager - Getter
    server.on("/state/get", HTTP_GET, []() {
        if (server.hasArg("deviceComponentId")) {
            String deviceComponentId = server.arg("deviceComponentId");

            JsonDocument doc;

            bool state =
                stateManager.getDeviceComponentState(deviceComponentId);
            doc["deviceComponentId"] = deviceComponentId;
            doc["state"] = state;

            String response;
            serializeJson(doc, response);

            server.send(200, "application/json", response);
        } else {
            server.send(400, "text/plain", "Bad Request");
        }
    });

    // Setup WiFi access point
    wifiManager.startAPMode();

    // Start access point
    Serial.println("AP Mode Started");
    Serial.print("IP Address: ");
    Serial.println(wifiManager.getIPAddress());
    wifiManager.begin();

    // String MDNSHostname = "intellios-" + deviceInfoManager.deviceInfo.chipId;
    String MDNSHostname = "myesp";
    MDNS.begin(MDNSHostname);
    Serial.println("Address: http://" + MDNSHostname + ".local");

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    MDNS.update();
    server.handleClient();
    runner.execute();

    // unsigned long currentMillis = millis();

    // if (currentMillis - previousMillis >= interval) {
    //     previousMillis = currentMillis;

    //     ledState = !ledState;  // Toggle the LED state
    //     stateManager.setDeviceComponentState("led_switch_deviceComponent",
    //                                          ledState);
    // }
}
