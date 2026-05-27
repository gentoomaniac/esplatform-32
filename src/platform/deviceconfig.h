#pragma once
#include <ArduinoJson.h>

#include <map>
#include <variant>

#include "system.h"

#define MAX_FIELD_LENGTH 64
#define MAX_DEVICE_NAME_LENGTH 32
#define MAX_DEVICE_VERSION_LEN MAX_FIELD_LENGTH
#define MAX_WIFI_SSID_LEN 32
#define MAX_WIFI_PASSWORD_LEN 32
#define MAX_IPV4_LEN 16
#define MAX_AUTH_USERNAME_LEN 16
#define MAX_AUTH_PASSWORD_LEN 32
#define MAX_INTERNAL_SECRET_KEY_LENGTH 32

struct Auth {
    bool enabled;
    char username[MAX_AUTH_USERNAME_LEN];
    char password[MAX_AUTH_PASSWORD_LEN];
};

struct Debug {
    bool serialEnabled;
    int baud;
};

struct Device {
    char name[MAX_DEVICE_NAME_LENGTH];
    char mac[MAC_STR_LEN];
    char fwVersion[MAX_DEVICE_VERSION_LEN];
};

struct Internal {
    char secretKey[MAX_INTERNAL_SECRET_KEY_LENGTH];
};

struct Sys {
    char id[MAX_FIELD_LENGTH];
    bool led;
    Auth auth;
    Debug debug;
    Device device;
    Internal internal;
};

struct Accespoint {
    bool enabled;
    char ssid[MAX_WIFI_SSID_LEN];
    char password[MAX_WIFI_PASSWORD_LEN];
};

struct Station {
    bool enabled;
    char ssid[MAX_WIFI_SSID_LEN];
    char password[MAX_WIFI_PASSWORD_LEN];
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

using ConfigValue = std::variant<bool, int32_t, float, double, size_t, std::string>;

struct Config {
    Sys sys;
    Wifi wifi;

    std::map<std::string, ConfigValue> customConfig;
};

void getDefaultConfig(Config*);
void serializeAuth(const Auth&, JsonObject);
void deserializeAuth(Auth&, JsonObjectConst);
void serializeDebug(const Debug&, JsonObject);
void deserializeDebug(Debug&, JsonObjectConst);
void serializeDevice(const Device&, JsonObject);
void deserializeDevice(Device&, JsonObjectConst);
void serializeSys(const Sys&, JsonObject);
void deserializeSys(Sys&, JsonObjectConst);
void serializeConfig(const Config&, JsonObject);
void deserializeConfig(Config&, JsonObjectConst);
int loadConfig(Config*);
int saveConfig(Config*);
int resetConfig();
