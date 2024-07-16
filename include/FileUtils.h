#ifndef FILE_UTILS_H
#define FILE_UTILS_H

#include "Arduino.h"
#include "ArduinoJson.h"
#include "LittleFS.h"

bool writeFile(const char* path, const char* data);
bool writeFileJson(const char* path, JsonObject& jsonObj);
String readFile(const char* path);

#endif