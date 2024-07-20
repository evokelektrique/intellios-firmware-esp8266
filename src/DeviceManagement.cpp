#include "DeviceManagement.h"

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

        for (JsonVariant script :
             componentJson["scripts"].as<JsonArray>()) {
            component.scripts.push_back(script.as<String>());
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

        JsonArray outputPinsArray =
            componentJson.createNestedArray("outputPins");
        for (int pin : component.outputPins) {
            outputPinsArray.add(pin);
        }

        JsonArray scriptsArray =
            componentJson.createNestedArray("scripts");
        for (const auto& script : component.scripts) {
            scriptsArray.add(script);
        }
    }

    String jsonString;
    serializeJson(doc, jsonString);
    writeFile("/config.json", jsonString.c_str());

    Serial.println("Config saved to LittleFS.");
}

void DeviceManager::configureComponents() {
    Serial.println("Configuring components...");
    setupPins();
    configureComponentScripts();
    Serial.println("Components configured.");
}

void DeviceManager::setupPins() {
    for (const auto& component : components) {
        Serial.print("Setting up pins for component: ");
        Serial.println(component.name);
        for (int pin : component.inputPins) {
            Serial.print("Setting up input pin: ");
            Serial.println(pin);
            pinMode(pin, INPUT);
        }
        for (int pin : component.outputPins) {
            Serial.print("Setting up output pin: ");
            Serial.println(pin);
            pinMode(pin, OUTPUT);
        }
    }
}

void DeviceManager::configureComponentScripts() {
    for (const auto& component : components) {
        Serial.print("Configuring scripts for component: ");
        Serial.println(component.name);
        for (const auto& script : component.scripts) {
            Serial.print("Adding script: ");
            Serial.println(script);
            scriptManager.addScript(script);
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
    scriptManager.clearScripts();

    JsonArray componentsArray = doc["components"].as<JsonArray>();
    if (componentsArray.isNull()) {
        server->send(400, "application/json",
                     "{\"error\":\"Invalid JSON structure\"}");
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

        for (JsonVariant script :
             componentJson["scripts"].as<JsonArray>()) {
            component.scripts.push_back(script.as<String>());
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

    scriptManager.setState(stateName, stateValue);

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

        JsonArray outputPinsArray =
            componentJson.createNestedArray("outputPins");
        for (int pin : component.outputPins) {
            outputPinsArray.add(pin);
        }

        JsonArray scriptsArray =
            componentJson.createNestedArray("scripts");
        for (const auto& script : component.scripts) {
            scriptsArray.add(script);
        }
    }

    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
    Serial.println("Component configurations and states sent.");
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
        auto it = std::find(component->inputPins.begin(),
                            component->inputPins.end(), pin);
        if (it != component->inputPins.end()) {
            return digitalRead(pin);
        }

        it = std::find(component->outputPins.begin(),
                       component->outputPins.end(), pin);
        if (it != component->outputPins.end()) {
            return analogRead(pin);
        }
    }
    return -1;
}

ScriptManager& DeviceManager::getScriptManager() {
    return scriptManager;
}
