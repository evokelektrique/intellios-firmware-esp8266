#include "DeviceManagement.h"

DeviceManager::DeviceManager() {}

bool DeviceManager::readDigitalSensor(int pin) {
    return digitalRead(pin) == HIGH;
}

bool DeviceManager::readAnalogSensor(int pin) {
    return analogRead(pin) > 512;
}

void DeviceManager::controlDigitalActuator(int pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

void DeviceManager::controlAnalogActuator(int pin, bool state) {
    analogWrite(pin, state ? 255 : 0);
}

void DeviceManager::toggleDigitalActuator(int pin) {
    digitalWrite(pin, !digitalRead(pin));
}

void DeviceManager::pulseDigitalActuator(int pin, int duration) {
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
}

void DeviceManager::populateFunctionPointers() {
    for (auto& device : devices) {
        for (auto& component : device.components) {
            if (component.componentType == "digital") {
                component.readDevice = [this](int pin) { return this->readDigitalSensor(pin); };
                component.performAction = [this](int pin, bool state) { this->controlDigitalActuator(pin, state); };
            } else if (component.componentType == "analog") {
                component.readDevice = [this](int pin) { return this->readAnalogSensor(pin); };
                component.performAction = [this](int pin, bool state) { this->controlAnalogActuator(pin, state); };
            }
        }
    }
}

void DeviceManager::handleManualBehavior(const ComponentConfig& config, ComponentState& state) {
    if (std::find(config.behaviors.begin(), config.behaviors.end(), "toggle") != config.behaviors.end()) {
        toggleDigitalActuator(config.actionPin);
        state.updateState(!state.currentState);
        state.updateManualOverride(true);
    } else if (std::find(config.behaviors.begin(), config.behaviors.end(), "pulse") != config.behaviors.end()) {
        pulseDigitalActuator(config.actionPin, 500);
    } else if (std::find(config.behaviors.begin(), config.behaviors.end(), "timed") != config.behaviors.end()) {
        controlDigitalActuator(config.actionPin, state.currentState);
        state.updateState(state.currentState);
    }
}

void DeviceManager::handleScheduledBehavior(const ComponentConfig& config, ComponentState& state) {
    if (std::find(config.behaviors.begin(), config.behaviors.end(), "scheduled") != config.behaviors.end() && !state.manualOverride) {
        controlDigitalActuator(config.actionPin, state.scheduledState);
        state.updateState(state.scheduledState);
    }
}

// Method to load configuration from LittleFS
void DeviceManager::loadConfig() {
    Serial.println("Loading config from LittleFS...");

    // Read the configuration file
    String configFile = readFile("/config.json");

    // Print the configuration file content
    Serial.println("Config File Content:");
    Serial.println(configFile);

    // Define a JsonDocumentan appropriate size
    JsonDocument doc;  // Adjust size as needed
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        Serial.println("Failed to read config file");
        Serial.println(error.c_str());
        return;
    }

    // Print the entire parsed JSON document
    serializeJson(doc, Serial);
    Serial.println();

    // Get the array of devices from the JSON document
    JsonArray devicesJsonArray = doc["devices"].as<JsonArray>();
    if (devicesJsonArray.isNull()) {
        Serial.println("Invalid config structure");
        return;
    }

    // Clear the current devices vector to prepare for new configuration
    devices.clear();

    // Loop through each device in the JSON array and populate the devices vector
    for (JsonObject deviceJson : devicesJsonArray) {
        Device device;

        // Loop through each component for the device
        for (JsonObject componentJson : deviceJson["components"].as<JsonArray>()) {
            ComponentConfig component;
            component.componentName = componentJson["componentName"].as<String>();
            component.componentType = componentJson["componentType"].as<String>();
            component.componentPin = componentJson["componentPin"];
            component.actionType = componentJson["actionType"].as<String>();
            component.actionPin = componentJson["actionPin"];

            // Add behaviors to the configuration
            for (JsonVariant behavior : componentJson["behaviors"].as<JsonArray>()) {
                component.behaviors.push_back(behavior.as<String>());
            }

            // Add schedule if it exists
            if (componentJson.containsKey("schedule")) {
                JsonObject scheduleJson = componentJson["schedule"];
                component.schedule.startHour = scheduleJson["startTime"]["hour"];
                component.schedule.startMinute = scheduleJson["startTime"]["minute"];
                component.schedule.endHour = scheduleJson["endTime"]["hour"];
                component.schedule.endMinute = scheduleJson["endTime"]["minute"];
            }

            // Initialize the component state
            component.state = ComponentState();

            // Add the component to the device's components vector
            device.components.push_back(component);
        }

        // Add the device to the devices vector
        devices.push_back(device);
    }

    Serial.println("Config loaded from LittleFS.");
}

