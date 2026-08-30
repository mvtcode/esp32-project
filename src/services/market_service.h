#ifndef MARKET_SERVICE_H
#define MARKET_SERVICE_H

#include <Arduino.h>

struct MarketInfo {
    String sjc_buy_str = "145,7";
    String sjc_sell_str = "148,7";
    String ring_buy_str = "145,7";
    String ring_sell_str = "148,7";

    int ron95_price = 22600;
    int ron95_delta = -60;
    int ron92_price = 21760;
    int e5_price = 21760;
    int ron92_delta = -70;
    int diesel_price = 28080;
    int diesel_delta = -460;
    int mazut_price = 18140;
    int mazut_delta = 460;

    bool is_valid = false;
    uint32_t last_update_time = 0;
};

class MarketService {
private:
    static MarketInfo current_market;
    static SemaphoreHandle_t marketMutex;
    static uint32_t last_fetch_time;
    static bool is_fetching;

    static void fetchMarketTask(void *param);

public:
    static void init();
    static void update(bool wifiConnected, bool force = false);
    static MarketInfo getMarket();
};

#endif // MARKET_SERVICE_H
