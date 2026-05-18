#pragma once

#include <stddef.h>

#define MAC_STR_LEN 13

#ifndef LED_BUILTIN
#define LED_BUILTIN A12
#endif

const char* getMac();
void onboardLed();
const char* getHardwareRevisionString();
void generateRandomSecret(char*, size_t);
