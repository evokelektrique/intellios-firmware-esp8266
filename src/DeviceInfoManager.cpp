#include "DeviceInfoManager.h"

DeviceInfoManager::DeviceInfoManager() {}

void DeviceInfoManager::setupDeviceInfo() {
    deviceInfo.version = FIRMWARE_VERSION;
    deviceInfo.chipId = String(ESP.getChipId());
    deviceInfo.chipModel = "ESP8266";
    deviceInfo.chipRevision = String(ESP.getCoreVersion());
}
