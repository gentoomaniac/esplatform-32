#include <Arduino.h>

#include "platform/platform.h"

#ifndef LED_PIN
#define LED_PIN A12
#endif

#ifndef BUTTON_PIN
#define BUTTON_PIN A11
#endif

uint32_t* prevMillis;

void setup() {
    // esPlatform32.registerConfig<std::string>("foo", "bar");
    prevMillis = esPlatform32.registerConfig<uint32_t>("prevMillis", millis());

    esPlatform32.begin(LED_PIN, BUTTON_PIN);
}

const long interval = 1000;
void loop() {
    uint32_t currentMillis = millis();
    if (currentMillis - *prevMillis >= interval) {
        *prevMillis = currentMillis;

        auto val = esPlatform32.getConfigValue<std::string>("foo");
        if (val.has_value()) {
            Serial.println(val->c_str());
        } else {
            Serial.println("no value for foo");
        }
    }
}
