#include "FileUtils.h"

// Write JSON object to a file
bool writeFileJson(const char* path, JsonObject& jsonObj) {
    // Open the file for writing
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }

    // Serialize JSON object to string
    String jsonString;
    serializeJson(jsonObj, jsonString);

    // Write JSON string to the file
    if (file.print(jsonString)) {
        file.close();
        return true;
    } else {
        Serial.println("Failed to write JSON to file");
        file.close();
        return false;
    }
}

// Write data to a file
bool writeFile(const char* path, const char* data) {
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }

    if (file.print(data)) {
        file.flush(); // Ensure all data is written
        file.close();
        Serial.println("File written successfully");
        return true;
    } else {
        Serial.println("Failed to write to file");
        file.close();
        return false;
    }
}

// Read data from a file and return contents as a String
String readFile(const char* path) {
    String content = "";

    // Open the file for reading
    File file = LittleFS.open(path, "r");
    if (!file) {
        Serial.print("Failed to open file for reading: ");
        Serial.println(path);
        return content;
    }

    // Read data from the file more efficiently
    size_t size = file.size();
    if (size > 0) {
        content.reserve(size);
        while (file.available()) {
            content += (char)file.read();
        }
    }

    // Close the file
    file.close();

    return content;
}

// Read data from a json file and return contents as a JsonDocument
JsonDocument readJsonFile(const char* path) {
    Serial.println("Loading json file from: " + String(path));

    String file = readFile(path);

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        Serial.println("Failed to read json file");
        Serial.println(error.c_str());
        // Need error handling here
    }

    return doc;
}