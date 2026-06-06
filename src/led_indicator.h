#ifndef LED_INDICATOR_H
#define LED_INDICATOR_H

#include <Arduino.h>

enum LedMode {
  LED_MODE_OFF = 0,
  LED_MODE_ON = 1,
  LED_MODE_SLOW = 2, // Connecting
  LED_MODE_FAST = 3  // Config portal
};

void initLed();
void setLedMode(LedMode mode);
bool isLedActive();

#endif
