#include "ConditionManager.h"

// Tokenize the condition string
std::vector<String> ConditionManager::tokenize(const String& condition) {
    std::vector<String> tokens;
    int start = 0;
    int end = condition.indexOf(' ');

    while (end != -1) {
        tokens.push_back(condition.substring(start, end));
        start = end + 1;
        end = condition.indexOf(' ', start);
    }

    tokens.push_back(condition.substring(start));
    return tokens;
}

// Create a map for comparison operators
std::map<String, std::function<bool(int, int)>>
ConditionManager::getComparisonMap() {
    return {{"=", std::equal_to<int>()},       {"!=", std::not_equal_to<int>()},
            {">", std::greater<int>()},        {"<", std::less<int>()},
            {">=", std::greater_equal<int>()}, {"<=", std::less_equal<int>()}};
}

// Helper function to toggle a digital pin
void ConditionManager::toggleDigital(int pin) {
    int currentState = digitalRead(pin);
    digitalWrite(pin, !currentState);
}

// Helper function to pulse a digital pin for a duration
void ConditionManager::pulseDigital(int pin, int duration) {
    digitalWrite(pin, HIGH);
    delay(duration);
    digitalWrite(pin, LOW);
}

// Helper function to write a digital state to a pin
void ConditionManager::writeDigital(int pin, bool state) {
    digitalWrite(pin, state ? HIGH : LOW);
}

// Helper function to write an analog value to a pin
void ConditionManager::writeAnalog(int pin, int value) {
    analogWrite(pin, value);
}

// Add condition to the list
void ConditionManager::addCondition(const String& conditionStr) {
    conditions.push_back(parseCondition(conditionStr));
}

