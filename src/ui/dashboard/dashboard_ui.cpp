#include "dashboard_ui.h"
#include "cyd_theme.h"
#include "../../services/audio_player_service.h"
#include "../../services/storage_service.h"
#include "log.h"
#include <stdio.h>
#include <string.h>

DashboardUI::DashboardUI() : 
    activeTabIndex(-1),
    lastCheckedTrackIdx(-2),
    homeScreen(nullptr), 
    calendarScreen(nullptr), 
    playerScreen(nullptr), 
    settingsScreen(nullptr),
    devHud(nullptr)
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

    // 5. Create Developer HUD
    devHud = new DevHud();

    // 6. Make Home screen active by default
    setTabActive(0);
}

DashboardUI::~DashboardUI() {
    if (devHud) delete devHud;
    if (homeScreen) delete homeScreen;
    if (calendarScreen) delete calendarScreen;
    if (playerScreen) delete playerScreen;
    if (settingsScreen) delete settingsScreen;
}

void DashboardUI::initTabs() {
    const char* tabIcons[4] = {LV_SYMBOL_HOME, LV_SYMBOL_LIST, LV_SYMBOL_AUDIO, LV_SYMBOL_SETTINGS};
    
    lv_obj_set_layout(bottomNavBar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(bottomNavBar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bottomNavBar, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    for (int i = 0; i < 4; i++) {
        tabContainers[i] = lv_obj_create(bottomNavBar);
        lv_obj_set_size(tabContainers[i], 100, 36);
        lv_obj_set_style_bg_opa(tabContainers[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(tabContainers[i], 0, 0);
        lv_obj_set_style_pad_all(tabContainers[i], 0, 0);
        lv_obj_set_user_data(tabContainers[i], (void*)(intptr_t)i);
        lv_obj_add_event_cb(tabContainers[i], tab_click_event_cb, LV_EVENT_CLICKED, this);
        lv_obj_clear_flag(tabContainers[i], LV_OBJ_FLAG_SCROLLABLE);

        tabIndicators[i] = lv_obj_create(tabContainers[i]);
        lv_obj_set_size(tabIndicators[i], 60, 2);
        lv_obj_align(tabIndicators[i], LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(tabIndicators[i], CydTheme::getAccentGlowColor(), 0);
        lv_obj_set_style_bg_opa(tabIndicators[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(tabIndicators[i], 0, 0);
        lv_obj_set_style_radius(tabIndicators[i], 2, 0);
        lv_obj_clear_flag(tabIndicators[i], LV_OBJ_FLAG_CLICKABLE);

        this->tabIcons[i] = lv_label_create(tabContainers[i]);
        lv_label_set_text(this->tabIcons[i], tabIcons[i]);
        CydTheme::applyTextFont(this->tabIcons[i], CydTheme::getFont20(), CydTheme::getTextSecondary());
        lv_obj_align(this->tabIcons[i], LV_ALIGN_CENTER, 0, 1);
        lv_obj_clear_flag(this->tabIcons[i], LV_OBJ_FLAG_CLICKABLE);

        tabLabels[i] = nullptr;
    }
}

void DashboardUI::setTabActive(int index) {
    if (index == activeTabIndex) return;

    LOG_I("UI", "Tab switch %d -> %d | Heap: %u bytes",
          activeTabIndex, index, ESP.getFreeHeap());

    // 1. DỌN DẸP & GIẢI PHÓNG TÀI NGUYÊN CỦA TAB CŨ
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
                lastCheckedTrackIdx = -2;
            } 
            // Thoát khỏi Tab MP3 -> Dừng phát nhạc và giải phóng ngay bộ đệm SD/decoder cho tab khác
            AudioPlayerService::stop();
            LOG_I("UI", "Left Player Tab: Audio playback stopped & SD buffers released.");
            break;
        case 3: 
            if (settingsScreen) { 
                delete settingsScreen; 
                settingsScreen = nullptr; 
            } 
            break;
        default: break;
    }

    LOG_D("UI", "Screen freed     | Heap: %u bytes", ESP.getFreeHeap());

    // 2. Cập nhật navigation indicators
    for (int i = 0; i < 4; i++) {
        if (i == index) {
            lv_obj_set_style_bg_opa(tabIndicators[i], LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(tabIcons[i], CydTheme::getAccentGlowColor(), 0);
        } else {
            lv_obj_set_style_bg_opa(tabIndicators[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(tabIcons[i], CydTheme::getTextSecondary(), 0);
        }
    }

    activeTabIndex = index;

    // 3. Tạo screen mới theo yêu cầu và khôi phục dữ liệu cache
    switch (activeTabIndex) {
        case 0: {
            homeScreen = new HomeScreen(activeViewArea);
            homeScreen->updateTime(homeCache.timeStr, homeCache.secStr, homeCache.dateStr, homeCache.isAm);
            homeScreen->updateLunarCalendar(homeCache.lunarDayStr, homeCache.lunarInfoStr);
            homeScreen->updateCalendarRibbon(homeCache.activeDayIndex, homeCache.dayNumbers);
            homeScreen->updateWeather(homeCache.temp, homeCache.condition, homeCache.feelsLike, homeCache.humidity, homeCache.windSpeed, homeCache.uvIndex);
            homeScreen->updateGoldPrices(homeCache.goldBuy, homeCache.goldSell);
            homeScreen->updateFuelPrices(homeCache.fuelRon95, homeCache.fuelRon92, homeCache.fuelDiesel, homeCache.fuelMazut,
                                        homeCache.fuelRon95Delta, homeCache.fuelRon92Delta, homeCache.fuelDieselDelta, homeCache.fuelMazutDelta);
            break;
        }
        case 1: {
            calendarScreen = new CalendarScreen(activeViewArea);
            calendarScreen->setToday(calCache.year, calCache.month, calCache.day);
            break;
        }
        case 2: {
            if (!AudioPlayerService::isInitialized()) {
                LOG_I("UI", "Lazy loading Audio Player Service...");
                AudioPlayerService::init();
            }

            // CHỈ QUÉT THẺ NHỚ KHI NGƯỜI DÙNG THỰC SỰ VÀO TAB MP3 LẦN ĐẦU TIÊN
            if (AudioPlayerService::getTrackCount() == 0 && StorageService::isMounted()) {
                LOG_I("UI", "Scanning SD card for music files on-demand...");
                AudioPlayerService::scanMusicFiles();
            }

            playerScreen = new PlayerScreen(activeViewArea);
            lastCheckedTrackIdx = -2;

            // Populate UI playlist (Nạp toàn bộ danh sách bài hát trên thẻ nhớ)
            int count = AudioPlayerService::getTrackCount();
            if (count > 0) {
                playerScreen->clearPlaylist();
                for (int i = 0; i < count; i++) {
                    const AudioTrack* t = AudioPlayerService::getTrack(i);
                    if (!t) continue;
                    char durStr[16];
                    if (t->durationSec >= 3600) {
                        int h = t->durationSec / 3600;
                        int m = (t->durationSec % 3600) / 60;
                        int s = t->durationSec % 60;
                        snprintf(durStr, sizeof(durStr), "%02d:%02d:%02d", h, m, s);
                    } else {
                        snprintf(durStr, sizeof(durStr), "%02d:%02d", t->durationSec / 60, t->durationSec % 60);
                    }
                    PlaylistItem item = {t->title, t->artist, durStr, i == AudioPlayerService::getCurrentTrackIndex()};
                    playerScreen->addPlaylistItem(item, i);
                }
            }

            // Auto-play immediately when entering player mode if not playing
            if (!AudioPlayerService::isPlaying() && AudioPlayerService::getTrackCount() > 0) {
                AudioPlayerService::play();
            }
            playerCache.isPlaying = AudioPlayerService::isPlaying();
            playerScreen->syncCurrentTrackUI();
            playerScreen->updateVolume(playerCache.volume);
            playerScreen->updatePlaybackMode(playerCache.shuffleActive, (int)AudioPlayerService::getRepeatMode());
            break;
        }
        case 3: {
            settingsScreen = new SettingsScreen(activeViewArea);
            settingsScreen->updateDeviceInfo(settingsCache.info);
            settingsScreen->updateTelemetry(settingsCache.freeHeap, settingsCache.uptimeStr, settingsCache.ipStr, settingsCache.macStr);
            settingsScreen->updateWifiStatus(settingsCache.wifiState, settingsCache.wifiSsid, settingsCache.ipStr, settingsCache.wifiRssi);
            settingsScreen->setActiveMenuItem(settingsCache.activeMenuItem);
            break;
        }
    }
    if (masterContainer) {
        lv_obj_invalidate(masterContainer);
    }
    LOG_D("UI", "Screen built      | Heap: %u bytes", ESP.getFreeHeap());
}


void DashboardUI::tab_click_event_cb(lv_event_t* e) {
    DashboardUI* self = (DashboardUI*)lv_event_get_user_data(e);
    lv_obj_t* target = lv_event_get_current_target(e);
    int tabIdx = (int)(intptr_t)lv_obj_get_user_data(target);
    self->setTabActive(tabIdx);
}

// --- Home Screen Setters ---
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
    for (int i = 0; i < 7; i++) homeCache.dayNumbers[i] = dayNumbers[i];
    if (homeScreen) homeScreen->updateCalendarRibbon(activeDayIndex, dayNumbers);
}

void DashboardUI::updateWeather(int temp, const char* condition, int feelsLike, int humidity, int windSpeed, int uvIndex, const char* cityName) {
    homeCache.temp = temp;
    strncpy(homeCache.condition, condition, sizeof(homeCache.condition) - 1);
    homeCache.feelsLike = feelsLike;
    homeCache.humidity = humidity;
    homeCache.windSpeed = windSpeed;
    homeCache.uvIndex = uvIndex;
    if (homeScreen) homeScreen->updateWeather(temp, condition, feelsLike, humidity, windSpeed, uvIndex, cityName);
}

void DashboardUI::updateGoldPrices(const char* buySJC, const char* sellSJC) {
    if (buySJC) strncpy(homeCache.goldBuy, buySJC, sizeof(homeCache.goldBuy) - 1);
    if (sellSJC) strncpy(homeCache.goldSell, sellSJC, sizeof(homeCache.goldSell) - 1);
    if (homeScreen) homeScreen->updateGoldPrices(buySJC, sellSJC);
}

void DashboardUI::updateFuelPrices(int ron95, int ron92, int diesel, int mazut, int ron95Delta, int ron92Delta, int dieselDelta, int mazutDelta) {
    homeCache.fuelRon95 = ron95;
    homeCache.fuelRon92 = ron92;
    homeCache.fuelDiesel = diesel;
    homeCache.fuelMazut = mazut;
    homeCache.fuelRon95Delta = ron95Delta;
    homeCache.fuelRon92Delta = ron92Delta;
    homeCache.fuelDieselDelta = dieselDelta;
    homeCache.fuelMazutDelta = mazutDelta;
    if (homeScreen) homeScreen->updateFuelPrices(ron95, ron92, diesel, mazut, ron95Delta, ron92Delta, dieselDelta, mazutDelta);
}

// --- Calendar Screen Setters ---
void DashboardUI::setCalendarToday(int year, int month, int day) {
    calCache.year = year;
    calCache.month = month;
    calCache.day = day;
    if (calendarScreen) calendarScreen->setToday(year, month, day);
}

// --- Player Screen Setters ---
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

void DashboardUI::updatePlaybackMode(bool shuffleActive, int repeatMode) {
    playerCache.shuffleActive = shuffleActive;
    playerCache.repeatMode = repeatMode;
    if (playerScreen) playerScreen->updatePlaybackMode(shuffleActive, repeatMode);
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

void DashboardUI::addPlaylistItem(const PlaylistItem& item, int trackIndex) {
    if (playerCache.playlistCount < 6) {
        playerCache.playlist[playerCache.playlistCount++] = item;
    }
    if (playerScreen) playerScreen->addPlaylistItem(item, trackIndex);
}

// --- Settings Screen Setters ---
void DashboardUI::updateDeviceInfo(const SettingsDeviceInfo& info) {
    settingsCache.info = info;
    if (settingsScreen) settingsScreen->updateDeviceInfo(info);
}

void DashboardUI::updateSettingsTelemetry(uint32_t freeHeap, const char* uptimeStr, const char* ipStr, const char* macStr) {
    settingsCache.freeHeap = freeHeap;
    if (uptimeStr) strncpy(settingsCache.uptimeStr, uptimeStr, sizeof(settingsCache.uptimeStr) - 1);
    if (ipStr) strncpy(settingsCache.ipStr, ipStr, sizeof(settingsCache.ipStr) - 1);
    if (macStr) strncpy(settingsCache.macStr, macStr, sizeof(settingsCache.macStr) - 1);

    if (settingsScreen) {
        settingsScreen->updateTelemetry(freeHeap, uptimeStr, ipStr, macStr);
    }
}

void DashboardUI::updateWifiSettings(const char* stateStr, const char* ssid, const char* ip, int rssi) {
    if (stateStr) strncpy(settingsCache.wifiState, stateStr, sizeof(settingsCache.wifiState) - 1);
    if (ssid) strncpy(settingsCache.wifiSsid, ssid, sizeof(settingsCache.wifiSsid) - 1);
    if (ip) strncpy(settingsCache.ipStr, ip, sizeof(settingsCache.ipStr) - 1);
    settingsCache.wifiRssi = rssi;

    if (settingsScreen) {
        settingsScreen->updateWifiStatus(stateStr, ssid, ip, rssi);
    }
}

void DashboardUI::setActiveMenuItem(int index) {
    settingsCache.activeMenuItem = index;
    if (settingsScreen) settingsScreen->setActiveMenuItem(index);
}

void DashboardUI::setDevHudVisible(bool visible) {
    if (devHud) devHud->setVisible(visible);
}

void DashboardUI::updateDevHud(float fps, uint32_t freeHeapKb, float memUsagePercent, uint8_t cpuPercent, int32_t rssi, const char* ip) {
    if (devHud) devHud->updateStats(fps, freeHeapKb, memUsagePercent, cpuPercent, rssi, ip);
}


void DashboardUI::tick() {
    if (playerScreen) {
        if (playerCache.isPlaying) {
            playerScreen->tickSpectrumAnimation();
        }

        // Dùng member lastCheckedTrackIdx (không phải static local) để có thể reset khi tạo mới PlayerScreen
        int curIdx = AudioPlayerService::getCurrentTrackIndex();
        if (curIdx != lastCheckedTrackIdx) {
            lastCheckedTrackIdx = curIdx;
            playerScreen->syncCurrentTrackUI();
        }
    }
}