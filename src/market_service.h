#ifndef MARKET_SERVICE_H
#define MARKET_SERVICE_H

#include <Arduino.h>

struct MarketInfo {
    String sjc_buy = "---";
    String sjc_sell = "---";
    String ring_buy = "---";
    String ring_sell = "---";
    String ron95_price = "---";
    String e5_price = "---";
    String diesel_price = "---";
    String updated_time = "";
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
    static void update(bool wifiConnected);
    static MarketInfo getMarket();
};

#endif // MARKET_SERVICE_H