// Parse and create condition and action functions
Condition ConditionManager::parseCondition(const String& conditionStr) {
    std::vector<String> tokens = tokenize(conditionStr);
    Condition cond;
    cond.checkPrevious = false;
    cond.waitActive = false;
    int i = 0;

    // Initialize the comparison map
    auto comparisonMap = getComparisonMap();

    // Parse the 'if' part
    if (tokens[i] == "if") {
        String sensor = tokens[i + 1];
        String operator_ = tokens[i + 2];
        String value = tokens[i + 3];
        int threshold = 512;  // Default threshold value

        // Check for an optional threshold value
        if (tokens.size() > i + 4 && tokens[i + 4].startsWith("threshold=")) {
            threshold = tokens[i + 4].substring(10).toInt();
            i++;
        }

        if (sensor.startsWith("readDigital")) {
            int pin = sensor.substring(10).toInt();
            cond.conditionFunc = [pin, operator_, value, comparisonMap]() {
                int sensorValue = digitalRead(pin);
                return comparisonMap.at(operator_)(sensorValue,
                                                   value == "on" ? HIGH : LOW);
            };
            cond.checkPreviousStateFunc = [this, pin, &cond]() {
                bool currentState = digitalRead(pin) == HIGH;
                return stateChanged(currentState, cond.previousState);
            };
        } else if (sensor.startsWith("readAnalog")) {
            int pin = sensor.substring(9).toInt();
            cond.conditionFunc = [pin, operator_, value, comparisonMap,
                                  threshold]() {
                int sensorValue = analogRead(pin);
                return comparisonMap.at(operator_)(sensorValue, value.toInt());
            };
            cond.checkPreviousStateFunc = [this, pin, &cond, threshold]() {
                bool currentState = analogRead(pin) > threshold;
                return stateChanged(currentState, cond.previousState);
            };
        } else if (sensor.startsWith("getState")) {
            String stateName = sensor.substring(8);
            cond.conditionFunc = [this, stateName, operator_, value,
                                  comparisonMap]() {
                bool stateValue = getState(stateName);
                return comparisonMap.at(operator_)(stateValue, value == "true");
            };
            cond.checkPreviousStateFunc = [this, stateName, &cond]() {
                bool currentState = getState(stateName);
                return stateChanged(currentState, cond.previousState);
            };
        }
        i += 4;
    }

    // Check if previous state checking is required
    if (tokens[i] == "check_previous") {
        cond.checkPrevious = true;
        i++;
    }

    // Parse the 'then' part
    if (tokens[i] == "then") {
        String action = tokens[i + 1];
        int actionPin = tokens[i + 2].substring(3).toInt();
        String actionValue = tokens.size() > i + 3 ? tokens[i + 3] : "";
        int duration = (tokens.size() > i + 5) ? tokens[i + 5].toInt() : 0;

        if (action == "toggle") {
            cond.actionFunc = [this, actionPin]() { toggleDigital(actionPin); };
        } else if (action == "pulse") {
            cond.actionFunc = [this, actionPin, duration]() {
                pulseDigital(actionPin, duration);
            };
        } else if (action == "writeDigital") {
            cond.actionFunc = [this, actionPin, actionValue]() {
                writeDigital(actionPin, actionValue == "on");
            };
        } else if (action == "writeAnalog") {
            cond.actionFunc = [this, actionPin, actionValue]() {
                writeAnalog(actionPin, actionValue.toInt());
            };
        } else if (action == "wait") {
            cond.waitTime = duration;
            cond.waitActive = true;
            cond.actionFunc = []() {};  // No action during wait, only delay
        }
        i += (tokens.size() > i + 5) ? 6 : 3;
    }

    // Parse the 'else' part
    if (tokens.size() > i && tokens[i] == "else") {
        String elseAction = tokens[i + 1];
        int elseActionPin = tokens[i + 2].substring(3).toInt();
        String elseActionValue = tokens.size() > i + 3 ? tokens[i + 3] : "";
        int elseDuration = (tokens.size() > i + 5) ? tokens[i + 5].toInt() : 0;

        if (elseAction == "toggle") {
            cond.elseActionFunc = [this, elseActionPin]() {
                toggleDigital(elseActionPin);
            };
        } else if (elseAction == "pulse") {
            cond.elseActionFunc = [this, elseActionPin, elseDuration]() {
                pulseDigital(elseActionPin, elseDuration);
            };
        } else if (elseAction == "writeDigital") {
            cond.elseActionFunc = [this, elseActionPin, elseActionValue]() {
                writeDigital(elseActionPin, elseActionValue == "on");
            };
        } else if (elseAction == "writeAnalog") {
            cond.elseActionFunc = [this, elseActionPin, elseActionValue]() {
                writeAnalog(elseActionPin, elseActionValue.toInt());
            };
        }
    }

    return cond;
}

// Evaluate conditions and execute actions
void ConditionManager::evaluateConditions() {
    unsigned long currentMillis = millis();
    for (auto& cond : conditions) {
        if (cond.waitActive) {
            if (currentMillis - cond.lastEvaluationTime >= cond.waitTime) {
                cond.waitActive = false;
                cond.lastEvaluationTime = currentMillis;
                if (evaluateCondition(cond)) {
                    executeAction(cond);
                } else if (cond.elseActionFunc) {
                    cond.elseActionFunc();
                }
            }
        } else {
            if (evaluateCondition(cond)) {
                executeAction(cond);
            } else if (cond.elseActionFunc) {
                cond.elseActionFunc();
            }
            cond.lastEvaluationTime = currentMillis;
        }
    }
}

// Evaluate a single condition
bool ConditionManager::evaluateCondition(const Condition& cond) {
    if (cond.checkPrevious) {
        return cond.checkPreviousStateFunc();
    } else {
        return cond.conditionFunc();
    }
}

// Execute the action of a condition
void ConditionManager::executeAction(const Condition& cond) {
    cond.actionFunc();
}

// Set the state value for a named state variable
void ConditionManager::setState(const String& stateName, bool stateValue) {
    states[stateName] = stateValue;
}

// Get the state value for a named state variable
bool ConditionManager::getState(const String& stateName) const {
    auto it = states.find(stateName);
    return it != states.end() ? it->second : false;
}

// Check if the state has changed
bool ConditionManager::stateChanged(bool currentState, bool& previousState) {
    bool changed = currentState != previousState;
    previousState = currentState;
    return changed;
}
