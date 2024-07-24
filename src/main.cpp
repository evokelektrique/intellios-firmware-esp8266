#include <Arduino.h>
#include <ArduinoJson.h>
#include <TaskScheduler.h>

#include <map>
#include <memory>
#include <vector>

// JSON configuration with version
const char* jsonConfig = R"(
{
  "version": "1.0",
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

class StateManager {
   public:
    void setState(const String& deviceId, const String& property, bool value) {
        states[deviceId][property] = value;
    }

    bool getState(const String& deviceId, const String& property) const {
        auto deviceIt = states.find(deviceId);
        if (deviceIt != states.end()) {
            auto propertyIt = deviceIt->second.find(property);
            if (propertyIt != deviceIt->second.end()) {
                return propertyIt->second;
            }
        }
        return false;
    }

   private:
    std::map<String, std::map<String, bool>> states;
};

StateManager stateManager;

// Struct Definitions
struct Pin {
    String id;
    String label;
    String type;
    int gpio;
    String mode;
};

struct Device {
    String id;
    String label;
    std::vector<String> pins;
    std::map<String, JsonVariant> state;
    unsigned long latency;
    unsigned long lastRunTime;
    bool previousState;
};

struct Condition {
    String device_id;
    String property;
    String operatorStr;
    JsonVariant value;
};

struct Action {
    String device_id;
    String property;
    String action_type;
    JsonVariant value;
};

struct Rule {
    String id;
    String label;
    std::vector<std::unique_ptr<Condition>> conditions;
    std::vector<std::unique_ptr<Action>> actions;
};

std::vector<Pin> pins;
std::vector<Device> devices;
std::vector<Rule> rules;

void setupPins(JsonArray& pinsArray);
void setupDevices(JsonArray& devicesArray);
void setupRules(JsonArray& rulesArray);
void evaluateAndUpdate();

// TaskScheduler
Scheduler runner;

void setup() {
    Serial.begin(9600);

    JsonDocument doc;

    DeserializationError error = deserializeJson(doc, jsonConfig);
    if (error) {
        Serial.print("Couldn't parse JSON: ");
        Serial.println(error.c_str());
        return;
    }

    JsonArray pinsArray = doc["pins"].as<JsonArray>();
    setupPins(pinsArray);

    JsonArray devicesArray = doc["devices"].as<JsonArray>();
    setupDevices(devicesArray);

    JsonArray rulesArray = doc["rules"].as<JsonArray>();
    setupRules(rulesArray);

    runner.startNow();
}

void loop() {
    evaluateAndUpdate();
}

void setupPins(JsonArray& pinsArray) {
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

void setupDevices(JsonArray& devicesArray) {
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

void setupRules(JsonArray& rulesArray) {
    for (JsonObject ruleObject : rulesArray) {
        Rule rule;
        rule.id = ruleObject["id"].as<String>();
        rule.label = ruleObject["label"].as<String>();

        JsonArray conditionsArray = ruleObject["conditions"].as<JsonArray>();

        for (JsonObject conditionObject : conditionsArray) {
            auto condition = std::make_unique<Condition>();
            if (conditionObject.containsKey("device_id")) {
                condition->device_id = conditionObject["device_id"].as<String>();
            }
            if (conditionObject.containsKey("property")) {
                condition->property = conditionObject["property"].as<String>();
            }
            if (conditionObject.containsKey("operator")) {
                condition->operatorStr = conditionObject["operator"].as<String>();
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

void evaluateAndUpdate() {
    unsigned long currentTime = millis();

    // Iterate through each rule
    for (Rule& rule : rules) {
        bool conditionsMet = true;
        bool sensorValueChanged = false;
        bool sensorValue = false;

        // Check each condition for the current rule
        for (const auto& condition : rule.conditions) {
            bool conditionMet = false;

            // Find the device associated with the condition
            auto deviceIt = std::find_if(
                devices.begin(), devices.end(),
                [&](const Device& d) { return d.id == condition->device_id; });

            if (deviceIt == devices.end()) {
                conditionsMet = false;
                break;
            }

            Device& device = *deviceIt;

            // Find the pin associated with the condition
            auto pinIt = std::find_if(pins.begin(), pins.end(), [&](const Pin& p) {
                return std::find(device.pins.begin(), device.pins.end(),
                                 p.id) != device.pins.end() &&
                       p.mode == "INPUT";
            });

            if (pinIt != pins.end()) {
                sensorValue = digitalRead(pinIt->gpio) == HIGH;

                if (condition->value.is<bool>()) {
                    // Handle boolean conditions
                    if (condition->operatorStr == "==") {
                        conditionMet = (sensorValue == condition->value.as<bool>());
                    } else if (condition->operatorStr == "!=") {
                        conditionMet = (sensorValue != condition->value.as<bool>());
                    }
                } else if (condition->value.is<int>()) {
                    // Handle integer conditions
                    int sensorValueInt = sensorValue ? 1 : 0;
                    int conditionValue = condition->value.as<int>();

                    if (condition->operatorStr == "==") {
                        conditionMet = (sensorValueInt == conditionValue);
                    } else if (condition->operatorStr == "!=") {
                        conditionMet = (sensorValueInt != conditionValue);
                    } else if (condition->operatorStr == ">") {
                        conditionMet = (sensorValueInt > conditionValue);
                    } else if (condition->operatorStr == "<") {
                        conditionMet = (sensorValueInt < conditionValue);
                    } else if (condition->operatorStr == ">=") {
                        conditionMet = (sensorValueInt >= conditionValue);
                    } else if (condition->operatorStr == "<=") {
                        conditionMet = (sensorValueInt <= conditionValue);
                    }
                }

                if (device.previousState != sensorValue) {
                    sensorValueChanged = true;
                }

                if (!conditionMet) {
                    conditionsMet = false;
                    break;
                }
            } else {
                conditionsMet = false;
                break;
            }
        }

        // Perform actions if all conditions are met and the sensor value changed
        if (conditionsMet && sensorValueChanged) {
            for (const auto& action : rule.actions) {
                // Find the device associated with the action
                auto deviceIt = std::find_if(
                    devices.begin(), devices.end(),
                    [&](const Device& d) { return d.id == action->device_id; });

                if (deviceIt == devices.end()) {
                    continue;
                }

                Device& device = *deviceIt;

                // Check if the latency period has elapsed for the device
                if (currentTime - device.lastRunTime < device.latency) {
                    Serial.println("Ignored due to (device latency: " +
                                   String(device.latency) +
                                   ") and (device lastRunTime: " +
                                   String(device.lastRunTime) + ")");
                    continue;  // Skip this device and move on to the next one
                }

                // Find the pin associated with the action
                auto pinIt = std::find_if(pins.begin(), pins.end(), [&](const Pin& p) {
                    return std::find(device.pins.begin(), device.pins.end(),
                                     p.id) != device.pins.end() &&
                           p.mode == "OUTPUT";
                });

                if (pinIt != pins.end()) {
                    if (action->action_type == "toggle") {
                        // Toggle the state only if the sensorValue has changed from false to true
                        if (sensorValue && !device.previousState) {
                            bool currentState = stateManager.getState(
                                action->device_id, action->property);
                            bool newState = !currentState;
                            stateManager.setState(action->device_id,
                                                  action->property, newState);
                            digitalWrite(pinIt->gpio, newState ? HIGH : LOW);
                        }
                    } else if (action->action_type == "set") {
                        // Set the state
                        bool newState = action->value.as<bool>();
                        stateManager.setState(action->device_id,
                                              action->property, newState);
                        digitalWrite(pinIt->gpio, newState ? HIGH : LOW);
                    } else {
                        // Handle other action types if needed
                        Serial.println("Unknown action type: " +
                                       action->action_type);
                    }

                    // Update lastRunTime for the device after performing the action
                    device.lastRunTime = currentTime;
                }
            }
        }

        // Update the device's previous state after evaluating all conditions
        for (auto& device : devices) {
            if (std::any_of(rule.conditions.begin(), rule.conditions.end(),
                            [&](const std::unique_ptr<Condition>& condition) {
                                return condition->device_id == device.id;
                            })) {
                device.previousState = sensorValue;
            }
        }
    }
}
