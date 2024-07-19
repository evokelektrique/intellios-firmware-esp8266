#include "ConditionManager.h"

ConditionManager::ConditionManager() {}

// Tokenize the condition string
std::vector<String> ConditionManager::tokenize(const String& condition) {
    std::vector<String> tokens;
    int start = 0;
    int end = condition.indexOf(' ');

    while (end != -1) {
        if (end > start) {
            tokens.push_back(condition.substring(start, end));
        }
        start = end + 1;
        end = condition.indexOf(' ', start);
    }

    if (start < condition.length()) {
        tokens.push_back(condition.substring(start));
    }

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
    if (pin == -1) {
        Serial.println("Error: Invalid pin in toggleDigital");
        return;
    }
    Serial.print("Toggling pin: ");
    Serial.println(pin);
    digitalWrite(pin, !digitalRead(pin));
}

// Add condition to the list
void ConditionManager::addCondition(const String& conditionStr) {
    conditions.push_back(parseCondition(conditionStr));
}

// Clear all conditions
void ConditionManager::clearConditions() { conditions.clear(); }

// Helper function to get the pin number from the pin name
int ConditionManager::getPinNumber(const String& pinName) {
    int pinNumber = -1;
    if (pinName.startsWith("D")) {
        int pinIndex = pinName.substring(1).toInt();
        switch (pinIndex) {
            case 0:
                pinNumber = D0;
                break;
            case 1:
                pinNumber = D1;
                break;
            case 2:
                pinNumber = D2;
                break;
            case 3:
                pinNumber = D3;
                break;
            case 4:
                pinNumber = D4;
                break;
            case 5:
                pinNumber = D5;
                break;
            case 6:
                pinNumber = D6;
                break;
            case 7:
                pinNumber = D7;
                break;
            case 8:
                pinNumber = D8;
                break;
            default:
                pinNumber = -1;
                break;
        }
    }
    Serial.print("Pin Name: ");
    Serial.print(pinName);
    Serial.print(" -> Pin Number: ");
    Serial.println(pinNumber);
    return pinNumber;
}

// Initialize the condition struct
void ConditionManager::initializeCondition(Condition& cond) {
    cond.checkPrevious = false;
    cond.waitActive = false;
    cond.hasSchedule = false;
    cond.previousState = false;  // Initialize the previous state
    cond.lastEvaluationTime = 0;
    cond.lastActionTime = 0;  // Initialize lastActionTime
}

Condition ConditionManager::parseCondition(const String& conditionStr) {
    Serial.print("Parsing condition: ");
    Serial.println(conditionStr);

    std::vector<String> tokens = tokenize(conditionStr);
    Condition cond;
    initializeCondition(cond);
    int tokenIndex = 0;

    if (tokens.size() == 0) {
        Serial.println("Error: Empty condition string.");
        return cond;
    }

    if (tokens[tokenIndex] == "if") {
        if (!parseIfCondition(tokens, tokenIndex, cond)) {
            Serial.println("Error: Failed to parse 'if' condition.");
            return cond;
        }
        if (tokenIndex < tokens.size() && tokens[tokenIndex] == "then") {
            if (!parseThenCondition(tokens, tokenIndex, cond)) {
                Serial.println("Error: Failed to parse 'then' condition.");
            }
        } else {
            Serial.println("Error: Missing 'then' after 'if' condition.");
        }
    } else if (tokens[tokenIndex] == "wait") {
        if (!parseWaitCondition(tokens, tokenIndex, cond)) {
            Serial.println("Error: Failed to parse 'wait' condition.");
            return cond;
        }
        if (tokenIndex < tokens.size() && tokens[tokenIndex] == "then") {
            if (!parseThenCondition(tokens, tokenIndex, cond)) {
                Serial.println("Error: Failed to parse 'then' condition.");
            }
        } else {
            Serial.println("Error: Missing 'then' after 'wait' condition.");
        }
    } else if (tokens[tokenIndex] == "then") {
        // Handle unconditional actions
        if (!parseThenCondition(tokens, tokenIndex, cond)) {
            Serial.println("Error: Failed to parse 'then' condition.");
        }
    } else {
        Serial.print("Unknown or unsupported token: ");
        Serial.println(tokens[tokenIndex]);
    }

    return cond;
}

bool ConditionManager::parseIfCondition(const std::vector<String>& tokens,
                                        int& tokenIndex, Condition& cond) {
    static const int IF_CONDITION_TOKENS = 5;
    auto comparisonMap = getComparisonMap();

    if (tokens.size() < tokenIndex + IF_CONDITION_TOKENS) {
        Serial.println("Error: Incomplete 'if' condition syntax.");
        return false;
    }

    String conditionType = tokens[tokenIndex + 1];
    String pinName = tokens[tokenIndex + 2];
    String comparisonOperator = tokens[tokenIndex + 3];
    String comparisonValue = tokens[tokenIndex + 4];

    Serial.print("Condition Type: ");
    Serial.println(conditionType);
    Serial.print("Pin Name: ");
    Serial.println(pinName);
    Serial.print("Comparison Operator: ");
    Serial.println(comparisonOperator);
    Serial.print("Comparison Value: ");
    Serial.println(comparisonValue);

    int pinNumber = getPinNumber(pinName);

    if (conditionType == "readDigital" && pinNumber != -1) {
        cond.conditionFunc = [pinNumber, comparisonOperator, comparisonValue,
                              comparisonMap]() {
            int sensorValue = digitalRead(pinNumber);
            bool result = comparisonMap.at(comparisonOperator)(
                sensorValue, comparisonValue == "on" ? HIGH : LOW);
            return result;
        };
        cond.checkPreviousStateFunc = [this, pinNumber, &cond]() {
            bool currentState = digitalRead(pinNumber) == HIGH;
            return stateChanged(currentState, cond.previousState);
        };
        Serial.println(
            "Condition function assigned successfully in parseIfCondition.");
    } else {
        Serial.println("Error: Unsupported condition type or invalid pin.");
        return false;
    }
    tokenIndex += IF_CONDITION_TOKENS;
    return true;
}

