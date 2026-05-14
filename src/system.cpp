#include <Arduino.h>

#include "system.h"

const char* getUniqueDeviceId() {
    static char deviceId[32] = {0}; 
    
    if (deviceId[0] == '\0') {
        uint64_t chipid = ESP.getEfuseMac(); 
        uint32_t high = (uint32_t)(chipid >> 32); 
        uint32_t low = (uint32_t)chipid;
        
        snprintf(deviceId, sizeof(deviceId), "ESP32-%04X%08X", high, low);
    }
    
    return deviceId; 
}