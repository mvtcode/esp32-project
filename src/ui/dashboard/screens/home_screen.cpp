#include "home_screen.h"
#include "../cyd_theme.h"
#include "../../../services/config_manager.h"
#include <stdio.h>

HomeScreen::HomeScreen(lv_obj_t* parent) {
    // 1. Create root screen container
    rootContainer = lv_obj_create(parent);
    lv_obj_set_size(rootContainer, 480, 282);
    lv_obj_set_style_bg_opa(rootContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rootContainer, 0, 0);
    lv_obj_set_style_pad_all(rootContainer, 0, 0);
    lv_obj_clear_flag(rootContainer, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Instantiate Grid Cards
    createClockCard(rootContainer);
    createWeatherCard(rootContainer);
    createGoldCard(rootContainer);
    createFuelCard(rootContainer);
}

HomeScreen::~HomeScreen() {
    if (rootContainer) {
        lv_obj_del(rootContainer);
        rootContainer = nullptr;
    }
}

void HomeScreen::createClockCard(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 230, 131);
    lv_obj_align(card, LV_ALIGN_TOP_LEFT, 6, 6);
    CydTheme::applyCardStyle(card);

    // 1. Digital Clock 24h Inner Container - Perfectly Centered in Card (204px)
    lv_obj_t* clockBox = lv_obj_create(card);
    lv_obj_set_size(clockBox, 204, 42);
    lv_obj_align(clockBox, LV_ALIGN_TOP_MID, 0, -3);
    lv_obj_set_style_bg_opa(clockBox, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(clockBox, 0, 0);
    lv_obj_set_style_pad_all(clockBox, 0, 0);
    lv_obj_clear_flag(clockBox, LV_OBJ_FLAG_SCROLLABLE);

    // Hours slot (X=0, W=56)
    lblClockHours = lv_label_create(clockBox);
    lv_obj_set_width(lblClockHours, 56);
    lv_label_set_text(lblClockHours, "--");
    CydTheme::applyTextFont(lblClockHours, CydTheme::getFont40(), CydTheme::getTextPrimary());
    lv_obj_set_style_text_align(lblClockHours, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lblClockHours, LV_ALIGN_TOP_LEFT, 0, 2);

    // Colon 1 (X=58, W=14)
    lv_obj_t* lblColon1 = lv_label_create(clockBox);
    lv_obj_set_width(lblColon1, 14);
    lv_label_set_text(lblColon1, ":");
    CydTheme::applyTextFont(lblColon1, CydTheme::getFont40(), CydTheme::getTextSecondary());
    lv_obj_set_style_text_align(lblColon1, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lblColon1, LV_ALIGN_TOP_LEFT, 58, 0);

    // Minutes slot (X=74, W=56)
    lblClockMinutes = lv_label_create(clockBox);
    lv_obj_set_width(lblClockMinutes, 56);
    lv_label_set_text(lblClockMinutes, "--");
    CydTheme::applyTextFont(lblClockMinutes, CydTheme::getFont40(), CydTheme::getTextPrimary());
    lv_obj_set_style_text_align(lblClockMinutes, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lblClockMinutes, LV_ALIGN_TOP_LEFT, 74, 2);

    // Colon 2 (X=132, W=14)
    lv_obj_t* lblColon2 = lv_label_create(clockBox);
    lv_obj_set_width(lblColon2, 14);
    lv_label_set_text(lblColon2, ":");
    CydTheme::applyTextFont(lblColon2, CydTheme::getFont40(), CydTheme::getTextSecondary());
    lv_obj_set_style_text_align(lblColon2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lblColon2, LV_ALIGN_TOP_LEFT, 132, 0);

    // Seconds slot (X=148, W=56 - đủ rộng cho '00', '08', '09' không bị che)
    lblClockSeconds = lv_label_create(clockBox);
    lv_obj_set_width(lblClockSeconds, 56);
    lv_label_set_text(lblClockSeconds, "--");
    CydTheme::applyTextFont(lblClockSeconds, CydTheme::getFont40(), CydTheme::getTextPrimary());
    lv_obj_set_style_text_align(lblClockSeconds, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lblClockSeconds, LV_ALIGN_TOP_LEFT, 148, 2);

    lblClockTime = nullptr;
    lblClockAmPm = nullptr;

    // 2. Textual Solar Date: "Chủ nhật, 30/08" (Căn Giữa)
    lblClockDate = lv_label_create(card);
    lv_label_set_text(lblClockDate, "Đang đồng bộ NTP...");
    CydTheme::applyTextFont(lblClockDate, CydTheme::getFont16(), CydTheme::getTextPrimary());
    lv_obj_set_style_text_align(lblClockDate, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lblClockDate, LV_ALIGN_TOP_MID, 0, 43);

    // 3. Lunar Date: "ÂL: 18/07 (Bính Tý)" (Căn Giữa)
    lblLunarDate = lv_label_create(card);
    lv_label_set_text(lblLunarDate, "ÂL: Đang đồng bộ...");
    CydTheme::applyTextFont(lblLunarDate, CydTheme::getFont14(), CydTheme::getGoldColor());
    lv_obj_set_style_text_align(lblLunarDate, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lblLunarDate, LV_ALIGN_TOP_MID, 0, 68);

    // 4. Hoàng Đạo / Hắc Đạo (Không dùng border)
    boxLunarBadge = nullptr;
    lblLunarInfo = lv_label_create(card);
    lv_label_set_text(lblLunarInfo, "--");
    CydTheme::applyTextFont(lblLunarInfo, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_set_style_text_align(lblLunarInfo, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(lblLunarInfo, LV_ALIGN_TOP_MID, 0, 90);

    for (int i = 0; i < 7; i++) {
        ribbonDayContainers[i] = nullptr;
        ribbonDayNumbers[i] = nullptr;
        ribbonDayNames[i] = nullptr;
    }
}

void HomeScreen::createWeatherCard(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 230, 131);
    lv_obj_align(card, LV_ALIGN_TOP_RIGHT, -6, 6);
    CydTheme::applyCardStyle(card);

    // ==========================================
    // LINE 1: Weather Icon + Temp (Left, Font 24, align-items: center), GPS + City (Right)
    // LINE 2: Weather Condition (Right, căn phải)
    // ==========================================

    // Flex container cho Icon + Nhiệt độ (align-items: center dọc nhau, to như ban đầu Font 24)
    lv_obj_t* boxIconTemp = lv_obj_create(card);
    lv_obj_set_size(boxIconTemp, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(boxIconTemp, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_style_bg_opa(boxIconTemp, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(boxIconTemp, 0, 0);
    lv_obj_set_style_pad_all(boxIconTemp, 0, 0);
    lv_obj_set_style_pad_column(boxIconTemp, 6, 0);
    lv_obj_set_layout(boxIconTemp, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(boxIconTemp, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(boxIconTemp, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(boxIconTemp, LV_OBJ_FLAG_SCROLLABLE);

    // 1. Weather Icon inside boxIconTemp (to ra như cũ: WeatherFont24)
    objWeatherIconCanvas = lv_label_create(boxIconTemp);
    lv_obj_set_style_text_font(objWeatherIconCanvas, CydTheme::getWeatherFont24(), 0);
    lv_obj_set_style_text_color(objWeatherIconCanvas, CydTheme::getGoldColor(), 0);
    lv_label_set_text(objWeatherIconCanvas, "\uF00D"); // Default to wi-day-sunny

    // 2. Dynamic temperature label inside boxIconTemp (to như lúc đầu: Font 24, căn giữa dọc cùng icon)
    lblWeatherTemp = lv_label_create(boxIconTemp);
    lv_label_set_text(lblWeatherTemp, "--°C");
    CydTheme::applyTextFont(lblWeatherTemp, CydTheme::getFont24(), CydTheme::getTextPrimary());

    // 3. City Title (Line 1: bên phải giữ nguyên, căn phải)
    const CityLocation& curCity = ConfigManager::getCurrentCity();
    char cBuf[64];
    snprintf(cBuf, sizeof(cBuf), LV_SYMBOL_GPS " %s", curCity.name);
    lblWeatherCity = lv_label_create(card);
    lv_label_set_text(lblWeatherCity, cBuf);
    CydTheme::applyTextFont(lblWeatherCity, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_set_style_text_align(lblWeatherCity, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(lblWeatherCity, LV_ALIGN_TOP_RIGHT, 0, 0);

    // 4. Condition text (Line 2: căn phải text thời tiết ví dụ trời nắng, hạ 3px)
    lblWeatherCond = lv_label_create(card);
    lv_label_set_text(lblWeatherCond, "Đang tải...");
    CydTheme::applyTextFont(lblWeatherCond, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_set_width(lblWeatherCond, 120);
    lv_label_set_long_mode(lblWeatherCond, LV_LABEL_LONG_DOT);
    lv_obj_set_style_text_align(lblWeatherCond, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(lblWeatherCond, LV_ALIGN_TOP_RIGHT, 0, 19);

    // ==========================================
    // LINE 3-6 (Line khác giữ nguyên): 4 chỉ số (Cảm giác, Độ ẩm, Gió, UV)
    // Label căn sát bên trái (X=0), Value căn phải (X=0), hạ 3px
    // ==========================================
    const char* paramTitles[4] = {"Cảm giác:", "Độ ẩm:", "Gió:", "Chỉ số UV:"};
    lv_obj_t** paramValLabels[4] = {&lblWeatherFeelsLike, &lblWeatherParamHumid, &lblWeatherParamWind, &lblWeatherParamUv};
    const char* defaultVals[4] = {"--°", "--%", "-- km/h", "--"};

    for (int i = 0; i < 4; i++) {
        int yOffset = 37 + i * 19;

        // Static label on the left (sát mép trái như tên xăng dầu)
        lv_obj_t* lblTitle = lv_label_create(card);
        lv_label_set_text(lblTitle, paramTitles[i]);
        CydTheme::applyTextFont(lblTitle, CydTheme::getFont12(), CydTheme::getTextSecondary());
        lv_obj_align(lblTitle, LV_ALIGN_TOP_LEFT, 0, yOffset);

        // Dynamic value on the right (căn phải thẳng hàng delta xăng dầu)
        *paramValLabels[i] = lv_label_create(card);
        lv_label_set_text(*paramValLabels[i], defaultVals[i]);
        CydTheme::applyTextFont(*paramValLabels[i], CydTheme::getFont12(), CydTheme::getTextPrimary());
        lv_obj_align(*paramValLabels[i], LV_ALIGN_TOP_RIGHT, 0, yOffset);
    }
}

void HomeScreen::createGoldCard(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 230, 131);
    lv_obj_align(card, LV_ALIGN_BOTTOM_LEFT, 6, -6);
    CydTheme::applyCardStyle(card);

    // Title label
    lv_obj_t* lblTitle = lv_label_create(card);
    lv_label_set_text(lblTitle, "Giá Vàng SJC (triệu đ/lượng)");
    CydTheme::applyTextFont(lblTitle, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    // Left Column: BUY
    lv_obj_t* lblBuyTitle = lv_label_create(card);
    lv_label_set_text(lblBuyTitle, "MUA VÀO");
    CydTheme::applyTextFont(lblBuyTitle, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblBuyTitle, LV_ALIGN_TOP_LEFT, 4, 26);

    lblGoldBuy = lv_label_create(card);
    lv_label_set_text(lblGoldBuy, "-----");
    CydTheme::applyTextFont(lblGoldBuy, CydTheme::getFont24(), CydTheme::getGoldColor());
    lv_obj_align(lblGoldBuy, LV_ALIGN_TOP_LEFT, 4, 46);

    // Right Column: SELL
    lv_obj_t* lblSellTitle = lv_label_create(card);
    lv_label_set_text(lblSellTitle, "BÁN RA");
    CydTheme::applyTextFont(lblSellTitle, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblSellTitle, LV_ALIGN_TOP_RIGHT, -4, 26);

    lblGoldSell = lv_label_create(card);
    lv_label_set_text(lblGoldSell, "-----");
    CydTheme::applyTextFont(lblGoldSell, CydTheme::getFont24(), CydTheme::getGoldColor());
    lv_obj_align(lblGoldSell, LV_ALIGN_TOP_RIGHT, -4, 46);

    // Note / World gold label
    lblGoldStatus = lv_label_create(card);
    lv_label_set_text(lblGoldStatus, "Thế giới: Đang tải...");
    CydTheme::applyTextFont(lblGoldStatus, CydTheme::getFont12(), CydTheme::getTextPrimary());
    lv_obj_align(lblGoldStatus, LV_ALIGN_BOTTOM_LEFT, 0, 0);

    lblGoldBuyDelta = nullptr;
    lblGoldSellDelta = nullptr;
}

void HomeScreen::createFuelCard(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 230, 131);
    lv_obj_align(card, LV_ALIGN_BOTTOM_RIGHT, -6, -6);
    CydTheme::applyCardStyle(card);

    // Title label
    lv_obj_t* lblTitle = lv_label_create(card);
    lv_label_set_text(lblTitle, "Bảng Xăng Dầu (đ/L)");
    CydTheme::applyTextFont(lblTitle, CydTheme::getFont12(), CydTheme::getBlueColor());
    lv_obj_align(lblTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    // Fuel row structures
    const char* fuelNames[4] = {"RON 95-III", "E5 RON 92", "Dầu Diesel", "Dầu Mazut"};
    lv_obj_t* prices[4];
    lv_obj_t* deltas[4];

    for (int i = 0; i < 4; i++) {
        int yOffset = 26 + i * 22;

        // Name
        lv_obj_t* lblName = lv_label_create(card);
        lv_label_set_text(lblName, fuelNames[i]);
        CydTheme::applyTextFont(lblName, CydTheme::getFont12(), CydTheme::getTextSecondary());
        lv_obj_align(lblName, LV_ALIGN_TOP_LEFT, 0, yOffset);

        // Price
        prices[i] = lv_label_create(card);
        lv_label_set_text(prices[i], "--");
        CydTheme::applyTextFont(prices[i], CydTheme::getFont12(), CydTheme::getTextPrimary());
        lv_obj_align(prices[i], LV_ALIGN_TOP_RIGHT, -56, yOffset);

        // Delta
        deltas[i] = lv_label_create(card);
        lv_label_set_text(deltas[i], "--");
        CydTheme::applyTextFont(deltas[i], CydTheme::getFont12(), CydTheme::getTextMuted());
        lv_obj_align(deltas[i], LV_ALIGN_TOP_RIGHT, 0, yOffset);
    }

    // Save bindings
    lblFuelRon95Price = prices[0];
    lblFuelRon95Delta = deltas[0];
    lblFuelRon92Price = prices[1];
    lblFuelRon92Delta = deltas[1];
    lblFuelDieselPrice = prices[2];
    lblFuelDieselDelta = deltas[2];
    lblFuelMazutPrice = prices[3];
    lblFuelMazutDelta = deltas[3];
}

// --- Dynamic Setter Updates ---

void HomeScreen::updateTime(const char* timeStr, const char* secondsStr, const char* dateStr, bool isAm) {
    if (lblClockHours && lblClockMinutes && lblClockSeconds) {
        if (!timeStr || strcmp(timeStr, "-- : -- : --") == 0 || strstr(timeStr, "--")) {
            lv_label_set_text(lblClockHours, "--");
            lv_label_set_text(lblClockMinutes, "--");
            lv_label_set_text(lblClockSeconds, "--");
        } else {
            char hBuf[8] = "00", mBuf[8] = "00", sBuf[8] = "00";
            if (sscanf(timeStr, "%2s : %2s : %2s", hBuf, mBuf, sBuf) == 3 ||
                sscanf(timeStr, "%2s:%2s:%2s", hBuf, mBuf, sBuf) == 3) {
                lv_label_set_text(lblClockHours, hBuf);
                lv_label_set_text(lblClockMinutes, mBuf);
                lv_label_set_text(lblClockSeconds, sBuf);
            } else {
                lv_label_set_text(lblClockHours, "--");
                lv_label_set_text(lblClockMinutes, "--");
                lv_label_set_text(lblClockSeconds, "--");
            }
        }
    }
    if (lblClockTime) {
        lv_label_set_text(lblClockTime, timeStr);
        lv_obj_align(lblClockTime, LV_ALIGN_TOP_MID, 0, 4);
    }
    if (lblClockDate) {
        lv_label_set_text(lblClockDate, dateStr);
        lv_obj_align(lblClockDate, LV_ALIGN_TOP_MID, 0, 43);
    }
}

void HomeScreen::updateLunarCalendar(const char* lunarDayStr, const char* lunarInfoStr, bool isHoangDao) {
    if (lblLunarDate && lunarDayStr) {
        lv_label_set_text(lblLunarDate, lunarDayStr);
        lv_obj_align(lblLunarDate, LV_ALIGN_TOP_MID, 0, 68);
    }
    if (lblLunarInfo && lunarInfoStr) {
        lv_label_set_text(lblLunarInfo, lunarInfoStr);
        lv_obj_align(lblLunarInfo, LV_ALIGN_TOP_MID, 0, 90);
        if (strlen(lunarInfoStr) == 0 || strcmp(lunarInfoStr, "--") == 0) {
            CydTheme::applyTextFont(lblLunarInfo, CydTheme::getFont12(), CydTheme::getTextMuted());
        } else if (isHoangDao) {
            CydTheme::applyTextFont(lblLunarInfo, CydTheme::getFont12(), CydTheme::getSuccessColor());
        } else {
            CydTheme::applyTextFont(lblLunarInfo, CydTheme::getFont12(), CydTheme::getDangerColor());
        }
    }
}

void HomeScreen::updateCalendarRibbon(int activeDayIndex, const int* dayNumbers) {
    for (int i = 0; i < 7; i++) {
        if (!ribbonDayNumbers[i]) continue;
        char buf[8];
        sprintf(buf, "%d", dayNumbers[i]);
        lv_label_set_text(ribbonDayNumbers[i], buf);

        if (i == activeDayIndex) {
            if (ribbonDayContainers[i]) {
                lv_obj_set_style_bg_color(ribbonDayContainers[i], CydTheme::getAccentColor(), 0);
                lv_obj_set_style_bg_opa(ribbonDayContainers[i], LV_OPA_COVER, 0);
            }
            lv_obj_set_style_text_color(ribbonDayNumbers[i], CydTheme::getWhiteColor(), 0);
            if (ribbonDayNames[i]) lv_obj_set_style_text_color(ribbonDayNames[i], CydTheme::getTextSecondary(), 0);
        } else {
            if (ribbonDayContainers[i]) lv_obj_set_style_bg_opa(ribbonDayContainers[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(ribbonDayNumbers[i], CydTheme::getTextSecondary(), 0);
            if (ribbonDayNames[i]) lv_obj_set_style_text_color(ribbonDayNames[i], CydTheme::getTextMuted(), 0);
        }
    }
}

void HomeScreen::updateWeather(int temp, const char* condition, int feelsLike, int humidity, int windSpeed, int uvIndex, const char* cityName) {
    char buf[32];
    
    if (lblWeatherCity) {
        const char* nameToUse = (cityName && strlen(cityName) > 0) ? cityName : ConfigManager::getCurrentCity().name;
        char cBuf[64];
        snprintf(cBuf, sizeof(cBuf), LV_SYMBOL_GPS " %s", nameToUse);
        lv_label_set_text(lblWeatherCity, cBuf);
        lv_obj_align(lblWeatherCity, LV_ALIGN_TOP_RIGHT, 0, 0);
    }

    // Custom weather icon mapping using dedicated Weather Icons font
    if (temp <= -100) {
        if (lblWeatherTemp) lv_label_set_text(lblWeatherTemp, "--°C");
        if (lblWeatherCond) {
            lv_label_set_text(lblWeatherCond, condition ? condition : "Đang tải...");
            lv_obj_align(lblWeatherCond, LV_ALIGN_TOP_RIGHT, 0, 19);
        }
        if (lblWeatherFeelsLike) {
            lv_label_set_text(lblWeatherFeelsLike, "--°");
            lv_obj_align(lblWeatherFeelsLike, LV_ALIGN_TOP_RIGHT, 0, 37);
        }
        if (lblWeatherParamHumid) {
            lv_label_set_text(lblWeatherParamHumid, "--%");
            lv_obj_align(lblWeatherParamHumid, LV_ALIGN_TOP_RIGHT, 0, 56);
        }
        if (lblWeatherParamWind) {
            lv_label_set_text(lblWeatherParamWind, "-- km/h");
            lv_obj_align(lblWeatherParamWind, LV_ALIGN_TOP_RIGHT, 0, 75);
        }
        if (lblWeatherParamUv) {
            lv_label_set_text(lblWeatherParamUv, "--");
            lv_obj_align(lblWeatherParamUv, LV_ALIGN_TOP_RIGHT, 0, 94);
        }
        return;
    }

    if (strstr(condition, "Mưa") || strstr(condition, "mưa") || strstr(condition, "Bão") || strstr(condition, "bão")) {
        lv_label_set_text(objWeatherIconCanvas, "\uF008"); // wi-day-rain
        lv_obj_set_style_text_color(objWeatherIconCanvas, lv_color_make(64, 156, 255), 0); // Beautiful rain blue
    } else if (strstr(condition, "Mây") || strstr(condition, "mây") || strstr(condition, "Âm u") || strstr(condition, "âm u")) {
        lv_label_set_text(objWeatherIconCanvas, "\uF002"); // wi-day-cloudy
        lv_obj_set_style_text_color(objWeatherIconCanvas, CydTheme::getTextSecondary(), 0); // Silver for cloudy skies
    } else {
        lv_label_set_text(objWeatherIconCanvas, "\uF00D"); // wi-day-sunny
        lv_obj_set_style_text_color(objWeatherIconCanvas, CydTheme::getGoldColor(), 0); // Gold for sunny skies
    }

    sprintf(buf, "%d°C", temp);
    lv_label_set_text(lblWeatherTemp, buf);

    if (lblWeatherCond) {
        lv_label_set_text(lblWeatherCond, condition);
        lv_obj_align(lblWeatherCond, LV_ALIGN_TOP_RIGHT, 0, 19);
    }

    sprintf(buf, "%d°", feelsLike);
    lv_label_set_text(lblWeatherFeelsLike, buf);
    lv_obj_align(lblWeatherFeelsLike, LV_ALIGN_TOP_RIGHT, 0, 37);

    sprintf(buf, "%d%%", humidity);
    lv_label_set_text(lblWeatherParamHumid, buf);
    lv_obj_align(lblWeatherParamHumid, LV_ALIGN_TOP_RIGHT, 0, 56);

    sprintf(buf, "%d km/h", windSpeed);
    lv_label_set_text(lblWeatherParamWind, buf);
    lv_obj_align(lblWeatherParamWind, LV_ALIGN_TOP_RIGHT, 0, 75);

    sprintf(buf, "%d", uvIndex);
    lv_label_set_text(lblWeatherParamUv, buf);
    lv_obj_align(lblWeatherParamUv, LV_ALIGN_TOP_RIGHT, 0, 94);
}

void HomeScreen::updateGoldPrices(const char* buySJC, const char* sellSJC, const char* worldBuy, const char* worldSell) {
    bool hasValidData = (buySJC && strlen(buySJC) > 0 && 
                         strcmp(buySJC, "----") != 0 && 
                         strcmp(buySJC, "-----") != 0);
    if (lblGoldBuy) {
        lv_label_set_text(lblGoldBuy, hasValidData ? buySJC : "-----");
    }
    if (lblGoldSell) {
        lv_label_set_text(lblGoldSell, (sellSJC && strlen(sellSJC) > 0 && 
                                       strcmp(sellSJC, "----") != 0 && 
                                       strcmp(sellSJC, "-----") != 0) ? sellSJC : "-----");
    }
    if (lblGoldStatus) {
        if (worldBuy && strlen(worldBuy) > 0 && strcmp(worldBuy, "-----") != 0 && strcmp(worldBuy, "----") != 0) {
            char tgBuf[48];
            if (worldSell && strlen(worldSell) > 0 && strcmp(worldSell, "-----") != 0) {
                snprintf(tgBuf, sizeof(tgBuf), "Thế giới: %s - %s USD/oz", worldBuy, worldSell);
            } else {
                snprintf(tgBuf, sizeof(tgBuf), "Thế giới: %s USD/oz", worldBuy);
            }
            lv_label_set_text(lblGoldStatus, tgBuf);
        } else {
            lv_label_set_text(lblGoldStatus, hasValidData ? "Đang cập nhật..." : "Đang tải...");
        }
    }
}

void HomeScreen::updateFuelPrices(int ron95, int ron92, int diesel, int mazut, int ron95Delta, int ron92Delta, int dieselDelta, int mazutDelta) {
    char priceBuf[16];
    char deltaBuf[16];
    
    auto applyRow = [](lv_obj_t* lblPrice, lv_obj_t* lblDelta, int price, int delta) {
        char pBuf[16];
        char dBuf[16];
        
        if (price <= 0) {
            if (lblPrice) lv_label_set_text(lblPrice, "--");
            if (lblDelta) {
                lv_label_set_text(lblDelta, "--");
                lv_obj_set_style_text_color(lblDelta, CydTheme::getTextMuted(), 0);
            }
            return;
        }

        if (price >= 1000) {
            sprintf(pBuf, "%d.%03d", price / 1000, price % 1000);
        } else {
            sprintf(pBuf, "%d", price);
        }
        if (lblPrice) lv_label_set_text(lblPrice, pBuf);

        if (!lblDelta) return;

        if (delta == 0) {
            sprintf(dBuf, "--");
            lv_label_set_text(lblDelta, dBuf);
            lv_obj_set_style_text_color(lblDelta, CydTheme::getTextMuted(), 0);
        } else if (delta > 0) {
            sprintf(dBuf, "%s%d", LV_SYMBOL_UP, delta);
            lv_label_set_text(lblDelta, dBuf);
            lv_obj_set_style_text_color(lblDelta, CydTheme::getSuccessColor(), 0);
        } else {
            sprintf(dBuf, "%s%d", LV_SYMBOL_DOWN, -delta);
            lv_label_set_text(lblDelta, dBuf);
            lv_obj_set_style_text_color(lblDelta, CydTheme::getDangerColor(), 0);
        }
    };

    applyRow(lblFuelRon95Price, lblFuelRon95Delta, ron95, ron95Delta);
    applyRow(lblFuelRon92Price, lblFuelRon92Delta, ron92, ron92Delta);
    applyRow(lblFuelDieselPrice, lblFuelDieselDelta, diesel, dieselDelta);
    applyRow(lblFuelMazutPrice, lblFuelMazutDelta, mazut, mazutDelta);
}