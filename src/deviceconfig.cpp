#include <string.h>

#include "deviceconfig.h"
#include "system.h"

DeviceConfig* GetDefaultConfig() {
    static DeviceConfig config; 

    static bool initialized = false;

    if (!initialized) {
        config.Sys.Id = getUniqueDeviceId();
        strncpy(config.Sys.Version, "1.0.0", MAX_VERSION_LEN);
        strncpy(config.Sys.DeviceName, "ESP32-test", MAX_FIELD_LENGTH);
        
        strncpy(config.Wifi.SSID, "Metalmania-iot", MAX_FIELD_LENGTH);
        strncpy(config.Wifi.Password, "Eu3cXH4aQX", MAX_FIELD_LENGTH);
        
        initialized = true;
    }
    
    return &config;
}