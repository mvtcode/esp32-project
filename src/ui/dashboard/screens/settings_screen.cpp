#include "settings_screen.h"
#include "../cyd_theme.h"
#include "../dialog_manager.h"
#include "../../../services/config_manager.h"
#include "../../../services/wifi_service.h"
#include "../../../services/storage_service.h"
#include "../../../services/backlight_manager.h"
#include "../../../services/system_telemetry.h"
#include "../../../services/weather_service.h"
#include "../../../services/market_service.h"
#include "../../../services/time_service.h"
#include "../../../services/ota_service.h"
#include "log.h"
#include "version.h"
#include <stdio.h>

static String s_pendingOtaUrl = "";
static lv_timer_t* s_otaMonitorTimer = nullptr;

SettingsScreen::SettingsScreen(lv_obj_t* parent) :
    currentMenuIndex(-1),
    rightPane(nullptr),
    lblDevName(nullptr), lblDevModel(nullptr), lblDevFw(nullptr), lblDevBuild(nullptr),
    lblDevOs(nullptr), lblDevSerial(nullptr), lblDevUptime(nullptr), lblDevRam(nullptr),
    lblDevWifi(nullptr), lblDevIp(nullptr), lblDevMac(nullptr),
    btnOtaCheck(nullptr), btnRestart(nullptr), btnFactoryReset(nullptr),
    lblWifiCurrentState(nullptr), lblWifiCurrentInfo(nullptr), btnWifiScan(nullptr),
    lblBtnScan(nullptr), wifiListContainer(nullptr),
    modalBackdrop(nullptr), modalCard(nullptr), modalSsidLabel(nullptr),
    taPassword(nullptr), keyboard(nullptr),

    lblSdStatus(nullptr), lblSdBusInfo(nullptr), barSdUsage(nullptr),
    lblSdCapacity(nullptr), lblSdPercent(nullptr), btnRefreshSd(nullptr),
    btnFormatSd(nullptr), lblSdActionMsg(nullptr),
    ddCity(nullptr), ddSyncInterval(nullptr), btnSyncNow(nullptr), lblSyncStatus(nullptr),
    sliderBrightness(nullptr), lblBrightnessVal(nullptr), ddSleepTimeout(nullptr), swAutoBrightness(nullptr),
    swDevMode(nullptr), sliderVolume(nullptr), lblVolumeVal(nullptr), swTouchBeep(nullptr),
    cachedFreeHeap(160000), cachedWifiRssi(-100),
    pendingCityIdx(-1), syncTimer(nullptr), syncCheckCount(0)
{
    strncpy(cachedDevName, "ESP32 CYD 3.5\" 480x320", sizeof(cachedDevName) - 1);
    strncpy(cachedDevModel, "ESP32-3248S035", sizeof(cachedDevModel) - 1);

    strncpy(cachedDevFw, FIRMWARE_VERSION, sizeof(cachedDevFw) - 1);
    strncpy(cachedAuthor, "Mạc Tân - 0964335688", sizeof(cachedAuthor) - 1);
    strncpy(cachedEmail, "macvantan@gmail.com", sizeof(cachedEmail) - 1);
    memset(selectedSsid, 0, sizeof(selectedSsid));
    strncpy(cachedUptime, "00:00:00", sizeof(cachedUptime) - 1);
    strncpy(cachedIp, "0.0.0.0", sizeof(cachedIp) - 1);
    strncpy(cachedMac, "--:--:--:--:--:--", sizeof(cachedMac) - 1);
    strncpy(cachedWifiState, "Chưa kết nối", sizeof(cachedWifiState) - 1);
    memset(cachedWifiSsid, 0, sizeof(cachedWifiSsid));

    // 1. Root container (480 x 282)
    rootContainer = lv_obj_create(parent);
    lv_obj_set_size(rootContainer, 480, 282);
    lv_obj_set_style_bg_opa(rootContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rootContainer, 0, 0);
    lv_obj_set_style_pad_all(rootContainer, 0, 0);
    lv_obj_clear_flag(rootContainer, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Build left sidebar (6 items)
    createSidebar(rootContainer);

    // 3. Right Content Host
    rightPane = lv_obj_create(rootContainer);
    lv_obj_set_size(rightPane, 372, 270);
    lv_obj_align(rightPane, LV_ALIGN_TOP_RIGHT, -6, 6);
    CydTheme::applyCardStyle(rightPane);
    lv_obj_set_style_pad_all(rightPane, 8, 0);

    // 4. Create Password Modal (hidden by default)
    createWifiPasswordModal();

    // Default select first item (Device Info)
    setActiveMenuItem(0);
}

SettingsScreen::~SettingsScreen() {
    if (syncTimer) {
        lv_timer_del(syncTimer);
        syncTimer = nullptr;
    }
    // Ẩn modal trước để ngăn bàn phím LVGL xử lý sự kiện sau khi destroyed
    hideWifiPasswordModal();
    DialogManager::dismissModal();
    DialogManager::hideLockOverlay();

    // Xóa modal backdrop (các child objects bị xóa theo cây LVGL)
    if (modalBackdrop) {
        lv_obj_del(modalBackdrop);
        // Null tất cả pointers con để tránh dangling reference
        modalBackdrop  = nullptr;
        modalCard      = nullptr;
        modalSsidLabel = nullptr;
        taPassword     = nullptr;
        keyboard       = nullptr;
    }

    // Dọn dẹp sub-pane đang active
    destroyCurrentPane();

    // Xóa root container (sidebar, rightPane và tất cả con bị xóa theo)
    if (rootContainer) {
        lv_obj_del(rootContainer);
        rootContainer = nullptr;
        // Null sidebar pointers vì chúng là con của rootContainer
        sidebarMenu = nullptr;
        rightPane   = nullptr;
        for (int i = 0; i < 6; i++) sidebarItems[i] = nullptr;
    }
}


void SettingsScreen::createSidebar(lv_obj_t* parent) {
    sidebarMenu = lv_obj_create(parent);
    lv_obj_set_size(sidebarMenu, 90, 270);
    lv_obj_align(sidebarMenu, LV_ALIGN_TOP_LEFT, 6, 6);
    CydTheme::applyCardStyle(sidebarMenu);

    lv_obj_set_layout(sidebarMenu, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sidebarMenu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sidebarMenu, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(sidebarMenu, 4, 0);
    lv_obj_set_style_pad_all(sidebarMenu, 3, 0);

    const char* menuNames[6] = {"Thiết Bị", "Mạng Wifi", "Thẻ SD", "Đồng Bộ", "Màn Hình", "Hệ Thống"};

    for (int i = 0; i < 6; i++) {
        sidebarItems[i] = lv_obj_create(sidebarMenu);
        lv_obj_set_size(sidebarItems[i], 82, 38);
        lv_obj_set_style_radius(sidebarItems[i], 6, 0);
        lv_obj_set_style_bg_color(sidebarItems[i], CydTheme::getCardBorderColor(), 0);
        lv_obj_set_style_bg_opa(sidebarItems[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sidebarItems[i], 0, 0);
        lv_obj_set_style_pad_all(sidebarItems[i], 0, 0);
        lv_obj_clear_flag(sidebarItems[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(sidebarItems[i], LV_OBJ_FLAG_CLICKABLE);

        lv_obj_set_user_data(sidebarItems[i], (void*)(intptr_t)i);
        lv_obj_add_event_cb(sidebarItems[i], sidebar_click_event_cb, LV_EVENT_CLICKED, this);

        lv_obj_t* lblItem = lv_label_create(sidebarItems[i]);
        lv_label_set_text(lblItem, menuNames[i]);
        CydTheme::applyTextFont(lblItem, CydTheme::getFont12(), CydTheme::getTextSecondary());
        lv_obj_center(lblItem);
        lv_obj_clear_flag(lblItem, LV_OBJ_FLAG_CLICKABLE);
    }
}

void SettingsScreen::destroyCurrentPane() {
    if (syncTimer) {
        lv_timer_del(syncTimer);
        syncTimer = nullptr;
    }
    if (s_otaMonitorTimer) {
        lv_timer_del(s_otaMonitorTimer);
        s_otaMonitorTimer = nullptr;
    }
    DialogManager::dismissModal();
    if (rightPane) {
        lv_obj_clean(rightPane);
    }

    // Reset pointers
    lblDevName = nullptr;
    lblDevAuthor = nullptr;
    lblDevEmail = nullptr;
    lblDevModel = nullptr;
    lblDevFw = nullptr;
    lblDevBuild = nullptr;
    lblDevOs = nullptr;
    lblDevSerial = nullptr;
    lblDevUptime = nullptr;
    lblDevRam = nullptr;
    lblDevWifi = nullptr;
    lblDevIp = nullptr;
    lblDevMac = nullptr;
    btnOtaCheck = nullptr;
    btnRestart = nullptr;
    btnFactoryReset = nullptr;


    lblWifiCurrentState = nullptr;
    lblWifiCurrentInfo = nullptr;
    btnWifiScan = nullptr;
    lblBtnScan = nullptr;
    wifiListContainer = nullptr;

    lblSdStatus = nullptr;
    lblSdBusInfo = nullptr;
    barSdUsage = nullptr;
    lblSdCapacity = nullptr;
    lblSdPercent = nullptr;
    btnRefreshSd = nullptr;
    btnFormatSd = nullptr;
    lblSdActionMsg = nullptr;

    ddCity = nullptr;
    ddSyncInterval = nullptr;
    btnSyncNow = nullptr;
    lblSyncStatus = nullptr;

    sliderBrightness = nullptr;
    lblBrightnessVal = nullptr;
    ddSleepTimeout = nullptr;
    swAutoBrightness = nullptr;

    swDevMode = nullptr;
    sliderVolume = nullptr;
    lblVolumeVal = nullptr;
    swTouchBeep = nullptr;
}

// -------------------------------------------------------------
// TAB 0: DEVICE INFO PANE
// -------------------------------------------------------------
void SettingsScreen::buildDevicePane() {
    lv_obj_t* cardInfo = lv_obj_create(rightPane);
    lv_obj_set_size(cardInfo, 354, 195);
    lv_obj_align(cardInfo, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(cardInfo, lv_color_make(10, 16, 30), 0);
    lv_obj_set_style_border_color(cardInfo, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_border_width(cardInfo, 1, 0);
    lv_obj_set_style_radius(cardInfo, 8, 0);
    lv_obj_set_style_pad_all(cardInfo, 8, 0);
    lv_obj_clear_flag(cardInfo, LV_OBJ_FLAG_SCROLLABLE);

    char titleBuf[96];
    if (strstr(cachedDevName, cachedDevFw) != nullptr) {
        snprintf(titleBuf, sizeof(titleBuf), "%s", cachedDevName);
    } else {
        snprintf(titleBuf, sizeof(titleBuf), "%s (%s - %s)", cachedDevName, cachedDevFw, FIRMWARE_RELEASE_DATE);
    }

    lblDevName = lv_label_create(cardInfo);
    lv_label_set_text(lblDevName, titleBuf);
    CydTheme::applyTextFont(lblDevName, CydTheme::getFont14(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblDevName, LV_ALIGN_TOP_LEFT, 0, 0);

    lblDevAuthor = lv_label_create(cardInfo);
    char authorBuf[96];
    snprintf(authorBuf, sizeof(authorBuf), "Tác giả: %s", cachedAuthor);
    lv_label_set_text(lblDevAuthor, authorBuf);
    CydTheme::applyTextFont(lblDevAuthor, CydTheme::getFont12(), CydTheme::getTextPrimary());
    lv_obj_align(lblDevAuthor, LV_ALIGN_TOP_LEFT, 0, 22);

    lblDevEmail = lv_label_create(cardInfo);
    char emailBuf[96];
    snprintf(emailBuf, sizeof(emailBuf), "Email: %s", cachedEmail);
    lv_label_set_text(lblDevEmail, emailBuf);
    CydTheme::applyTextFont(lblDevEmail, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblDevEmail, LV_ALIGN_TOP_LEFT, 0, 44);

    lblDevRam = lv_label_create(cardInfo);
    char ramBuf[64];
    snprintf(ramBuf, sizeof(ramBuf), "RAM Trống: %u KB (%.1f%%)", cachedFreeHeap / 1024, SystemTelemetry::getHeapUsagePercent());
    lv_label_set_text(lblDevRam, ramBuf);
    CydTheme::applyTextFont(lblDevRam, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDevRam, LV_ALIGN_TOP_LEFT, 0, 66);

    lblDevUptime = lv_label_create(cardInfo);
    char upBuf[64];
    snprintf(upBuf, sizeof(upBuf), "Thời gian chạy: %s", cachedUptime);
    lv_label_set_text(lblDevUptime, upBuf);
    CydTheme::applyTextFont(lblDevUptime, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDevUptime, LV_ALIGN_TOP_LEFT, 0, 88);

    lblDevWifi = lv_label_create(cardInfo);
    char wifiBuf[64];
    String ssid = WifiService::getConnectedSSID();
    if (ssid.length() == 0 && ConfigManager::hasWifiCredentials()) {
        ssid = ConfigManager::getWifiSSID();
    }
    if (ssid.length() > 0) {
        snprintf(wifiBuf, sizeof(wifiBuf), "Mạng WiFi: %s", ssid.c_str());
    } else {
        snprintf(wifiBuf, sizeof(wifiBuf), "Mạng WiFi: Chưa kết nối");
    }
    lv_label_set_text(lblDevWifi, wifiBuf);
    CydTheme::applyTextFont(lblDevWifi, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDevWifi, LV_ALIGN_TOP_LEFT, 0, 110);

    lblDevIp = lv_label_create(cardInfo);
    char ipBuf[64];
    snprintf(ipBuf, sizeof(ipBuf), "Địa chỉ IP: %s", cachedIp);
    lv_label_set_text(lblDevIp, ipBuf);
    CydTheme::applyTextFont(lblDevIp, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblDevIp, LV_ALIGN_TOP_LEFT, 0, 132);

    lblDevMac = lv_label_create(cardInfo);
    char macBuf[64];
    snprintf(macBuf, sizeof(macBuf), "Địa chỉ MAC: %s", cachedMac);
    lv_label_set_text(lblDevMac, macBuf);
    CydTheme::applyTextFont(lblDevMac, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblDevMac, LV_ALIGN_TOP_LEFT, 0, 154);

    // Bottom Action Row (3 nút: Cập nhật OTA, Khởi động lại, Khôi phục gốc)
    btnOtaCheck = lv_btn_create(rightPane);
    lv_obj_set_size(btnOtaCheck, 112, 38);
    lv_obj_align(btnOtaCheck, LV_ALIGN_BOTTOM_LEFT, 0, -4);
    lv_obj_set_style_bg_color(btnOtaCheck, lv_color_make(20, 110, 80), 0);
    lv_obj_set_style_radius(btnOtaCheck, 6, 0);
    lv_obj_add_event_cb(btnOtaCheck, ota_check_click_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* lblBtnOta = lv_label_create(btnOtaCheck);
    lv_label_set_text(lblBtnOta, LV_SYMBOL_DOWNLOAD " Cập Nhật");
    CydTheme::applyTextFont(lblBtnOta, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblBtnOta);

    btnRestart = lv_btn_create(rightPane);
    lv_obj_set_size(btnRestart, 112, 38);
    lv_obj_align(btnRestart, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_obj_set_style_bg_color(btnRestart, lv_color_make(30, 60, 110), 0);
    lv_obj_set_style_radius(btnRestart, 6, 0);
    lv_obj_add_event_cb(btnRestart, restart_click_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* lblBtnRst = lv_label_create(btnRestart);
    lv_label_set_text(lblBtnRst, LV_SYMBOL_REFRESH " Khởi Động");
    CydTheme::applyTextFont(lblBtnRst, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblBtnRst);

    btnFactoryReset = lv_btn_create(rightPane);
    lv_obj_set_size(btnFactoryReset, 112, 38);
    lv_obj_align(btnFactoryReset, LV_ALIGN_BOTTOM_RIGHT, 0, -4);
    lv_obj_set_style_bg_color(btnFactoryReset, lv_color_make(100, 30, 40), 0);
    lv_obj_set_style_radius(btnFactoryReset, 6, 0);
    lv_obj_add_event_cb(btnFactoryReset, factory_reset_click_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* lblBtnFac = lv_label_create(btnFactoryReset);
    lv_label_set_text(lblBtnFac, LV_SYMBOL_TRASH " Khôi Phục");
    CydTheme::applyTextFont(lblBtnFac, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblBtnFac);
}

// -------------------------------------------------------------
// TAB 1: WIFI MANAGER PANE
// -------------------------------------------------------------
void SettingsScreen::buildWifiPane() {
    lv_obj_t* topHeader = lv_obj_create(rightPane);
    lv_obj_set_size(topHeader, 354, 44);
    lv_obj_align(topHeader, LV_ALIGN_TOP_MID, 0, 0);
    CydTheme::applyCardStyle(topHeader);
    lv_obj_set_style_pad_all(topHeader, 4, 0);

    lblWifiCurrentState = lv_label_create(topHeader);
    char wBuf[64];
    if (strlen(cachedWifiSsid) > 0) {
        snprintf(wBuf, sizeof(wBuf), "WiFi: %s (%ddBm)", cachedWifiSsid, cachedWifiRssi);
    } else {
        snprintf(wBuf, sizeof(wBuf), "WiFi: %s", cachedWifiState);
    }
    lv_label_set_text(lblWifiCurrentState, wBuf);
    CydTheme::applyTextFont(lblWifiCurrentState, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblWifiCurrentState, LV_ALIGN_TOP_LEFT, 4, 2);

    lblWifiCurrentInfo = lv_label_create(topHeader);
    char ipBuf[64];
    snprintf(ipBuf, sizeof(ipBuf), "IP: %s", (strlen(cachedIp) > 0) ? cachedIp : "0.0.0.0");
    lv_label_set_text(lblWifiCurrentInfo, ipBuf);
    CydTheme::applyTextFont(lblWifiCurrentInfo, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblWifiCurrentInfo, LV_ALIGN_TOP_LEFT, 4, 18);

    btnWifiScan = lv_btn_create(topHeader);
    lv_obj_set_size(btnWifiScan, 110, 32);
    lv_obj_align(btnWifiScan, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_bg_color(btnWifiScan, CydTheme::getAccentColor(), 0);
    lv_obj_set_style_radius(btnWifiScan, 6, 0);
    lv_obj_add_event_cb(btnWifiScan, wifi_scan_click_cb, LV_EVENT_CLICKED, this);

    lblBtnScan = lv_label_create(btnWifiScan);
    if (WifiService::isScanning()) {
        lv_label_set_text(lblBtnScan, LV_SYMBOL_REFRESH " Đang Quét...");
    } else {
        lv_label_set_text(lblBtnScan, LV_SYMBOL_REFRESH " Quét Mạng");
    }
    CydTheme::applyTextFont(lblBtnScan, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblBtnScan);

    // Scrollable WiFi List container
    wifiListContainer = lv_list_create(rightPane);
    lv_obj_set_size(wifiListContainer, 354, 195);
    lv_obj_align(wifiListContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
    CydTheme::applyCardStyle(wifiListContainer);
    lv_obj_set_style_pad_all(wifiListContainer, 4, 0);
    lv_obj_set_style_bg_color(wifiListContainer, lv_color_make(12, 18, 32), 0);

    updateWifiScanList();
}

// -------------------------------------------------------------
// TAB 2: DEDICATED SD CARD PANE
// -------------------------------------------------------------
void SettingsScreen::buildSdCardPane() {
    StorageInfo sd = StorageService::getInfo();

    // 1. Hardware & Status Card
    lv_obj_t* cardStatus = lv_obj_create(rightPane);
    lv_obj_set_size(cardStatus, 354, 78);
    lv_obj_align(cardStatus, LV_ALIGN_TOP_MID, 0, 0);
    CydTheme::applyCardStyle(cardStatus);
    lv_obj_set_style_pad_all(cardStatus, 8, 0);

    lv_obj_t* lblSdTitle = lv_label_create(cardStatus);
    lv_label_set_text(lblSdTitle, LV_SYMBOL_DRIVE " THẺ NHỚ MICROSD");
    CydTheme::applyTextFont(lblSdTitle, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblSdTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    lblSdStatus = lv_label_create(cardStatus);
    char stBuf[64];
    if (sd.isMounted) {
        snprintf(stBuf, sizeof(stBuf), "Trạng thái: Đã nhận thẻ (%s)", sd.cardType);
        CydTheme::applyTextFont(lblSdStatus, CydTheme::getFont12(), CydTheme::getSuccessColor());
    } else {
        snprintf(stBuf, sizeof(stBuf), "Trạng thái: Chưa gắn thẻ MicroSD");
        CydTheme::applyTextFont(lblSdStatus, CydTheme::getFont12(), CydTheme::getDangerColor());
    }
    lv_label_set_text(lblSdStatus, stBuf);
    lv_obj_align(lblSdStatus, LV_ALIGN_TOP_LEFT, 0, 22);

    lblSdBusInfo = lv_label_create(cardStatus);
    lv_label_set_text(lblSdBusInfo, "Giao tiếp: VSPI (CS=5, CLK=18, MOSI=23, MISO=19) | 20MHz");
    CydTheme::applyTextFont(lblSdBusInfo, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblSdBusInfo, LV_ALIGN_TOP_LEFT, 0, 44);

    // 2. Storage Capacity Breakdown Card
    lv_obj_t* cardCap = lv_obj_create(rightPane);
    lv_obj_set_size(cardCap, 354, 110);
    lv_obj_align(cardCap, LV_ALIGN_TOP_MID, 0, 86);
    CydTheme::applyCardStyle(cardCap);
    lv_obj_set_style_pad_all(cardCap, 8, 0);

    lv_obj_t* lblCapTitle = lv_label_create(cardCap);
    lv_label_set_text(lblCapTitle, LV_SYMBOL_DIRECTORY " Dung Lượng Lưu Trữ:");
    CydTheme::applyTextFont(lblCapTitle, CydTheme::getFont12(), CydTheme::getTextPrimary());
    lv_obj_align(lblCapTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    int percentUsed = 0;
    if (sd.isMounted && sd.totalBytes > 0) {
        if (sd.usedBytes > 0) {
            percentUsed = (int)((sd.usedBytes * 100) / sd.totalBytes);
            if (percentUsed < 1) percentUsed = 1;
        } else {
            percentUsed = 0;
        }
    }

    lblSdPercent = lv_label_create(cardCap);
    char pBuf[16];
    snprintf(pBuf, sizeof(pBuf), "%d%%", percentUsed);
    lv_label_set_text(lblSdPercent, pBuf);
    CydTheme::applyTextFont(lblSdPercent, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblSdPercent, LV_ALIGN_TOP_RIGHT, 0, 0);

    barSdUsage = lv_bar_create(cardCap);
    lv_obj_set_size(barSdUsage, 334, 14);
    lv_obj_align(barSdUsage, LV_ALIGN_TOP_MID, 0, 26);
    lv_bar_set_range(barSdUsage, 0, 100);
    lv_bar_set_value(barSdUsage, percentUsed, LV_ANIM_OFF);
    lv_obj_set_style_bg_color(barSdUsage, lv_color_make(20, 30, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(barSdUsage, CydTheme::getAccentColor(), LV_PART_INDICATOR);

    lblSdCapacity = lv_label_create(cardCap);
    char capBuf[128];
    if (sd.isMounted && sd.totalBytes > 0) {
        double usedMB = (double)sd.usedBytes / (1024.0 * 1024.0);
        double totalGB = (double)sd.totalBytes / (1024.0 * 1024.0 * 1024.0);
        double freeGB = (double)sd.freeBytes / (1024.0 * 1024.0 * 1024.0);
        if (usedMB >= 1024.0) {
            snprintf(capBuf, sizeof(capBuf), "Đã dùng: %.2f GB / Tổng: %.2f GB (Còn trống: %.2f GB)", usedMB / 1024.0, totalGB, freeGB);
        } else {
            snprintf(capBuf, sizeof(capBuf), "Đã dùng: %.1f MB / Tổng: %.2f GB (Còn trống: %.2f GB)", usedMB, totalGB, freeGB);
        }
    } else if (sd.isMounted) {
        snprintf(capBuf, sizeof(capBuf), "Đã nhận thẻ nhưng chưa đọc được phân vùng.");
    } else {
        snprintf(capBuf, sizeof(capBuf), "Chưa nhận thẻ nhớ. Vui lòng cắm thẻ MicroSD.");
    }
    lv_label_set_text(lblSdCapacity, capBuf);
    CydTheme::applyTextFont(lblSdCapacity, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblSdCapacity, LV_ALIGN_TOP_LEFT, 0, 48);

    lblSdActionMsg = lv_label_create(cardCap);
    lv_label_set_text(lblSdActionMsg, "Chuẩn định dạng khuyến nghị: FAT32 cho MP3 Player.");
    CydTheme::applyTextFont(lblSdActionMsg, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblSdActionMsg, LV_ALIGN_TOP_LEFT, 0, 72);

    // 3. Action Buttons Row
    btnRefreshSd = lv_btn_create(rightPane);
    lv_obj_set_size(btnRefreshSd, 168, 40);
    lv_obj_align(btnRefreshSd, LV_ALIGN_BOTTOM_LEFT, 0, -4);
    lv_obj_set_style_bg_color(btnRefreshSd, lv_color_make(30, 70, 130), 0);
    lv_obj_set_style_radius(btnRefreshSd, 6, 0);
    lv_obj_add_event_cb(btnRefreshSd, refresh_sd_click_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* lblRef = lv_label_create(btnRefreshSd);
    lv_label_set_text(lblRef, LV_SYMBOL_REFRESH " Kiểm Tra Thẻ");
    CydTheme::applyTextFont(lblRef, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblRef);

    btnFormatSd = lv_btn_create(rightPane);
    lv_obj_set_size(btnFormatSd, 168, 40);
    lv_obj_align(btnFormatSd, LV_ALIGN_BOTTOM_RIGHT, 0, -4);
    lv_obj_set_style_bg_color(btnFormatSd, lv_color_make(120, 40, 50), 0);
    lv_obj_set_style_radius(btnFormatSd, 6, 0);
    lv_obj_add_event_cb(btnFormatSd, format_sd_click_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* lblFmt = lv_label_create(btnFormatSd);
    lv_label_set_text(lblFmt, LV_SYMBOL_TRASH " Format Thẻ Nhớ");
    CydTheme::applyTextFont(lblFmt, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblFmt);
}

// -------------------------------------------------------------
// TAB 3: LOCATION & SYNC PANE
// -------------------------------------------------------------
void SettingsScreen::buildSyncPane() {
    // 1. City Dropdown Row
    lv_obj_t* cardCity = lv_obj_create(rightPane);
    lv_obj_set_size(cardCity, 354, 76);
    lv_obj_align(cardCity, LV_ALIGN_TOP_MID, 0, 0);
    CydTheme::applyCardStyle(cardCity);
    lv_obj_set_style_pad_all(cardCity, 8, 0);

    lv_obj_t* lblCityTitle = lv_label_create(cardCity);
    lv_label_set_text(lblCityTitle, LV_SYMBOL_GPS " Tỉnh / Thành Phố Thời Tiết:");
    CydTheme::applyTextFont(lblCityTitle, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblCityTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    ddCity = lv_dropdown_create(cardCity);
    lv_obj_set_size(ddCity, 334, 36);
    lv_obj_align(ddCity, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(ddCity, lv_color_make(18, 26, 44), 0);
    lv_obj_set_style_text_color(ddCity, CydTheme::getWhiteColor(), 0);
    lv_obj_set_style_text_font(ddCity, CydTheme::getFont12(), 0);
    lv_obj_set_style_border_color(ddCity, CydTheme::getCardBorderColor(), 0);

    String cityOptions = "";
    for (size_t i = 0; i < VIETNAM_CITIES_COUNT; ++i) {
        cityOptions += VIETNAM_CITIES[i].name;
        if (i < VIETNAM_CITIES_COUNT - 1) cityOptions += "\n";
    }
    lv_dropdown_set_options(ddCity, cityOptions.c_str());
    lv_dropdown_set_selected(ddCity, ConfigManager::getCityIndex());
    lv_obj_add_event_cb(ddCity, city_changed_cb, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t* listCity = lv_dropdown_get_list(ddCity);
    if (listCity) {
        lv_obj_set_style_text_font(listCity, CydTheme::getFont12(), 0);
        lv_obj_set_style_bg_color(listCity, lv_color_make(18, 26, 44), 0);
        lv_obj_set_style_text_color(listCity, CydTheme::getWhiteColor(), 0);
        lv_obj_set_style_border_color(listCity, CydTheme::getCardBorderColor(), 0);
        lv_obj_set_style_max_height(listCity, 160, 0);
    }

    // 2. Sync Interval Row
    lv_obj_t* cardInterval = lv_obj_create(rightPane);
    lv_obj_set_size(cardInterval, 354, 76);
    lv_obj_align(cardInterval, LV_ALIGN_TOP_MID, 0, 84);
    CydTheme::applyCardStyle(cardInterval);
    lv_obj_set_style_pad_all(cardInterval, 8, 0);

    lv_obj_t* lblIntTitle = lv_label_create(cardInterval);
    lv_label_set_text(lblIntTitle, LV_SYMBOL_LOOP " Tần Suất Cập Nhật Dữ Liệu:");
    CydTheme::applyTextFont(lblIntTitle, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblIntTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    ddSyncInterval = lv_dropdown_create(cardInterval);
    lv_obj_set_size(ddSyncInterval, 334, 36);
    lv_obj_align(ddSyncInterval, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(ddSyncInterval, lv_color_make(18, 26, 44), 0);
    lv_obj_set_style_text_color(ddSyncInterval, CydTheme::getWhiteColor(), 0);
    lv_obj_set_style_text_font(ddSyncInterval, CydTheme::getFont12(), 0);
    lv_dropdown_set_options(ddSyncInterval, "15 Phút\n30 Phút\n1 Giờ\n2 Giờ");

    int curInt = ConfigManager::getSyncIntervalMinutes();
    int intIdx = 1;
    if (curInt == 15) intIdx = 0;
    else if (curInt == 60) intIdx = 2;
    else if (curInt == 120) intIdx = 3;
    lv_dropdown_set_selected(ddSyncInterval, intIdx);
    lv_obj_add_event_cb(ddSyncInterval, sync_interval_changed_cb, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t* listInt = lv_dropdown_get_list(ddSyncInterval);
    if (listInt) {
        lv_obj_set_style_text_font(listInt, CydTheme::getFont12(), 0);
        lv_obj_set_style_bg_color(listInt, lv_color_make(18, 26, 44), 0);
        lv_obj_set_style_text_color(listInt, CydTheme::getWhiteColor(), 0);
        lv_obj_set_style_border_color(listInt, CydTheme::getCardBorderColor(), 0);
    }

    // 3. Sync Now Button
    btnSyncNow = lv_btn_create(rightPane);
    lv_obj_set_size(btnSyncNow, 354, 40);
    lv_obj_align(btnSyncNow, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(btnSyncNow, CydTheme::getAccentColor(), 0);
    lv_obj_set_style_radius(btnSyncNow, 8, 0);
    lv_obj_add_event_cb(btnSyncNow, sync_now_click_cb, LV_EVENT_CLICKED, this);

    lblSyncStatus = lv_label_create(btnSyncNow);
    lv_label_set_text(lblSyncStatus, LV_SYMBOL_DOWNLOAD " Đồng Bộ Dữ Liệu Ngay");
    CydTheme::applyTextFont(lblSyncStatus, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblSyncStatus);
}

// -------------------------------------------------------------
// TAB 4: DISPLAY & POWER PANE
// -------------------------------------------------------------
void SettingsScreen::buildDisplayPane() {
    // 1. Brightness Slider Card
    lv_obj_t* cardBright = lv_obj_create(rightPane);
    lv_obj_set_size(cardBright, 354, 82);
    lv_obj_align(cardBright, LV_ALIGN_TOP_MID, 0, 0);
    CydTheme::applyCardStyle(cardBright);
    lv_obj_set_style_pad_all(cardBright, 8, 0);

    lv_obj_t* lblBrTitle = lv_label_create(cardBright);
    lv_label_set_text(lblBrTitle, LV_SYMBOL_IMAGE " Độ Sáng Màn Hình (PWM):");
    CydTheme::applyTextFont(lblBrTitle, CydTheme::getFont12(), CydTheme::getTextPrimary());
    lv_obj_align(lblBrTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    lblBrightnessVal = lv_label_create(cardBright);
    char brBuf[16];
    snprintf(brBuf, sizeof(brBuf), "%d%%", ConfigManager::getBrightness());
    lv_label_set_text(lblBrightnessVal, brBuf);
    CydTheme::applyTextFont(lblBrightnessVal, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblBrightnessVal, LV_ALIGN_TOP_RIGHT, 0, 0);

    sliderBrightness = lv_slider_create(cardBright);
    lv_obj_set_size(sliderBrightness, 334, 14);
    lv_obj_align(sliderBrightness, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_slider_set_range(sliderBrightness, 10, 100);
    lv_slider_set_value(sliderBrightness, ConfigManager::getBrightness(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sliderBrightness, lv_color_make(20, 30, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sliderBrightness, CydTheme::getAccentColor(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sliderBrightness, CydTheme::getWhiteColor(), LV_PART_KNOB);
    lv_obj_add_event_cb(sliderBrightness, brightness_changed_cb, LV_EVENT_VALUE_CHANGED, this);

    // 2. Sleep Timeout Card
    lv_obj_t* cardSleep = lv_obj_create(rightPane);
    lv_obj_set_size(cardSleep, 354, 76);
    lv_obj_align(cardSleep, LV_ALIGN_TOP_MID, 0, 90);
    CydTheme::applyCardStyle(cardSleep);
    lv_obj_set_style_pad_all(cardSleep, 8, 0);

    lv_obj_t* lblSleepTitle = lv_label_create(cardSleep);
    lv_label_set_text(lblSleepTitle, LV_SYMBOL_POWER " Tự Động Tắt Màn Hình:");
    CydTheme::applyTextFont(lblSleepTitle, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblSleepTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    ddSleepTimeout = lv_dropdown_create(cardSleep);
    lv_obj_set_size(ddSleepTimeout, 334, 36);
    lv_obj_align(ddSleepTimeout, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(ddSleepTimeout, lv_color_make(18, 26, 44), 0);
    lv_obj_set_style_text_color(ddSleepTimeout, CydTheme::getWhiteColor(), 0);
    lv_obj_set_style_text_font(ddSleepTimeout, CydTheme::getFont12(), 0);
    lv_dropdown_set_options(ddSleepTimeout, "30 Giây\n1 Phút\n3 Phút\n5 Phút\nKhông bao giờ tắt");

    int sleepSec = ConfigManager::getSleepTimeoutSeconds();
    int sIdx = 1;
    if (sleepSec == 30) sIdx = 0;
    else if (sleepSec == 60) sIdx = 1;
    else if (sleepSec == 180) sIdx = 2;
    else if (sleepSec == 300) sIdx = 3;
    else if (sleepSec == 0) sIdx = 4;
    lv_dropdown_set_selected(ddSleepTimeout, sIdx);
    lv_obj_add_event_cb(ddSleepTimeout, sleep_timeout_changed_cb, LV_EVENT_VALUE_CHANGED, this);

    lv_obj_t* listSleep = lv_dropdown_get_list(ddSleepTimeout);
    if (listSleep) {
        lv_obj_set_style_text_font(listSleep, CydTheme::getFont12(), 0);
        lv_obj_set_style_bg_color(listSleep, lv_color_make(18, 26, 44), 0);
        lv_obj_set_style_text_color(listSleep, CydTheme::getWhiteColor(), 0);
        lv_obj_set_style_border_color(listSleep, CydTheme::getCardBorderColor(), 0);
    }

    // 3. Touch Wakeup Hint Card
    lv_obj_t* cardHint = lv_obj_create(rightPane);
    lv_obj_set_size(cardHint, 354, 66);
    lv_obj_align(cardHint, LV_ALIGN_BOTTOM_MID, 0, 0);
    CydTheme::applyCardStyle(cardHint);
    lv_obj_set_style_pad_all(cardHint, 8, 0);

    lv_obj_t* lblHint = lv_label_create(cardHint);
    lv_label_set_text(lblHint, LV_SYMBOL_BELL " Chạm vào bất kỳ điểm nào trên màn hình cảm ứng để đánh thức thiết bị.");
    CydTheme::applyTextFont(lblHint, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_label_set_long_mode(lblHint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lblHint, 334);
    lv_obj_center(lblHint);
}

// -------------------------------------------------------------
// TAB 5: SYSTEM & DEVELOPER MODE PANE
// -------------------------------------------------------------
void SettingsScreen::buildSystemPane() {
    // 1. Development Mode Switch Card
    lv_obj_t* cardDev = lv_obj_create(rightPane);
    lv_obj_set_size(cardDev, 354, 76);
    lv_obj_align(cardDev, LV_ALIGN_TOP_MID, 0, 0);
    CydTheme::applyCardStyle(cardDev);
    lv_obj_set_style_pad_all(cardDev, 8, 0);

    lv_obj_t* lblDevTitle = lv_label_create(cardDev);
    lv_label_set_text(lblDevTitle, LV_SYMBOL_SETTINGS " Chế Độ Nhà Phát Triển (HUD):");
    CydTheme::applyTextFont(lblDevTitle, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblDevTitle, LV_ALIGN_TOP_LEFT, 0, 4);

    lv_obj_t* lblDevSub = lv_label_create(cardDev);
    lv_label_set_text(lblDevSub, "Hiển thị FPS, RAM, CPU & WiFi nổi");
    CydTheme::applyTextFont(lblDevSub, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblDevSub, LV_ALIGN_BOTTOM_LEFT, 0, -4);

    swDevMode = lv_switch_create(cardDev);
    lv_obj_align(swDevMode, LV_ALIGN_RIGHT_MID, 0, 0);
    if (ConfigManager::isDevModeEnabled()) {
        lv_obj_add_state(swDevMode, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(swDevMode, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(swDevMode, dev_mode_toggle_cb, LV_EVENT_VALUE_CHANGED, this);

    // 2. Default Volume Card
    lv_obj_t* cardVol = lv_obj_create(rightPane);
    lv_obj_set_size(cardVol, 354, 82);
    lv_obj_align(cardVol, LV_ALIGN_TOP_MID, 0, 84);
    CydTheme::applyCardStyle(cardVol);
    lv_obj_set_style_pad_all(cardVol, 8, 0);

    lv_obj_t* lblVolTitle = lv_label_create(cardVol);
    lv_label_set_text(lblVolTitle, LV_SYMBOL_VOLUME_MAX " Âm Lượng Khởi Động:");
    CydTheme::applyTextFont(lblVolTitle, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_align(lblVolTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    lblVolumeVal = lv_label_create(cardVol);
    char volBuf[16];
    snprintf(volBuf, sizeof(volBuf), "%d%%", ConfigManager::getDefaultVolume());
    lv_label_set_text(lblVolumeVal, volBuf);
    CydTheme::applyTextFont(lblVolumeVal, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblVolumeVal, LV_ALIGN_TOP_RIGHT, 0, 0);

    sliderVolume = lv_slider_create(cardVol);
    lv_obj_set_size(sliderVolume, 334, 14);
    lv_obj_align(sliderVolume, LV_ALIGN_BOTTOM_MID, 0, -4);
    lv_slider_set_range(sliderVolume, 0, 100);
    lv_slider_set_value(sliderVolume, ConfigManager::getDefaultVolume(), LV_ANIM_OFF);
    lv_obj_set_style_bg_color(sliderVolume, lv_color_make(20, 30, 50), LV_PART_MAIN);
    lv_obj_set_style_bg_color(sliderVolume, CydTheme::getGoldColor(), LV_PART_INDICATOR);
    lv_obj_set_style_bg_color(sliderVolume, CydTheme::getWhiteColor(), LV_PART_KNOB);
    lv_obj_add_event_cb(sliderVolume, volume_changed_cb, LV_EVENT_VALUE_CHANGED, this);

    // 3. System Info Hint
    lv_obj_t* cardSysHint = lv_obj_create(rightPane);
    lv_obj_set_size(cardSysHint, 354, 66);
    lv_obj_align(cardSysHint, LV_ALIGN_BOTTOM_MID, 0, 0);
    CydTheme::applyCardStyle(cardSysHint);
    lv_obj_set_style_pad_all(cardSysHint, 8, 0);

    lv_obj_t* lblSysHint = lv_label_create(cardSysHint);
    lv_label_set_text(lblSysHint, LV_SYMBOL_OK " Cấu hình được lưu tự động vào NVS Flash và không bị mất khi khởi động lại.");
    CydTheme::applyTextFont(lblSysHint, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_label_set_long_mode(lblSysHint, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(lblSysHint, 334);
    lv_obj_center(lblSysHint);
}

// -------------------------------------------------------------
// WIFI PASSWORD MODAL & VIRTUAL KEYBOARD
// -------------------------------------------------------------
void SettingsScreen::createWifiPasswordModal() {
    modalBackdrop = lv_obj_create(lv_layer_top());
    lv_obj_set_size(modalBackdrop, 480, 320);
    lv_obj_align(modalBackdrop, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(modalBackdrop, lv_color_black(), 0);
    lv_obj_set_style_bg_opa(modalBackdrop, LV_OPA_70, 0);
    lv_obj_set_style_border_width(modalBackdrop, 0, 0);
    lv_obj_set_style_pad_all(modalBackdrop, 0, 0);
    lv_obj_clear_flag(modalBackdrop, LV_OBJ_FLAG_SCROLLABLE);

    modalCard = lv_obj_create(modalBackdrop);
    lv_obj_set_size(modalCard, 460, 300);
    lv_obj_align(modalCard, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(modalCard, lv_color_make(16, 22, 38), 0);
    lv_obj_set_style_border_color(modalCard, CydTheme::getAccentColor(), 0);
    lv_obj_set_style_border_width(modalCard, 2, 0);
    lv_obj_set_style_radius(modalCard, 10, 0);
    lv_obj_set_style_pad_all(modalCard, 8, 0);
    lv_obj_clear_flag(modalCard, LV_OBJ_FLAG_SCROLLABLE);

    modalSsidLabel = lv_label_create(modalCard);
    lv_label_set_text(modalSsidLabel, "Nhập mật khẩu cho WiFi");
    CydTheme::applyTextFont(modalSsidLabel, CydTheme::getFont14(), CydTheme::getAccentGlowColor());
    lv_obj_align(modalSsidLabel, LV_ALIGN_TOP_LEFT, 6, 2);

    lv_obj_t* btnClose = lv_btn_create(modalCard);
    lv_obj_set_size(btnClose, 32, 28);
    lv_obj_align(btnClose, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_style_bg_color(btnClose, lv_color_make(100, 30, 40), 0);
    lv_obj_add_event_cb(btnClose, wifi_modal_cancel_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* lblClose = lv_label_create(btnClose);
    lv_label_set_text(lblClose, LV_SYMBOL_CLOSE);
    lv_obj_center(lblClose);

    taPassword = lv_textarea_create(modalCard);
    lv_obj_set_size(taPassword, 330, 36);
    lv_obj_align(taPassword, LV_ALIGN_TOP_LEFT, 6, 32);
    lv_textarea_set_password_mode(taPassword, true);
    lv_textarea_set_one_line(taPassword, true);
    lv_textarea_set_placeholder_text(taPassword, "Mật khẩu WiFi...");
    lv_obj_set_style_bg_color(taPassword, lv_color_make(10, 14, 24), 0);
    lv_obj_set_style_text_color(taPassword, CydTheme::getWhiteColor(), 0);
    lv_obj_set_style_text_font(taPassword, CydTheme::getFont12(), 0);
    lv_obj_set_style_border_color(taPassword, CydTheme::getCardBorderColor(), 0);

    lv_obj_t* btnConnect = lv_btn_create(modalCard);
    lv_obj_set_size(btnConnect, 100, 36);
    lv_obj_align(btnConnect, LV_ALIGN_TOP_RIGHT, -6, 32);
    lv_obj_set_style_bg_color(btnConnect, CydTheme::getSuccessColor(), 0);
    lv_obj_add_event_cb(btnConnect, wifi_connect_submit_cb, LV_EVENT_CLICKED, this);

    lv_obj_t* lblConn = lv_label_create(btnConnect);
    lv_label_set_text(lblConn, LV_SYMBOL_OK " Kết Nối");
    CydTheme::applyTextFont(lblConn, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_center(lblConn);

    keyboard = lv_keyboard_create(modalCard);
    lv_obj_set_size(keyboard, 444, 200);
    lv_obj_align(keyboard, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(keyboard, taPassword);
    lv_obj_set_style_bg_color(keyboard, lv_color_make(14, 18, 30), 0);

    lv_obj_add_flag(modalBackdrop, LV_OBJ_FLAG_HIDDEN);
}

void SettingsScreen::showWifiPasswordModal(const char* ssid) {
    if (!ssid || strlen(ssid) == 0) return;
    strncpy(selectedSsid, ssid, sizeof(selectedSsid) - 1);

    char titleBuf[128];
    snprintf(titleBuf, sizeof(titleBuf), "WiFi: %s", ssid);
    if (modalSsidLabel) lv_label_set_text(modalSsidLabel, titleBuf);

    if (taPassword) lv_textarea_set_text(taPassword, "");
    if (modalBackdrop) lv_obj_clear_flag(modalBackdrop, LV_OBJ_FLAG_HIDDEN);
}

void SettingsScreen::hideWifiPasswordModal() {
    if (modalBackdrop) {
        lv_obj_add_flag(modalBackdrop, LV_OBJ_FLAG_HIDDEN);
    }
}

// -------------------------------------------------------------
// NAVIGATION & DYNAMIC TELEMETRY UPDATES
// -------------------------------------------------------------
void SettingsScreen::setActiveMenuItem(int index) {
    if (index < 0 || index >= 6) index = 0;
    if (index == currentMenuIndex) return;

    currentMenuIndex = index;

    // Highlight sidebar item
    for (int i = 0; i < 6; i++) {
        if (i == index) {
            lv_obj_set_style_bg_color(sidebarItems[i], CydTheme::getAccentColor(), 0);
            lv_obj_set_style_bg_opa(sidebarItems[i], LV_OPA_COVER, 0);
            lv_obj_t* label = lv_obj_get_child(sidebarItems[i], 0);
            if (label) lv_obj_set_style_text_color(label, CydTheme::getWhiteColor(), 0);
        } else {
            lv_obj_set_style_bg_opa(sidebarItems[i], LV_OPA_TRANSP, 0);
            lv_obj_t* label = lv_obj_get_child(sidebarItems[i], 0);
            if (label) lv_obj_set_style_text_color(label, CydTheme::getTextSecondary(), 0);
        }
    }

    // Completely destroy old sub-pane to free memory, and build only the active one
    destroyCurrentPane();

    switch (index) {
        case 0: buildDevicePane(); break;
        case 1: buildWifiPane(); break;
        case 2: buildSdCardPane(); break;
        case 3: buildSyncPane(); break;
        case 4: buildDisplayPane(); break;
        case 5: buildSystemPane(); break;
    }
}

void SettingsScreen::updateDeviceInfo(const SettingsDeviceInfo& info) {
    if (info.deviceName && strlen(info.deviceName) > 0) {
        strncpy(cachedDevName, info.deviceName, sizeof(cachedDevName) - 1);
    }
    if (info.model && strlen(info.model) > 0) {
        strncpy(cachedDevModel, info.model, sizeof(cachedDevModel) - 1);
    }
    if (info.fwVersion && strlen(info.fwVersion) > 0) {
        strncpy(cachedDevFw, info.fwVersion, sizeof(cachedDevFw) - 1);
    }

    if (lblDevName) {
        char titleBuf[96];
        if (strstr(cachedDevName, cachedDevFw) != nullptr) {
            snprintf(titleBuf, sizeof(titleBuf), "%s", cachedDevName);
        } else {
            snprintf(titleBuf, sizeof(titleBuf), "%s (%s - %s)", cachedDevName, cachedDevFw, FIRMWARE_RELEASE_DATE);
        }
        lv_label_set_text(lblDevName, titleBuf);
    }
    if (lblDevModel) lv_label_set_text(lblDevModel, cachedDevModel);
    if (lblDevFw) lv_label_set_text(lblDevFw, cachedDevFw);
}

void SettingsScreen::updateTelemetry(uint32_t freeHeap, const char* uptimeStr, const char* ipStr, const char* macStr) {
    cachedFreeHeap = freeHeap;
    if (uptimeStr) strncpy(cachedUptime, uptimeStr, sizeof(cachedUptime) - 1);
    if (ipStr) strncpy(cachedIp, ipStr, sizeof(cachedIp) - 1);
    if (macStr) strncpy(cachedMac, macStr, sizeof(cachedMac) - 1);

    char buf[64];
    if (lblDevRam) {
        snprintf(buf, sizeof(buf), "RAM Trống: %u KB (%.1f%%)", freeHeap / 1024, SystemTelemetry::getHeapUsagePercent());
        lv_label_set_text(lblDevRam, buf);
    }
    if (lblDevUptime && uptimeStr) {
        snprintf(buf, sizeof(buf), "Thời gian chạy: %s", uptimeStr);
        lv_label_set_text(lblDevUptime, buf);
    }
    if (lblDevWifi) {
        String ssid = WifiService::getConnectedSSID();
        if (ssid.length() == 0 && ConfigManager::hasWifiCredentials()) {
            ssid = ConfigManager::getWifiSSID();
        }
        if (ssid.length() > 0) {
            snprintf(buf, sizeof(buf), "Mạng WiFi: %s", ssid.c_str());
        } else {
            snprintf(buf, sizeof(buf), "Mạng WiFi: Chưa kết nối");
        }
        lv_label_set_text(lblDevWifi, buf);
    }
    if (lblDevIp && ipStr) {
        snprintf(buf, sizeof(buf), "Địa chỉ IP: %s", ipStr);
        lv_label_set_text(lblDevIp, buf);
    }
    if (lblDevMac && macStr) {
        snprintf(buf, sizeof(buf), "Địa chỉ MAC: %s", macStr);
        lv_label_set_text(lblDevMac, buf);
    }
}

void SettingsScreen::updateWifiStatus(const char* stateStr, const char* ssid, const char* ip, int rssi) {
    if (stateStr) strncpy(cachedWifiState, stateStr, sizeof(cachedWifiState) - 1);
    if (ssid) strncpy(cachedWifiSsid, ssid, sizeof(cachedWifiSsid) - 1);
    if (ip) strncpy(cachedIp, ip, sizeof(cachedIp) - 1);
    cachedWifiRssi = rssi;

    char buf[64];
    if (lblDevWifi) {
        String s = (ssid && strlen(ssid) > 0) ? String(ssid) : ConfigManager::getWifiSSID();
        if (s.length() > 0) {
            snprintf(buf, sizeof(buf), "Mạng WiFi: %s", s.c_str());
        } else {
            snprintf(buf, sizeof(buf), "Mạng WiFi: Chưa kết nối");
        }
        lv_label_set_text(lblDevWifi, buf);
    }
    if (lblWifiCurrentState) {
        if (ssid && strlen(ssid) > 0) {
            snprintf(buf, sizeof(buf), "WiFi: %s (%ddBm)", ssid, rssi);
        } else {
            snprintf(buf, sizeof(buf), "WiFi: %s", stateStr);
        }
        lv_label_set_text(lblWifiCurrentState, buf);
    }

    if (lblWifiCurrentInfo) {
        snprintf(buf, sizeof(buf), "IP: %s", (ip && strlen(ip) > 0) ? ip : "0.0.0.0");
        lv_label_set_text(lblWifiCurrentInfo, buf);
    }

    if (lblBtnScan) {
        static bool lastScanning = false;
        bool isScanning = WifiService::isScanning();
        if (isScanning) {
            lv_label_set_text(lblBtnScan, LV_SYMBOL_REFRESH " Đang Quét...");
        } else {
            lv_label_set_text(lblBtnScan, LV_SYMBOL_REFRESH " Quét Mạng");
            if (lastScanning && currentMenuIndex == 1) {
                updateWifiScanList();
            }
        }
        lastScanning = isScanning;
    }
}

void SettingsScreen::updateWifiScanList() {
    if (!wifiListContainer) return;

    // Clear old list children
    lv_obj_clean(wifiListContainer);

    std::vector<WifiScanItem> list = WifiService::getScanResults();

    if (list.empty()) {
        lv_obj_t* msg = lv_label_create(wifiListContainer);
        if (WifiService::isScanning()) {
            lv_label_set_text(msg, "Đang quét mạng WiFi xung quanh...");
        } else {
            lv_label_set_text(msg, "Chưa có danh sách. Nhấn \"Quét Mạng\" để tìm.");
        }
        CydTheme::applyTextFont(msg, CydTheme::getFont12(), CydTheme::getTextMuted());
        lv_obj_set_style_pad_all(msg, 10, 0);
        return;
    }

    for (size_t i = 0; i < list.size(); ++i) {
        const auto& net = list[i];
        const char* icon = net.isEncrypted ? LV_SYMBOL_SETTINGS : LV_SYMBOL_WIFI;

        char itemText[96];
        snprintf(itemText, sizeof(itemText), "%s (%ddBm)", net.ssid.c_str(), net.rssi);

        lv_obj_t* btn = lv_list_add_btn(wifiListContainer, icon, itemText);
        lv_obj_set_style_bg_color(btn, lv_color_make(18, 26, 44), 0);
        lv_obj_set_style_text_color(btn, CydTheme::getWhiteColor(), 0);
        lv_obj_set_style_text_font(btn, CydTheme::getFont12(), 0);
        lv_obj_set_style_pad_ver(btn, 6, 0);
        lv_obj_set_style_radius(btn, 4, 0);

        lv_obj_add_event_cb(btn, wifi_item_click_cb, LV_EVENT_CLICKED, this);
    }
}

// -------------------------------------------------------------
// EVENT CALLBACKS
// -------------------------------------------------------------
void SettingsScreen::sidebar_click_event_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    lv_obj_t* target = lv_event_get_current_target(e);
    int idx = (int)(intptr_t)lv_obj_get_user_data(target);
    self->setActiveMenuItem(idx);
}

void SettingsScreen::wifi_scan_click_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    WifiService::startScan();
    if (self) self->updateWifiScanList();
}

void SettingsScreen::wifi_item_click_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    lv_obj_t* btn = lv_event_get_target(e);
    const char* txt = lv_list_get_btn_text(self->wifiListContainer, btn);
    if (!txt) return;

    char ssidBuf[64];
    strncpy(ssidBuf, txt, sizeof(ssidBuf) - 1);
    char* paren = strrchr(ssidBuf, '(');
    if (paren) {
        *paren = '\0';
        while (strlen(ssidBuf) > 0 && ssidBuf[strlen(ssidBuf) - 1] == ' ') {
            ssidBuf[strlen(ssidBuf) - 1] = '\0';
        }
    }

    self->showWifiPasswordModal(ssidBuf);
}

void SettingsScreen::wifi_connect_submit_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    const char* pass = lv_textarea_get_text(self->taPassword);
    
    LOG_I("UI", "User submit connect to SSID: %s", self->selectedSsid);
    WifiService::connect(String(self->selectedSsid), String(pass ? pass : ""));
    strncpy(self->cachedWifiSsid, self->selectedSsid, sizeof(self->cachedWifiSsid) - 1);
    if (self->lblDevWifi) {
        char buf[64];
        snprintf(buf, sizeof(buf), "Mạng WiFi: %s", self->selectedSsid);
        lv_label_set_text(self->lblDevWifi, buf);
    }
    self->hideWifiPasswordModal();
}

void SettingsScreen::wifi_modal_cancel_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    self->hideWifiPasswordModal();
}

void SettingsScreen::refresh_sd_click_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    LOG_I("UI", "Refresh SD Card status requested.");
    StorageService::init();
    if (self) {
        self->destroyCurrentPane();
        self->buildSdCardPane();

        StorageInfo sd = StorageService::getInfo();
        char toastMsg[96];
        if (sd.isMounted && sd.totalBytes > 0) {
            double totalGB = (double)sd.totalBytes / (1024.0 * 1024.0 * 1024.0);
            double freeGB = (double)sd.freeBytes / (1024.0 * 1024.0 * 1024.0);
            snprintf(toastMsg, sizeof(toastMsg), LV_SYMBOL_OK " Thẻ SD: %.2f GB (Còn trống: %.2f GB)", totalGB, freeGB);
        } else if (sd.isMounted) {
            snprintf(toastMsg, sizeof(toastMsg), LV_SYMBOL_WARNING " Thẻ SD: Đã nhận nhưng chưa đọc phân vùng");
        } else {
            snprintf(toastMsg, sizeof(toastMsg), LV_SYMBOL_CLOSE " Thẻ SD: Chưa nhận hoặc chưa cắm thẻ!");
        }
        DialogManager::showToast(toastMsg, 2500);
    }
}

void SettingsScreen::format_sd_click_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    if (self) {
        DialogManager::showConfirm(
            LV_SYMBOL_WARNING " XÁC NHẬN FORMAT THẺ NHỚ",
            "Cảnh báo: Toàn bộ dữ liệu trên thẻ MicroSD\nsẽ bị xóa vĩnh viễn và không thể khôi phục.\nBạn có chắc chắn muốn format không?",
            LV_SYMBOL_TRASH " Format Ngay",
            format_confirm_execute_cb,
            self,
            LV_SYMBOL_CLOSE " Hủy Bỏ",
            format_confirm_cancel_cb
        );
    }
}

void SettingsScreen::format_confirm_cancel_cb(lv_event_t* e) {
    DialogManager::dismissModal();
}

void SettingsScreen::format_confirm_execute_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    if (!self) return;

    // 1. Đóng popup xác nhận
    DialogManager::dismissModal();

    // 2. Khóa toàn bộ màn hình (chặn 100% cảm ứng cho đến khi xong)
    DialogManager::showLockOverlay(
        "ĐANG FORMAT THẺ NHỚ...",
        "Đang xóa sạch toàn bộ dữ liệu.\nVui lòng không rút thẻ hoặc tắt nguồn!",
        "Màn hình tạm khóa cho đến khi xong...",
        LV_SYMBOL_TRASH
    );

    // 3. Thực hiện xóa toàn bộ file trên thẻ nhớ
    LOG_I("UI", "Executing formatCard with screen locked...");
    bool ok = StorageService::formatCard();

    // 4. Mở khóa màn hình
    DialogManager::hideLockOverlay();

    // 5. Cập nhật lại giao diện tab Thẻ SD
    self->destroyCurrentPane();
    self->buildSdCardPane();

    // 6. Hiển thị thông báo Toast kết quả
    if (ok) {
        DialogManager::showToast(LV_SYMBOL_OK " Thẻ SD: Đã format sạch thành công!", 3000);
    } else {
        DialogManager::showToast(LV_SYMBOL_CLOSE " Thẻ SD: Format thất bại!", 3000);
    }
}

void SettingsScreen::city_changed_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    if (!self || !self->ddCity) return;

    int newIdx = lv_dropdown_get_selected(self->ddCity);
    int curIdx = ConfigManager::getCityIndex();
    if (newIdx == curIdx) return;

    if (newIdx >= 0 && newIdx < (int)VIETNAM_CITIES_COUNT) {
        self->pendingCityIdx = newIdx;
        char msg[128];
        snprintf(msg, sizeof(msg), "Bạn có muốn đổi sang %s\nvà đồng bộ lại thời tiết ngay bây giờ không?", VIETNAM_CITIES[newIdx].name);
        DialogManager::showConfirm(
            LV_SYMBOL_GPS " ĐỔI TỈNH / THÀNH PHỐ",
            msg,
            LV_SYMBOL_OK " Đồng Bộ Ngay",
            city_confirm_execute_cb,
            self,
            LV_SYMBOL_CLOSE " Hủy Bỏ",
            city_confirm_cancel_cb,
            CydTheme::getAccentColor()
        );
    }
}

void SettingsScreen::city_confirm_cancel_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    DialogManager::dismissModal();
    if (self && self->ddCity) {
        lv_dropdown_set_selected(self->ddCity, ConfigManager::getCityIndex());
    }
}

void SettingsScreen::city_confirm_execute_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    DialogManager::dismissModal();
    if (!self) return;

    int idx = self->pendingCityIdx;
    if (idx >= 0 && idx < (int)VIETNAM_CITIES_COUNT) {
        ConfigManager::setCityIndex(idx);
        const CityLocation& c = ConfigManager::getCurrentCity();
        WeatherService::setLocation(c.name, c.latitude, c.longitude);
        LOG_I("Config", "Selected city: %s (%.4f, %.4f)", c.name, c.latitude, c.longitude);

        self->startDataSync();
    }
}

void SettingsScreen::sync_interval_changed_cb(lv_event_t* e) {
    lv_obj_t* dd = lv_event_get_target(e);
    int idx = lv_dropdown_get_selected(dd);
    int minutes = 30;
    if (idx == 0) minutes = 15;
    else if (idx == 1) minutes = 30;
    else if (idx == 2) minutes = 60;
    else if (idx == 3) minutes = 120;
    ConfigManager::setSyncIntervalMinutes(minutes);
    LOG_I("Config", "Selected sync interval: %d minutes", minutes);
}

void SettingsScreen::sync_now_click_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    if (!self) return;
    LOG_I("UI", "Sync Now requested by user");
    self->startDataSync();
}

void SettingsScreen::startDataSync() {
    bool wifiOk = WifiService::isConnected();
    if (!wifiOk) {
        DialogManager::showToast("Không thể đồng bộ: Chưa kết nối Wi-Fi!", 2500);
        if (lblSyncStatus) {
            lv_label_set_text(lblSyncStatus, LV_SYMBOL_DOWNLOAD " Đồng Bộ Dữ Liệu Ngay");
        }
        if (btnSyncNow) {
            lv_obj_add_flag(btnSyncNow, LV_OBJ_FLAG_CLICKABLE);
        }
        return;
    }

    if (lblSyncStatus) {
        lv_label_set_text(lblSyncStatus, LV_SYMBOL_REFRESH " Đang đồng bộ dữ liệu...");
    }
    if (btnSyncNow) {
        lv_obj_clear_flag(btnSyncNow, LV_OBJ_FLAG_CLICKABLE);
    }

    TimeService::update(wifiOk);
    WeatherService::update(wifiOk, true);
    MarketService::update(wifiOk, true);

    syncCheckCount = 0;
    if (syncTimer) {
        lv_timer_del(syncTimer);
        syncTimer = nullptr;
    }
    syncTimer = lv_timer_create(sync_monitor_timer_cb, 250, this);
}

void SettingsScreen::sync_monitor_timer_cb(lv_timer_t* t) {
    SettingsScreen* self = (SettingsScreen*)t->user_data;
    if (!self) return;

    self->syncCheckCount++;
    // Đợi ít nhất 500ms (2 tick) để task FreeRTOS bắt đầu chạy
    if (self->syncCheckCount < 2) return;

    bool weatherBusy = WeatherService::isFetching();
    bool marketBusy = MarketService::isFetching();

    // Hoàn tất nếu cả 2 service đều không còn bận hoặc đã quá timeout (8s = 32 tick)
    if ((!weatherBusy && !marketBusy) || self->syncCheckCount >= 32) {
        if (self->syncTimer) {
            lv_timer_del(self->syncTimer);
            self->syncTimer = nullptr;
        }

        if (self->lblSyncStatus) {
            lv_label_set_text(self->lblSyncStatus, LV_SYMBOL_DOWNLOAD " Đồng Bộ Dữ Liệu Ngay");
        }
        if (self->btnSyncNow) {
            lv_obj_add_flag(self->btnSyncNow, LV_OBJ_FLAG_CLICKABLE);
        }

        if (!weatherBusy && !marketBusy) {
            DialogManager::showToast("Đồng bộ dữ liệu thành công!", 2500);
        } else {
            DialogManager::showToast("Đồng bộ hoàn tất!", 2500);
        }
    }
}

void SettingsScreen::brightness_changed_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    lv_obj_t* slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    BacklightManager::setBrightness((uint8_t)val);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", val);
    if (self->lblBrightnessVal) lv_label_set_text(self->lblBrightnessVal, buf);
}

void SettingsScreen::sleep_timeout_changed_cb(lv_event_t* e) {
    lv_obj_t* dd = lv_event_get_target(e);
    int idx = lv_dropdown_get_selected(dd);
    int sec = 60;
    if (idx == 0) sec = 30;
    else if (idx == 1) sec = 60;
    else if (idx == 2) sec = 180;
    else if (idx == 3) sec = 300;
    else if (idx == 4) sec = 0;
    ConfigManager::setSleepTimeoutSeconds(sec);
    LOG_I("Config", "Selected sleep timeout: %d seconds", sec);
}

void SettingsScreen::dev_mode_toggle_cb(lv_event_t* e) {
    lv_obj_t* sw = lv_event_get_target(e);
    bool enabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
    ConfigManager::setDevModeEnabled(enabled);
    LOG_I("Config", "Dev Mode toggled: %d", enabled);
}

void SettingsScreen::volume_changed_cb(lv_event_t* e) {
    SettingsScreen* self = (SettingsScreen*)lv_event_get_user_data(e);
    lv_obj_t* slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    ConfigManager::setDefaultVolume((uint8_t)val);

    char buf[16];
    snprintf(buf, sizeof(buf), "%d%%", val);
    if (self->lblVolumeVal) lv_label_set_text(self->lblVolumeVal, buf);
}

static void ota_monitor_timer_cb(lv_timer_t* t) {
    OtaState state = OtaService::getState();
    int pct = OtaService::getProgressPercent();
    size_t dl = OtaService::getDownloadedBytes();
    size_t tot = OtaService::getTotalBytes();

    if (state == OtaState::DOWNLOADING) {
        char msg[128];
        char hint[64];
        snprintf(msg, sizeof(msg), "Đang tải và ghi Flash (%d%%)...\nVui lòng tuyệt đối KHÔNG tắt nguồn!", pct);
        snprintf(hint, sizeof(hint), "Tiến trình: %d%% (%u / %u KB)", pct, (uint32_t)(dl / 1024), (uint32_t)(tot / 1024));
        DialogManager::showLockOverlay("ĐANG NẠP FIRMWARE OTA...", msg, hint, LV_SYMBOL_DOWNLOAD, lv_color_make(20, 140, 80));
    } else if (state == OtaState::SUCCESS) {
        DialogManager::showLockOverlay(
            "CẬP NHẬT THÀNH CÔNG!",
            "Firmware mới đã nạp hoàn tất.\nThiết bị sẽ tự khởi động lại sau giây lát...",
            "Đang khởi động lại...",
            LV_SYMBOL_OK,
            lv_color_make(40, 160, 60)
        );
        if (s_otaMonitorTimer) {
            lv_timer_del(s_otaMonitorTimer);
            s_otaMonitorTimer = nullptr;
        }
    } else if (state == OtaState::ERROR) {
        DialogManager::hideLockOverlay();
        DialogManager::showAlert(
            LV_SYMBOL_WARNING " LỖI CẬP NHẬT",
            OtaService::getErrorMessage(),
            "Đóng",
            nullptr,
            nullptr,
            lv_color_make(180, 40, 50)
        );
        if (s_otaMonitorTimer) {
            lv_timer_del(s_otaMonitorTimer);
            s_otaMonitorTimer = nullptr;
        }
    }
}

void SettingsScreen::ota_check_click_cb(lv_event_t* e) {
    if (WiFi.status() != WL_CONNECTED) {
        DialogManager::showAlert(
            LV_SYMBOL_WARNING " CẬP NHẬT OTA",
            "Vui lòng kết nối mạng WiFi trước khi\nkiểm tra bản cập nhật mới!",
            "Đã Hiểu"
        );
        return;
    }

    DialogManager::showToast("Đang kiểm tra bản cập nhật từ máy chủ...", 3000);

    OtaInfo info;
    bool ok = OtaService::checkUpdate(info);

    if (!ok) {
        DialogManager::showAlert(
            LV_SYMBOL_WARNING " LỖI KIỂM TRA",
            OtaService::getErrorMessage(),
            "Đóng",
            nullptr,
            nullptr,
            lv_color_make(180, 40, 50)
        );
        return;
    }

    if (!info.hasUpdate) {
        char buf[128];
        snprintf(buf, sizeof(buf), "Thiết bị đang sử dụng phiên bản mới nhất (%s).\nKhông có bản cập nhật nào cần nạp.", info.version.c_str());
        DialogManager::showAlert(
            LV_SYMBOL_OK " PHIÊN BẢN MỚI NHẤT",
            buf,
            "Đóng",
            nullptr,
            nullptr,
            lv_color_make(40, 150, 60)
        );
        return;
    }

    s_pendingOtaUrl = info.firmwareUrl;

    String body = "Đã có bản cập nhật: " + info.version;
    if (info.releaseDate.length() > 0) {
        body += " (" + info.releaseDate + ")";
    }
    body += "\n";
    if (info.changelog.length() > 0) {
        body += "Nội dung: " + info.changelog + "\n";
    }
    body += "\nBạn có muốn tải và nạp ngay bây giờ?";


    DialogManager::showConfirm(
        LV_SYMBOL_DOWNLOAD " NÂNG CẤP FIRMWARE",
        body.c_str(),
        LV_SYMBOL_DOWNLOAD " Nâng Cấp",
        ota_confirm_click_cb,
        nullptr,
        LV_SYMBOL_CLOSE " Để Sau",
        nullptr,
        lv_color_make(20, 140, 80)
    );
}

void SettingsScreen::ota_confirm_click_cb(lv_event_t* e) {
    DialogManager::dismissModal();
    DialogManager::showLockOverlay(
        "ĐANG NẠP FIRMWARE OTA...",
        "Đang chuẩn bị kết nối và nạp Flash...\nVui lòng tuyệt đối KHÔNG tắt nguồn!",
        "Tiến trình: 0%",
        LV_SYMBOL_DOWNLOAD,
        lv_color_make(20, 140, 80)
    );

    LOG_I("OTA", "User confirmed OTA update from: %s", s_pendingOtaUrl.c_str());
    OtaService::startUpdate(s_pendingOtaUrl, nullptr, nullptr);

    if (s_otaMonitorTimer) {
        lv_timer_del(s_otaMonitorTimer);
    }
    s_otaMonitorTimer = lv_timer_create(ota_monitor_timer_cb, 150, nullptr);
}

void SettingsScreen::restart_click_cb(lv_event_t* e) {

    DialogManager::showLockOverlay(
        "ĐANG KHỞI ĐỘNG LẠI...",
        "Hệ thống đang chuẩn bị khởi động lại...\nVui lòng chờ giây lát!",
        nullptr,
        LV_SYMBOL_REFRESH,
        lv_color_make(30, 80, 150)
    );
    LOG_I("System", "Restarting ESP32 by user request...");
    ConfigManager::flush();
    delay(500);
    ESP.restart();

}

void SettingsScreen::factory_reset_click_cb(lv_event_t* e) {
    DialogManager::showConfirm(
        LV_SYMBOL_WARNING " KHÔI PHỤC CÀI ĐẶT GỐC",
        "Cảnh báo: Toàn bộ thông tin WiFi và cấu hình cá nhân\nsẽ bị xóa sạch về trạng thái ban đầu của nhà sản xuất.\nThiết bị sẽ tự động khởi động lại.\nBạn có chắc chắn muốn tiếp tục?",
        LV_SYMBOL_TRASH " Khôi Phục",
        factory_reset_confirm_cb,
        nullptr,
        LV_SYMBOL_CLOSE " Hủy Bỏ",
        nullptr,
        lv_color_make(180, 40, 50)
    );
}

void SettingsScreen::factory_reset_confirm_cb(lv_event_t* e) {
    DialogManager::dismissModal();
    DialogManager::showLockOverlay(
        "ĐANG KHÔI PHỤC GỐC...",
        "Đang xóa sạch dữ liệu cấu hình NVS Flash.\nThiết bị sẽ tự khởi động lại sau giây lát!",
        "Vui lòng không tắt nguồn thiết bị...",
        LV_SYMBOL_TRASH,
        lv_color_make(180, 40, 50)
    );
    LOG_I("System", "Factory resetting config by user request...");
    ConfigManager::resetToDefaults();
    delay(800);
    ESP.restart();
}