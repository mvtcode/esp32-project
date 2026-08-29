#ifndef PORTAL_SERVICE_H
#define PORTAL_SERVICE_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <LittleFS.h>

struct AppConfig {
    String wifi_ssid;
    String wifi_pass;
    String google_script_url;
    int upload_cooldown = 5; // Mặc định 5s
    int res_mode = 0;        // 0: VGA 640x480, 1: QVGA 320x240
    bool upload_enabled = false;
    bool led_enabled = true; // Mặc định BẬT chỉ báo LED
    String weather_city = "Hà Nội";
    int standby_timeout = 15; // Mặc định 15s tự vào Standby Clock
};

class PortalService {
private:
    static WebServer server;
    static DNSServer dnsServer;
    static Preferences prefs;
    static bool is_portal_active;
    static bool is_server_started;
    static bool is_scanning;

    static void setupRoutes();
    static void scanTaskWorker(void *param);
    static String getContentType(const String &filename);
    static bool handleFileRead(String path);
    static void handleRoot();
    static void handleScan();
    static void handleScanResults();
    static void handleGetConfig();
    static void handleSaveConfig();
    static void handleGetWeather();
    static void handleNotFound();

public:
    static AppConfig loadConfig();
    static void saveConfig(const AppConfig &cfg);
    static void startCaptivePortal();
    static void startWebServer();
    static void loop();
    static bool isPortalActive() { return is_portal_active; }
};

#endif // PORTAL_SERVICE_H
