#ifndef SCRIPTPARSER_H
#define SCRIPTPARSER_H

#include <Arduino.h>
#include "Statement.h"
#include <vector>

class ScriptParser {
public:
    Statement parse(const String& script);

private:
    std::vector<String> tokenize(const String& script);
    time_t parseTime(const String& timeStr);
    std::vector<int> parseDays(const String& daysStr);
    void validate(const Statement& stmt);
};

#endif  // SCRIPTPARSER_H
