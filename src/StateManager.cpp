#include <StateManager.h>

void StateManager::setState(const String& deviceId, const String& property,
                            bool value, Pin& pin) {
    states[deviceId][property] = value;
    digitalWrite(pin->gpio, value ? HIGH : LOW);
}

bool StateManager::getState(const String& deviceId,
                            const String& property) const {
    auto deviceIt = states.find(deviceId);
    if (deviceIt != states.end()) {
        auto propertyIt = deviceIt->second.find(property);
        if (propertyIt != deviceIt->second.end()) {
            return propertyIt->second;
        }
    }
    return false;
}

// Function to set the state of a device deviceComponent
void StateManager::setDeviceComponentState(const String& deviceComponentId,
                                           bool newState) {
    // Find the deviceComponent by id
    auto deviceComponentIt = std::find_if(
        configManager.deviceComponents.begin(),
        configManager.deviceComponents.end(),
        [&](const DeviceComponent& c) { return c.id == deviceComponentId; });

    if (deviceComponentIt == configManager.deviceComponents.end()) {
        Serial.println("DeviceComponent not found: " + deviceComponentId);
        return;
    }

    DeviceComponent& deviceComponent = *deviceComponentIt;

    // Find the device associated with the deviceComponent
    auto deviceIt = std::find_if(
        configManager.devices.begin(), configManager.devices.end(),
        [&](const Device& d) { return d.id == deviceComponent.device_id; });

    if (deviceIt == configManager.devices.end()) {
        Serial.println("Device not found: " + deviceComponent.device_id);
        return;
    }

    Device& device = *deviceIt;

    // Find the pin associated with the device
    auto pinIt = std::find_if(configManager.pins.begin(),
                              configManager.pins.end(), [&](const Pin& p) {
                                  return std::find(device.pins.begin(),
                                                   device.pins.end(),
                                                   p.id) != device.pins.end() &&
                                         p.mode == "OUTPUT";
                              });

    if (pinIt == configManager.pins.end()) {
        Serial.println("Output pin not found for device: " +
                       deviceComponent.device_id);
        return;
    }

    // Set the state in the state manager
    setState(deviceComponent.device_id, deviceComponent.property, newState);

    // Update the pin state
    digitalWrite(pinIt->gpio, newState ? HIGH : LOW);

    Serial.println("DeviceComponent " + deviceComponentId + " set to " +
                   String(newState));
}

// Function to get the state of a device deviceComponent
bool StateManager::getDeviceComponentState(const String& deviceComponentId) {
    // Find the deviceComponent by id
    auto deviceComponentIt = std::find_if(
        configManager.deviceComponents.begin(),
        configManager.deviceComponents.end(),
        [&](const DeviceComponent& c) { return c.id == deviceComponentId; });

    if (deviceComponentIt == configManager.deviceComponents.end()) {
        Serial.println("DeviceComponent not found: " + deviceComponentId);
        return false;
    }

    DeviceComponent& deviceComponent = *deviceComponentIt;

    // Get the state from the state manager
    bool state = getState(deviceComponent.device_id, deviceComponent.property);
    Serial.println("DeviceComponent " + deviceComponentId + " is " +
                   String(state));
    return state;
}

// Function to toggle the state of a device deviceComponent
void StateManager::toggleDeviceComponentState(const String& deviceComponentId) {
    bool currentState = getDeviceComponentState(deviceComponentId);
    setDeviceComponentState(deviceComponentId, !currentState);
}
