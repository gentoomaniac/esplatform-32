#include <Arduino.h>

#include "platform/platform.h"

#ifndef LED_PIN
#define LED_PIN A12
#endif

#ifndef BUTTON_PIN
#define BUTTON_PIN A11
#endif

void setup() {
    esPlatform32.begin(LED_PIN, BUTTON_PIN);
}

unsigned long prevMillis = 0;
const long interval = 1000;
void loop() {
    unsigned long currentMillis = millis();

    if (currentMillis - prevMillis >= interval) {
        prevMillis = currentMillis;
        Serial.println("application loop");
    }
}
