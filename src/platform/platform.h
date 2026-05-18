#pragma once

#include "Arduino.h"

class ESPlatform32 {
   private:
    int ledPin;
    int buttonPin;

    static void taskWrapper(void* pvParameters);
    void run();

   public:
    void begin(uint8_t, uint8_t);
};

extern ESPlatform32 esPlatform32;
