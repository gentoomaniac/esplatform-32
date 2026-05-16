#include "deviceconfig.h"

#include <Arduino.h>

#include "system.h"

Config* GetDefaultConfig() {
    static Config config;

    static bool initialized = false;

    if (!initialized) {
        config.sys.id = getMac();
        config.sys.led = false;

        config.sys.auth.enabled = true;
        strncpy(config.sys.auth.username, "admin", MAX_AUTH_USERNAME_LEN);
        strncpy(config.sys.auth.password, config.sys.id, MAX_AUTH_PASSWORD_LEN);

        snprintf(config.sys.device.name, MAX_DEVICE_NAME_LENGTH, "ESP32-%s", config.sys.id);
        strncpy(config.sys.device.fwVersion, "qwerty13", MAX_DEVICE_VERSION_LEN);

        config.sys.debug.serialEnabled = true;
        config.sys.debug.baud = 115200;

        generateRandomSecret(config.sys.internal.secretKey, sizeof(config.sys.internal.secretKey));

        config.wifi.ap.enabled = true;
        snprintf(config.wifi.ap.ssid, MAX_WIFI_SSID_LEN, "ESP32-%s", config.sys.id);
        strncpy(config.wifi.sta.password, config.sys.id, MAX_WIFI_PASSWORD_LEN);

        config.wifi.sta.enabled = true;
        strncpy(config.wifi.sta.ssid, "Metalmania-iot", MAX_WIFI_SSID_LEN);
        strncpy(config.wifi.sta.password, "Eu3cXH4aQX", MAX_WIFI_PASSWORD_LEN);
        config.wifi.sta.dhcp = true;

        initialized = true;
    }

    return &config;
}
