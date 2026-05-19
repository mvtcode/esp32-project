#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "ui/dashboard/dashboard_ui.h"

// --- 1. Hardware Initialization ---
TFT_eSPI tft = TFT_eSPI();

// --- 2. LVGL Configuration ---
static const uint32_t screenWidth  = 480;
static const uint32_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10]; 

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
    }
}

// --- 3. Dynamic Mock Telemetry States ---
DashboardUI* ui = nullptr;

// Time and calendar state
int hourVal = 15;
int minVal = 45;
int secVal = 0;
int activeDay = 15; // 15/05/2025
int lunarDay = 18;
int lunarMonth = 4;
const char* lunarYear = "Ất Tỵ";

// Weather state
float temp = 28.5f;
float humidity = 84.0f;
float wind = 12.0f;
float uv = 6.2f;

// Gold Prices state
uint32_t goldBuy = 87500000;
uint32_t goldSell = 89500000;
bool goldUp = true;

// Fuel Prices state
uint32_t ron95 = 23780;
uint32_t e5 = 22620;
uint32_t diesel = 19850;
uint32_t mazut = 16420;

// Music Player state
int currentTrackSec = 84; // 01:24
int totalTrackSec = 275;  // 04:35
bool trackIsPlaying = true;
int activeTrackIdx = 0;
int volumeVal = 20;

// Timers
unsigned long last1sTick = 0;
unsigned long last5sTick = 0;
unsigned long last50msTick = 0;

// Helper to push all telemetry values to the HomeScreen
void syncHomeScreenTelemetry() {
    if (!ui) return;

    // 1. Clock Update
    char timeStr[12];
    char secStr[6];
    char dateStr[48];
    sprintf(timeStr, "%02d:%02d", hourVal, minVal);
    sprintf(secStr, "%02d", secVal);
    sprintf(dateStr, "Thứ Năm, 15/05/2025");
    ui->updateTime(timeStr, secStr, dateStr, (hourVal < 12));

    // 2. Lunar Calendar Update
    char lunarDayStr[32];
    char lunarInfoStr[48];
    sprintf(lunarDayStr, "%d/%d Lịch âm", lunarDay, lunarMonth);
    sprintf(lunarInfoStr, "Năm %s", lunarYear);
    ui->updateLunarCalendar(lunarDayStr, lunarInfoStr);

    // 3. Weekly Ribbon Update (Thursday is active day 15, index 3 of 7 columns)
    int dayNumbers[7] = {12, 13, 14, 15, 16, 17, 18};
    ui->updateCalendarRibbon(3, dayNumbers);

    // 4. Weather Update
    // signature expects: int temp, const char* condition, int feelsLike, int humidity, int windSpeed, int uvIndex
    ui->updateWeather((int)temp, "Nhiều mây", (int)(temp - 1.5f), (int)humidity, (int)wind, (int)uv);

    // 5. Gold prices update (values represent k-VND like the mockup strings)
    // signature expects: int buySJC, int sellSJC, int buyDelta, int sellDelta
    int goldBuyK = (int)(goldBuy / 1000);
    int goldSellK = (int)(goldSell / 1000);
    int buyDeltaK = goldUp ? 150 : -100;
    int sellDeltaK = goldUp ? 200 : -50;
    ui->updateGoldPrices(goldBuyK, goldSellK, buyDeltaK, sellDeltaK);

    // 6. Fuel prices update
    // signature expects: int ron95, int ron92, int diesel, int kerosene, int ron95Delta, int ron92Delta, int dieselDelta, int keroseneDelta
    ui->updateFuelPrices((int)ron95, (int)e5, (int)diesel, (int)mazut, 150, -80, 20, 0);
}

// --- 4. Setup and Main Loop ---
void setup() {
    Serial.begin(115200);
    delay(100);
    Serial.println("System starting...");

    // Turn on Backlight (CYD 3.5 uses GPIO 27)
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    // TFT & Touch initialization
    tft.init();
    tft.setRotation(1);
    
    // Save/Restore touch calibration data (matches standard ST7796 resistive CYD)
    uint16_t calData[5] = { 334, 3478, 384, 3435, 3 }; 
    tft.setTouch(calData);

    // LVGL System Initialization
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    /* Initialize display driver */
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /* Initialize touchpad driver */
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // Initialize custom modular C++ UI
    Serial.println("Initializing UI panels...");
    ui = new DashboardUI();
    Serial.println("UI Initialization complete!");

    // Set initial values on home screen
    syncHomeScreenTelemetry();

    if (ui) {
        // Set initial values on Calendar Screen
        ui->updateMonthYearHeader("Tháng 05, 2026");
        uint32_t dots[42] = {0};
        dots[4 + 15 - 1] = 0x5E35B1; // Active event dot on May 15th (Thursday)
        dots[4 + 20 - 1] = 0x4CAF50; // Active event dot on May 20th
        ui->updateCalendarDays(4, 31, 15, dots);
        ui->updateSyncStatus(true, true);
        ui->clearEvents();
        
        CalendarEvent ev1 = {"09:30\n10:30", "Họp Kế Hoạch Tuần", "Phòng họp A", "1 giờ", lv_color_hex(0x5E35B1)};
        CalendarEvent ev2 = {"14:00\n15:30", "Gặp Khách Hàng", "Phòng Khách", "1.5h", lv_color_hex(0x4CAF50)};
        ui->addEvent(ev1);
        ui->addEvent(ev2);

        // Set initial values on Player Screen
        ui->updateTrackInfo("Nơi Này Có Anh", "Sơn Tùng M-TP", "Album 2017", "MP3 320kbps");
        ui->updatePlaybackProgress(currentTrackSec, totalTrackSec);
        ui->setPlayState(true);
        ui->updatePlaybackMode(true, false);
        ui->updateVolume(volumeVal);
        ui->updateEQ("POP");
        ui->clearPlaylist();
        
        PlaylistItem track1 = {"Nơi Này Có Anh", "Sơn Tùng M-TP", "04:35", true};
        PlaylistItem track2 = {"Duyên Mình Lỡ", "Hương Tràm", "04:20", false};
        PlaylistItem track3 = {"Một Bước Yêu", "Mr. Siro", "05:12", false};
        ui->addPlaylistItem(track1);
        ui->addPlaylistItem(track2);
        ui->addPlaylistItem(track3);

        // Set initial values on Settings Screen
        SettingsDeviceInfo info = {"CYD 3.5 Controller", "ESP32-3248S035", "v1.0.0", "19/05/2026", "FreeRTOS", "SN-9842A1"};
        ui->updateDeviceInfo(info);
        ui->updateMemoryUsage(8.6f, 16.0f);
        ui->updateBatteryStatus(95, "1 giờ 15 phút");
        ui->updateWiFiConnection("Wifi_Home", "192.168.1.15");
        ui->updateLanguage("Tiếng Việt");
        ui->setActiveMenuItem(0);
    }

    last1sTick = millis();
    last5sTick = millis();
    last50msTick = millis();
}

