#include "TimeManagement.h"

bool saveTimeConfig(const TimeConfig &config) {
    File file = LittleFS.open("/time.json", "w");
    if (!file) {
        log("Failed to open time config file for writing");
        return false;
    }

    DynamicJsonDocument doc(1024);
    doc["currentTime"] = config.currentTime;
    doc["timezone"] = config.timezone;

    if (serializeJson(doc, file) == 0) {
        log("Failed to write time config to file");
        file.close();
        return false;
    }

    file.close();
    return true;
}

bool loadTimeConfig(TimeConfig &config) {
    File file = LittleFS.open("/time.json", "r");
    if (!file) {
        log("Failed to open time config file for reading");
        return false;
    }

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        log("Failed to read time config file");
        file.close();
        return false;
    }

    config.currentTime = doc["currentTime"];
    config.timezone = doc["timezone"].as<String>();

    file.close();
    return true;
}

void updateInternalTime(const TimeConfig &config) {
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");
    setenv("TZ", config.timezone.c_str(), 1);
    tzset();
    time_t now = config.currentTime;
    struct timeval tv = {now, 0};
    settimeofday(&tv, NULL);
}

void handleSetTime(&ESP8266WebServer server) {
    log("Handling /settime request...");
    if (!server.hasArg("plain")) {
        server.send(400, "application/json", "{\"error\":\"No body\"}");
        return;
    }

    String body = server.arg("plain");
    Serial.print("Request body: ");
    log(body);

    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        server.send(400, "application/json", "{\"error\":\"Invalid JSON\"}");
        Serial.print("JSON deserialization failed: ");
        log(error.c_str());
        return;
    }

    if (!doc.containsKey("currentTime") || !doc.containsKey("timezone")) {
        server.send(400, "application/json", "{\"error\":\"Missing fields\"}");
        log("Error: JSON missing fields");
        return;
    }

    timeConfig.currentTime = doc["currentTime"];
    timeConfig.timezone = doc["timezone"].as<String>();

    if (saveTimeConfig(timeConfig)) {
        updateInternalTime(timeConfig);
        server.send(200, "application/json", "{\"status\":\"Time updated\"}");
        log("Time updated successfully.");
    } else {
        server.send(500, "application/json", "{\"error\":\"Failed to save time config\"}");
        log("Failed to save time config");
    }
}