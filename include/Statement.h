#ifndef STATEMENT_H
#define STATEMENT_H

#include <Arduino.h>
#include <functional>
#include <vector>

struct Statement {
    String subject;
    String pin;
    String operator_;
    String value;
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

#endif  // STATEMENT_H
