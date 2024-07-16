#include "TimeManagement.h"

unsigned long TimeManagement::getCurrentTimestamp() {
    time_t now;
    time(&now);
    return static_cast<unsigned long>(now);
}

String TimeManagement::formatTimestamp(unsigned long timestamp) {
    char buffer[20];
    struct tm* tm_info;
    tm_info = localtime((time_t*)&timestamp);
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    return String(buffer);
}

void TimeManagement::initializeTime(const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec) {
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    Serial.print("Waiting for NTP time sync");
    while (!time(nullptr)) {
        Serial.print(".");
        delay(1000);
    }
    Serial.println();
    Serial.println("Time synchronized successfully");
}
