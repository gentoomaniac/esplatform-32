#pragma once

#include "system.h"

#define MAX_FIELD_LENGTH 64
#define MAX_DEVICE_NAME_LENGTH 32
#define MAX_DEVICE_VERSION_LEN MAX_FIELD_LENGTH
#define MAX_WIFI_SSID_LEN 32
#define MAX_WIFI_PASSWORD_LEN 32
#define MAX_IPV4_LEN 16

struct Device {
    char name[MAX_DEVICE_NAME_LENGTH];
    char mac[MAC_STR_LEN];
    char fwVersion[MAX_DEVICE_VERSION_LEN];
};

struct Debug {
    bool serialEnabled;
    int baud;
};

struct Sys {
    const char* id;
    bool led;
    Device device;
    Debug debug;
};

struct Accespoint {
    char ssid[MAX_WIFI_SSID_LEN];
    char password[MAX_WIFI_PASSWORD_LEN];
    bool enabled;
};

struct Station {
    char ssid[MAX_WIFI_SSID_LEN];
    char password[MAX_WIFI_PASSWORD_LEN];
    bool enabled;
    bool dhcp;
    char ip[MAX_IPV4_LEN];
    char netmask[MAX_IPV4_LEN];
    char gateway[MAX_IPV4_LEN];
    char nameserver[MAX_IPV4_LEN];
};

struct Wifi {
    Accespoint ap;
    Station sta;
};

struct Config {
    Sys sys;
    Wifi wifi;
};

Config* GetDefaultConfig();
