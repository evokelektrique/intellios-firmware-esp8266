#include "ConditionManager.h"

// Constructor to initialize lastEvalTime
ConditionManager::ConditionManager() : lastEvalTime(0) {}

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

// Add condition to the list
void ConditionManager::addCondition(const JsonObject& conditionJson) {
    conditions.push_back(parseCondition(conditionJson));
}

// Clear all conditions
void ConditionManager::clearConditions() { conditions.clear(); }

// Helper function to get the pin number from the pin name
int ConditionManager::getPinNumber(const String& pinName) {
    static std::map<String, int> pinMap = {
        {"D0", 16}, {"D1", 5},  {"D2", 4},  {"D3", 0}, {"D4", 2}, {"D5", 14},
        {"D6", 12}, {"D7", 13}, {"D8", 15}, {"D9", 3}, {"D10", 1}};
    auto it = pinMap.find(pinName);
    if (it != pinMap.end()) {
        return it->second;
    }
    return -1;  // Return -1 if the pin name is invalid
}

// Parse and create condition and action functions
Condition ConditionManager::parseCondition(const JsonObject& conditionJson) {
    Serial.print("Parsing condition: ");
    serializeJson(conditionJson, Serial);
    Serial.println();

    Condition cond;
    cond.checkPrevious = conditionJson["check_previous"] | false;
    cond.waitActive = false;
    cond.hasSchedule = false;
    cond.previousState = false;  // Initialize the previous state

    // Initialize the comparison map
    auto comparisonMap = getComparisonMap();

    // Parse the 'if' part
    String sensor = conditionJson["if"]["sensor"];
    String pinName = conditionJson["if"]["pin"];
    String operator_ = conditionJson["if"]["operator"];
    String value = conditionJson["if"]["value"];

    Serial.print("Sensor: ");
    Serial.println(sensor);
    Serial.print("Pin: ");
    Serial.println(pinName);
    Serial.print("Operator: ");
    Serial.println(operator_);
    Serial.print("Value: ");
    Serial.println(value);

    int pin = getPinNumber(pinName);

    if (sensor == "readDigital" && pin != -1) {
        cond.conditionFunc = [pin, operator_, value, comparisonMap]() {
            int sensorValue = digitalRead(pin);
            bool result = comparisonMap.at(operator_)(
                sensorValue, value == "on" ? HIGH : LOW);
            return result;
        };
        cond.checkPreviousStateFunc = [this, pin, &cond]() {
            bool currentState = digitalRead(pin) == HIGH;
            return stateChanged(currentState, cond.previousState);
        };
    }

    // Parse the 'then' part
    String action = conditionJson["then"]["action"];
    String actionPinName = conditionJson["then"]["pin"];
    int actionPin = getPinNumber(actionPinName);

    Serial.print("Action: ");
    Serial.println(action);
    Serial.print("Action pin: ");
    Serial.println(actionPinName);

    if (action == "toggle" && actionPin != -1) {
        cond.actionFunc = [this, actionPin]() { toggleDigital(actionPin); };
    }

    return cond;
}

// Evaluate conditions and execute actions
void ConditionManager::evaluateConditions() {
    time_t now = time(nullptr);

    for (auto& cond : conditions) {
        if (cond.waitActive) {
            if (millis() - cond.lastEvaluationTime >= cond.waitTime) {
                cond.waitActive = false;
                cond.lastEvaluationTime = millis();
                if (evaluateCondition(cond)) {
                    Serial.println("Condition met after wait, executing action.");
                    executeAction(cond);
                }
            }
        } else {
            if (evaluateCondition(cond) && isWithinSchedule(cond, now)) {
                Serial.println("Condition met, executing action.");
                executeAction(cond);
                cond.waitActive = true;
                cond.lastEvaluationTime = millis();
            }
        }
    }
}

// Evaluate a single condition
bool ConditionManager::evaluateCondition(Condition& cond) {
    if (cond.checkPrevious) {
        bool currentState = cond.conditionFunc();
        bool changed = stateChanged(currentState, cond.previousState);
        if (changed) {
            bool result = cond.conditionFunc();
            return result;
        } else {
            return false;
        }
    } else {
        bool result = cond.conditionFunc();
        return result;
    }
}

// Check if the state has changed
bool ConditionManager::stateChanged(bool currentState, bool& previousState) {
    bool changed = currentState != previousState;
    previousState = currentState;  // Update previous state always
    return changed;
}

// Execute the action of a condition
void ConditionManager::executeAction(const Condition& cond) {
    cond.actionFunc();
}

// Check if the current time is within the scheduled time
bool ConditionManager::isWithinSchedule(const Condition& cond,
                                        const time_t& now) {
    if (!cond.hasSchedule) return true;

    struct tm* currentTime = localtime(&now);
    struct tm* startTime = localtime(&cond.startTime);
    struct tm* endTime = localtime(&cond.endTime);

    if (cond.daysOfWeek.empty()) {
        return (now >= cond.startTime && now <= cond.endTime);
    } else {
        return isDayOfWeekMatch(cond, now) &&
               (currentTime->tm_hour >= startTime->tm_hour &&
                currentTime->tm_min >= startTime->tm_min) &&
               (currentTime->tm_hour <= endTime->tm_hour &&
                currentTime->tm_min <= endTime->tm_min);
    }
}

// Check if the current day of the week matches the scheduled days
bool ConditionManager::isDayOfWeekMatch(const Condition& cond,
                                        const time_t& now) {
    if (cond.daysOfWeek.empty()) return true;

    struct tm* currentTime = localtime(&now);
    int currentDayOfWeek = currentTime->tm_wday;  // 0 = Sunday, 6 = Saturday
    return std::find(cond.daysOfWeek.begin(), cond.daysOfWeek.end(),
                     currentDayOfWeek) != cond.daysOfWeek.end();
}

// Set the state value for a named state variable
void ConditionManager::setState(const String& stateName, bool stateValue) {
    states[stateName] = stateValue;
    Serial.print("State set: ");
    Serial.print(stateName);
    Serial.print(" = ");
    Serial.println(stateValue);
}