void loop() {
    lv_timer_handler(); // Run core LVGL drawing ticks
    
    unsigned long now = millis();

    // 50ms tick: updates the visualizer spectrum waveforms fluidly
    if (now - last50msTick >= 50) {
        last50msTick = now;
        if (ui) ui->tick();
    }

    // 1s tick: updates clock and active track progress timelines
    if (now - last1sTick >= 1000) {
        last1sTick = now;

        // Clock Update
        secVal++;
        if (secVal >= 60) {
            secVal = 0;
            minVal++;
            if (minVal >= 60) {
                minVal = 0;
                hourVal++;
                if (hourVal >= 24) {
                    hourVal = 0;
                }
            }
        }
        
        // Sync time to Home Screen
        if (ui) {
            char timeStr[12];
            char secStr[6];
            char dateStr[48];
            sprintf(timeStr, "%02d:%02d", hourVal, minVal);
            sprintf(secStr, "%02d", secVal);
            sprintf(dateStr, "Thứ Năm, 15/05/2025");
            ui->updateTime(timeStr, secStr, dateStr, (hourVal < 12));
        }

        // Music Progress Update
        if (trackIsPlaying) {
            currentTrackSec++;
            if (currentTrackSec >= totalTrackSec) {
                currentTrackSec = 0;
                // cycle songs in mockup playlist!
                activeTrackIdx = (activeTrackIdx + 1) % 6;
                if (activeTrackIdx == 0) {
                    ui->updateTrackInfo("Nơi Này Có Anh", "Sơn Tùng M-TP", "Album 2017", "MP3 320kbps");
                    totalTrackSec = 275;
                } else if (activeTrackIdx == 1) {
                    ui->updateTrackInfo("Duyên Mình Lỡ", "Hương Tràm", "Single 2018", "FLAC 1411kbps");
                    totalTrackSec = 260;
                } else if (activeTrackIdx == 2) {
                    ui->updateTrackInfo("Một Bước Yêu", "Mr. Siro", "Single 2019", "MP3 320kbps");
                    totalTrackSec = 312;
                } else {
                    ui->updateTrackInfo("Hẹn Em Ở Lần Yêu 2", "Nguyễn Duy Anh", "Album 2021", "MP3 320kbps");
                    totalTrackSec = 288;
                }
            }
            if (ui) {
                ui->updatePlaybackProgress(currentTrackSec, totalTrackSec);
            }
        }
    }

    // 5s tick: simulates weather fluctuations and price index changes
    if (now - last5sTick >= 5000) {
        last5sTick = now;

        // Fluctuating Weather slightly
        float tempDelta = ((rand() % 11) - 5) * 0.1f; // -0.5 to +0.5
        temp += tempDelta;
        if (temp < 15.0f) temp = 15.0f;
        if (temp > 38.0f) temp = 38.0f;

        float humDelta = ((rand() % 5) - 2) * 1.0f; // -2 to +2
        humidity += humDelta;
        if (humidity < 40.0f) humidity = 40.0f;
        if (humidity > 95.0f) humidity = 95.0f;

        // Fluctuating SJC Gold price
        int goldDelta = ((rand() % 3) - 1) * 100000; // -100k, 0, or +100k
        goldBuy += goldDelta;
        goldSell += goldDelta;
        goldUp = (goldDelta >= 0);

        // Fluctuating Fuel slightly
        int fuelDelta = (rand() % 3) - 1; // -1, 0, +1 VND
        ron95 += fuelDelta * 20;
        e5 += fuelDelta * 10;

        // Push everything to the HomeScreen
        syncHomeScreenTelemetry();

        // Fluctuating Settings details
        static int batLvl = 78;
        static int batCountdown = 12; // decrease every 12 ticks
        batCountdown--;
        if (batCountdown <= 0) {
            batCountdown = 12;
            batLvl--;
            if (batLvl < 5) batLvl = 100;
            if (ui) {
                ui->updateBatteryStatus(batLvl, "~ 5 giờ 10 phút");
            }
        }
    }

    delay(2); // Short sleep to prevent starving background watchdog timers on ESP32
}