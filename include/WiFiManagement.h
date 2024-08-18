#ifndef WIFIMANAGEMENT_H
#define WIFIMANAGEMENT_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <DeviceInfoManager.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <FileUtils.h>
#include <LittleFS.h>

class WiFiManager {
   public:
    WiFiManager(DeviceInfoManager& deviceInfoManager)
        : deviceInfoManager(deviceInfoManager) {};

    void begin();

    // WiFi
    void reconnectWiFi();
    bool saveWiFiCredentials(const char* ssid, const char* pass);
    bool loadWiFiCredentials(String& ssid, String& pass);

    // AP
    bool setupDefaultAP(String& ssid, String& pass);
    bool startAPMode();
    bool saveAPCredentials(const String& ssid, const String& pass);
    bool loadAPCredentials(String& ssid, String& pass);
    IPAddress getIPAddress();

    // WiFi WebServer handlers
    void handleRoot(ESP8266WebServer* server);
    void handleScan(ESP8266WebServer* server);
    void handleConnect(ESP8266WebServer* server);
    void handleStatus(ESP8266WebServer* server);

    // AP WebServer handlers
    void handleUpdateAccessPointCredentials(ESP8266WebServer* server);

   private:
    DeviceInfoManager deviceInfoManager;

    unsigned long lastReconnectAttempt = 0;
    const unsigned long reconnectInterval = 10000;  // 10 seconds
    int reconnectCounter = 0;
    const int maxReconnectAttempts = 20;
    bool reconnecting = false;
};

#endif  // WIFIMANAGEMENT_H