bool ConditionManager::parseWaitCondition(const std::vector<String>& tokens, int& tokenIndex, Condition& cond) {
    static const int WAIT_CONDITION_TOKENS = 3;

    if (tokens.size() < tokenIndex + WAIT_CONDITION_TOKENS) {
        Serial.println("Error: Incomplete 'wait' condition syntax.");
        return false;
    }

    String waitDurationStr = tokens[tokenIndex + 1];
    String timeUnit = tokens[tokenIndex + 2];

    unsigned long waitDuration = waitDurationStr.toInt();
    if (timeUnit == "seconds") {
        waitDuration *= 1000;  // Convert seconds to milliseconds
    } else if (timeUnit == "minutes") {
        waitDuration *= 60000;  // Convert minutes to milliseconds
    } else {
        Serial.println("Error: Unsupported time unit.");
        return false;
    }

    cond.waitTime = waitDuration;
    cond.waitActive = true;

    // Assign a dummy condition function to ensure it's not null
    cond.conditionFunc = []() { return true; };
    Serial.println("Condition function assigned successfully in parseWaitCondition.");

    Serial.print("Wait Duration: ");
    Serial.print(waitDuration);
    Serial.println(" ms");

    tokenIndex += WAIT_CONDITION_TOKENS;
    return true;
}

bool ConditionManager::parseThenCondition(const std::vector<String>& tokens,
                                          int& tokenIndex, Condition& cond) {
    static const int THEN_CONDITION_TOKENS = 4;

    if (tokens.size() < tokenIndex + 3) {  // Check to ensure sufficient tokens
        Serial.println("Error: Incomplete 'then' condition syntax.");
        return false;
    }

    tokenIndex++;  // Skip the 'then' token

    String actionType = tokens[tokenIndex];
    String targetPinName =
        tokens[tokenIndex + 2];  // Extract the correct token for the pin name

    int targetPinNumber = getPinNumber(targetPinName);

    Serial.print("Action Type: ");
    Serial.println(actionType);
    Serial.print("Target Pin Name: ");
    Serial.println(targetPinName);
    Serial.print("Target Pin Number: ");
    Serial.println(targetPinNumber);

    if (actionType == "toggle" && targetPinNumber != -1) {
        cond.actionFunc = [this, targetPinNumber]() {
            toggleDigital(targetPinNumber);
        };
        Serial.println("Action function assigned successfully.");
    } else {
        Serial.println("Error: Unsupported action type or invalid pin.");
        return false;
    }

    tokenIndex +=
        THEN_CONDITION_TOKENS;  // Adjust index based on the tokens processed
    return true;
}

void ConditionManager::evaluateConditions() {
    unsigned long currentTime = millis();
    for (Condition& cond : conditions) {
        if (cond.waitActive && (currentTime - cond.lastEvaluationTime >= cond.waitTime)) {
            Serial.println("Evaluating condition after wait period.");
            if (evaluateCondition(cond)) {
                if (currentTime - cond.lastActionTime >= cooldown) {
                    executeAction(cond);
                    cond.lastActionTime = currentTime;
                }
            }
            cond.lastEvaluationTime = currentTime;
            cond.waitActive = false;
        } else if (!cond.waitActive) {
            Serial.println("Evaluating condition without wait period.");
            if (evaluateCondition(cond)) {
                if (currentTime - cond.lastActionTime >= cooldown) {
                    executeAction(cond);
                    cond.lastActionTime = currentTime;
                }
            }
        }
    }
}

// Evaluate a single condition
bool ConditionManager::evaluateCondition(Condition& cond) {
    Serial.println("Evaluating condition function.");

    if (!cond.conditionFunc) {
        Serial.println("Error: No condition function assigned.");
        return false;
    }

    if (cond.checkPrevious) {
        bool currentState = cond.conditionFunc();
        Serial.print("Current state: ");
        Serial.println(currentState);
        bool changed = stateChanged(currentState, cond.previousState);
        if (changed) {
            Serial.println("State changed.");
            bool result = cond.conditionFunc();
            Serial.print("Condition result after state change: ");
            Serial.println(result);
            return result;
        } else {
            Serial.println("State did not change.");
            return false;
        }
    } else {
        bool result = cond.conditionFunc();
        Serial.print("Condition result: ");
        Serial.println(result);
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
    Serial.println("Executing action.");
    if (cond.actionFunc) {
        cond.actionFunc();
    } else {
        Serial.println("Error: No action function assigned.");
    }
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
