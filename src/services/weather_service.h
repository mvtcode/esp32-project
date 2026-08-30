#ifndef WEATHER_SERVICE_H
#define WEATHER_SERVICE_H

#include <Arduino.h>

struct WeatherInfo {
    float temperature = 0.0f;
    int humidity = 0;
    int weather_code = 0;
    String condition_text = "N/A";
    float feels_like = 0.0f;
    int wind_speed = 0;
    int uv_index = 0;
    String city_name = "Hà Nội";
    bool is_valid = false;
    uint32_t last_update_time = 0;
};

class WeatherService {
private:
    static WeatherInfo current_weather;
    static SemaphoreHandle_t weatherMutex;
    static float latitude;
    static float longitude;
    static String city;
    static uint32_t last_fetch_time;
    static bool is_fetching;

    static void fetchWeatherTask(void *param);

public:
    static void init();
    static void setLocation(const char *cityName, float lat, float lon);
    static void update(bool wifiConnected, bool force = false);
    static WeatherInfo getWeather();
    static const char *getWeatherDescription(int code);
};

#endif // WEATHER_SERVICE_H
