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
      }
    },
    {
      "id": "touch_sensor",
      "label": "Touch Sensor",
      "pins": ["pin2"],
      "state": {
        "touched": false
      }
    }
  ],
  "rules": [
    {
      "id": "rule1",
      "label": "Touch Sensor to LED On",
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
          "value": true
        }
      ]
    },
    {
      "id": "rule2",
      "label": "Touch Sensor to LED Off",
      "conditions": [
        {
          "device_id": "touch_sensor",
          "property": "touched",
          "operator": "==",
          "value": false
        }
      ],
      "actions": [
        {
          "device_id": "led",
          "property": "status",
          "value": false
        }
      ]
    }
  ]
}
)";

// struct Version {
//     float number;
// };

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
Task evaluateRulesTask(10, TASK_FOREVER, &evaluateAndUpdate, &runner, true);

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

    // Version version;
    // version.number = doc["version"];

    for (Rule& rule : rules) {
        Serial.println("Rule Label: " + rule.label);

        // Print conditions
        Serial.println("Conditions:");
        for (const auto& condition : rule.conditions) {
            Serial.println("  Device ID: " + condition->device_id);
            Serial.println("  Property: " + condition->property);
            Serial.println("  Operator: " + condition->operatorStr);
            Serial.print("  Value: ");
            if (condition->value.is<bool>()) {
                Serial.println(condition->value.as<bool>());
            } else if (condition->value.is<int>()) {
                Serial.println(condition->value.as<int>());
            } else if (condition->value.is<float>()) {
                Serial.println(condition->value.as<float>());
            } else if (condition->value.is<const char*>()) {
                Serial.println(condition->value.as<const char*>());
            } else {
                Serial.println("Unknown type");
            }
        }

        // Print actions
        Serial.println("Actions:");
        for (const auto& action : rule.actions) {
            Serial.println("  Device ID: " + action->device_id);
            Serial.println("  Property: " + action->property);
            Serial.print("  Value: ");
            if (action->value.is<bool>()) {
                Serial.println(action->value.as<bool>());
            } else if (action->value.is<int>()) {
                Serial.println(action->value.as<int>());
            } else if (action->value.is<float>()) {
                Serial.println(action->value.as<float>());
            } else if (action->value.is<const char*>()) {
                Serial.println(action->value.as<const char*>());
            } else {
                Serial.println("Unknown type");
            }
        }

        Serial.println();
    }

    runner.startNow();
}

void loop() {
    runner.execute();
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

        devices.push_back(device);

        Serial.println("Device ID: " + device.id);
        Serial.println("Label: " + device.label);
    }
}

void setupRules(JsonArray& rulesArray) {
    for (JsonObject ruleObject : rulesArray) {
        Rule rule;
        rule.id = ruleObject["id"].as<String>();
        Serial.println("Parsed Rule ID: " + rule.id);

        rule.label = ruleObject["label"].as<String>();
        Serial.println("Parsed Rule Label: " + rule.label);

        JsonArray conditionsArray = ruleObject["conditions"].as<JsonArray>();
        Serial.println("Parsed conditionsArray");

        for (JsonObject conditionObject : conditionsArray) {
            auto condition = std::make_unique<Condition>();
            if (conditionObject.containsKey("device_id")) {
                condition->device_id =
                    conditionObject["device_id"].as<String>();
                Serial.println("  Parsed Condition Device ID: " +
                               condition->device_id);
            }
            if (conditionObject.containsKey("property")) {
                condition->property = conditionObject["property"].as<String>();
                Serial.println("  Parsed Condition Property: " +
                               condition->property);
            }
            if (conditionObject.containsKey("operator")) {
                condition->operatorStr =
                    conditionObject["operator"].as<String>();
                Serial.println("  Parsed Condition Operator: " +
                               condition->operatorStr);
            }
            if (conditionObject.containsKey("value")) {
                condition->value = conditionObject["value"];
            }

            rule.conditions.push_back(std::move(condition));
            Serial.println("Added condition to rule.conditions");
        }

        JsonArray actionsArray = ruleObject["actions"].as<JsonArray>();
        Serial.println("Parsed actionsArray");
        for (JsonObject actionObject : actionsArray) {
            auto action = std::make_unique<Action>();
            if (actionObject.containsKey("device_id")) {
                action->device_id = actionObject["device_id"].as<String>();
                Serial.println("  Parsed Action Device ID: " +
                               action->device_id);
            }
            if (actionObject.containsKey("property")) {
                action->property = actionObject["property"].as<String>();
                Serial.println("  Parsed Action Property: " + action->property);
            }
            if (actionObject.containsKey("value")) {
                action->value = actionObject["value"];
            }

            rule.actions.push_back(std::move(action));
            Serial.println("Added action to rule.actions");
        }

        rules.push_back(std::move(rule));
        Serial.println("Added rule to rules");

        // Debug prints for rules
        Serial.println("Rule ID: " + rule.id);
        Serial.println("Rule Label: " + rule.label);
    }
}

