#ifndef TIMEMANAGEMENT_H
#define TIMEMANAGEMENT_H

#include <Arduino.h>
#include <time.h>

class TimeManagement {
   public:
    static unsigned long getCurrentTimestamp();
    static String formatTimestamp(unsigned long timestamp);
    static void initializeTime(const char* ntpServer, long gmtOffset_sec, int daylightOffset_sec);

   private:
    TimeManagement() {}
};

#endif // TIMEMANAGEMENT_H
