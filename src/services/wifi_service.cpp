#include "wifi_service.h"
#include "config_manager.h"

WifiState WifiService::currentState = WIFI_STATE_DISCONNECTED;
std::vector<WifiScanItem> WifiService::scanList;
SemaphoreHandle_t WifiService::wifiMutex = NULL;
unsigned long WifiService::lastConnectAttempt = 0;
unsigned long WifiService::lastCheckTime = 0;
bool WifiService::isConnecting = false;
bool WifiService::isScanRunning = false;

struct ConnectParams {
    String ssid;
    String pass;
};

void WifiService::init() {
    if (wifiMutex == NULL) {
        wifiMutex = xSemaphoreCreateMutex();
    }

    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);

    // Tự động kết nối lại nếu đã có SSID lưu trong Flash
    if (ConfigManager::hasWifiCredentials()) {
        String savedSSID = ConfigManager::getWifiSSID();
        String savedPass = ConfigManager::getWifiPassword();
        Serial.printf("[WiFi] Found saved credentials for: %s, connecting...\n", savedSSID.c_str());
        connect(savedSSID, savedPass);
    }
}

#include <algorithm>

void WifiService::scanTask(void* param) {
    Serial.println("[WiFi] Starting fast async scan...");
    // Active scan, không quét mạng ẩn, 80ms mỗi kênh -> Quét siêu nhanh ~1s
    int n = WiFi.scanNetworks(false, false, false, 80);
    
    std::vector<WifiScanItem> tempResults;
    if (n > 0) {
        for (int i = 0; i < n; ++i) {
            String s = WiFi.SSID(i);
            if (s.length() > 0) {
                bool found = false;
                for (auto& item : tempResults) {
                    if (item.ssid == s) {
                        found = true;
                        if (WiFi.RSSI(i) > item.rssi) {
                            item.rssi = WiFi.RSSI(i);
                            item.channel = WiFi.channel(i);
                        }
                        break;
                    }
                }
                if (!found) {
                    WifiScanItem item;
                    item.ssid = s;
                    item.rssi = WiFi.RSSI(i);
                    item.isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                    item.channel = WiFi.channel(i);
                    tempResults.push_back(item);
                }
            }
        }
        // Sắp xếp mạng sóng khỏe nhất lên đầu
        std::sort(tempResults.begin(), tempResults.end(), [](const WifiScanItem& a, const WifiScanItem& b) {
            return a.rssi > b.rssi;
        });
    }
    WiFi.scanDelete();

    if (wifiMutex != NULL && xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        scanList = tempResults;
        isScanRunning = false;
        if (currentState == WIFI_STATE_SCANNING) {
            currentState = (WiFi.status() == WL_CONNECTED) ? WIFI_STATE_CONNECTED : WIFI_STATE_DISCONNECTED;
        }
        xSemaphoreGive(wifiMutex);
    }

    Serial.printf("[WiFi] Fast scan finished, found %d unique networks.\n", (int)tempResults.size());
    vTaskDelete(NULL);
}

void WifiService::startScan() {
    if (isScanRunning) return;
    isScanRunning = true;
    currentState = WIFI_STATE_SCANNING;

    BaseType_t res = xTaskCreatePinnedToCore(
        scanTask,
        "WifiScanTask",
        4 * 1024,
        NULL,
        1,
        NULL,
        0 // Run on Core 0
    );

    if (res != pdPASS) {
        isScanRunning = false;
        currentState = WIFI_STATE_DISCONNECTED;
    }
}

bool WifiService::isScanning() {
    return isScanRunning;
}

std::vector<WifiScanItem> WifiService::getScanResults() {
    std::vector<WifiScanItem> copy;
    if (wifiMutex != NULL && xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        copy = scanList;
        xSemaphoreGive(wifiMutex);
    }
    return copy;
}

void WifiService::connectTask(void* param) {
    ConnectParams* p = (ConnectParams*)param;
    String ssid = p->ssid;
    String pass = p->pass;
    delete p;

    Serial.printf("[WiFi] Connecting to: %s\n", ssid.c_str());

    WiFi.disconnect(true);
    vTaskDelay(pdMS_TO_TICKS(200));
    WiFi.begin(ssid.c_str(), pass.c_str());

    unsigned long startMs = millis();
    bool success = false;

    while (millis() - startMs < 12000) { // Timeout 12s
        if (WiFi.status() == WL_CONNECTED) {
            success = true;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(250));
    }

    if (wifiMutex != NULL && xSemaphoreTake(wifiMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        if (success) {
            currentState = WIFI_STATE_CONNECTED;
            Serial.printf("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            ConfigManager::setWifiCredentials(ssid, pass);
        } else {
            currentState = WIFI_STATE_CONNECT_FAILED;
            Serial.println("[WiFi] Connection failed!");
        }
        isConnecting = false;
        xSemaphoreGive(wifiMutex);
    }

    vTaskDelete(NULL);
}

void WifiService::connect(const String& ssid, const String& password) {
    if (isConnecting) return;
    isConnecting = true;
    currentState = WIFI_STATE_CONNECTING;
    lastConnectAttempt = millis();

    ConnectParams* params = new ConnectParams{ssid, password};

    BaseType_t res = xTaskCreatePinnedToCore(
        connectTask,
        "WifiConnectTask",
        4 * 1024,
        params,
        1,
        NULL,
        0 // Run on Core 0
    );

    if (res != pdPASS) {
        delete params;
        isConnecting = false;
        currentState = WIFI_STATE_CONNECT_FAILED;
    }
}

void WifiService::disconnect() {
    WiFi.disconnect(true);
    currentState = WIFI_STATE_DISCONNECTED;
}

bool WifiService::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

WifiState WifiService::getState() {
    if (WiFi.status() == WL_CONNECTED) {
        return WIFI_STATE_CONNECTED;
    }
    return currentState;
}

const char* WifiService::getStateString() {
    switch (getState()) {
        case WIFI_STATE_CONNECTED: return "Đã kết nối";
        case WIFI_STATE_CONNECTING: return "Đang kết nối...";
        case WIFI_STATE_SCANNING: return "Đang quét mạng...";
        case WIFI_STATE_CONNECT_FAILED: return "Kết nối thất bại";
        default: return "Chưa kết nối";
    }
}

String WifiService::getConnectedSSID() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.SSID();
    }
    return "";
}

String WifiService::getIPAddress() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

String WifiService::getMacAddress() {
    return WiFi.macAddress();
}

int32_t WifiService::getRSSI() {
    if (WiFi.status() == WL_CONNECTED) {
        return WiFi.RSSI();
    }
    return -100;
}

int WifiService::getSignalPercent() {
    if (WiFi.status() != WL_CONNECTED) return 0;
    int rssi = WiFi.RSSI();
    if (rssi <= -100) return 0;
    if (rssi >= -50) return 100;
    return 2 * (rssi + 100);
}

void WifiService::update() {
    unsigned long now = millis();
    if (now - lastCheckTime >= 5000) { // Mỗi 5s kiểm tra trạng thái
        lastCheckTime = now;
        if (WiFi.status() == WL_CONNECTED) {
            currentState = WIFI_STATE_CONNECTED;
        } else if (!isConnecting && !isScanRunning && ConfigManager::hasWifiCredentials()) {
            // Tự động kết nối lại nếu mất mạng
            if (now - lastConnectAttempt >= 30000) { // Thử lại sau mỗi 30s
                String ssid = ConfigManager::getWifiSSID();
                String pass = ConfigManager::getWifiPassword();
                connect(ssid, pass);
            }
        }
    }
}
