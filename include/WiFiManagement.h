#ifndef WIFIMANAGEMENT_H
#define WIFIMANAGEMENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <LittleFS.h>

#include "ESP8266mDNS.h"
#include "FileUtils.h"

class WiFiManager {
   public:
    WiFiManager();
    void startAPMode();
    void reconnectWiFi();
    bool saveWiFiCredentials(const char* ssid, const char* password);
    bool loadWiFiCredentials(String& ssid, String& password);

    void handleRoot(ESP8266WebServer* server);
    void handleScan(ESP8266WebServer* server);
    void handleConnect(ESP8266WebServer* server);
    void handleStatus(ESP8266WebServer* server);
    void begin();

   private:
    unsigned long lastReconnectAttempt = 0;
    const unsigned long reconnectInterval = 10000;  // 10 seconds
    int reconnectCounter = 0;
    const int maxReconnectAttempts = 20;
    bool reconnecting = false;
};

extern WiFiManager wifiManager;

#endif  // WIFIMANAGEMENT_H
