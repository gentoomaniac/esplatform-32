#pragma once

#include <stddef.h>

#include "Arduino.h"

#define MAC_STR_LEN 13

void getMac(char*, size_t);
void onboardLed(uint8_t);
const char* getHardwareRevisionString();
void generateRandomSecret(char*, size_t);