void evaluateAndUpdate() {
    // Update sensor states
    for (Device& device : devices) {
        for (const String& pinId : device.pins) {
            auto pinIt = std::find_if(pins.begin(), pins.end(), [&](const Pin& p) {
                return p.id == pinId;
            });

            if (pinIt != pins.end() && pinIt->mode == "INPUT") {
                bool sensorValue = digitalRead(pinIt->gpio) == HIGH;
                device.state["touched"].set(sensorValue);
                Serial.println("Updated device " + device.id + " state: " + String(sensorValue));
            }
        }
    }

    // Evaluate rules
    for (Rule& rule : rules) {
        bool conditionsMet = true;
        Serial.print("Evaluating Rule: " + rule.label);

        for (const auto& condition : rule.conditions) {
            auto deviceIt = std::find_if(devices.begin(), devices.end(), [&](const Device& d) {
                return d.id == condition->device_id;
            });

            if (deviceIt == devices.end()) {
                Serial.println(" - Device not found: " + condition->device_id);
                conditionsMet = false;
                break;
            }

            Device& device = *deviceIt;
            auto stateIt = device.state.find(condition->property);
            if (stateIt == device.state.end()) {
                Serial.println(" - Property not found: " + condition->property);
                conditionsMet = false;
                break;
            }

            bool conditionMet = false;
            if (condition->operatorStr == "==") {
                conditionMet = (stateIt->second == condition->value);
            } else if (condition->operatorStr == "!=") {
                conditionMet = (stateIt->second != condition->value);
            } else if (condition->operatorStr == "<") {
                conditionMet = (stateIt->second < condition->value);
            } else if (condition->operatorStr == ">") {
                conditionMet = (stateIt->second > condition->value);
            } else if (condition->operatorStr == "<=") {
                conditionMet = (stateIt->second <= condition->value);
            } else if (condition->operatorStr == ">=") {
                conditionMet = (stateIt->second >= condition->value);
            } else {
                Serial.println(" - Unsupported operator: " + condition->operatorStr);
            }

            Serial.print(" - Condition: " + condition->device_id + "." + condition->property + " " + condition->operatorStr + " " + String(condition->value.as<bool>()));
            Serial.println(conditionMet ? " - Met" : " - Not Met");

            if (!conditionMet) {
                conditionsMet = false;
                break;
            }
        }

        if (conditionsMet) {
            Serial.println(" - Conditions met");
            for (const auto& action : rule.actions) {
                auto deviceIt = std::find_if(devices.begin(), devices.end(), [&](const Device& d) {
                    return d.id == action->device_id;
                });

                if (deviceIt == devices.end()) {
                    Serial.println(" - Action device not found: " + action->device_id);
                    continue;
                }

                Device& device = *deviceIt;
                device.state[action->property].set(action->value);

                Serial.println(" - Action: " + action->device_id + "." + action->property + " = " + String(action->value.as<bool>()));

                auto pinIt = std::find_if(pins.begin(), pins.end(), [&](const Pin& p) {
                    return std::find(device.pins.begin(), device.pins.end(), p.id) != device.pins.end();
                });

                if (pinIt != pins.end() && action->property == "status") {
                    bool value = action->value.as<bool>();
                    digitalWrite(pinIt->gpio, value ? HIGH : LOW);
                    Serial.println(" - Setting GPIO " + String(pinIt->gpio) + " to " + String(value ? "HIGH" : "LOW"));
                }
            }
        } else {
            Serial.println(" - Conditions not met");
        }
    }
}
