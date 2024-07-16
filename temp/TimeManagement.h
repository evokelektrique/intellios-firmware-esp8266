#ifndef TIME_MANAGEMENT
#define TIME_MANAGEMENT

#include <Arduino.h>
#include <TZ.h>
#include <time.h>
#include "ArduinoJson.h"
#include "LittleFS.h"

struct TimeConfig {
    time_t currentTime;
    String timezone;
};

TimeConfig timeConfig;

bool saveTimeConfig(const TimeConfig &config);
bool loadTimeConfig(TimeConfig &config);
void updateInternalTime(const TimeConfig &config);
void handleSetTime();

#endif