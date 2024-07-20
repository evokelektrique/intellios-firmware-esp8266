#include "ScriptParser.h"
#include <TimeLib.h>

std::vector<String> ScriptParser::tokenize(const String& script) {
    std::vector<String> tokens;
    int start = 0;
    int end = script.indexOf(' ');

    while (end != -1) {
        tokens.push_back(script.substring(start, end));
        start = end + 1;
        end = script.indexOf(' ', start);
    }

    tokens.push_back(script.substring(start));
    return tokens;
}

time_t ScriptParser::parseTime(const String& timeStr) {
    int hours = timeStr.substring(0, 2).toInt();
    int minutes = timeStr.substring(3, 5).toInt();
    return (hours * 3600 + minutes * 60);
}

std::vector<int> ScriptParser::parseDays(const String& daysStr) {
    std::vector<int> days;
    if (daysStr.indexOf("Sun") != -1) days.push_back(0);
    if (daysStr.indexOf("Mon") != -1) days.push_back(1);
    if (daysStr.indexOf("Tue") != -1) days.push_back(2);
    if (daysStr.indexOf("Wed") != -1) days.push_back(3);
    if (daysStr.indexOf("Thu") != -1) days.push_back(4);
    if (daysStr.indexOf("Fri") != -1) days.push_back(5);
    if (daysStr.indexOf("Sat") != -1) days.push_back(6);
    return days;
}

void ScriptParser::validate(const Statement& stmt) {
    // Add validation logic here
}

Statement ScriptParser::parse(const String& script) {
    Serial.print("Parsing script: ");
    Serial.println(script);

    Statement stmt;
    auto tokens = tokenize(script);
    int i = 0;

    // Look for the condition or action part
    while (i < tokens.size()) {
        if (tokens[i] == "sensor" && tokens[i + 1] == "readDigital") {
            stmt.subject = tokens[i + 1];
            stmt.pin = tokens[i + 3];
            stmt.operator_ = tokens[i + 5];
            stmt.value = tokens[i + 7];
            i += 8;
            Serial.print("Subject: ");
            Serial.println(stmt.subject);
            Serial.print("Pin: ");
            Serial.println(stmt.pin);
            Serial.print("Operator: ");
            Serial.println(stmt.operator_);
            Serial.print("Value: ");
            Serial.println(stmt.value);
        } else if (tokens[i] == "check_previous") {
            stmt.checkPrevious = true;
            i += 1;
            Serial.println("Found check_previous");
        } else if (tokens[i] == "THEN") {
            String action = tokens[i + 1];
            String actionPin = tokens[i + 3];
            Serial.print("Action: ");
            Serial.println(action);
            Serial.print("Action Pin: ");
            Serial.println(actionPin);
            stmt.actionFunc = [action, actionPin]() {
                int pinNumber = actionPin.toInt();
                if (action == "toggle") {
                    int currentState = digitalRead(pinNumber);
                    digitalWrite(pinNumber, !currentState);
                }
            };
            i += 4;
        } else if (tokens[i] == "WAIT") {
            stmt.waitTime = tokens[i + 1].toInt();
            i += 2;
            Serial.print("Wait Time: ");
            Serial.println(stmt.waitTime);
        } else if (tokens[i] == "SCHEDULE") {
            stmt.startTime = parseTime(tokens[i + 1]);
            stmt.endTime = parseTime(tokens[i + 3]);
            stmt.daysOfWeek = parseDays(tokens[i + 5]);
            i += 6;
            Serial.print("Start Time: ");
            Serial.println(tokens[i + 1]);
            Serial.print("End Time: ");
            Serial.println(tokens[i + 3]);
            Serial.print("Days of Week: ");
            for (int day : stmt.daysOfWeek) {
                Serial.print(day);
                Serial.print(" ");
            }
            Serial.println();
        } else {
            i++;
        }
    }

    validate(stmt);
    return stmt;
}
