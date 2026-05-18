#pragma once

#include <stddef.h>

#include "Arduino.h"

#define MAC_STR_LEN 13

const char* getMac();
void onboardLed(uint8_t);
const char* getHardwareRevisionString();
void generateRandomSecret(char*, size_t);