// Method to save configuration to LittleFS
void DeviceManager::saveConfig(JsonDocument& doc) {
    Serial.println("Saving config to LittleFS...");
    JsonArray devicesJsonArray = doc["devices"].to<JsonArray>();

    for (const auto& device : devices) {
        JsonObject deviceJson = devicesJsonArray.add<JsonObject>();

        JsonArray componentsJsonArray = deviceJson["components"].to<JsonArray>();
        for (const auto& component : device.components) {
            JsonObject componentJson = componentsJsonArray.add<JsonObject>();
            componentJson["componentName"] = component.componentName;
            componentJson["componentType"] = component.componentType;
            componentJson["componentPin"] = component.componentPin;
            componentJson["actionType"] = component.actionType;
            componentJson["actionPin"] = component.actionPin;

            JsonArray behaviorsJsonArray = componentJson["behaviors"].to<JsonArray>();
            for (const auto& behavior : component.behaviors) {
                behaviorsJsonArray.add(behavior);
            }

            if (std::find(component.behaviors.begin(), component.behaviors.end(), "scheduled") != component.behaviors.end()) {
                JsonObject scheduleJson = componentJson["schedule"].to<JsonObject>();
                scheduleJson["startTime"]["hour"] = component.schedule.startHour;
                scheduleJson["startTime"]["minute"] = component.schedule.startMinute;
                scheduleJson["endTime"]["hour"] = component.schedule.endHour;
                scheduleJson["endTime"]["minute"] = component.schedule.endMinute;
            }
        }
    }

    // Convert the document to JsonObject
    JsonObject jsonObject = doc.as<JsonObject>();

    // Save the JSON object to the file
    bool result = writeFileJson("/config.json", jsonObject);
    if (result) {
        Serial.println("Config saved to LittleFS.");
    } else {
        Serial.println("Failed to save config to LittleFS.");
    }
}

void DeviceManager::configureDevices() {
    Serial.println("Configuring devices...");
    for (auto& device : devices) {
        for (auto& component : device.components) {
            Serial.print("Component ");
            Serial.print(component.componentName);  // Print the component name for better identification
            Serial.print(": componentPin=");
            Serial.print(component.componentPin);
            Serial.print(", actionPin=");
            Serial.print(component.actionPin);
            Serial.print(", componentType=");
            Serial.print(component.componentType);
            Serial.print(", actionType=");
            Serial.println(component.actionType);

            pinMode(component.componentPin, INPUT);
            pinMode(component.actionPin, OUTPUT);
            if (component.componentType == "digital") {
                digitalWrite(component.actionPin, LOW);
            } else if (component.componentType == "analog") {
                analogWrite(component.actionPin, 0);
            }

            // Initialize the component state
            component.state = ComponentState();

            Serial.print("Initialized component state for component ");
            Serial.print(component.componentName);
            Serial.print(": currentState=");
            Serial.print(component.state.currentState);
            Serial.print(", scheduledState=");
            Serial.print(component.state.scheduledState);
            Serial.print(", manualOverride=");
            Serial.println(component.state.manualOverride);
        }
    }
    Serial.println("Devices configured.");
}

