#include "weather_service.h"
#include <WiFi.h>
#include <HTTPClient.h>

WeatherInfo WeatherService::current_weather;
SemaphoreHandle_t WeatherService::weatherMutex = NULL;
float WeatherService::latitude = 21.0285f;
float WeatherService::longitude = 105.8542f;
String WeatherService::city = "Hà Nội";
uint32_t WeatherService::last_fetch_time = 0;
bool WeatherService::is_fetching = false;

// 15 phút cập nhật một lần (15 * 60 * 1000 ms)
#define WEATHER_UPDATE_INTERVAL (15 * 60 * 1000)

void WeatherService::init(const char *cityName, float lat, float lon) {
    if (weatherMutex == NULL) {
        weatherMutex = xSemaphoreCreateMutex();
    }
    city = String(cityName);
    latitude = lat;
    longitude = lon;
    current_weather.city_name = city;
    last_fetch_time = 0;
    is_fetching = false;
}

void WeatherService::setLocation(const char *cityName, float lat, float lon) {
    city = String(cityName);
    latitude = lat;
    longitude = lon;
    current_weather.city_name = city;
    last_fetch_time = 0; // Kích hoạt fetch lại ngay
}

const char *WeatherService::getWeatherIcon(int code) {
    if (code == 0) return "Trời nắng";
    if (code >= 1 && code <= 3) return "Có mây";
    if (code == 45 || code == 48) return "Sương mù";
    if (code >= 51 && code <= 55) return "Mưa phùn";
    if (code >= 61 && code <= 65) return "Trời mưa";
    if (code >= 71 && code <= 77) return "Có tuyết";
    if (code >= 80 && code <= 82) return "Mưa rào";
    if (code >= 95 && code <= 99) return "Giông bão";
    return "Trời quang";
}

void WeatherService::fetchWeatherTask(void *param) {
    HTTPClient http;
    http.setTimeout(5000);

    char url[180];
    snprintf(url, sizeof(url), "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m,weather_code",
             latitude, longitude);

    Serial.printf("[WeatherService] Fetching: %s\n", url);
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        int curIdx = payload.indexOf("\"current\":");
        if (curIdx != -1) {
            float temp = 0.0f;
            int hum = 0;
            int wCode = 0;
            bool parsed = false;

            int tempIdx = payload.indexOf("\"temperature_2m\":", curIdx);
            if (tempIdx != -1) temp = payload.substring(tempIdx + 17).toFloat();

            int humIdx = payload.indexOf("\"relative_humidity_2m\":", curIdx);
            if (humIdx != -1) hum = payload.substring(humIdx + 23).toInt();

            int codeIdx = payload.indexOf("\"weather_code\":", curIdx);
            if (codeIdx != -1) wCode = payload.substring(codeIdx + 15).toInt();

            parsed = (tempIdx != -1 || humIdx != -1 || codeIdx != -1);

            // Lock mutex khi ghi toàn bộ struct một lần (tránh Core 1 đọc giữa chừng)
            if (weatherMutex != NULL && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                current_weather.temperature = temp;
                current_weather.humidity = hum;
                current_weather.weather_code = wCode;
                current_weather.condition_text = getWeatherIcon(wCode);
                current_weather.is_valid = parsed;
                current_weather.last_update_time = millis();
                xSemaphoreGive(weatherMutex);
            }

            Serial.printf("[WeatherService] -> Temp: %.1f C, Hum: %d%%, Code: %d (%s)\n",
                          temp, hum, wCode, getWeatherIcon(wCode));
        }
    } else {
        Serial.printf("[WeatherService] HTTP Error: %d\n", httpCode);
    }

    http.end();
    is_fetching = false;
    vTaskDelete(NULL);
}

void WeatherService::update(bool wifiConnected) {
    if (!wifiConnected) return;

    uint32_t now = millis();
    if (!is_fetching && (last_fetch_time == 0 || (now - last_fetch_time >= WEATHER_UPDATE_INTERVAL))) {
        last_fetch_time = now;
        is_fetching = true;
        
        // Tạo task nhẹ để fetch không làm gián đoạn pipeline render
        xTaskCreatePinnedToCore(
            fetchWeatherTask,
            "WeatherFetchTask",
            8 * 1024, // Fix #1: 4KB quá nhỏ cho HTTPClient + String, tăng lên 8KB
            NULL,
            1,
            NULL,
            0
        );
    }
}

// Thread-safe: copy toàn bộ struct dưới mutex trước khi trả về
WeatherInfo WeatherService::getWeather() {
    WeatherInfo copy;
    if (weatherMutex != NULL && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        copy = current_weather;
        xSemaphoreGive(weatherMutex);
    }
    return copy;
}
