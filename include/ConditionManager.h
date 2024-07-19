#ifndef CONDITIONMANAGER_H
#define CONDITIONMANAGER_H

#include <Arduino.h>

#include <functional>
#include <map>
#include <vector>

struct Condition {
    std::function<bool()> conditionFunc;
    std::function<void()> actionFunc;
    unsigned long lastEvaluationTime;
    unsigned long waitTime;
    bool waitActive;
    bool checkPrevious;
    std::function<bool()> checkPreviousStateFunc;
    bool previousState;
    bool hasSchedule;
    time_t startTime;
    time_t endTime;
    std::vector<int> daysOfWeek;  // 0 = Sunday, 6 = Saturday
};

class ConditionManager {
   public:
    ConditionManager();  // Constructor declaration
    void addCondition(const String& conditionStr);
    void evaluateConditions();
    void setState(const String& stateName, bool stateValue);
    bool getState(const String& stateName) const;
    void clearConditions();

   private:
    std::vector<Condition> conditions;
    std::map<String, bool> states;

    Condition parseCondition(const String& conditionStr);
    std::vector<String> tokenize(const String& condition);
    bool evaluateCondition(Condition& cond);
    void executeAction(const Condition& cond);
    bool stateChanged(bool currentState, bool& previousState);
    bool isWithinSchedule(const Condition& cond, const time_t& now);
    bool isDayOfWeekMatch(const Condition& cond, const time_t& now);

    static std::map<String, std::function<bool(int, int)>> getComparisonMap();

    // Helper functions
    void toggleDigital(int pin);
    void pulseDigital(int pin, int duration);
    void writeDigital(int pin, bool state);
    void writeAnalog(int pin, int value);
    int getPinNumber(const String& pinName);

    unsigned long lastEvalTime;
    const unsigned long cooldown = 10;  // Cooldown period in milliseconds
};

#endif  // CONDITIONMANAGER_H
