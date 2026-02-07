#ifndef INDOOR_SENSOR_H
#define INDOOR_SENSOR_H

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>

// Global variables
extern SemaphoreHandle_t indoorMutex;
extern TaskHandle_t indoorTaskHandle;

// Initialize AHT10 indoor temperature/humidity sensor
void initIndoorSensor();

// Start background task for indoor sensor reading
void startIndoorSensorTask();

// Get indoor temperature and humidity (thread-safe)
bool getIndoorData(float& temp, int& hum);

// Background task for indoor sensor updates
void indoorSensorTask(void* parameter);

// Check if indoor sensor is ready
bool isIndoorSensorReady();

// Cleanup indoor sensor resources
void cleanupIndoorSensor();

#endif
