#include <TaskDefinitions.h>
#include <WiFiManagement.h>

bool WiFiManager::saveAPCredentials(const String& ssid, const String& pass) {
    const char* path = "/ap_credentials.json";

    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["pass"] = pass;
    JsonObject jsonObject = doc.as<JsonObject>();

    bool result = writeFileJson(path, jsonObject);

    if (result) {
        Serial.println("AP credentials saved successfully");
    } else {
        Serial.println("Failed to save AP credentials");
    }

    return result;
}

// Always returns true, becaue this is a default setup.
bool WiFiManager::setupDefaultAP(String& ssid, String& pass) {
    String chipId = this->deviceInfoManager.deviceInfo.chipId;
    ssid = "intellios-" + chipId;
    pass = "";

    return true;
}

bool WiFiManager::loadAPCredentials(String& ssid, String& pass) {
    String fileContent = readFile("/ap_credentials.json");

    if (fileContent.length() == 0) {
        Serial.println("Config file is empty or not found.");
        return this->setupDefaultAP(ssid, pass);
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileContent);

    if (error) {
        Serial.print("Failed to deserialize JSON: ");
        Serial.println(error.c_str());
        return this->setupDefaultAP(ssid, pass);
    }

    String chipId = this->deviceInfoManager.deviceInfo.chipId;
    ssid = "intellios-" + chipId;
    pass = "";

    if (doc.containsKey("pass")) {
        pass = doc["pass"].as<String>();
    }

    return true;
}

void WiFiManager::handleUpdateAccessPointCredentials(ESP8266WebServer* server) {
    if (!server->hasArg("pass")) {
        server->send(400, "text/plain", "Bad Request: Missing pass argument");
        return;
    }

    String chipId = this->deviceInfoManager.deviceInfo.chipId;
    String ssid = "intellios-" + chipId;
    String pass = server->arg("pass");

    if (saveAPCredentials(ssid, pass)) {
        this->startAPMode();
        server->send(200, "text/plain", "AP credentials changed");
    } else {
        server->send(500, "text/plain",
                     "Internal Server Error: Failed to save credentials");
    }
}

bool WiFiManager::startAPMode() {
    WiFi.mode(WIFI_AP_STA);

    String ssid;
    String pass;

    if (loadAPCredentials(ssid, pass)) {
        int channel = 1;
        int broadcastStatus = 0;
        int maxConnections = 8;

        // Debugging information
        Serial.print("CHIP ID: ");
        Serial.println(deviceInfoManager.deviceInfo.chipId);
        Serial.print("Starting AP with SSID: ");
        Serial.println(ssid);
        Serial.print("Password: ");
        Serial.println(pass);

        bool result = WiFi.softAP(ssid.c_str(), pass.c_str(), channel,
                                  broadcastStatus, maxConnections);
        if (result) {
            Serial.println("AP Mode Started");
            Serial.print("IP Address: ");
            Serial.println(WiFi.softAPIP());
            return true;
        } else {
            Serial.println("Failed to start AP mode");
            return false;
        }

        return true;
    }

    Serial.println("Failed to load AP credentials.");
    return false;
}

IPAddress WiFiManager::getIPAddress() { return WiFi.softAPIP(); }

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
    if (server->hasArg("ssid") && server->hasArg("pass")) {
        String ssid = server->arg("ssid");
        String pass = server->arg("pass");

        WiFi.begin(ssid.c_str(), pass.c_str());

        int counter = 0;
        while (WiFi.status() != WL_CONNECTED && counter < 20) {
            delay(500);
            Serial.println("Connecting to WiFi... SSID: " + ssid +
                           " - PASS: " + pass);
            counter++;
        }

        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Connected to WiFi");
            saveWiFiCredentials(ssid.c_str(), pass.c_str());
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

bool WiFiManager::saveWiFiCredentials(const char* ssid, const char* pass) {
    JsonDocument doc;
    doc["ssid"] = ssid;
    doc["pass"] = pass;

    JsonObject jsonObject = doc.as<JsonObject>();

    bool result = writeFileJson("/wifi.json", jsonObject);
    if (result) {
        Serial.println("WiFi credentials saved successfully");
    } else {
        Serial.println("Failed to save WiFi credentials");
    }
    return result;
}

bool WiFiManager::loadWiFiCredentials(String& ssid, String& pass) {
    String fileContent = readFile("/wifi.json");

    if (fileContent.length() == 0) {
        Serial.println("Config file is empty or not found.");
        return false;
    }

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, fileContent);

    if (error) {
        Serial.print("Failed to deserialize JSON: ");
        Serial.println(error.c_str());
        return false;
    }

    // Extract SSID and pass
    if (doc.containsKey("ssid") && doc.containsKey("pass")) {
        ssid = doc["ssid"].as<String>();
        pass = doc["pass"].as<String>();
    } else {
        Serial.println("JSON does not contain ssid or pass fields.");
        return false;
    }

    // Check if the strings are empty
    if (ssid.length() == 0 || pass.length() == 0) {
        Serial.println("SSID or pass is empty");
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
            String ssid, pass;
            if (loadWiFiCredentials(ssid, pass)) {
                Serial.println("SSID: " + ssid + " - PASS: " + pass);
                WiFi.begin(ssid.c_str(), pass.c_str());
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

            // Call yield to allow other tasks to run
            yield();
        } else {
            Serial.println("Failed to reconnect to WiFi");
            reconnecting = false;
        }
    }
}

void WiFiManager::begin() {
    yield();  // maybe?

    String ssid, pass;
    if (loadWiFiCredentials(ssid, pass)) {
        WiFi.begin(ssid.c_str(), pass.c_str());
        taskReconnectWiFi.enableDelayed();
        Serial.println(
            "Attempting to connect to WiFi with saved credentials...");
    } else {
        taskReconnectWiFi.disable();
        Serial.println("No saved WiFi credentials found.");
    }
}