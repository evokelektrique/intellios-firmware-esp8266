#include "ConfigManager.h"

ConfigManager::ConfigManager() {}

void ConfigManager::populateFromFile(const char* path) {
    JsonDocument doc = readJsonFile(path);

    // Serial.println("Config File Content:");
    // serializeJson(doc, Serial);
    // Serial.println();

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

        // // Print pin details
        // Serial.println("Pin ID: " + pin.id);
        // Serial.println("Label: " + pin.label);
        // Serial.println("Type: " + pin.type);
        // Serial.println("GPIO: " + String(pin.gpio));
        // Serial.println("Mode: " + pin.mode);
        // Serial.println();
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

        // Serial.println("Device ID: " + device.id);
        // Serial.println("Label: " + device.label);
        // Serial.println("Latency: " + String(device.latency));
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

        // // Print deviceComponent details
        // Serial.println("DeviceComponent ID: " + deviceComponent.id);
        // Serial.println("Device ID: " + deviceComponent.device_id);
        // Serial.println("Property: " + deviceComponent.property);
        // Serial.println("Type: " + deviceComponent.type);
        // Serial.println();
    }
}

void ConfigManager::setupRules(JsonArray& rulesArray) {
    for (JsonObject ruleObject : rulesArray) {
        Rule rule;
        rule.id = ruleObject["id"].as<String>();
        rule.label = ruleObject["label"].as<String>();
        rule.triggerOnChange = ruleObject["triggerOnChange"].as<bool>();

        JsonArray conditionsArray = ruleObject["conditions"].as<JsonArray>();

        for (JsonObject conditionObject : conditionsArray) {
            auto condition = std::make_unique<Condition>();
            condition->device_id = conditionObject["device_id"].as<String>();
            condition->property = conditionObject["property"].as<String>();
            condition->operatorStr = conditionObject["operator"].as<String>();
            if (conditionObject.containsKey("value")) {
                if (conditionObject["value"].is<bool>()) {
                    condition->value = conditionObject["value"].as<bool>();
                } else if (conditionObject["value"].is<int>()) {
                    condition->value = conditionObject["value"].as<int>();
                } else if (conditionObject["value"].is<float>()) {
                    condition->value = conditionObject["value"].as<float>();
                } else if (conditionObject["value"].is<double>()) {
                    condition->value = conditionObject["value"].as<double>();
                } else if (conditionObject["value"].is<String>()) {
                    condition->value = conditionObject["value"].as<String>();
                } else {
                    Serial.println("Unsupported type in condition value");
                }
            }

            rule.conditions.push_back(std::move(condition));
        }

        JsonArray actionsArray = ruleObject["actions"].as<JsonArray>();

        for (JsonObject actionObject : actionsArray) {
            auto action = std::make_unique<Action>();
            action->device_id = actionObject["device_id"].as<String>();
            action->property = actionObject["property"].as<String>();
            action->action_type = actionObject["action_type"].as<String>();
            if (actionObject.containsKey("value")) {
                if (actionObject["value"].is<bool>()) {
                    action->value = actionObject["value"].as<bool>();
                } else if (actionObject["value"].is<int>()) {
                    action->value = actionObject["value"].as<int>();
                } else if (actionObject["value"].is<float>()) {
                    action->value = actionObject["value"].as<float>();
                } else if (actionObject["value"].is<double>()) {
                    action->value = actionObject["value"].as<double>();
                } else if (actionObject["value"].is<String>()) {
                    action->value = actionObject["value"].as<String>();
                } else {
                    Serial.println("Unsupported type in action value");
                }
            }

            rule.actions.push_back(std::move(action));
        }

        rules.push_back(std::move(rule));
    }
}
