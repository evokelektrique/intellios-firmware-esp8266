#include "WiFiManagement.h"

#include "TaskDefinitions.h"

WiFiManager::WiFiManager() {}

void WiFiManager::startAPMode() {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("ESP8266_AP", "12345678");

    Serial.println("AP Mode Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
}

void WiFiManager::handleRoot(ESP8266WebServer* server) {
    String message = "ESP8266 WiFi Manager\n";
    message += "Available endpoints:\n";
    message += "/scan - Scan for WiFi networks\n";
    message += "/connect - Connect to WiFi\n";
    message += "/status - Get WiFi status\n";
    server->send(200, "text/plain", message);
}

void WiFiManager::handleScan(ESP8266WebServer* server) {
    int n = WiFi.scanNetworks();
    JsonDocument doc;
    JsonArray networks = doc.to<JsonArray>();

    for (int i = 0; i < n; ++i) {
        JsonObject network = networks.add<JsonObject>();
        network["ssid"] = WiFi.SSID(i);
        network["rssi"] = WiFi.RSSI(i);
        network["encryptionType"] = WiFi.encryptionType(i);
    }

    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

void WiFiManager::handleConnect(ESP8266WebServer* server) {
    if (server->hasArg("ssid") && server->hasArg("password")) {
        String ssid = server->arg("ssid");
        String password = server->arg("password");

        WiFi.begin(ssid.c_str(), password.c_str());

        int counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 20) {
            delay(500);
            Serial.println("Connecting to WiFi... SSID: " + ssid + " - PASS: " + password );
            counter++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected to WiFi");
            saveWiFiCredentials(ssid.c_str(), password.c_str());
            server->send(200, "text/plain", "Connected to WiFi");
        } else {
            Serial.println("Failed to connect to WiFi");
            server->send(500, "text/plain", "Failed to connect to WiFi");
        }
    } else {
        server->send(400, "text/plain", "Bad Request");
    }
}

void WiFiManager::handleStatus(ESP8266WebServer* server) {
    JsonDocument doc;
    doc["status"] = WiFi.status();
    doc["ssid"] = WiFi.SSID();
    doc["ip"] = WiFi.localIP().toString();

    String response;
    serializeJson(doc, response);
    server->send(200, "application/json", response);
}

bool WiFiManager::saveWiFiCredentials(const char* ssid, const char* password) {
    // Define a StaticJsonDocument with an appropriate size
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["password"] = password;

    JsonObject jsonObject = doc.as<JsonObject>();

    bool result = writeFileJson("/wifi.json", jsonObject);
    if (result) {
        Serial.println("WiFi credentials saved successfully");
    } else {
        Serial.println("Failed to save WiFi credentials");
    }
    return result;
}

bool WiFiManager::loadWiFiCredentials(String& ssid, String& password) {
    String fileContent = readFile("/wifi.json");

    if (fileContent.length() == 0) {
        Serial.println("Config file is empty or not found.");
        return false;
    }

    // Define a StaticJsonDocument with an appropriate size
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileContent);

    if (error) {
        Serial.print("Failed to deserialize JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    // Extract SSID and password
    if (doc.containsKey("ssid") && doc.containsKey("password")) {
        ssid = doc["ssid"].as<String>();
        password = doc["password"].as<String>();
    } else {
        Serial.println("JSON does not contain ssid or password fields.");
        return false;
    }

    // Check if the strings are empty
    if (ssid.length() == 0 || password.length() == 0) {
        Serial.println("SSID or password is empty");
        return false;
    }

    return true;
}

void WiFiManager::reconnectWiFi() {
    if (WiFi.status() != WL_CONNECTED && !reconnecting) {
        unsigned long currentMillis = millis();
        if (currentMillis - lastReconnectAttempt >= reconnectInterval) {
            lastReconnectAttempt = currentMillis;

            Serial.println("Attempting to reconnect...");
            String ssid, password;
            if (loadWiFiCredentials(ssid, password)) {
                Serial.println("SSID: " + ssid + " - PASS: " + password);
                WiFi.begin(ssid.c_str(), password.c_str());
                reconnectCounter = 0;
                reconnecting = true;
            } else {
                Serial.println("No saved WiFi credentials found.");
            }
        }
    } else if (reconnecting) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Reconnected to WiFi");
            reconnecting = false;
        } else if (reconnectCounter < maxReconnectAttempts) {
            reconnectCounter++;
            Serial.println("Reconnecting to WiFi...");
        } else {
            Serial.println("Failed to reconnect to WiFi");
            reconnecting = false;
        }
    }
}

void WiFiManager::begin() {
    String ssid, password;
    if (loadWiFiCredentials(ssid, password)) {
        WiFi.begin(ssid.c_str(), password.c_str());
        taskReconnectWiFi.enableDelayed();
        Serial.println("Attempting to connect to WiFi with saved credentials...");
    } else {
        taskReconnectWiFi.disable();
        Serial.println("No saved WiFi credentials found.");
    }
}