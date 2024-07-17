#ifndef CONDITIONMANAGER_H
#define CONDITIONMANAGER_H

#include <functional>
#include <map>
#include <vector>
#include <Arduino.h>

struct Condition {
    std::function<bool()> conditionFunc;
    std::function<void()> actionFunc;
    std::function<void()> elseActionFunc;
    unsigned long lastEvaluationTime;
    unsigned long waitTime;
    bool waitActive;
    bool checkPrevious;
    std::function<bool()> checkPreviousStateFunc;
    bool previousState;
};

class ConditionManager {
public:
    void addCondition(const String& conditionStr);
    void evaluateConditions();
    void setState(const String& stateName, bool stateValue);
    bool getState(const String& stateName) const;

private:
    std::vector<Condition> conditions;
    std::map<String, bool> states;

    Condition parseCondition(const String& conditionStr);
    std::vector<String> tokenize(const String& condition);
    bool evaluateCondition(const Condition& cond);
    void executeAction(const Condition& cond);
    bool stateChanged(bool currentState, bool& previousState);

    static std::map<String, std::function<bool(int, int)>> getComparisonMap(); // Static member function for comparison map

    // Helper functions
    void toggleDigital(int pin);
    void pulseDigital(int pin, int duration);
    void writeDigital(int pin, bool state);
    void writeAnalog(int pin, int value);
};

#endif // CONDITIONMANAGER_H
