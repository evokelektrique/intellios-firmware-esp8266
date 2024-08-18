#ifndef INTELLIOS_DEVICE_INFO_MANAGER
#define INTELLIOS_DEVICE_INFO_MANAGER
#define FIRMWARE_VERSION 0.1

#include "Arduino.h"

struct DeviceInfo {
    float version;
    String chipId;
    String chipModel;
    String chipRevision;
};

class DeviceInfoManager {
   public:
    DeviceInfoManager();
    void setupDeviceInfo();
    DeviceInfo deviceInfo;
};

#endif