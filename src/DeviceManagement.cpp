#include "DeviceManagement.h"
#include "ConditionManager.h"

DeviceManager::DeviceManager() {}

void DeviceManager::loadConfig() {
    Serial.println("Loading config from LittleFS...");

    String configFile = readFile("/config.json");

    Serial.println("Config File Content:");
    Serial.println(configFile);

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, configFile);
    if (error) {
        Serial.println("Failed to read config file");
        Serial.println(error.c_str());
        return;
    }

    serializeJson(doc, Serial);
    Serial.println();

    JsonArray componentsArray = doc["components"].as<JsonArray>();
    if (componentsArray.isNull()) {
        Serial.println("Invalid config structure");
        return;
    }

    components.clear();
    for (JsonObject componentJson : componentsArray) {
        Component component;
        component.name = componentJson["name"].as<String>();

        for (JsonVariant pin : componentJson["inputPins"].as<JsonArray>()) {
            component.inputPins.push_back(pin.as<int>());
        }

        for (JsonVariant pin : componentJson["outputPins"].as<JsonArray>()) {
            component.outputPins.push_back(pin.as<int>());
        }

        for (JsonVariant condition : componentJson["conditions"].as<JsonArray>()) {
            component.conditions.push_back(condition.as<String>());
        }

        components.push_back(component);
    }

    Serial.println("Config loaded from LittleFS.");
}

void DeviceManager::saveConfig() {
    Serial.println("Saving config to LittleFS...");

    StaticJsonDocument<2048> doc;
    JsonArray componentsArray = doc.createNestedArray("components");

    for (const auto& component : components) {
        JsonObject componentJson = componentsArray.createNestedObject();
        componentJson["name"] = component.name;

        JsonArray inputPinsArray = componentJson.createNestedArray("inputPins");
        for (int pin : component.inputPins) {
            inputPinsArray.add(pin);
        }

        JsonArray outputPinsArray = componentJson.createNestedArray("outputPins");
        for (int pin : component.outputPins) {
            outputPinsArray.add(pin);
        }

        JsonArray conditionsArray = componentJson.createNestedArray("conditions");
        for (const auto& condition : component.conditions) {
            conditionsArray.add(condition);
        }
    }

    String jsonString;
    serializeJson(doc, jsonString);

    writeFile("/config.json", jsonString.c_str());  // Convert String to const char*

    Serial.println("Config saved to LittleFS.");
}

void DeviceManager::configureComponents() {
    Serial.println("Configuring components...");
    setupPins();
    configureComponentConditions();
    Serial.println("Components configured.");
}

void DeviceManager::setupPins() {
    for (const auto& component : components) {
        for (int pin : component.inputPins) {
            pinMode(pin, INPUT);
        }
        for (int pin : component.outputPins) {
            pinMode(pin, OUTPUT);
        }
    }
}

void DeviceManager::configureComponentConditions() {
    for (const auto& component : components) {
        for (const auto& condition : component.conditions) {
            conditionManager.addCondition(condition);
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

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        Serial.println("JSON deserialization failed");
        return;
    }

    components.clear();
    JsonArray componentsArray = doc["components"].as<JsonArray>();
    if (componentsArray.isNull()) {
        server->send(400, "application/json", "{\"error\":\"Invalid JSON structure\"}");
        Serial.println("Invalid JSON structure");
        return;
    }

    for (JsonObject componentJson : componentsArray) {
        Component component;
        component.name = componentJson["name"].as<String>();

        for (JsonVariant pin : componentJson["inputPins"].as<JsonArray>()) {
            component.inputPins.push_back(pin.as<int>());
        }

        for (JsonVariant pin : componentJson["outputPins"].as<JsonArray>()) {
            component.outputPins.push_back(pin.as<int>());
        }

        for (JsonVariant condition : componentJson["conditions"].as<JsonArray>()) {
            component.conditions.push_back(condition.as<String>());
        }

        components.push_back(component);
    }

    saveConfig();
    configureComponents();

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

    StaticJsonDocument<2048> doc;
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        server->send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        Serial.println("JSON deserialization failed");
        return;
    }

    String stateName = doc["stateName"];
    bool stateValue = doc["stateValue"];

    conditionManager.setState(stateName, stateValue);

    server->send(200, "application/json", "{\"status\":\"State updated\"}");
    Serial.println("State updated successfully.");
}

void DeviceManager::handleGetComponents(ESP8266WebServer* server) {
    Serial.println("Handling /components request...");

    StaticJsonDocument<2048> doc;
    JsonArray componentsArray = doc.createNestedArray("components");

    for (const auto& component : components) {
        JsonObject componentJson = componentsArray.createNestedObject();
        componentJson["name"] = component.name;

        JsonArray inputPinsArray = componentJson.createNestedArray("inputPins");
        for (int pin : component.inputPins) {
            inputPinsArray.add(pin);
        }

        JsonArray outputPinsArray = componentJson.createNestedArray("outputPins");
        for (int pin : component.outputPins) {
            outputPinsArray.add(pin);
        }

        JsonArray conditionsArray = componentJson.createNestedArray("conditions");
        for (const auto& condition : component.conditions) {
            conditionsArray.add(condition);
        }
    }

    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
    Serial.println("Component configurations and states sent.");
}

void DeviceManager::evaluateConditions() {
    conditionManager.evaluateConditions();
}

Component* DeviceManager::getComponentByName(const String& name) {
    for (auto& component : components) {
        if (component.name == name) {
            return &component;
        }
    }
    return nullptr;
}

int DeviceManager::readComponentState(const String& componentName, int pin) {
    Component* component = getComponentByName(componentName);
    if (component) {
        // Determine if the pin is an input pin
        auto it = std::find(component->inputPins.begin(), component->inputPins.end(), pin);
        if (it != component->inputPins.end()) {
            // Perform a digital read if it's an input pin
            return digitalRead(pin);
        }

        // Determine if the pin is an output pin
        it = std::find(component->outputPins.begin(), component->outputPins.end(), pin);
        if (it != component->outputPins.end()) {
            // Perform an analog read if it's an output pin
            return analogRead(pin);
        }
    }
    return -1; // Invalid state if the pin is not found
}
