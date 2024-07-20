#include "StatementCompiler.h"
#include <Arduino.h>
#include <functional>
#include <map>

std::function<bool()> StatementCompiler::compileConditionFunc(const String& subject, const String& pin, const String& operator_, const String& value) {
    int pinNumber = pin.toInt(); // Assume pin is passed as a number string

    static std::map<String, std::function<bool(int, int)>> comparisonMap = {
        {"=", std::equal_to<int>()},
        {"!=", std::not_equal_to<int>()},
        {">", std::greater<int>()},
        {"<", std::less<int>()},
        {">=", std::greater_equal<int>()},
        {"<=", std::less_equal<int>()}
    };

    // Capturing comparisonMap by reference since it's static
    return [pinNumber, operator_, value, &comparisonMap]() {
        int sensorValue = digitalRead(pinNumber);
        bool result = comparisonMap.at(operator_)(sensorValue, value == "HIGH" ? HIGH : LOW);
        Serial.print("Evaluating condition: pin ");
        Serial.print(pinNumber);
        Serial.print(" with value ");
        Serial.print(sensorValue);
        Serial.print(" expected ");
        Serial.print(value == "HIGH" ? HIGH : LOW);
        Serial.print(" result: ");
        Serial.println(result);
        return result;
    };
}

std::function<void()> StatementCompiler::compileActionFunc(const String& action, const String& pin) {
    int pinNumber = pin.toInt(); // Assume pin is passed as a number string

    if (action == "toggle") {
        return [pinNumber]() {
            int currentState = digitalRead(pinNumber);
            digitalWrite(pinNumber, !currentState);
            Serial.print("Toggling pin: ");
            Serial.println(pinNumber);
        };
    }
    return []() {}; // Default empty action
}

void StatementCompiler::compile(Statement& stmt) {
    stmt.conditionFunc = compileConditionFunc(stmt.subject, stmt.pin, stmt.operator_, stmt.value);
    stmt.actionFunc = compileActionFunc(stmt.actionFunc ? "toggle" : "", stmt.pin); // Handle the action function separately
}
