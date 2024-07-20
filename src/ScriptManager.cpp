#include "ScriptManager.h"

#include <Arduino.h>
#include <TimeLib.h>

ScriptManager::ScriptManager() : lastEvalTime(0) {}

void ScriptManager::addScript(const String& script) {
    Statement stmt = parser.parse(script);
    compiler.compile(stmt);
    statements.push_back(stmt);
}

void ScriptManager::clearScripts() { statements.clear(); }

void ScriptManager::evaluateScripts() {
    Serial.println("Evaluating scripts...");

    for (auto& stmt : statements) {
        if (evaluateStatement(stmt)) {
            Serial.println("Condition met, executing action.");
            executeAction(stmt);
            stmt.waitActive = true;
            stmt.lastEvaluationTime = millis();
        }
    }
}

bool ScriptManager::evaluateStatement(Statement& stmt) {
    Serial.println("Evaluating statement...");
    if (stmt.checkPrevious) {
        bool currentState = stmt.conditionFunc();
        bool changed = stateChanged(currentState, stmt.previousState);
        if (changed) {
            Serial.println("State changed, condition met.");
            return currentState;
        }
        Serial.println("State did not change.");
        return false;
    }
    return stmt.conditionFunc();
}

bool ScriptManager::stateChanged(bool currentState, bool& previousState) {
    bool changed = currentState != previousState;
    previousState = currentState;
    return changed;
}

void ScriptManager::executeAction(const Statement& stmt) {
    Serial.println("Executing action...");
    stmt.actionFunc();
    Serial.println("Action executed.");
}

bool ScriptManager::isWithinSchedule(const Statement& stmt, const time_t& now) {
    if (!stmt.hasSchedule) return true;

    struct tm* currentTime = localtime(&now);
    struct tm* startTime = localtime(&stmt.startTime);
    struct tm* endTime = localtime(&stmt.endTime);

    if (stmt.daysOfWeek.empty()) {
        return (now >= stmt.startTime && now <= stmt.endTime);
    } else {
        return isDayOfWeekMatch(stmt, now) &&
               (currentTime->tm_hour >= startTime->tm_hour &&
                currentTime->tm_min >= startTime->tm_min) &&
               (currentTime->tm_hour <= endTime->tm_hour &&
                currentTime->tm_min <= endTime->tm_min);
    }
}

bool ScriptManager::isDayOfWeekMatch(const Statement& stmt, const time_t& now) {
    if (stmt.daysOfWeek.empty()) return true;

    struct tm* currentTime = localtime(&now);
    int currentDayOfWeek = currentTime->tm_wday;
    return std::find(stmt.daysOfWeek.begin(), stmt.daysOfWeek.end(),
                     currentDayOfWeek) != stmt.daysOfWeek.end();
}

void ScriptManager::setState(const String& stateName, bool stateValue) {
    states[stateName] = stateValue;
}

bool ScriptManager::getState(const String& stateName) const {
    auto it = states.find(stateName);
    if (it != states.end()) {
        return it->second;
    }
    return false;
}

int ScriptManager::getPinNumber(const String& pinName) {
    static std::map<String, int> pinMap = {
        {"D0", 16}, {"D1", 5},  {"D2", 4},  {"D3", 0}, {"D4", 2}, {"D5", 14},
        {"D6", 12}, {"D7", 13}, {"D8", 15}, {"D9", 3}, {"D10", 1}};
    auto it = pinMap.find(pinName);
    if (it != pinMap.end()) {
        return it->second;
    }
    return -1;
}

void ScriptManager::toggleDigital(int pin) {
    int currentState = digitalRead(pin);
    digitalWrite(pin, !currentState);
}
