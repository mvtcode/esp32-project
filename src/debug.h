#ifndef DEBUG_H
#define DEBUG_H

#include <Arduino.h>

#if defined(ENABLE_DEBUG) && (ENABLE_DEBUG == 1)
  #define DEBUG_BEGIN(speed) Serial.begin(speed)
  #define DEBUG_PRINT(...) Serial.print(__VA_ARGS__)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
  #define DEBUG_PRINTLN(...) Serial.println(__VA_ARGS__)
#else
  #define DEBUG_BEGIN(speed)
  #define DEBUG_PRINT(...)
  #define DEBUG_PRINTF(...)
  #define DEBUG_PRINTLN(...)
#endif

#endif
