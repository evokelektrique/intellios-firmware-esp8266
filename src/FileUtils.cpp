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
    // Open the file for writing
    File file = LittleFS.open(path, "w");
    if (!file) {
        Serial.println("Failed to open file for writing");
        return false;
    }

    // Write data to the file
    if (file.print(data)) {
        file.close();
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
