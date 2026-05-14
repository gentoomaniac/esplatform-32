#include <Arduino.h>

#include "system.h"

const char* getMac() {
    static char macStr[MAC_STR_LEN]; 

    uint8_t mac[6];
    esp_efuse_mac_get_default(mac); //

    snprintf(macStr, sizeof(macStr), "%02X%02X%02X%02X%02X%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

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
