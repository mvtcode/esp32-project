#include "market_service.h"
#include "config_manager.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

MarketInfo MarketService::current_market;
SemaphoreHandle_t MarketService::marketMutex = NULL;
uint32_t MarketService::last_fetch_time = 0;
bool MarketService::is_fetching = false;

static String extractJsonStr(const String &str, int startPos, const char *key) {
    int keyIdx = str.indexOf(key, startPos);
    if (keyIdx == -1) return "";
    int valStart = keyIdx + strlen(key);
    while (valStart < (int)str.length() && (str[valStart] == ' ' || str[valStart] == ':')) {
        valStart++;
    }
    if (valStart < (int)str.length() && str[valStart] == '"') {
        valStart++; // bỏ qua dấu nháy mở "
        int valEnd = str.indexOf('"', valStart);
        if (valEnd == -1) return "";
        return str.substring(valStart, valEnd);
    }
    int valEnd = valStart;
    while (valEnd < (int)str.length() && str[valEnd] != ',' && str[valEnd] != '}') {
        valEnd++;
    }
    String res = str.substring(valStart, valEnd);
    res.trim();
    return res;
}

static int extractJsonInt(const String &str, int startPos, const char *key) {
    int keyIdx = str.indexOf(key, startPos);
    if (keyIdx == -1) return 0;
    int valStart = keyIdx + strlen(key);
    while (valStart < (int)str.length() && (str[valStart] == ' ' || str[valStart] == '"' || str[valStart] == ':')) {
        valStart++;
    }
    int valEnd = valStart;
    while (valEnd < (int)str.length() && (isdigit(str[valEnd]) || str[valEnd] == '-' || str[valEnd] == '.')) {
        valEnd++;
    }
    String numStr = str.substring(valStart, valEnd);
    numStr.replace(".", "");
    numStr.replace(",", "");
    return numStr.toInt();
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
    client.setInsecure(); // Bỏ qua kiểm tra SSL cert để tiết kiệm bộ nhớ

    HTTPClient http;
    http.setTimeout(8000);
    http.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36");
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

    const char *url = "https://gw.vnexpress.net/th?types=gia_vang_v2,gia_xang_dau";
    Serial.printf("[MarketService] Fetching VNExpress (HTTPS): %s\n", url);

    if (http.begin(client, url)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();

            MarketInfo info = current_market;
            bool parsed = false;

            // 1. Phân tích Giá Vàng SJC (lấy trực tiếp chuỗi từ VNExpress ví dụ "145,7")
            int sjcIdx = payload.indexOf("\"sjc\":");
            if (sjcIdx != -1) {
                String bStr = extractJsonStr(payload, sjcIdx, "\"buy\":");
                String sStr = extractJsonStr(payload, sjcIdx, "\"sell\":");
                if (bStr.length() > 0) {
                    info.sjc_buy_str = bStr;
                    parsed = true;
                }
                if (sStr.length() > 0) {
                    info.sjc_sell_str = sStr;
                    parsed = true;
                }
            }

            // 2. Phân tích Giá Vàng Nhẫn 9999
            int ringIdx = payload.indexOf("\"vangnhan9999\":");
            if (ringIdx != -1) {
                String rbStr = extractJsonStr(payload, ringIdx, "\"buy\":");
                String rsStr = extractJsonStr(payload, ringIdx, "\"sell\":");
                if (rbStr.length() > 0) info.ring_buy_str = rbStr;
                if (rsStr.length() > 0) info.ring_sell_str = rsStr;
            }

            // 3. Phân tích Giá Xăng RON 95 (price & diff)
            int ron95Idx = payload.indexOf("\"ron_95\":");
            if (ron95Idx != -1) {
                int price = extractJsonInt(payload, ron95Idx, "\"price\":");
                int diff = extractJsonInt(payload, ron95Idx, "\"diff\":");
                if (price > 0) {
                    info.ron95_price = price;
                    info.ron95_delta = diff;
                    parsed = true;
                }
            }

            // 4. Phân tích Giá Xăng E5 RON 92 (price & diff)
            int e5Idx = payload.indexOf("\"e5_ron_92\":");
            if (e5Idx != -1) {
                int price = extractJsonInt(payload, e5Idx, "\"price\":");
                int diff = extractJsonInt(payload, e5Idx, "\"diff\":");
                if (price > 0) {
                    info.ron92_price = price;
                    info.e5_price = price;
                    info.ron92_delta = diff;
                    parsed = true;
                }
            }

            // 5. Phân tích Giá Dầu Diesel (price & diff)
            int dieselIdx = payload.indexOf("\"dau_diesel\":");
            if (dieselIdx != -1) {
                int price = extractJsonInt(payload, dieselIdx, "\"price\":");
                int diff = extractJsonInt(payload, dieselIdx, "\"diff\":");
                if (price > 0) {
                    info.diesel_price = price;
                    info.diesel_delta = diff;
                    parsed = true;
                }
            }

            // 6. Phân tích Giá Dầu Mazut (key: dau_madut)
            int madutIdx = payload.indexOf("\"dau_madut\":");
            if (madutIdx != -1) {
                int price = extractJsonInt(payload, madutIdx, "\"price\":");
                int diff = extractJsonInt(payload, madutIdx, "\"diff\":");
                if (price > 0) {
                    info.mazut_price = price;
                    info.mazut_delta = diff;
                    parsed = true;
                }
            }

            if (parsed && marketMutex != NULL && xSemaphoreTake(marketMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
                info.is_valid = true;
                info.last_update_time = millis();
                current_market = info;
                xSemaphoreGive(marketMutex);
            }

            Serial.printf("[MarketService] -> SJC: %s - %s | RON95: %d (%d) | E5: %d (%d) | Diesel: %d (%d) | Mazut: %d (%d)\n",
                          info.sjc_buy_str.c_str(), info.sjc_sell_str.c_str(), 
                          info.ron95_price, info.ron95_delta,
                          info.ron92_price, info.ron92_delta,
                          info.diesel_price, info.diesel_delta,
                          info.mazut_price, info.mazut_delta);
        } else {
            Serial.printf("[MarketService] HTTP Error: %d\n", httpCode);
        }
        http.end();
    }

    is_fetching = false;
    vTaskDelete(NULL);
}

void MarketService::update(bool wifiConnected, bool force) {
    if (!wifiConnected) return;

    uint32_t intervalMs = (uint32_t)ConfigManager::getSyncIntervalMinutes() * 60 * 1000;
    if (intervalMs < 15 * 60 * 1000) intervalMs = 15 * 60 * 1000;

    uint32_t now = millis();
    if (!is_fetching && (force || last_fetch_time == 0 || (now - last_fetch_time >= intervalMs))) {
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
