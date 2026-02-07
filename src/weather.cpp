#include "weather.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFi.h>

// Global variables
float temperature = 0.0;
int humidity = 0;
bool hasWeatherData = false;
SemaphoreHandle_t weatherMutex = NULL;
TaskHandle_t weatherTaskHandle = NULL;

// Private variables
static String weatherApiUrl = "";
static unsigned long lastWeatherUpdate = 0;
static const unsigned long weatherUpdateInterval = 600000; // 10 minutes in milliseconds

// Fetch weather data from Open-Meteo API (thread-safe)
static void fetchWeatherData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi not connected, skipping weather update");
    return;
  }

  if (weatherApiUrl.length() == 0) {
    Serial.println("Weather API URL not configured");
    return;
  }

  Serial.println("[Weather Task] Fetching weather data...");
  HTTPClient http;
  http.begin(weatherApiUrl.c_str());
  http.setTimeout(5000); // 5 second timeout
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    Serial.println("[Weather Task] API Response received");

    // Parse JSON
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      // Lock mutex before updating shared variables
      if (xSemaphoreTake(weatherMutex, portMAX_DELAY) == pdTRUE) {
        temperature = doc["current"]["temperature_2m"];
        humidity = doc["current"]["relative_humidity_2m"];
        hasWeatherData = true;
        xSemaphoreGive(weatherMutex);
        Serial.printf("[Weather Task] Updated: %.1f°C, %d%%\n", temperature,
                      humidity);
      }
    } else {
      Serial.print("[Weather Task] JSON parse error: ");
      Serial.println(error.c_str());
    }
  } else {
    Serial.printf("[Weather Task] HTTP GET failed, error: %d\n", httpCode);
  }

  http.end();
  lastWeatherUpdate = millis();
}

// FreeRTOS task for background weather updates
void weatherUpdateTask(void *parameter) {
  Serial.println("[Weather Task] Started on Core " + String(xPortGetCoreID()));

  // Initial fetch after 5 seconds
  vTaskDelay(5000 / portTICK_PERIOD_MS);
  fetchWeatherData();

  // Periodic updates every 10 minutes
  while (true) {
    vTaskDelay(weatherUpdateInterval / portTICK_PERIOD_MS);
    fetchWeatherData();
  }
}

// Initialize weather system with API URL
void initWeather(const String& apiUrl) {
  weatherApiUrl = apiUrl;

  // Create mutex for thread-safe weather data access
  weatherMutex = xSemaphoreCreateMutex();
  if (weatherMutex == NULL) {
    Serial.println("Failed to create weather mutex!");
    return;
  }

  // Create weather update task on Core 0 (main loop runs on Core 1)
  if (WiFi.status() == WL_CONNECTED) {
    xTaskCreatePinnedToCore(weatherUpdateTask, // Task function
                            "WeatherTask",     // Task name
                            8192,              // Stack size (bytes)
                            NULL,              // Task parameter
                            1,                 // Priority (lower than default)
                            &weatherTaskHandle, // Task handle
                            0                   // Core 0
    );
    Serial.println("Weather task created successfully");
  }
}

// Get current weather data (thread-safe)
bool getWeatherData(float& temp, int& hum) {
  if (xSemaphoreTake(weatherMutex, 10 / portTICK_PERIOD_MS) == pdTRUE) {
    bool hasData = hasWeatherData;
    temp = temperature;
    hum = humidity;
    xSemaphoreGive(weatherMutex);
    return hasData;
  }
  return false;
}

// Cleanup weather resources
void cleanupWeather() {
  if (weatherTaskHandle != NULL) {
    vTaskDelete(weatherTaskHandle);
    weatherTaskHandle = NULL;
  }
  
  if (weatherMutex != NULL) {
    vSemaphoreDelete(weatherMutex);
    weatherMutex = NULL;
  }
}
