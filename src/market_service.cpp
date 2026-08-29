#include "market_service.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

MarketInfo MarketService::current_market;
SemaphoreHandle_t MarketService::marketMutex = NULL;
uint32_t MarketService::last_fetch_time = 0;
bool MarketService::is_fetching = false;

// 30 phút cập nhật một lần (30 * 60 * 1000 ms)
#define MARKET_UPDATE_INTERVAL (30 * 60 * 1000)

static String formatPriceVND(int val) {
    if (val <= 0) return "---";
    int thousands = val / 1000;
    int remainder = val % 1000;
    char buf[16];
    snprintf(buf, sizeof(buf), "%d.%03d", thousands, remainder);
    return String(buf);
}

static String extractJsonField(const String &str, int startPos, const char *key) {
    int keyIdx = str.indexOf(key, startPos);
    if (keyIdx == -1) return "";
    int valStart = keyIdx + strlen(key);
    // Nếu là chuỗi bao bởi dấu ngoặc kép ""
    if (valStart < str.length() && str[valStart] == '"') {
        valStart++;
        int valEnd = str.indexOf('"', valStart);
        if (valEnd != -1) {
            return str.substring(valStart, valEnd);
        }
    } else {
        // Số nguyên
        int valEnd = valStart;
        while (valEnd < str.length() && (isdigit(str[valEnd]) || str[valEnd] == '-')) {
            valEnd++;
        }
        return str.substring(valStart, valEnd);
    }
    return "";
}

void MarketService::init() {
    if (marketMutex == NULL) {
        marketMutex = xSemaphoreCreateMutex();
    }
    last_fetch_time = 0;
    is_fetching = false;
}

void MarketService::fetchMarketTask(void *param) {
    WiFiClientSecure client;
    client.setInsecure();
    client.setTimeout(8000);

    HTTPClient http;
    http.setTimeout(8000);
    http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko)");

    const char *url = "https://gw.vnexpress.net/th?types=gia_vang_v2,gia_xang_dau";
    Serial.printf("[MarketService] Fetching VNExpress Market Data: %s\n", url);

    if (http.begin(client, url)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();

            MarketInfo info;
            bool parsed = false;

            // 1. Phân tích Giá Vàng SJC
            int sjcIdx = payload.indexOf("\"sjc\":");
            if (sjcIdx != -1) {
                info.sjc_buy = extractJsonField(payload, sjcIdx, "\"buy\":");
                info.sjc_sell = extractJsonField(payload, sjcIdx, "\"sell\":");
                parsed = true;
            }

            // 2. Phân tích Giá Vàng Nhẫn 9999
            int ringIdx = payload.indexOf("\"vangnhan9999\":");
            if (ringIdx != -1) {
                info.ring_buy = extractJsonField(payload, ringIdx, "\"buy\":");
                info.ring_sell = extractJsonField(payload, ringIdx, "\"sell\":");
                parsed = true;
            }

            // 3. Phân tích Giá Xăng RON 95
            int ron95Idx = payload.indexOf("\"ron_95\":");
            if (ron95Idx != -1) {
                int price = extractJsonField(payload, ron95Idx, "\"price\":").toInt();
                info.ron95_price = formatPriceVND(price);
                parsed = true;
            }

            // 4. Phân tích Giá Xăng E5 RON 92
            int e5Idx = payload.indexOf("\"e5_ron_92\":");
            if (e5Idx != -1) {
                int price = extractJsonField(payload, e5Idx, "\"price\":").toInt();
                info.e5_price = formatPriceVND(price);
                parsed = true;
            }

            // 5. Phân tích Giá Dầu Diesel
            int dieselIdx = payload.indexOf("\"dau_diesel\":");
            if (dieselIdx != -1) {
                int price = extractJsonField(payload, dieselIdx, "\"price\":").toInt();
                info.diesel_price = formatPriceVND(price);
                parsed = true;
            }

            if (parsed) {
                info.is_valid = true;
                info.last_update_time = millis();

                if (marketMutex != NULL && xSemaphoreTake(marketMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                    current_market = info;
                    xSemaphoreGive(marketMutex);
                }

                Serial.printf("[MarketService] -> SJC: %s - %s | Nhẫn: %s - %s | RON95: %s | E5: %s | Dầu: %s\n",
                              info.sjc_buy.c_str(), info.sjc_sell.c_str(),
                              info.ring_buy.c_str(), info.ring_sell.c_str(),
                              info.ron95_price.c_str(), info.e5_price.c_str(), info.diesel_price.c_str());
            }
        } else {
            Serial.printf("[MarketService] HTTP Error: %d\n", httpCode);
        }
        http.end();
    }

    is_fetching = false;
    vTaskDelete(NULL);
}

void MarketService::update(bool wifiConnected) {
    if (!wifiConnected) return;

    uint32_t now = millis();
    if (!is_fetching && (last_fetch_time == 0 || (now - last_fetch_time >= MARKET_UPDATE_INTERVAL))) {
        last_fetch_time = now;
        is_fetching = true;

        BaseType_t res = xTaskCreatePinnedToCore(
            fetchMarketTask,
            "MarketFetchTask",
            8 * 1024,
            NULL,
            1,
            NULL,
            0
        );
        if (res != pdPASS) {
            Serial.println("[MarketService] Task creation failed");
            is_fetching = false;
        }
    }
}

MarketInfo MarketService::getMarket() {
    MarketInfo copy;
    if (marketMutex != NULL && xSemaphoreTake(marketMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
        copy = current_market;
        xSemaphoreGive(marketMutex);
    }
    return copy;
}
