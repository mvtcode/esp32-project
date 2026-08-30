#ifndef WIFI_SERVICE_H
#define WIFI_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <vector>

enum WifiState {
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_SCANNING,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_CONNECT_FAILED
};

struct WifiScanItem {
    String ssid;
    int32_t rssi;
    bool isEncrypted;
    uint8_t channel;
};

class WifiService {
public:
    static void init();
    static void update();

    // Async Scan
    static void startScan();
    static bool isScanning();
    static std::vector<WifiScanItem> getScanResults();

    // Async Connect
    static void connect(const String& ssid, const String& password);
    static void disconnect();
    static bool isConnected();
    static WifiState getState();
    static const char* getStateString();

    // Telemetry getters
    static String getConnectedSSID();
    static String getIPAddress();
    static String getMacAddress();
    static int32_t getRSSI();
    static int getSignalPercent(); // 0 - 100%

private:
    static WifiState currentState;
    static std::vector<WifiScanItem> scanList;
    static SemaphoreHandle_t wifiMutex;
    static unsigned long lastConnectAttempt;
    static unsigned long lastCheckTime;
    static bool isConnecting;
    static bool isScanRunning;

    static void scanTask(void* param);
    static void connectTask(void* param);
};

#endif // WIFI_SERVICE_H
