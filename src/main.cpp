#include <Arduino.h>
#include <ArduinoJson.h>
#include <ConfigManager.h>
#include <DeviceInfoManager.h>
#include <StateManager.h>
#include <TaskDefinitions.h>
#include <TaskManager.h>
#include <TaskScheduler.h>
#include <WiFiManagement.h>
#include <ESP8266WebServer.h>

#include <map>
#include <memory>
#include <vector>

// JSON configuration with version
const char* jsonConfig = R"(
{
  "version": "0.1",
  "pins": [
    {
      "id": "pin1",
      "label": "D1",
      "type": "digital",
      "gpio": 5,
      "mode": "OUTPUT"
    },
    {
      "id": "pin2",
      "label": "D2",
      "type": "digital",
      "gpio": 4,
      "mode": "INPUT"
    }
  ],
  "devices": [
    {
      "id": "led",
      "label": "LED",
      "pins": ["pin1"],
      "state": {
        "status": false
      },
      "latency": 150
    },
    {
      "id": "touch_sensor",
      "label": "Touch Sensor",
      "pins": ["pin2"],
      "state": {
        "touched": false
      },
      "latency": 0
    }
  ],
  "components": [
    {
        "id": "led_switch_deviceComponent",
        "device_id": "led",
        "property": "status",
        "type": "switch"
    }
  ],
  "rules": [
    {
      "id": "rule3",
      "label": "Touch Sensor to Toggle LED",
      "conditions": [
        {
          "device_id": "touch_sensor",
          "property": "touched",
          "operator": "==",
          "value": true
        }
      ],
      "actions": [
        {
          "device_id": "led",
          "property": "status",
          "action_type": "toggle",
          "value": true
        }
      ]
    }
  ]
}
)";

DeviceInfoManager deviceInfoManager;
ConfigManager configManager;
StateManager stateManager(configManager);
TaskManager taskManager(configManager, stateManager);
Scheduler runner;
WiFiManager wifiManager;
ESP8266WebServer server(80);

// Tasks
Task evaluateAndUpdateTask(
    5, TASK_FOREVER, []() { taskManager.evaluateAndUpdate(); }, &runner, true);

bool ledState = false;
unsigned long previousMillis = 0;
const long interval = 2000;  // 2 seconds

void setup() {
    Serial.begin(9600);

    if (!LittleFS.begin()) {
        Serial.println("Could not initialize LittleFS");
        return;
    }

    // Setup device information
    deviceInfoManager.setupDeviceInfo();

    // Setup WiFi
    wifiManager.startAPMode();
    wifiManager.begin();

    // Save and load json config
    const char* configPath = "/config.json";
    configManager.save(configPath, jsonConfig);
    configManager.populateFromFile(configPath);

    MDNS.begin("myesp");
    Serial.println("Address: http://myesp.local");

    // Wifi Manager Routes
    server.on("/", HTTP_GET, []() { wifiManager.handleRoot(&server); });
    server.on("/scan", HTTP_GET, []() { wifiManager.handleScan(&server); });
    server.on("/connect", HTTP_POST,
              []() { wifiManager.handleConnect(&server); });
    server.on("/status", HTTP_GET, []() { wifiManager.handleStatus(&server); });

    // // Device Manager Routes
    // server.on("/config", HTTP_POST,
    //           []() { deviceManager.handleConfig(&server); });
    // server.on("/control", HTTP_POST,
    //           []() { deviceManager.handleControl(&server); });
    // server.on("/devices", HTTP_GET,
    //           []() { deviceManager.handleGetDevices(&server); });

    // Start local web server
    server.begin();
    Serial.println("HTTP server started");

    // Start the task scheduler
    runner.startNow();
}

void loop() {
    runner.execute();
    MDNS.update();
    server.handleClient();

    unsigned long currentMillis = millis();

    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        ledState = !ledState;  // Toggle the LED state
        stateManager.setDeviceComponentState("led_switch_deviceComponent",
                                             ledState);
    }
}
