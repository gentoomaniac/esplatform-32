#include "deviceconfig.h"
#include <string.h>

DeviceConfig* GetDefaultConfig() {
    static DeviceConfig config; 

    static bool initialized = false;

    if (!initialized) {
        strncpy(config.Version, "1.0.0", MAX_VERSION_LEN);
        strncpy(config.DeviceName, "ESP32-test", MAX_FIELD_LENGTH);
        
        strncpy(config.Wifi.SSID, "Metalmania-iot", MAX_FIELD_LENGTH);
        strncpy(config.Wifi.Password, "Eu3cXH4aQX", MAX_FIELD_LENGTH);
        
        initialized = true;
    }
    
    return &config;
}