#include "deviceconfig.h"

#include <Arduino.h>
#include <ArduinoJson.h>
#include <LittleFS.h>

#include "system.h"

const char* FW_VERSION = "v0.0.0-alpha";
const char* CONFIG_FILE_PATH = "/config.json";

void getDefaultConfig(Config* config) {
    static bool initialized = false;

    config->sys.id = getMac();
    config->sys.led = false;

    config->sys.auth.enabled = true;
    strncpy(config->sys.auth.username, "admin", MAX_AUTH_USERNAME_LEN);
    strncpy(config->sys.auth.password, config->sys.id, MAX_AUTH_PASSWORD_LEN);

    snprintf(config->sys.device.name, MAX_DEVICE_NAME_LENGTH, "ESP32-%s", config->sys.id);
    strncpy(config->sys.device.mac, getMac(), MAC_STR_LEN);
    strncpy(config->sys.device.fwVersion, FW_VERSION, MAX_DEVICE_VERSION_LEN);

    config->sys.debug.serialEnabled = true;
    config->sys.debug.baud = 115200;

    generateRandomSecret(config->sys.internal.secretKey, sizeof(config->sys.internal.secretKey));

    config->wifi.ap.enabled = true;
    snprintf(config->wifi.ap.ssid, MAX_WIFI_SSID_LEN, "ESP32-%s", config->sys.id);
    strncpy(config->wifi.sta.password, config->sys.id, MAX_WIFI_PASSWORD_LEN);

    config->wifi.sta.dhcp = true;

    initialized = true;
}

void serializeAuth(const Auth& auth, JsonObject obj) {
    obj["enabled"] = auth.enabled;
    obj["username"] = auth.username;
}

void deserializeAuth(Auth& auth, JsonObjectConst obj) {
    if (obj["enabled"].is<bool>()) {
        auth.enabled = obj["enabled"];
    }
    if (obj["username"].is<const char*>()) {
        strlcpy(auth.username, obj["username"], MAX_AUTH_USERNAME_LEN);
    }
    if (obj["password"].is<const char*>()) {
        strlcpy(auth.password, obj["password"], MAX_AUTH_PASSWORD_LEN);
    }
}

void serializeDebug(const Debug& debug, JsonObject obj) {
    obj["serialEnabled"] = debug.serialEnabled;
    obj["baud"] = debug.baud;
}

void deserializeDebug(Debug& debug, JsonObjectConst obj) {
    if (obj["enabled"].is<bool>()) {
        debug.serialEnabled = obj["serialEnabled"];
    }
    if (obj["baud"].is<int>()) {
        debug.baud = obj["baud"];
    }
}

void serializeDevice(const Device& device, JsonObject obj) {
    obj["name"] = device.name;
    obj["mac"] = device.mac;
    obj["fwVersion"] = device.fwVersion;
}

void deserializeDevice(Device& device, JsonObjectConst obj) {
    if (obj["name"].is<const char*>()) {
        strlcpy(device.name, obj["name"], MAX_DEVICE_NAME_LENGTH);
    }
}

void serializeSys(const Sys& sys, JsonObject obj) {
    obj["id"] = sys.id;
    obj["led"] = sys.led;

    JsonObject authObj = obj["auth"].to<JsonObject>();
    serializeAuth(sys.auth, authObj);

    JsonObject devObj = obj["device"].to<JsonObject>();
    serializeDevice(sys.device, devObj);
}

void deserializeSys(Sys& sys, JsonObjectConst obj) {
    if (obj["led"].is<bool>()) {
        sys.led = obj["led"];
    }
    if (obj["auth"].is<JsonObjectConst>()) {
        deserializeAuth(sys.auth, obj["auth"]);
    }
    if (obj["device"].is<JsonObjectConst>()) {
        deserializeDevice(sys.device, obj["device"]);
    }
}

void serializeAp(const Accespoint& ap, JsonObject obj) {
    obj["enabled"] = ap.enabled;
    obj["ssid"] = ap.ssid;
}

