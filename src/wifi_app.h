#pragma once
#include <Arduino.h>

#define AP_SSID_NAME "MVT-Audio-Setup"

struct WeatherData {
    float temp_c;
    int   humidity;
    char  condition[32];
    bool  valid;
};

/**
 * @brief Check if WiFi subsystem is currently in AP setup mode.
 */
bool wifi_app_is_ap_mode();

/**
 * @brief Initialize WiFi (connect to saved STA, or launch AP Captive Portal).
 * Non-blocking / fault-tolerant: if WiFi fails or times out, continues immediately.
 */
void wifi_app_init();

/**
 * @brief Turn off WiFi completely to free RF antenna and save power/CPU.
 */
void wifi_app_stop();

/**
 * @brief Run WiFi, NTP, and OTA maintenance tasks in main loop.
 * @param is_bt_streaming If true, suspends heavy network operations for smooth audio.
 */
void wifi_app_loop(bool is_bt_streaming = false);

/**
 * @brief Erase stored WiFi credentials and restart into Captive Portal AP mode.
 */
void wifi_app_reset_settings();

/**
 * @brief Launch AP Web Config Portal directly without restarting.
 */
void wifi_app_start_ap_portal();

/**
 * @brief Check if WiFi is connected in STA mode.
 */
bool wifi_app_is_connected();

/**
 * @brief Check if WiFi is currently connecting in background.
 */
bool wifi_app_is_connecting();

/**
 * @brief Get formatted current time string "HH:MM:SS" (or millis-based fallback).
 */
void wifi_app_get_time_str(char *out, size_t max_len);

struct SolarDate {
    int year;
    int month;
    int day;
    int day_of_week; // 0=Sun, 1=Mon, ..., 6=Sat
    bool valid;
};

/**
 * @brief Get solar date components from NTP client.
 */
SolarDate wifi_app_get_solar_date();

/**
 * @brief Get latest cached weather data.
 */
WeatherData wifi_app_get_weather();
