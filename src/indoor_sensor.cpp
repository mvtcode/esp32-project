#include "indoor_sensor.h"
#include <Adafruit_AHTX0.h>
#include <Arduino.h>
#include <Wire.h>

// I2C pins for AHT10 (explicitly configured to avoid conflicts on ESP32-S3-N16R8)
#define I2C_SDA_PIN 9
#define I2C_SCL_PIN 10

// AHT10 sensor instance
Adafruit_AHTX0 aht;

// Cache variables (thread-safe with mutex)
static float cachedTemp = 0.0;
static int cachedHum = 0;
static bool sensorReady = false;
static bool hasValidData = false;

// FreeRTOS task handle and mutex
SemaphoreHandle_t indoorMutex = NULL;
TaskHandle_t indoorTaskHandle = NULL;

// Read interval
static const unsigned long READ_INTERVAL = 20000; // Read every 20 seconds

// Initialize AHT10 indoor temperature/humidity sensor
void initIndoorSensor() {
  Serial.println("Initializing AHT10 indoor sensor...");
  
  // Explicitly end I2C and restart on the correct pins to avoid default pin conflicts
  Wire.end();
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  if (aht.begin()) {
    Serial.println("AHT10 sensor found and initialized!");
    sensorReady = true;
    
    // Do initial read
    sensors_event_t humidity, temp;
    if (aht.getEvent(&humidity, &temp)) {
      cachedTemp = temp.temperature;
      cachedHum = (int)humidity.relative_humidity;
      hasValidData = true;
      Serial.printf("Initial indoor reading: %.1f°C, %d%%\n", cachedTemp, cachedHum);
    }
  } else {
    Serial.println("Could not find AHT10 sensor! Check wiring.");
    sensorReady = false;
  }
}

// Background task for indoor sensor updates
void indoorSensorTask(void* parameter) {
  Serial.println("[Indoor Task] Started on Core " + String(xPortGetCoreID()));
  
  // Initial delay
  vTaskDelay(2000 / portTICK_PERIOD_MS);
  
  // Periodic updates
  while (true) {
    if (sensorReady) {
      sensors_event_t humidity, tempEvent;
      
      if (aht.getEvent(&humidity, &tempEvent)) {
        // Lock mutex before updating shared variables
        if (xSemaphoreTake(indoorMutex, portMAX_DELAY) == pdTRUE) {
          cachedTemp = tempEvent.temperature;
          cachedHum = (int)humidity.relative_humidity;
          hasValidData = true;
          xSemaphoreGive(indoorMutex);
          
          Serial.printf("[Indoor Task] Updated: %.1f°C, %d%%\n", cachedTemp, cachedHum);
        }
      } else {
        Serial.println("[Indoor Task] Failed to read sensor!");
      }
    }
    
    vTaskDelay(READ_INTERVAL / portTICK_PERIOD_MS);
  }
}

// Start background task for indoor sensor reading
void startIndoorSensorTask() {
  // Only start task if sensor is ready
  if (!sensorReady) {
    Serial.println("Indoor sensor not ready, skipping task creation");
    return;
  }
  
  // Create mutex for thread-safe data access
  indoorMutex = xSemaphoreCreateMutex();
  if (indoorMutex == NULL) {
    Serial.println("Failed to create indoor sensor mutex!");
    return;
  }
  
  Serial.println("Creating indoor sensor task...");
  
  // Create indoor sensor task on Core 0 (same as weather task)
  BaseType_t result = xTaskCreatePinnedToCore(
    indoorSensorTask,       // Task function
    "IndoorTask",           // Task name
    4096,                   // Stack size (bytes)
    NULL,                   // Task parameter
    1,                      // Priority (same as weather)
    &indoorTaskHandle,      // Task handle
    0                       // Core 0
  );
  
  if (result == pdPASS) {
    Serial.println("Indoor sensor task created successfully");
  } else {
    Serial.println("Failed to create indoor sensor task!");
    if (indoorMutex != NULL) {
      vSemaphoreDelete(indoorMutex);
      indoorMutex = NULL;
    }
  }
}

// Get indoor temperature and humidity (thread-safe)
bool getIndoorData(float& temp, int& hum) {
  if (!sensorReady || !hasValidData) {
    return false;
  }
  
  // Lock mutex before reading shared variables
  if (xSemaphoreTake(indoorMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
    temp = cachedTemp;
    hum = cachedHum;
    xSemaphoreGive(indoorMutex);
    return true;
  }
  
  return false;
}

// Check if indoor sensor is ready
bool isIndoorSensorReady() {
  return sensorReady && hasValidData;
}

// Cleanup indoor sensor resources
void cleanupIndoorSensor() {
  if (indoorTaskHandle != NULL) {
    vTaskDelete(indoorTaskHandle);
    indoorTaskHandle = NULL;
  }
  
  if (indoorMutex != NULL) {
    vSemaphoreDelete(indoorMutex);
    indoorMutex = NULL;
  }
}
