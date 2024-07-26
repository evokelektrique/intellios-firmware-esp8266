#include "ConfigManager.h"

ConfigManager::ConfigManager() {}

void ConfigManager::populateFromFile(const char* path) {
    JsonDocument doc = readJsonFile(path);

    Serial.println("Config File Content:");
    serializeJson(doc, Serial);
    Serial.println();

    JsonArray pinsArray = doc["pins"].as<JsonArray>();
    JsonArray devicesArray = doc["devices"].as<JsonArray>();
    JsonArray componentsArray = doc["components"].as<JsonArray>();
    JsonArray rulesArray = doc["rules"].as<JsonArray>();

    if (pinsArray.isNull() || devicesArray.isNull() ||
        componentsArray.isNull() || rulesArray.isNull()) {
        Serial.println("Invalid config structure");
        return;
    }

    setupPins(pinsArray);
    setupDevices(devicesArray);
    setupDeviceComponents(componentsArray);
    setupRules(rulesArray);

    Serial.println("Config loaded from LittleFS.");
}

bool ConfigManager::save(const char* path, const char* data) {
    bool file = writeFile(path, data);

    if (!file) {
        Serial.println("Could not save to file: " + String(path));
    }

    return file;
}

void ConfigManager::setupPins(JsonArray& pinsArray) {
    for (JsonObject pinObject : pinsArray) {
        Pin pin;
        pin.id = pinObject["id"].as<String>();
        pin.label = pinObject["label"].as<String>();
        pin.type = pinObject["type"].as<String>();
        pin.gpio = pinObject["gpio"].as<int>();
        pin.mode = pinObject["mode"].as<String>();

        pins.push_back(pin);

        pinMode(pin.gpio, pin.mode == "OUTPUT" ? OUTPUT : INPUT);

        // Print pin details
        Serial.println("Pin ID: " + pin.id);
        Serial.println("Label: " + pin.label);
        Serial.println("Type: " + pin.type);
        Serial.println("GPIO: " + String(pin.gpio));
        Serial.println("Mode: " + pin.mode);
        Serial.println();
    }
}

void ConfigManager::setupDevices(JsonArray& devicesArray) {
    for (JsonObject deviceObject : devicesArray) {
        Device device;
        device.id = deviceObject["id"].as<String>();
        device.label = deviceObject["label"].as<String>();

        JsonArray pinsArray = deviceObject["pins"].as<JsonArray>();
        for (JsonVariant pin : pinsArray) {
            device.pins.push_back(pin.as<String>());
        }

        JsonObject stateObject = deviceObject["state"].as<JsonObject>();
        for (JsonPair kv : stateObject) {
            device.state[kv.key().c_str()] = kv.value();
        }

        device.latency = deviceObject["latency"].as<unsigned long>();
        device.lastRunTime = 0;
        device.previousState = false;

        devices.push_back(device);

        Serial.println("Device ID: " + device.id);
        Serial.println("Label: " + device.label);
        Serial.println("Latency: " + String(device.latency));
    }
}

void ConfigManager::setupDeviceComponents(JsonArray& componentsArray) {
    for (JsonObject deviceComponentObject : componentsArray) {
        DeviceComponent deviceComponent;
        deviceComponent.id = deviceComponentObject["id"].as<String>();
        deviceComponent.device_id =
            deviceComponentObject["device_id"].as<String>();
        deviceComponent.property =
            deviceComponentObject["property"].as<String>();
        deviceComponent.type = deviceComponentObject["type"].as<String>();

        deviceComponents.push_back(deviceComponent);

        // Print deviceComponent details
        Serial.println("DeviceComponent ID: " + deviceComponent.id);
        Serial.println("Device ID: " + deviceComponent.device_id);
        Serial.println("Property: " + deviceComponent.property);
        Serial.println("Type: " + deviceComponent.type);
        Serial.println();
    }
}

void ConfigManager::setupRules(JsonArray& rulesArray) {
    for (JsonObject ruleObject : rulesArray) {
        Rule rule;
        rule.id = ruleObject["id"].as<String>();
        rule.label = ruleObject["label"].as<String>();

        JsonArray conditionsArray = ruleObject["conditions"].as<JsonArray>();

        for (JsonObject conditionObject : conditionsArray) {
            auto condition = std::make_unique<Condition>();
            if (conditionObject.containsKey("device_id")) {
                condition->device_id =
                    conditionObject["device_id"].as<String>();
            }
            if (conditionObject.containsKey("property")) {
                condition->property = conditionObject["property"].as<String>();
            }
            if (conditionObject.containsKey("operator")) {
                condition->operatorStr =
                    conditionObject["operator"].as<String>();
            }
            if (conditionObject.containsKey("value")) {
                condition->value = conditionObject["value"];
            }

            rule.conditions.push_back(std::move(condition));
        }

        JsonArray actionsArray = ruleObject["actions"].as<JsonArray>();

        for (JsonObject actionObject : actionsArray) {
            auto action = std::make_unique<Action>();
            if (actionObject.containsKey("device_id")) {
                action->device_id = actionObject["device_id"].as<String>();
            }

            if (actionObject.containsKey("property")) {
                action->property = actionObject["property"].as<String>();
            }

            if (actionObject.containsKey("action_type")) {
                action->action_type = actionObject["action_type"].as<String>();
            }

            if (actionObject.containsKey("value")) {
                action->value = actionObject["value"];
            }

            rule.actions.push_back(std::move(action));
        }

        rules.push_back(std::move(rule));
    }
}
