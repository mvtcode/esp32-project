#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "ui/dashboard/dashboard_ui.h"
#include "services/config_manager.h"
#include "services/backlight_manager.h"
#include "services/wifi_service.h"
#include "services/storage_service.h"
#include "services/system_telemetry.h"
#include "services/time_service.h"
#include "services/weather_service.h"
#include "services/market_service.h"
#include "services/audio_player_service.h"
#include "log.h"

// --- 1. Hardware Initialization ---
TFT_eSPI tft = TFT_eSPI();

// --- 2. LVGL Configuration ---
static const uint32_t screenWidth  = 480;
static const uint32_t screenHeight = 320;
static const uint32_t bufferLines  = 40; // Tăng 25→40 dòng: giảm SPI flush transactions từ ~13 xuống ~8/frame
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * bufferLines]; 


/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

/* Reading input device (Touchpad) */
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
        // Đánh thức màn hình và reset bộ đếm sleep timer
        BacklightManager::feedActivity();
    }
}

DashboardUI* ui = nullptr;

// Timers
unsigned long last1sTick = 0;
unsigned long last500msTick = 0;
unsigned long last5sTick = 0;
unsigned long last25msTick = 0;

// Push live telemetry and data to HomeScreen
void syncHomeScreenTelemetry() {
    if (!ui) return;

    // 1. Clock & Lunar Calendar Update
    TimeInfo t = TimeService::getTimeInfo();
    ui->updateTime(t.timeStr, t.secStr, t.dateStr, t.isAm);
    if (t.isSynced && t.year >= 2024) {
        ui->setCalendarToday(t.year, t.month, t.day);
    }

    char lunarDayStr[64];
    snprintf(lunarDayStr, sizeof(lunarDayStr), "ÂL: %02d/%02d (%s)", t.lunar.day, t.lunar.month, t.lunar.dayName);
    ui->updateLunarCalendar(lunarDayStr, "");

    // 2. Weather Live Update
    WeatherInfo w = WeatherService::getWeather();
    if (w.is_valid) {
        ui->updateWeather((int)roundf(w.temperature), w.condition_text.c_str(), 
                          (int)roundf(w.feels_like), w.humidity, w.wind_speed, w.uv_index, w.city_name.c_str());
    }

    // 3. Market (Gold & Fuel) Live Update
    MarketInfo m = MarketService::getMarket();
    if (m.is_valid) {
        ui->updateGoldPrices(m.sjc_buy_str.c_str(), m.sjc_sell_str.c_str());
        ui->updateFuelPrices(m.ron95_price, m.e5_price, m.diesel_price, m.mazut_price, 
                             m.ron95_delta, m.ron92_delta, m.diesel_delta, m.mazut_delta);
    }
}

// --- Setup and Main Loop ---
void setup() {
    Serial.begin(115200);
    delay(100);
    LOG_I("Main", "====================================");
    LOG_I("Main", " ESP32 CYD 3.5 Dashboard Starting ");
    LOG_I("Main", "====================================");

    // 1. Initialize NVS Storage & Configuration
    ConfigManager::init();

    // 2. Initialize Backlight PWM (LEDC on GPIO 27)
    BacklightManager::init();

    // 3. Initialize Audio Player Service FIRST (Preallocates Helix DSP buffers on pristine heap)
    AudioPlayerService::init();

    // 4. Initialize WiFi Service (auto-reconnects if saved)
    WifiService::init();

    // 5. Initialize SD Card Storage Service (chỉ mount phần cứng, KHÔNG scan nhạc cho tới khi vào tab MP3)
    StorageService::init();

    // 6. Initialize Live Online Services (Phase 2)
    TimeService::init();
    WeatherService::init();
    MarketService::init();

    // 6. Initialize System Telemetry (FPS, RAM, CPU)
    SystemTelemetry::init();

    // 7. TFT & Touch initialization
    tft.init();
    tft.setRotation(1);
    
    // Save/Restore touch calibration data (CYD 3.5 resistive touch)
    uint16_t calData[5] = { 334, 3478, 384, 3435, 3 }; 
    tft.setTouch(calData);

    // 8. LVGL System Initialization
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * bufferLines);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    indev_drv.scroll_limit = 6;
    indev_drv.long_press_time = 400;
    lv_indev_drv_register(&indev_drv);

    // 9. Initialize Dashboard UI
    LOG_I("Main", "Initializing UI panels...");
    ui = new DashboardUI();
    LOG_I("Main", "UI Initialization complete!");

    // Set initial values
    syncHomeScreenTelemetry();

    if (ui) {
        // Set initial values on Calendar Screen
        TimeInfo t = TimeService::getTimeInfo();
        if (t.isSynced && t.year >= 2024) {
            ui->setCalendarToday(t.year, t.month, t.day);
        } else {
            ui->setCalendarToday(2026, 8, 30);
        }

        // Set initial values on Player Screen
        ui->updateTrackInfo("Chưa mở trình phát", "Chạm Tab để phát nhạc", "Thẻ nhớ SD", "MP3/WAV");
        ui->updatePlaybackProgress(0, 0);
        ui->setPlayState(false);
        ui->updateVolume(50);
        ui->updatePlaybackMode(false, (int)AudioPlayerService::getRepeatMode());
        ui->updateEQ("DAC OUT");


        // Set initial values on Settings Screen
        SettingsDeviceInfo info = {"CYD 3.5 Controller", "ESP32-3248S035", "v2.5.0", "19/05/2026", "FreeRTOS", "CYD-35-ESP32"};
        ui->updateDeviceInfo(info);
        ui->updateSettingsTelemetry(SystemTelemetry::getFreeHeap(), SystemTelemetry::getUptimeFormatted().c_str(), WifiService::getIPAddress().c_str(), WifiService::getMacAddress().c_str());
        ui->updateWifiSettings(WifiService::getStateString(), WifiService::getConnectedSSID().c_str(), WifiService::getIPAddress().c_str(), WifiService::getRSSI());

        // Developer HUD state
        ui->setDevHudVisible(ConfigManager::isDevModeEnabled());
    }

    last1sTick = millis();
    last500msTick = millis();
    last5sTick = millis();
    last25msTick = millis();
}