void DeviceManager::checkScheduler() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        return;
    }

    int currentHour = timeinfo.tm_hour;
    int currentMinute = timeinfo.tm_min;

    for (auto& device : devices) {
        for (auto& component : device.components) {
            auto& state = component.state;

            if (std::find(component.behaviors.begin(), component.behaviors.end(), "scheduled") != component.behaviors.end()) {
                bool isInSchedule =
                    (currentHour > component.schedule.startHour ||
                     (currentHour == component.schedule.startHour && currentMinute >= component.schedule.startMinute)) &&
                    (currentHour < component.schedule.endHour ||
                     (currentHour == component.schedule.endHour && currentMinute <= component.schedule.endMinute));

                if (isInSchedule != state.scheduledState) {
                    state.scheduledState = isInSchedule;
                    state.manualOverride = false;
                    controlDigitalActuator(component.actionPin, state.scheduledState);
                    state.currentState = state.scheduledState;
                    Serial.println(state.scheduledState
                                       ? "Component turned ON based on schedule"
                                       : "Component turned OFF based on schedule");
                }
            }
        }
    }
}

bool DeviceManager::shouldHandleManualBehavior(const ComponentConfig& config, const ComponentState& state) {
    // Add specific conditions to decide if manual behavior should be handled
    if (std::find(config.behaviors.begin(), config.behaviors.end(), "toggle") != config.behaviors.end()) {
        // Example condition: manual override not already active
        return !state.manualOverride;
    } else if (std::find(config.behaviors.begin(), config.behaviors.end(), "pulse") != config.behaviors.end()) {
        // Example condition: some other specific condition
        return true;  // Adjust as needed
    } else if (std::find(config.behaviors.begin(), config.behaviors.end(), "timed") != config.behaviors.end()) {
        // Example condition: another specific condition
        return true;  // Adjust as needed
    }
    return false;  // Default to not handling if no conditions are met
}

void DeviceManager::readSensorsAndHandleBehaviors() {
    for (auto& device : devices) {
        for (auto& component : device.components) {
            bool sensorState = component.readDevice(component.componentPin);

            if (sensorState != component.state.previousSensorState) {  // Edge detection
                if (sensorState) {  // Only act on rising edge
                    handleManualBehavior(component, component.state);
                }
                component.state.previousSensorState = sensorState;  // Update previous state
            }
        }
    }
}

void DeviceManager::handleConfig(ESP8266WebServer* server) {
    Serial.println("Handling /config request...");
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"error\":\"No body\"}");
        return;
    }

    String body = server->arg("plain");
    Serial.print("Request body: ");
    Serial.println(body);

    JsonDocument doc;  // Adjust size as needed
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        Serial.print("JSON deserialization failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Backup current configuration
    auto backupDevices = devices;

    devices.clear();

    JsonArray devicesArray = doc["devices"].as<JsonArray>();
    if (devicesArray.isNull()) {
        server->send(400, "application/json", "{\"error\":\"Invalid JSON structure\"}");
        Serial.println("Invalid JSON structure: 'devices' is not an array");
        devices = backupDevices;  // Restore backup
        return;
    }

    for (JsonObject deviceJson : devicesArray) {
        Device newDevice;

        JsonArray componentsArray = deviceJson["components"].as<JsonArray>();
        if (componentsArray.isNull()) {
            server->send(400, "application/json", "{\"error\":\"Invalid JSON structure\"}");
            Serial.println("Invalid JSON structure: 'components' is not an array");
            devices = backupDevices;  // Restore backup
            return;
        }

        for (JsonObject componentJson : componentsArray) {
            ComponentConfig component;
            if (!componentJson.containsKey("componentName") ||
                !componentJson.containsKey("componentType") ||
                !componentJson.containsKey("componentPin") ||
                !componentJson.containsKey("actionType") ||
                !componentJson.containsKey("actionPin") ||
                !componentJson.containsKey("behaviors")) {
                server->send(400, "application/json", "{\"error\":\"Missing fields\"}");
                Serial.println("Error: JSON missing fields");
                devices = backupDevices;  // Restore backup
                return;
            }

            // Config
            component.componentName = componentJson["componentName"].as<String>();
            component.componentType = componentJson["componentType"].as<String>();
            component.componentPin = componentJson["componentPin"];
            component.actionType = componentJson["actionType"].as<String>();
            component.actionPin = componentJson["actionPin"];
            for (String behavior : componentJson["behaviors"].as<JsonArray>()) {
                component.behaviors.push_back(behavior);
            }

            // Schedule
            if (std::find(component.behaviors.begin(), component.behaviors.end(), "scheduled") != component.behaviors.end()) {
                if (!componentJson["schedule"].is<JsonObject>()) {
                    server->send(400, "application/json", "{\"error\":\"Invalid schedule format\"}");
                    Serial.println("Error: Invalid schedule format");
                    devices = backupDevices;  // Restore backup
                    return;
                }
                JsonObject scheduleJson = componentJson["schedule"];
                component.schedule.startHour = scheduleJson["startTime"]["hour"];
                component.schedule.startMinute = scheduleJson["startTime"]["minute"];
                component.schedule.endHour = scheduleJson["endTime"]["hour"];
                component.schedule.endMinute = scheduleJson["endTime"]["minute"];
            }

            newDevice.components.push_back(component);
        }

        devices.push_back(newDevice);

        Serial.print("Added device with components: ");
        for (const auto& component : newDevice.components) {
            Serial.print(component.componentName);
            Serial.print(" ");
        }
        Serial.println();
    }

    saveConfig(doc);
    configureDevices();
    populateFunctionPointers();
    server->send(200, "application/json", "{\"status\":\"Config updated\"}");
    Serial.println("Config updated successfully.");
}

