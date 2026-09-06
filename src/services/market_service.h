#pragma once


#include <Arduino.h>

struct MarketInfo {
    String sjc_buy_str = "-----";
    String sjc_sell_str = "-----";
    String ring_buy_str = "-----";
    String ring_sell_str = "-----";
    String world_buy_str = "-----";
    String world_sell_str = "-----";

    int ron95_price = 0;
    int ron95_delta = 0;
    int ron92_price = 0;
    int e5_price = 0;
    int ron92_delta = 0;
    int diesel_price = 0;
    int diesel_delta = 0;
    int mazut_price = 0;
    int mazut_delta = 0;

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
    static bool isFetching() { return is_fetching; }
};

