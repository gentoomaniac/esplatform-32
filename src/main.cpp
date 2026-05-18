#include <Arduino.h>

#include "platform/platform.h"

void setup() {
    esPlatform32.begin();
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
