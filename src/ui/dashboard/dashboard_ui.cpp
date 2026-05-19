#include "dashboard_ui.h"
#include "cyd_theme.h"
#include <stdio.h>
#include <string.h>

DashboardUI::DashboardUI() : 
    activeTabIndex(-1), 
    homeScreen(nullptr), 
    calendarScreen(nullptr), 
    playerScreen(nullptr), 
    settingsScreen(nullptr) 
{
    // 1. Create Master Container (Whole screen: 480x320)
    masterContainer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(masterContainer, 480, 320);
    lv_obj_set_style_bg_color(masterContainer, CydTheme::getBgColor(), 0);
    lv_obj_set_style_bg_opa(masterContainer, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(masterContainer, 0, 0);
    lv_obj_set_style_border_width(masterContainer, 0, 0);
    lv_obj_set_style_radius(masterContainer, 0, 0);
    lv_obj_clear_flag(masterContainer, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Active View Area (Top part: 480x282)
    activeViewArea = lv_obj_create(masterContainer);
    lv_obj_set_size(activeViewArea, 480, 282);
    lv_obj_align(activeViewArea, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(activeViewArea, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(activeViewArea, 0, 0);
    lv_obj_set_style_pad_all(activeViewArea, 0, 0);
    lv_obj_clear_flag(activeViewArea, LV_OBJ_FLAG_SCROLLABLE);

    // 3. Bottom Nav Bar Container (Bottom part: 480x38)
    bottomNavBar = lv_obj_create(masterContainer);
    lv_obj_set_size(bottomNavBar, 480, 38);
    lv_obj_align(bottomNavBar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(bottomNavBar, CydTheme::getCardColor(), 0);
    lv_obj_set_style_bg_opa(bottomNavBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(bottomNavBar, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_border_width(bottomNavBar, 1, 0);
    lv_obj_set_style_border_side(bottomNavBar, LV_BORDER_SIDE_TOP, 0);
    lv_obj_set_style_radius(bottomNavBar, 0, 0);
    lv_obj_set_style_pad_all(bottomNavBar, 0, 0);
    lv_obj_clear_flag(bottomNavBar, LV_OBJ_FLAG_SCROLLABLE);

    // 4. Build the Tabs Navigation
    initTabs();

    // 5. Make Home screen active by default
    setTabActive(0);
}

DashboardUI::~DashboardUI() {
    if (homeScreen) delete homeScreen;
    if (calendarScreen) delete calendarScreen;
    if (playerScreen) delete playerScreen;
    if (settingsScreen) delete settingsScreen;
}

void DashboardUI::initTabs() {
    const char* tabIcons[4] = {LV_SYMBOL_HOME, LV_SYMBOL_LIST, LV_SYMBOL_AUDIO, LV_SYMBOL_SETTINGS};
    
    // Create tab items using flex layout inside bottom nav bar
    lv_obj_set_layout(bottomNavBar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottomNavBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottomNavBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < 4; i++) {
        // Tab button container
        tabContainers[i] = lv_obj_create(bottomNavBar);
        lv_obj_set_size(tabContainers[i], 100, 36);
        lv_obj_set_style_bg_opa(tabContainers[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(tabContainers[i], 0, 0);
        lv_obj_set_style_pad_all(tabContainers[i], 0, 0);
        lv_obj_set_user_data(tabContainers[i], (void*)(intptr_t)i);
        lv_obj_add_event_cb(tabContainers[i], tab_click_event_cb, LV_EVENT_CLICKED, this);
        lv_obj_clear_flag(tabContainers[i], LV_OBJ_FLAG_SCROLLABLE);

        // Indicator bar (Active glow line at top of tab button)
        tabIndicators[i] = lv_obj_create(tabContainers[i]);
        lv_obj_set_size(tabIndicators[i], 60, 2);
        lv_obj_align(tabIndicators[i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(tabIndicators[i], CydTheme::getAccentGlowColor(), 0);
        lv_obj_set_style_bg_opa(tabIndicators[i], LV_OPA_TRANSP, 0); // hidden by default
        lv_obj_set_style_border_width(tabIndicators[i], 0, 0);
        lv_obj_set_style_radius(tabIndicators[i], 2, 0);

        // Icon label (Pure icon style)
        this->tabIcons[i] = lv_label_create(tabContainers[i]);
        lv_label_set_text(this->tabIcons[i], tabIcons[i]);
        CydTheme::applyTextFont(this->tabIcons[i], CydTheme::getFont20(), CydTheme::getTextSecondary());
        lv_obj_align(this->tabIcons[i], LV_ALIGN_CENTER, 0, 1);
        lv_obj_clear_flag(this->tabIcons[i], LV_OBJ_FLAG_CLICKABLE);

        // Text label is no longer used for pure icon navigation
        tabLabels[i] = nullptr;
    }
}

void DashboardUI::setTabActive(int index) {
    if (index == activeTabIndex) return;

    // Reset previous active tab styling & DESTROY the old screen to free up heap RAM!
    if (activeTabIndex >= 0) {
        lv_obj_set_style_bg_opa(tabIndicators[activeTabIndex], LV_OPA_TRANSP, 0);
        lv_obj_set_style_text_color(tabIcons[activeTabIndex], CydTheme::getTextSecondary(), 0);
        if (tabLabels[activeTabIndex]) {
            lv_obj_set_style_text_color(tabLabels[activeTabIndex], CydTheme::getTextSecondary(), 0);
        }
        
        switch (activeTabIndex) {
            case 0:
                if (homeScreen) {
                    delete homeScreen;
                    homeScreen = nullptr;
                }
                break;
            case 1:
                if (calendarScreen) {
                    delete calendarScreen;
                    calendarScreen = nullptr;
                }
                break;
            case 2:
                if (playerScreen) {
                    delete playerScreen;
                    playerScreen = nullptr;
                }
                break;
            case 3:
                if (settingsScreen) {
                    delete settingsScreen;
                    settingsScreen = nullptr;
                }
                break;
        }
    }

    // Set new active tab styling
    activeTabIndex = index;
    lv_obj_set_style_bg_opa(tabIndicators[activeTabIndex], LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(tabIcons[activeTabIndex], CydTheme::getAccentGlowColor(), 0);
    if (tabLabels[activeTabIndex]) {
        lv_obj_set_style_text_color(tabLabels[activeTabIndex], CydTheme::getAccentGlowColor(), 0);
    }

    // CREATE and POPULATE new active screen!
    lv_obj_t* activeRoot = nullptr;
    switch (activeTabIndex) {
        case 0:
            homeScreen = new HomeScreen(activeViewArea);
            activeRoot = homeScreen->getRoot();
            // Restore from Cache
            homeScreen->updateTime(homeCache.timeStr, homeCache.secStr, homeCache.dateStr, homeCache.isAm);
            homeScreen->updateLunarCalendar(homeCache.lunarDayStr, homeCache.lunarInfoStr);
            homeScreen->updateCalendarRibbon(homeCache.activeDayIndex, homeCache.dayNumbers);
            homeScreen->updateWeather(homeCache.temp, homeCache.condition, homeCache.feelsLike, homeCache.humidity, homeCache.windSpeed, homeCache.uvIndex);
            homeScreen->updateGoldPrices(homeCache.goldBuy, homeCache.goldSell, homeCache.goldBuyDelta, homeCache.goldSellDelta);
            homeScreen->updateFuelPrices(homeCache.fuelRon95, homeCache.fuelRon92, homeCache.fuelDiesel, homeCache.fuelKerosene, homeCache.fuelRon95Delta, homeCache.fuelRon92Delta, homeCache.fuelDieselDelta, homeCache.fuelKeroseneDelta);
            break;
            
        case 1:
            calendarScreen = new CalendarScreen(activeViewArea);
            activeRoot = calendarScreen->getRoot();
            // Restore from Cache
            calendarScreen->updateMonthYearHeader(calCache.monthYearStr);
            calendarScreen->updateCalendarDays(calCache.startDayOfWeek, calCache.daysInMonth, calCache.activeDay, calCache.dotColors);
            calendarScreen->updateSyncStatus(calCache.googleConnected, calCache.appleConnected);
            calendarScreen->clearEvents();
            for (int i = 0; i < calCache.eventCount; i++) {
                calendarScreen->addEvent(calCache.events[i]);
            }
            break;
            
        case 2:
            playerScreen = new PlayerScreen(activeViewArea);
            activeRoot = playerScreen->getRoot();
            // Restore from Cache
            playerScreen->updateTrackInfo(playerCache.title, playerCache.artist, playerCache.album, playerCache.qualityStr);
            playerScreen->updatePlaybackProgress(playerCache.currentSec, playerCache.totalSec);
            playerScreen->setPlayState(playerCache.isPlaying);
            playerScreen->updatePlaybackMode(playerCache.shuffleActive, playerCache.repeatActive);
            playerScreen->updateVolume(playerCache.volume);
            playerScreen->updateEQ(playerCache.eqMode);
            playerScreen->clearPlaylist();
            for (int i = 0; i < playerCache.playlistCount; i++) {
                playerScreen->addPlaylistItem(playerCache.playlist[i]);
            }
            break;
            
        case 3:
            settingsScreen = new SettingsScreen(activeViewArea);
            activeRoot = settingsScreen->getRoot();
            // Restore from Cache
            settingsScreen->updateDeviceInfo(settingsCache.info);
            settingsScreen->updateMemoryUsage(settingsCache.usedGB, settingsCache.totalGB);
            settingsScreen->updateBatteryStatus(settingsCache.batPercent, settingsCache.batDuration);
            settingsScreen->updateWiFiConnection(settingsCache.wifiSsid, settingsCache.wifiIp);
            settingsScreen->updateLanguage(settingsCache.language);
            settingsScreen->setActiveMenuItem(settingsCache.activeMenuItem);
            break;
    }
    
    if (activeRoot) {
        lv_obj_clear_flag(activeRoot, LV_OBJ_FLAG_HIDDEN);
        // Play subtle opacity animation on transition
        lv_obj_set_style_opa(activeRoot, LV_OPA_TRANSP, 0);
        lv_obj_fade_in(activeRoot, 150, 0);
    }
}

void DashboardUI::tab_click_event_cb(lv_event_t* e) {
    lv_obj_t* tabBtn = lv_event_get_target(e);
    DashboardUI* uiInstance = (DashboardUI*)lv_event_get_user_data(e);
    int index = (int)(intptr_t)lv_obj_get_user_data(tabBtn);
    uiInstance->setTabActive(index);
}

void DashboardUI::tick() {
    // Only run player visualizer ticks if the player tab is active to save MCU cycles
    if (activeTabIndex == 2 && playerScreen) {
        playerScreen->tickSpectrumAnimation();
    }
}

// --- Dynamic Cache-Populating Setters ---

void DashboardUI::updateTime(const char* timeStr, const char* secondsStr, const char* dateStr, bool isAm) {
    strncpy(homeCache.timeStr, timeStr, sizeof(homeCache.timeStr) - 1);
    strncpy(homeCache.secStr, secondsStr, sizeof(homeCache.secStr) - 1);
    strncpy(homeCache.dateStr, dateStr, sizeof(homeCache.dateStr) - 1);
    homeCache.isAm = isAm;
    if (homeScreen) homeScreen->updateTime(timeStr, secondsStr, dateStr, isAm);
}

void DashboardUI::updateLunarCalendar(const char* lunarDayStr, const char* lunarInfoStr) {
    strncpy(homeCache.lunarDayStr, lunarDayStr, sizeof(homeCache.lunarDayStr) - 1);
    strncpy(homeCache.lunarInfoStr, lunarInfoStr, sizeof(homeCache.lunarInfoStr) - 1);
    if (homeScreen) homeScreen->updateLunarCalendar(lunarDayStr, lunarInfoStr);
}

void DashboardUI::updateCalendarRibbon(int activeDayIndex, const int* dayNumbers) {
    homeCache.activeDayIndex = activeDayIndex;
    for (int i = 0; i < 7; i++) {
        homeCache.dayNumbers[i] = dayNumbers[i];
    }
    if (homeScreen) homeScreen->updateCalendarRibbon(activeDayIndex, dayNumbers);
}

void DashboardUI::updateWeather(int temp, const char* condition, int feelsLike, int humidity, int windSpeed, int uvIndex) {
    homeCache.temp = temp;
    strncpy(homeCache.condition, condition, sizeof(homeCache.condition) - 1);
    homeCache.feelsLike = feelsLike;
    homeCache.humidity = humidity;
    homeCache.windSpeed = windSpeed;
    homeCache.uvIndex = uvIndex;
    if (homeScreen) homeScreen->updateWeather(temp, condition, feelsLike, humidity, windSpeed, uvIndex);
}

void DashboardUI::updateGoldPrices(int buySJC, int sellSJC, int buyDelta, int sellDelta) {
    homeCache.goldBuy = buySJC;
    homeCache.goldSell = sellSJC;
    homeCache.goldBuyDelta = buyDelta;
    homeCache.goldSellDelta = sellDelta;
    if (homeScreen) homeScreen->updateGoldPrices(buySJC, sellSJC, buyDelta, sellDelta);
}

void DashboardUI::updateFuelPrices(int ron95, int ron92, int diesel, int kerosene, int ron95Delta, int ron92Delta, int dieselDelta, int keroseneDelta) {
    homeCache.fuelRon95 = ron95;
    homeCache.fuelRon92 = ron92;
    homeCache.fuelDiesel = diesel;
    homeCache.fuelKerosene = kerosene;
    homeCache.fuelRon95Delta = ron95Delta;
    homeCache.fuelRon92Delta = ron92Delta;
    homeCache.fuelDieselDelta = dieselDelta;
    homeCache.fuelKeroseneDelta = keroseneDelta;
    if (homeScreen) homeScreen->updateFuelPrices(ron95, ron92, diesel, kerosene, ron95Delta, ron92Delta, dieselDelta, keroseneDelta);
}

void DashboardUI::updateMonthYearHeader(const char* monthYearStr) {
    strncpy(calCache.monthYearStr, monthYearStr, sizeof(calCache.monthYearStr) - 1);
    if (calendarScreen) calendarScreen->updateMonthYearHeader(monthYearStr);
}

void DashboardUI::updateCalendarDays(int startDayOfWeek, int daysInMonth, int activeDay, const uint32_t* dotColorsMatrix) {
    calCache.startDayOfWeek = startDayOfWeek;
    calCache.daysInMonth = daysInMonth;
    calCache.activeDay = activeDay;
    for (int i = 0; i < 42; i++) {
        calCache.dotColors[i] = dotColorsMatrix[i];
    }
    if (calendarScreen) calendarScreen->updateCalendarDays(startDayOfWeek, daysInMonth, activeDay, dotColorsMatrix);
}

void DashboardUI::updateSyncStatus(bool googleConnected, bool appleConnected) {
    calCache.googleConnected = googleConnected;
    calCache.appleConnected = appleConnected;
    if (calendarScreen) calendarScreen->updateSyncStatus(googleConnected, appleConnected);
}

void DashboardUI::clearEvents() {
    calCache.eventCount = 0;
    if (calendarScreen) calendarScreen->clearEvents();
}

void DashboardUI::addEvent(const CalendarEvent& event) {
    if (calCache.eventCount < 4) {
        calCache.events[calCache.eventCount] = event;
        calCache.eventCount++;
    }
    if (calendarScreen) calendarScreen->addEvent(event);
}

void DashboardUI::updateTrackInfo(const char* title, const char* artist, const char* album, const char* qualityStr) {
    strncpy(playerCache.title, title, sizeof(playerCache.title) - 1);
    strncpy(playerCache.artist, artist, sizeof(playerCache.artist) - 1);
    strncpy(playerCache.album, album, sizeof(playerCache.album) - 1);
    strncpy(playerCache.qualityStr, qualityStr, sizeof(playerCache.qualityStr) - 1);
    if (playerScreen) playerScreen->updateTrackInfo(title, artist, album, qualityStr);
}

void DashboardUI::updatePlaybackProgress(int currentTimeSecs, int totalTimeSecs) {
    playerCache.currentSec = currentTimeSecs;
    playerCache.totalSec = totalTimeSecs;
    if (playerScreen) playerScreen->updatePlaybackProgress(currentTimeSecs, totalTimeSecs);
}

void DashboardUI::setPlayState(bool isPlaying) {
    playerCache.isPlaying = isPlaying;
    if (playerScreen) playerScreen->setPlayState(isPlaying);
}

void DashboardUI::updatePlaybackMode(bool shuffleActive, bool repeatActive) {
    playerCache.shuffleActive = shuffleActive;
    playerCache.repeatActive = repeatActive;
    if (playerScreen) playerScreen->updatePlaybackMode(shuffleActive, repeatActive);
}

void DashboardUI::updateVolume(int volume) {
    playerCache.volume = volume;
    if (playerScreen) playerScreen->updateVolume(volume);
}

void DashboardUI::updateEQ(const char* eqMode) {
    strncpy(playerCache.eqMode, eqMode, sizeof(playerCache.eqMode) - 1);
    if (playerScreen) playerScreen->updateEQ(eqMode);
}

void DashboardUI::clearPlaylist() {
    playerCache.playlistCount = 0;
    if (playerScreen) playerScreen->clearPlaylist();
}

void DashboardUI::addPlaylistItem(const PlaylistItem& item) {
    if (playerCache.playlistCount < 6) {
        playerCache.playlist[playerCache.playlistCount] = item;
        playerCache.playlistCount++;
    }
    if (playerScreen) playerScreen->addPlaylistItem(item);
}

void DashboardUI::updateDeviceInfo(const SettingsDeviceInfo& info) {
    settingsCache.info = info;
    if (settingsScreen) settingsScreen->updateDeviceInfo(info);
}

void DashboardUI::updateMemoryUsage(float usedGB, float totalGB) {
    settingsCache.usedGB = usedGB;
    settingsCache.totalGB = totalGB;
    if (settingsScreen) settingsScreen->updateMemoryUsage(usedGB, totalGB);
}

void DashboardUI::updateBatteryStatus(int percent, const char* durationStr) {
    settingsCache.batPercent = percent;
    strncpy(settingsCache.batDuration, durationStr, sizeof(settingsCache.batDuration) - 1);
    if (settingsScreen) settingsScreen->updateBatteryStatus(percent, durationStr);
}

void DashboardUI::updateWiFiConnection(const char* ssid, const char* ip) {
    strncpy(settingsCache.wifiSsid, ssid, sizeof(settingsCache.wifiSsid) - 1);
    strncpy(settingsCache.wifiIp, ip, sizeof(settingsCache.wifiIp) - 1);
    if (settingsScreen) settingsScreen->updateWiFiConnection(ssid, ip);
}

void DashboardUI::updateLanguage(const char* langStr) {
    strncpy(settingsCache.language, langStr, sizeof(settingsCache.language) - 1);
    if (settingsScreen) settingsScreen->updateLanguage(langStr);
}

void DashboardUI::setActiveMenuItem(int index) {
    settingsCache.activeMenuItem = index;
    if (settingsScreen) settingsScreen->setActiveMenuItem(index);
}