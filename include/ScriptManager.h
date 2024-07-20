#ifndef SCRIPTMANAGER_H
#define SCRIPTMANAGER_H

#include <Arduino.h>
#include <functional>
#include <map>
#include <vector>
#include "ScriptParser.h"
#include "Statement.h"
#include "StatementCompiler.h"  // Ensure this include is present

class ScriptManager {
public:
    ScriptManager();
    void addScript(const String& script);
    void evaluateScripts();
    void setState(const String& stateName, bool stateValue);
    bool getState(const String& stateName) const;
    void clearScripts();

private:
    std::vector<Statement> statements;
    std::map<String, bool> states;
    ScriptParser parser;
    StatementCompiler compiler;  // Ensure this member is declared

    bool evaluateStatement(Statement& stmt);
    void executeAction(const Statement& stmt);
    bool stateChanged(bool currentState, bool& previousState);
    bool isWithinSchedule(const Statement& stmt, const time_t& now);
    bool isDayOfWeekMatch(const Statement& stmt, const time_t& now);

    static std::map<String, std::function<bool(int, int)>> getComparisonMap();
    void toggleDigital(int pin);
    void pulseDigital(int pin, int duration);
    void writeDigital(int pin, bool state);
    void writeAnalog(int pin, int value);
    int getPinNumber(const String& pinName);

    unsigned long lastEvalTime;
    const unsigned long cooldown = 10;  // Cooldown period in milliseconds
};

#endif  // SCRIPTMANAGER_H
