#include <Arduino.h>

#include <string>

#include "platform/platform.h"

#ifndef LED_PIN
#define LED_PIN A12
#endif

#ifndef BUTTON_PIN
#define BUTTON_PIN A11
#endif

uint32_t* prevMillis;
std::string* name;

void setup() {
    // esPlatform32.registerConfig<std::string>("foo", "bar");
    prevMillis = esPlatform32.registerConfig<uint32_t>("prevMillis", millis());
    name = esPlatform32.registerConfig<std::string>("name", "fizz");

    esPlatform32.begin(LED_PIN, BUTTON_PIN);
}

const long interval = 10000;
void loop() {
    uint32_t currentMillis = millis();
    if (currentMillis - *prevMillis >= interval) {
        *prevMillis = currentMillis;

        auto val = esPlatform32.getConfigValue<std::string>("name");
        if (val.has_value()) {
            Serial.println(val->c_str());
        } else {
            Serial.println("no value for foo");
            Serial.println();
        }
    }
}