void deserializeAp(Accespoint& ap, JsonObjectConst obj) {
    if (obj["enabled"].is<bool>()) {
        ap.enabled = obj["enabled"];
    }
    if (obj["ssid"].is<const char*>()) {
        strlcpy(ap.ssid, obj["ssid"], MAX_WIFI_SSID_LEN);
    }
    if (obj["password"].is<const char*>()) {
        strlcpy(ap.password, obj["password"], MAX_WIFI_PASSWORD_LEN);
    }
}

void serializeSta(const Station& sta, JsonObject obj) {
    obj["enabled"] = sta.enabled;
    obj["ssid"] = sta.ssid;
}

void deserializeSta(Station& sta, JsonObjectConst obj) {
    if (obj["enabled"].is<bool>()) {
        sta.enabled = obj["enabled"];
    }
    if (obj["ssid"].is<const char*>()) {
        strlcpy(sta.ssid, obj["ssid"], MAX_WIFI_SSID_LEN);
    }
    if (obj["password"].is<const char*>()) {
        strlcpy(sta.password, obj["password"], MAX_WIFI_PASSWORD_LEN);
    }
    // bool dhcp;
    // char ip[MAX_IPV4_LEN];
    // char netmask[MAX_IPV4_LEN];
    // char gateway[MAX_IPV4_LEN];
    // char nameserver[MAX_IPV4_LEN];
}

void serializeWifi(const Wifi& wifi, JsonObject obj) {
    JsonObject apObj = obj["ap"].to<JsonObject>();
    serializeAp(wifi.ap, apObj);

    JsonObject staObj = obj["sta"].to<JsonObject>();
    serializeSta(wifi.sta, staObj);
}

void deserializeWifi(Wifi& wifi, JsonObjectConst obj) {
    if (obj["ap"].is<JsonObjectConst>()) {
        deserializeAp(wifi.ap, obj["ap"]);
    }
    if (obj["sta"].is<JsonObjectConst>()) {
        deserializeSta(wifi.sta, obj["sta"]);
    }
}

void serializeConfig(const Config& config, JsonObject obj) {
    JsonObject sysObj = obj["sys"].to<JsonObject>();
    serializeSys(config.sys, sysObj);
    JsonObject wifiObj = obj["wifi"].to<JsonObject>();
    serializeWifi(config.wifi, wifiObj);
}

void deserializeConfig(Config& config, JsonObjectConst obj) {
    if (obj["sys"].is<JsonObjectConst>()) {
        deserializeSys(config.sys, obj["sys"]);
    }
    if (obj["wifi"].is<JsonObjectConst>()) {
        deserializeWifi(config.wifi, obj["wifi"]);
    }
}

int loadConfig(Config* config) {
    Serial.println("Loading config defaults ...");
    getDefaultConfig(config);

    if (LittleFS.exists(CONFIG_FILE_PATH)) {
        Serial.println("config found, loading and updating life config ...");

        File file = LittleFS.open(CONFIG_FILE_PATH, "r");
        if (!file) {
            Serial.println("Failed to open file for reading");
            return 1;
        }

        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        deserializeConfig(*config, doc.as<JsonObjectConst>());
    }

    return 0;
}

int saveConfig(Config* config) {
    if (config == nullptr) return 1;

    JsonDocument* doc = new JsonDocument();
    JsonObject root = doc->to<JsonObject>();

    serializeConfig(*config, root);

    File file = LittleFS.open(CONFIG_FILE_PATH, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open file for writing");
        delete doc;
        return 1;
    }

    size_t bytesWritten = serializeJson(*doc, file);
    file.close();

    delete doc;

    if (bytesWritten > 0) {
        Serial.println("Config saved successfully");
        return 0;
    } else {
        Serial.println("Write failed: 0 bytes written");
        return 1;
    }
}

int resetConfig() {
    if (LittleFS.exists(CONFIG_FILE_PATH)) {
        LittleFS.remove(CONFIG_FILE_PATH);
    }
    return 0;
}
