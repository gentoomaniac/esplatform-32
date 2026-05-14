#pragma once

#define MAX_VERSION_LEN 16
#define MAX_FIELD_LENGTH 64

struct SysConfig {
    const char* Id;
    char Version[MAX_VERSION_LEN];
    char DeviceName[MAX_FIELD_LENGTH];
};

struct WifiConfig {
    char SSID[MAX_FIELD_LENGTH];
    char Password[MAX_FIELD_LENGTH];
};

struct DeviceConfig {
    SysConfig Sys;
    WifiConfig Wifi;
};


DeviceConfig* GetDefaultConfig();
