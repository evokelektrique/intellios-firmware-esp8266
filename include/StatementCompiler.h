#ifndef STATEMENTCOMPILER_H
#define STATEMENTCOMPILER_H

#include "Statement.h"
#include <Arduino.h>
#include <map>
#include <functional>

class StatementCompiler {
public:
    void compile(Statement& stmt);

private:
    std::function<bool()> compileConditionFunc(const String& subject, const String& pin, const String& operator_, const String& value);
    std::function<void()> compileActionFunc(const String& action, const String& pin);
};

#endif  // STATEMENTCOMPILER_H
