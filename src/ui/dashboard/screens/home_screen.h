#ifndef HOME_SCREEN_H
#define HOME_SCREEN_H

#include <lvgl.h>

class HomeScreen {
public:
    HomeScreen(lv_obj_t* parent);
    ~HomeScreen() {
        if (rootContainer) lv_obj_del(rootContainer);
    }
    
    // Setters for dynamic updates
    void updateTime(const char* timeStr, const char* secondsStr, const char* dateStr, bool isAm);
    void updateLunarCalendar(const char* lunarDayStr, const char* lunarInfoStr);
    void updateCalendarRibbon(int activeDayIndex, const int* dayNumbers); // activeDayIndex: 0-6 (Mon-Sun), dayNumbers is array of 7 ints
    void updateWeather(int temp, const char* condition, int feelsLike, int humidity, int windSpeed, int uvIndex);
    void updateGoldPrices(int buySJC, int sellSJC, int buyDelta, int sellDelta);
    void updateFuelPrices(int ron95, int ron92, int diesel, int kerosene, int ron95Delta, int ron92Delta, int dieselDelta, int keroseneDelta);

    lv_obj_t* getRoot() { return rootContainer; }

private:
    lv_obj_t* rootContainer;

    // Clock & Date widgets
    lv_obj_t* lblClockTime;
    lv_obj_t* lblClockSeconds;
    lv_obj_t* lblClockAmPm;
    lv_obj_t* lblClockDate;
    
    // Ribbon widgets (T2-CN)
    lv_obj_t* ribbonDayContainers[7];
    lv_obj_t* ribbonDayNumbers[7];
    lv_obj_t* ribbonDayNames[7];

    // Lunar calendar widgets
    lv_obj_t* lblLunarDate;
    lv_obj_t* lblLunarInfo;

    // Weather widgets
    lv_obj_t* lblWeatherTemp;
    lv_obj_t* lblWeatherCond;
    lv_obj_t* lblWeatherFeelsLike;
    lv_obj_t* lblWeatherParamTemp;
    lv_obj_t* lblWeatherParamHumid;
    lv_obj_t* lblWeatherParamWind;
    lv_obj_t* lblWeatherParamUv;
    lv_obj_t* objWeatherIconCanvas; // simulated custom graphics

    // Gold widgets
    lv_obj_t* lblGoldBuy;
    lv_obj_t* lblGoldSell;
    lv_obj_t* lblGoldBuyDelta;
    lv_obj_t* lblGoldSellDelta;

    // Fuel widgets
    lv_obj_t* lblFuelRon95Price;
    lv_obj_t* lblFuelRon95Delta;
    lv_obj_t* lblFuelRon92Price;
    lv_obj_t* lblFuelRon92Delta;
    lv_obj_t* lblFuelDieselPrice;
    lv_obj_t* lblFuelDieselDelta;
    lv_obj_t* lblFuelKerosenePrice;
    lv_obj_t* lblFuelKeroseneDelta;

    // Helper functions
    void createClockCard(lv_obj_t* parent);
    void createWeatherCard(lv_obj_t* parent);
    void createGoldCard(lv_obj_t* parent);
    void createFuelCard(lv_obj_t* parent);
};

#endif // HOME_SCREEN_H
