#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <Arduino.h>

struct WeatherInfo {
    float temperature = 0.0f;
    int humidity = 0;
    int weather_code = 0;
    String condition_text = "N/A";
    String city_name = "Hà Nội";
    bool is_valid = false;
    uint32_t last_update_time = 0;
};

class WeatherService {
private:
    static WeatherInfo current_weather;
    static SemaphoreHandle_t weatherMutex; // Bảo vệ current_weather khỏi race condition Core0/Core1
    static float latitude;
    static float longitude;
    static String city;
    static uint32_t last_fetch_time;
    static bool is_fetching;

    static void fetchWeatherTask(void *param);

public:
    static void init(const char *cityName = "Hà Nội", float lat = 21.0285f, float lon = 105.8542f);
    static void setLocation(const char *cityName, float lat, float lon);
    static void update(bool wifiConnected);
    static WeatherInfo getWeather(); // Thread-safe copy với mutex
    static const char *getWeatherIcon(int code);
};

#endif // WEATHER_SERVICE_H