void DeviceManager::handleControl(ESP8266WebServer* server) {
    Serial.println("Handling /control request...");
    if (!server->hasArg("plain")) {
        server->send(400, "application/json", "{\"error\":\"No body\"}");
        return;
    }

    String body = server->arg("plain");
    Serial.print("Request body: ");
    Serial.println(body);

    JsonDocument doc;  // Adjust size as needed
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        Serial.println("Invalid JSON");
        return;
    }

    String componentName = doc["componentName"];
    String action = doc["action"];
    bool state = doc["state"];

    bool componentFound = false;

    // Loop through the devices to find the component with the matching name
    for (auto& device : devices) {
        for (auto& component : device.components) {
            if (component.componentName == componentName) {
                componentFound = true;
                // Perform the control action on the component
                if (action == "control") {
                    if (component.componentType == "digital") {
                        controlDigitalActuator(component.componentPin, state);
                    } else if (component.componentType == "analog") {
                        controlAnalogActuator(component.componentPin, state);
                    }
                }
                component.state.updateManualOverride(true);
                handleManualBehavior(component, component.state);
                break;
            }
        }
        if (componentFound) {
            break;
        }
    }

    if (componentFound) {
        server->send(200, "application/json", "{\"status\":\"Action performed\"}");
        Serial.println("Action performed successfully.");
    } else {
        server->send(400, "application/json", "{\"error\":\"Invalid component name\"}");
        Serial.println("Invalid component name");
    }
}

void DeviceManager::handleGetDevices(ESP8266WebServer* server) {
    Serial.println("Handling /devices request...");
    JsonDocument doc;
    JsonArray devicesArray = doc["devices"].to<JsonArray>();

    for (const auto& device : devices) {
        JsonObject deviceJson = devicesArray.add<JsonObject>();
        JsonArray componentsArray = deviceJson["components"].to<JsonArray>();

        for (const auto& component : device.components) {
            JsonObject componentJson = componentsArray.add<JsonObject>();
            componentJson["componentName"] = component.componentName;
            componentJson["componentType"] = component.componentType;
            componentJson["componentPin"] = component.componentPin;
            componentJson["actionType"] = component.actionType;
            componentJson["actionPin"] = component.actionPin;

            JsonArray behaviorsArray = componentJson["behaviors"].to<JsonArray>();
            for (const auto& behavior : component.behaviors) {
                behaviorsArray.add(behavior);
            }

            JsonObject scheduleJson = componentJson["schedule"].to<JsonObject>();
            scheduleJson["startTime"]["hour"] = component.schedule.startHour;
            scheduleJson["startTime"]["minute"] = component.schedule.startMinute;
            scheduleJson["endTime"]["hour"] = component.schedule.endHour;
            scheduleJson["endTime"]["minute"] = component.schedule.endMinute;

            JsonObject stateJson = componentJson["state"].to<JsonObject>();
            stateJson["currentState"] = component.state.currentState;
            stateJson["scheduledState"] = component.state.scheduledState;
            stateJson["manualOverride"] = component.state.manualOverride;
        }
    }

    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
    Serial.println("Device configurations and states sent.");
}
