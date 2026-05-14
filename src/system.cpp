#include "system.h"

#include <Arduino.h>

const char* getMac() {
    static char macStr[MAC_STR_LEN];

    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);  //

    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    return macStr;
}

unsigned long previousLedMillis = 0;
const long ledInterval = 1000;
bool ledState = false;

void onboardLed() {
    unsigned long currentMillis = millis();

    if (currentMillis - previousLedMillis >= ledInterval) {
        previousLedMillis = currentMillis;
        ledState = !ledState;
        digitalWrite(LED_BUILTIN, ledState);
    }
}

const char* getChipModelName(esp_chip_model_t model) {
    switch (model) {
        case CHIP_ESP32:
            return "ESP32";
        case CHIP_ESP32S2:
            return "ESP32-S2";
        case CHIP_ESP32S3:
            return "ESP32-S3";
        case CHIP_ESP32C3:
            return "ESP32-C3";
        case CHIP_ESP32H2:
            return "ESP32-H2";
        default:
            return "Unknown";
    }
}

static char hardware_rev_buffer[64];
static bool hardware_rev_initialized = false;

const char* getHardwareRevisionString() {
    if (!hardware_rev_initialized) {
        esp_chip_info_t chip_info;
        esp_chip_info(&chip_info);

        snprintf(hardware_rev_buffer, sizeof(hardware_rev_buffer), "%s v%d.%d", getChipModelName(chip_info.model),
                 chip_info.revision / 100, chip_info.revision % 100);

        hardware_rev_initialized = true;
    }

    return hardware_rev_buffer;
}
