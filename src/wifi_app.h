#pragma once
#include <Arduino.h>

#define AP_SSID_NAME "MVT-VU-METER-SETUP"

struct WeatherData {
    float temp_c;
    int   humidity;
    char  condition[32];
    bool  valid;
};

/**
 * @brief Initialize WiFi (connect to saved STA, or launch AP Captive Portal).
 * Non-blocking / fault-tolerant: if WiFi fails or times out, continues immediately.
 */
void wifi_app_init();

/**
 * @brief Run WiFi, NTP, and OTA maintenance tasks in main loop.
 * @param is_bt_streaming If true, suspends heavy network operations for smooth audio.
 */
void wifi_app_loop(bool is_bt_streaming);

/**
 * @brief Erase stored WiFi credentials and restart into Captive Portal AP mode.
 */
void wifi_app_reset_settings();

/**
 * @brief Check if WiFi is connected in STA mode.
 */
bool wifi_app_is_connected();

/**
 * @brief Get formatted current time string "HH:MM:SS" (or millis-based fallback).
 */
void wifi_app_get_time_str(char *out, size_t max_len);

/**
 * @brief Get latest cached weather data.
 */
WeatherData wifi_app_get_weather();
