#include "weather_service.h"
#include "config_manager.h"
#include <WiFi.h>
#include <HTTPClient.h>

WeatherInfo WeatherService::current_weather;
SemaphoreHandle_t WeatherService::weatherMutex = NULL;
float WeatherService::latitude = 21.0285f;
float WeatherService::longitude = 105.8542f;
String WeatherService::city = "Hà Nội";
uint32_t WeatherService::last_fetch_time = 0;
bool WeatherService::is_fetching = false;

void WeatherService::init() {
    if (weatherMutex == NULL) {
        weatherMutex = xSemaphoreCreateMutex();
    }
    const CityLocation& curCity = ConfigManager::getCurrentCity();
    city = String(curCity.name);
    latitude = curCity.latitude;
    longitude = curCity.longitude;
    current_weather.city_name = city;
    last_fetch_time = 0;
    is_fetching = false;
    Serial.printf("[WeatherService] Initialized for City: %s (%.4f, %.4f)\n", city.c_str(), latitude, longitude);
}

void WeatherService::setLocation(const char *cityName, float lat, float lon) {
    if (weatherMutex != NULL && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        city = String(cityName);
        latitude = lat;
        longitude = lon;
        current_weather.city_name = city;
        xSemaphoreGive(weatherMutex);
    }
    last_fetch_time = 0; // Kích hoạt fetch lại ngay
    Serial.printf("[WeatherService] Location changed to: %s (%.4f, %.4f)\n", cityName, lat, lon);
}

const char* WeatherService::getWeatherDescription(int code) {
    if (code == 0) return "Trời nắng";
    if (code == 1 || code == 2) return "Ít mây";
    if (code == 3) return "Nhiều mây";
    if (code == 45 || code == 48) return "Sương mù";
    if (code >= 51 && code <= 57) return "Mưa phùn";
    if (code >= 61 && code <= 67) return "Trời mưa";
    if (code >= 71 && code <= 77) return "Có tuyết";
    if (code >= 80 && code <= 82) return "Mưa rào";
    if (code >= 95 && code <= 99) return "Giông bão";
    return "Trời quang";
}

void WeatherService::fetchWeatherTask(void *param) {
    HTTPClient http;
    http.setTimeout(8000);

    char url[256];
    snprintf(url, sizeof(url), 
             "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m,uv_index",
             latitude, longitude);

    Serial.printf("[WeatherService] Fetching Open-Meteo: %s\n", url);
    http.begin(url);
    int httpCode = http.GET();

    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();

        int curIdx = payload.indexOf("\"current\":");
        if (curIdx != -1) {
            float temp = 28.0f;
            int hum = 70;
            float feelsLike = 28.0f;
            int wCode = 0;
            int wind = 10;
            int uv = 5;
            bool parsed = false;

            int tempIdx = payload.indexOf("\"temperature_2m\":", curIdx);
            if (tempIdx != -1) {
                temp = payload.substring(tempIdx + 17).toFloat();
                parsed = true;
            }

            int humIdx = payload.indexOf("\"relative_humidity_2m\":", curIdx);
            if (humIdx != -1) {
                hum = payload.substring(humIdx + 23).toInt();
                parsed = true;
            }

            int feelIdx = payload.indexOf("\"apparent_temperature\":", curIdx);
            if (feelIdx != -1) {
                feelsLike = payload.substring(feelIdx + 23).toFloat();
            } else {
                feelsLike = temp;
            }

            int codeIdx = payload.indexOf("\"weather_code\":", curIdx);
            if (codeIdx != -1) {
                wCode = payload.substring(codeIdx + 15).toInt();
            }

            int windIdx = payload.indexOf("\"wind_speed_10m\":", curIdx);
            if (windIdx != -1) {
                wind = (int)payload.substring(windIdx + 17).toFloat();
            }

            int uvIdx = payload.indexOf("\"uv_index\":", curIdx);
            if (uvIdx != -1) {
                uv = (int)payload.substring(uvIdx + 11).toFloat();
            }

            if (parsed && weatherMutex != NULL && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                current_weather.temperature = temp;
                current_weather.humidity = hum;
                current_weather.feels_like = feelsLike;
                current_weather.weather_code = wCode;
                current_weather.wind_speed = wind;
                current_weather.uv_index = uv;
                current_weather.condition_text = getWeatherDescription(wCode);
                current_weather.is_valid = true;
                current_weather.last_update_time = millis();
                xSemaphoreGive(weatherMutex);
            }

            Serial.printf("[WeatherService] -> %s: %.1f C (Feels: %.1f), Hum: %d%%, Code: %d (%s), Wind: %d km/h, UV: %d\n",
                          city.c_str(), temp, feelsLike, hum, wCode, getWeatherDescription(wCode), wind, uv);
        }
    } else {
        Serial.printf("[WeatherService] HTTP Error: %d\n", httpCode);
    }

    http.end();
    is_fetching = false;
    vTaskDelete(NULL);
}

void WeatherService::update(bool wifiConnected, bool force) {
    if (!wifiConnected) return;

    uint32_t intervalMs = (uint32_t)ConfigManager::getSyncIntervalMinutes() * 60 * 1000;
    if (intervalMs < 15 * 60 * 1000) intervalMs = 15 * 60 * 1000;

    uint32_t now = millis();
    if (!is_fetching && (force || last_fetch_time == 0 || (now - last_fetch_time >= intervalMs))) {
        last_fetch_time = now;
        is_fetching = true;

        BaseType_t res = xTaskCreatePinnedToCore(
            fetchWeatherTask,
            "WeatherFetchTask",
            6 * 1024,
            NULL,
            1,
            NULL,
            0
        );
        if (res != pdPASS) {
            Serial.println("[WeatherService] Task creation failed");
            is_fetching = false;
        }
    }
}

WeatherInfo WeatherService::getWeather() {
    WeatherInfo copy;
    if (weatherMutex != NULL && xSemaphoreTake(weatherMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        copy = current_weather;
        xSemaphoreGive(weatherMutex);
    }
    return copy;
}
