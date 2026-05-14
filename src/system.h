#pragma once

#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif

#define MAC_STR_LEN 13

const char* getMac();
void onboardLed();
const char* getHardwareRevisionString();
