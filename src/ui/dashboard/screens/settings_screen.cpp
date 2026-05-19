#include "settings_screen.h"
#include "../cyd_theme.h"
#include <stdio.h>

SettingsScreen::SettingsScreen(lv_obj_t* parent) {
    // 1. Create root screen container
    rootContainer = lv_obj_create(parent);
    lv_obj_set_size(rootContainer, 480, 282);
    lv_obj_set_style_bg_opa(rootContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rootContainer, 0, 0);
    lv_obj_set_style_pad_all(rootContainer, 0, 0);
    lv_obj_clear_flag(rootContainer, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Build left sidebar and right content pane
    createSidebar(rootContainer);
    createContentPane(rootContainer);

    // Default select first item
    setActiveMenuItem(0);
}

void SettingsScreen::createSidebar(lv_obj_t* parent) {
    sidebarMenu = lv_obj_create(parent);
    lv_obj_set_size(sidebarMenu, 86, 270);
    lv_obj_align(sidebarMenu, LV_ALIGN_TOP_LEFT, 6, 6);
    CydTheme::applyCardStyle(sidebarMenu);
    
    // Vertical row flex list
    lv_obj_set_layout(sidebarMenu, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(sidebarMenu, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(sidebarMenu, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(sidebarMenu, 8, 0);
    lv_obj_set_style_pad_all(sidebarMenu, 4, 0);

    const char* menuNames[5] = {"Thiết Bị", "Mạng Wifi", "Hệ Thống", "Pin Nguồn", "Ngôn Ngữ"};

    for (int i = 0; i < 5; i++) {
        sidebarItems[i] = lv_obj_create(sidebarMenu);
        lv_obj_set_size(sidebarItems[i], 76, 34);
        lv_obj_set_style_radius(sidebarItems[i], 6, 0);
        lv_obj_set_style_bg_color(sidebarItems[i], CydTheme::getCardBorderColor(), 0);
        lv_obj_set_style_bg_opa(sidebarItems[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(sidebarItems[i], 0, 0);
        lv_obj_set_style_pad_all(sidebarItems[i], 0, 0);
        lv_obj_clear_flag(sidebarItems[i], LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* lblItem = lv_label_create(sidebarItems[i]);
        lv_label_set_text(lblItem, menuNames[i]);
        CydTheme::applyTextFont(lblItem, CydTheme::getFont12(), CydTheme::getTextSecondary());
        lv_obj_center(lblItem);
    }
}

void SettingsScreen::createContentPane(lv_obj_t* parent) {
    lv_obj_t* rightPane = lv_obj_create(parent);
    lv_obj_set_size(rightPane, 376, 270);
    lv_obj_align(rightPane, LV_ALIGN_TOP_RIGHT, -6, 6);
    CydTheme::applyCardStyle(rightPane);

    // 1. Right upper simulated night sky photo card (Device details frame)
    imgNightSky = lv_obj_create(rightPane);
    lv_obj_set_size(imgNightSky, 356, 102);
    lv_obj_align(imgNightSky, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(imgNightSky, lv_color_make(10, 16, 32), 0); // deep night sky color
    lv_obj_set_style_bg_opa(imgNightSky, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(imgNightSky, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_border_width(imgNightSky, 1, 0);
    lv_obj_set_style_radius(imgNightSky, 8, 0);
    lv_obj_set_style_pad_all(imgNightSky, 6, 0);
    lv_obj_clear_flag(imgNightSky, LV_OBJ_FLAG_SCROLLABLE);

    lblDevName = lv_label_create(imgNightSky);
    lv_label_set_text(lblDevName, "CYD 3.5 Smart Display");
    CydTheme::applyTextFont(lblDevName, CydTheme::getFont14(), CydTheme::getWhiteColor());
    lv_obj_align(lblDevName, LV_ALIGN_TOP_LEFT, 6, 4);

    lblDevModel = lv_label_create(imgNightSky);
    lv_label_set_text(lblDevModel, "Mẫu: ESP32-WROOM-32");
    CydTheme::applyTextFont(lblDevModel, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDevModel, LV_ALIGN_TOP_LEFT, 6, 24);

    lblDevFw = lv_label_create(imgNightSky);
    lv_label_set_text(lblDevFw, "Phiên bản FW: v2.4.1");
    CydTheme::applyTextFont(lblDevFw, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDevFw, LV_ALIGN_TOP_LEFT, 6, 44);

    lblDevBuild = lv_label_create(imgNightSky);
    lv_label_set_text(lblDevBuild, "Bản dựng: 19/05/2026");
    CydTheme::applyTextFont(lblDevBuild, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDevBuild, LV_ALIGN_TOP_LEFT, 6, 64);

    lblDevOs = lv_label_create(imgNightSky);
    lv_label_set_text(lblDevOs, "Hệ điều hành: RTOS v4.4");
    CydTheme::applyTextFont(lblDevOs, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDevOs, LV_ALIGN_TOP_RIGHT, -6, 24);

    lblDevSerial = lv_label_create(imgNightSky);
    lv_label_set_text(lblDevSerial, "Số Sê-ri: ESP32-35-A8F2");
    CydTheme::applyTextFont(lblDevSerial, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDevSerial, LV_ALIGN_TOP_RIGHT, -6, 44);

    // 2. Right lower 2x2 System Stats grid
    lv_obj_t* statsGrid = lv_obj_create(rightPane);
    lv_obj_set_size(statsGrid, 356, 162);
    lv_obj_align(statsGrid, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(statsGrid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(statsGrid, 0, 0);
    lv_obj_set_style_pad_all(statsGrid, 0, 0);
    lv_obj_clear_flag(statsGrid, LV_OBJ_FLAG_SCROLLABLE);

    // 2x2 blocks: Memory, Wifi, Battery, Language
    int boxW = 172;
    int boxH = 72;

    // Block 0: Memory stats (Top Left)
    lv_obj_t* blockMem = lv_obj_create(statsGrid);
    lv_obj_set_size(blockMem, boxW, boxH);
    lv_obj_set_pos(blockMem, 0, 6);
    CydTheme::applyCardStyle(blockMem);
    lv_obj_set_style_bg_color(blockMem, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_bg_opa(blockMem, LV_OPA_20, 0);

    lv_obj_t* lblMemTitle = lv_label_create(blockMem);
    lv_label_set_text(lblMemTitle, "Lưu Trữ Hệ Thống");
    CydTheme::applyTextFont(lblMemTitle, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblMemTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    barMemory = lv_bar_create(blockMem);
    lv_obj_set_size(barMemory, 148, 6);
    lv_obj_align(barMemory, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_bg_color(barMemory, CydTheme::getCardBorderColor(), LV_PART_MAIN);
    lv_obj_set_style_bg_color(barMemory, CydTheme::getAccentGlowColor(), LV_PART_INDICATOR);

    lblMemoryDetails = lv_label_create(blockMem);
    lv_label_set_text(lblMemoryDetails, "8.6 GB / 16.0 GB");
    CydTheme::applyTextFont(lblMemoryDetails, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblMemoryDetails, LV_ALIGN_BOTTOM_LEFT, 0, -2);

    // Block 1: WiFi (Top Right)
    lv_obj_t* blockWifi = lv_obj_create(statsGrid);
    lv_obj_set_size(blockWifi, boxW, boxH);
    lv_obj_set_pos(blockWifi, 180, 6);
    CydTheme::applyCardStyle(blockWifi);
    lv_obj_set_style_bg_color(blockWifi, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_bg_opa(blockWifi, LV_OPA_20, 0);

    lv_obj_t* lblWifiTitle = lv_label_create(blockWifi);
    lv_label_set_text(lblWifiTitle, "Kết Nối Mạng");
    CydTheme::applyTextFont(lblWifiTitle, CydTheme::getFont12(), CydTheme::getBlueColor());
    lv_obj_align(lblWifiTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    lblWiFiSsid = lv_label_create(blockWifi);
    lv_label_set_text(lblWiFiSsid, "SSID: Wifi_Home");
    CydTheme::applyTextFont(lblWiFiSsid, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblWiFiSsid, LV_ALIGN_TOP_LEFT, 0, 22);
    lv_label_set_long_mode(lblWiFiSsid, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lblWiFiSsid, 148);

    lblWiFiIp = lv_label_create(blockWifi);
    lv_label_set_text(lblWiFiIp, "IP: 192.168.1.15");
    CydTheme::applyTextFont(lblWiFiIp, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblWiFiIp, LV_ALIGN_BOTTOM_LEFT, 0, -2);

    // Block 2: Battery (Bottom Left)
    lv_obj_t* blockBatt = lv_obj_create(statsGrid);
    lv_obj_set_size(blockBatt, boxW, boxH);
    lv_obj_set_pos(blockBatt, 0, 84);
    CydTheme::applyCardStyle(blockBatt);
    lv_obj_set_style_bg_color(blockBatt, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_bg_opa(blockBatt, LV_OPA_20, 0);

    lv_obj_t* lblBattTitle = lv_label_create(blockBatt);
    lv_label_set_text(lblBattTitle, "Nguồn Điện Pin");
    CydTheme::applyTextFont(lblBattTitle, CydTheme::getFont12(), CydTheme::getSuccessColor());
    lv_obj_align(lblBattTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    lblBatteryDetails = lv_label_create(blockBatt);
    lv_label_set_text(lblBatteryDetails, "95% (Đang sạc)");
    CydTheme::applyTextFont(lblBatteryDetails, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblBatteryDetails, LV_ALIGN_TOP_LEFT, 0, 22);

    lblBatteryRemaining = lv_label_create(blockBatt);
    lv_label_set_text(lblBatteryRemaining, "Còn: 1h 15m");
    CydTheme::applyTextFont(lblBatteryRemaining, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblBatteryRemaining, LV_ALIGN_BOTTOM_LEFT, 0, -2);

    // Block 3: Language (Bottom Right)
    lv_obj_t* blockLang = lv_obj_create(statsGrid);
    lv_obj_set_size(blockLang, boxW, boxH);
    lv_obj_set_pos(blockLang, 180, 84);
    CydTheme::applyCardStyle(blockLang);
    lv_obj_set_style_bg_color(blockLang, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_bg_opa(blockLang, LV_OPA_20, 0);

    lv_obj_t* lblLangTitle = lv_label_create(blockLang);
    lv_label_set_text(lblLangTitle, "Ngôn Ngữ");
    CydTheme::applyTextFont(lblLangTitle, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblLangTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    lblLanguageVal = lv_label_create(blockLang);
    lv_label_set_text(lblLanguageVal, "Tiếng Việt");
    CydTheme::applyTextFont(lblLanguageVal, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblLanguageVal, LV_ALIGN_TOP_LEFT, 0, 22);
}

// --- Dynamic Setter Updates ---

void SettingsScreen::updateDeviceInfo(const SettingsDeviceInfo& info) {
    if (lblDevName) lv_label_set_text(lblDevName, info.deviceName);
    if (lblDevModel) lv_label_set_text(lblDevModel, info.model);
    if (lblDevFw) lv_label_set_text(lblDevFw, info.fwVersion);
    if (lblDevBuild) lv_label_set_text(lblDevBuild, info.buildDate);
    if (lblDevOs) lv_label_set_text(lblDevOs, info.osName);
    if (lblDevSerial) lv_label_set_text(lblDevSerial, info.serialNumber);
}

void SettingsScreen::updateMemoryUsage(float usedGB, float totalGB) {
    char buf[32];
    
    // Update progress percentage
    if (totalGB > 0) {
        int pct = (int)((usedGB * 100.0f) / totalGB);
        lv_bar_set_value(barMemory, pct, LV_ANIM_OFF);
    }

    // Format text details
    sprintf(buf, "%.1f GB / %.1f GB", usedGB, totalGB);
    if (lblMemoryDetails) lv_label_set_text(lblMemoryDetails, buf);
}

void SettingsScreen::updateBatteryStatus(int percent, const char* durationStr) {
    char detailBuf[32];
    
    if (strstr(durationStr, "sạc") || strstr(durationStr, "Sạc") || strstr(durationStr, "charging") || strstr(durationStr, "Charging")) {
        sprintf(detailBuf, "%d%% (Đang sạc)", percent);
    } else {
        sprintf(detailBuf, "%d%% (Sử dụng pin)", percent);
    }
    
    if (lblBatteryDetails) lv_label_set_text(lblBatteryDetails, detailBuf);
    
    char remBuf[32];
    sprintf(remBuf, "Còn: %s", durationStr);
    if (lblBatteryRemaining) lv_label_set_text(lblBatteryRemaining, remBuf);
}

void SettingsScreen::updateWiFiConnection(const char* ssid, const char* ip) {
    char ssidBuf[48];
    sprintf(ssidBuf, "SSID: %s", ssid);
    if (lblWiFiSsid) lv_label_set_text(lblWiFiSsid, ssidBuf);
    
    char ipBuf[32];
    sprintf(ipBuf, "IP: %s", ip);
    if (lblWiFiIp) lv_label_set_text(lblWiFiIp, ipBuf);
}

void SettingsScreen::updateLanguage(const char* langStr) {
    if (lblLanguageVal) lv_label_set_text(lblLanguageVal, langStr);
}

void SettingsScreen::setActiveMenuItem(int index) {
    for (int i = 0; i < 5; i++) {
        if (i == index) {
            // Highlight active side item
            lv_obj_set_style_bg_color(sidebarItems[i], CydTheme::getAccentColor(), 0);
            lv_obj_set_style_bg_opa(sidebarItems[i], LV_OPA_COVER, 0);
            
            lv_obj_t* label = lv_obj_get_child(sidebarItems[i], 0);
            if (label) lv_obj_set_style_text_color(label, CydTheme::getWhiteColor(), 0);
        } else {
            // Idle side item
            lv_obj_set_style_bg_opa(sidebarItems[i], LV_OPA_TRANSP, 0);
            
            lv_obj_t* label = lv_obj_get_child(sidebarItems[i], 0);
            if (label) lv_obj_set_style_text_color(label, CydTheme::getTextSecondary(), 0);
        }
    }
}