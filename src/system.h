#pragma once

#include <stddef.h>

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#define MAC_STR_LEN 13

const char* getMac();
void onboardLed();
const char* getHardwareRevisionString();
void generateRandomSecret(char*, size_t);