void loop() {
    int64_t loopStartUs = esp_timer_get_time();

    lv_timer_handler(); // Run core LVGL drawing ticks
    SystemTelemetry::recordFrame();
    
    // Background services loop
    WifiService::update();
    BacklightManager::update();

    bool wifiConnected = WifiService::isConnected();
    TimeService::update(wifiConnected);

    int currentTab = ui ? ui->getActiveTab() : 0;

    // CHỈ chạy interval HTTP/HTTPS của Thời tiết & Giá vàng khi ĐANG Ở TAB HOME (Tab 0)
    // Rời khỏi Tab Home -> Dừng ngay các tác vụ mạng ngầm để giải phóng 100% RAM và CPU
    if (currentTab == 0) {
        WeatherService::update(wifiConnected);
        MarketService::update(wifiConnected);
    }

    // CHỈ xử lý auto-advance bài hát khi ĐANG Ở TAB PLAYER (Tab 2)
    if (currentTab == 2) {
        AudioPlayerService::update();
    }

    unsigned long now = millis();

    // 25ms tick: updates visualizer & micro-animations at smooth 40 FPS
    if (now - last25msTick >= 25) {
        last25msTick = now;
        if (ui) {
            ui->tick();
        }
    }

    // 500ms tick: updates Dev HUD & System Telemetry
    if (now - last500msTick >= 500) {
        last500msTick = now;
        SystemTelemetry::update();

        if (ui) {
            bool devMode = ConfigManager::isDevModeEnabled();
            ui->setDevHudVisible(devMode);
            if (devMode) {
                ui->updateDevHud(
                    SystemTelemetry::getFPS(),
                    SystemTelemetry::getFreeHeap() / 1024,
                    SystemTelemetry::getHeapUsagePercent(),
                    SystemTelemetry::getCpuUsage(),
                    WifiService::getRSSI(),
                    WifiService::getIPAddress().c_str()
                );
            }

            // Real-time telemetry in Settings Screen (chỉ cập nhật nếu đang ở Tab 3)
            if (currentTab == 3) {
                ui->updateSettingsTelemetry(
                    SystemTelemetry::getFreeHeap(),
                    SystemTelemetry::getUptimeFormatted().c_str(),
                    WifiService::getIPAddress().c_str(),
                    WifiService::getMacAddress().c_str()
                );
                ui->updateWifiSettings(
                    WifiService::getStateString(),
                    WifiService::getConnectedSSID().c_str(),
                    WifiService::getIPAddress().c_str(),
                    WifiService::getRSSI()
                );
            }
        }
    }

    // 1s tick: updates clock and live telemetry
    if (now - last1sTick >= 1000) {
        last1sTick = now;
        
        // Cập nhật Home Telemetry khi ở Tab 0
        if (currentTab == 0) {
            syncHomeScreenTelemetry();
        }

        // Real-time Audio Player state sync khi ở Tab 2
        if (currentTab == 2 && ui) {
            bool isPlaying = AudioPlayerService::isPlaying();
            ui->setPlayState(isPlaying);
            
            AudioTrack cur = AudioPlayerService::getCurrentTrack();
            int elapsed = AudioPlayerService::getCurrentElapsedSec();
            int curIdx = AudioPlayerService::getCurrentTrackIndex();
            int total = AudioPlayerService::getCurrentTotalSec();
            ui->updateTrackInfo(cur.title, cur.artist, "SD Music", cur.format);
            ui->updatePlaybackProgress(elapsed, total);

            ui->updatePlaybackMode(AudioPlayerService::isShuffle(), (int)AudioPlayerService::getRepeatMode());
            ui->updateVolume(AudioPlayerService::getVolume());

            // Lưu vết bài hát đang phát vào NVS an toàn tại loop() khi đổi bài
            static int lastSavedTrackIdx = -1;
            if (curIdx >= 0 && curIdx != lastSavedTrackIdx && isPlaying && cur.path[0] != '\0') {
                lastSavedTrackIdx = curIdx;
                ConfigManager::setLastAudioTrackPath(cur.path);
                ConfigManager::setLastAudioTrackIndex(curIdx);
            }
        }
    }

    // 5s tick: Force-sync đảm bảo dữ liệu không bị stale (chỉ khi đang ở Tab Home)
    if (now - last5sTick >= 5000) {
        last5sTick = now;
        if (currentTab == 0 && (now - last1sTick > 10)) {
            syncHomeScreenTelemetry();
        }
    }

    // Ghi nhận thời gian CPU Core 1 bận rộn thực tế (Microseconds)
    int64_t activeDurationUs = esp_timer_get_time() - loopStartUs;
    SystemTelemetry::recordActiveTime(activeDurationUs);

    delay(2);
}