#pragma once
/**
 * log.h - Centralized conditional logging macros
 *
 * Control via build flag in platformio.ini:
 *   -DENABLE_SERIAL_LOG   -> all logs ON  (development)
 *   (flag absent)          -> all logs OFF (release / production, saves ~4ms per log line)
 *
 * Usage:
 *   LOG_D("tag", "msg %d", value)  -> debug   (hot path / frequent calls)
 *   LOG_I("tag", "msg %s", str)    -> info    (state changes, one-time events)
 *   LOG_W("tag", "msg")            -> warning (non-fatal errors)
 *   LOG_E("tag", "msg")            -> error   (always prints in dev, disabled in prod)
 */

#ifdef ENABLE_SERIAL_LOG
  #define LOG_D(tag, fmt, ...) Serial.printf("[" tag "] " fmt "\n", ##__VA_ARGS__)
  #define LOG_I(tag, fmt, ...) Serial.printf("[" tag "] " fmt "\n", ##__VA_ARGS__)
  #define LOG_W(tag, fmt, ...) Serial.printf("[WARN:" tag "] " fmt "\n", ##__VA_ARGS__)
  #define LOG_E(tag, fmt, ...) Serial.printf("[ERR:" tag "] " fmt "\n", ##__VA_ARGS__)
#else
  #define LOG_D(tag, fmt, ...) do {} while(0)
  #define LOG_I(tag, fmt, ...) do {} while(0)
  #define LOG_W(tag, fmt, ...) do {} while(0)
  #define LOG_E(tag, fmt, ...) do {} while(0)
#endif
