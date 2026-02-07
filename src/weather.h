#ifndef WEATHER_H
#define WEATHER_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// Global variables
extern float temperature;
extern int humidity;
extern bool hasWeatherData;
extern SemaphoreHandle_t weatherMutex;
extern TaskHandle_t weatherTaskHandle;

// Initialize weather system with API URL
void initWeather(const String& apiUrl);

// Get current weather data (thread-safe)
bool getWeatherData(float& temp, int& hum);

// Background task for weather updates
void weatherUpdateTask(void* parameter);

// Cleanup weather resources
void cleanupWeather();

#endif
