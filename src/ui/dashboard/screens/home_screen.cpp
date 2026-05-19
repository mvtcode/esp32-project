#include "home_screen.h"
#include "../cyd_theme.h"
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

void HomeScreen::createClockCard(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 230, 131);
    lv_obj_align(card, LV_ALIGN_TOP_LEFT, 6, 6);
    CydTheme::applyCardStyle(card);

    // 1. Digital Clock Hours & Minutes
    lblClockTime = lv_label_create(card);
    lv_label_set_text(lblClockTime, "12:00");
    CydTheme::applyTextFont(lblClockTime, CydTheme::getFont32(), CydTheme::getTextPrimary());
    lv_obj_align(lblClockTime, LV_ALIGN_TOP_LEFT, 0, 0);

    // 2. Seconds Label
    lblClockSeconds = lv_label_create(card);
    lv_label_set_text(lblClockSeconds, "00");
    CydTheme::applyTextFont(lblClockSeconds, CydTheme::getFont14(), CydTheme::getTextSecondary());
    lv_obj_align(lblClockSeconds, LV_ALIGN_TOP_LEFT, 96, 4);

    // 3. AM / PM
    lblClockAmPm = lv_label_create(card);
    lv_label_set_text(lblClockAmPm, "SA");
    CydTheme::applyTextFont(lblClockAmPm, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblClockAmPm, LV_ALIGN_TOP_LEFT, 96, 20);

    // 4. Textual Gregorian Date
    lblClockDate = lv_label_create(card);
    lv_label_set_text(lblClockDate, "Thứ Năm, 01/01/2026");
    CydTheme::applyTextFont(lblClockDate, CydTheme::getFont12(), CydTheme::getTextPrimary());
    lv_obj_align(lblClockDate, LV_ALIGN_TOP_LEFT, 0, 36);

    // 5. Lunar Calendar info on the top-right
    lblLunarDate = lv_label_create(card);
    lv_label_set_text(lblLunarDate, "15/11 Âm lịch");
    CydTheme::applyTextFont(lblLunarDate, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblLunarDate, LV_ALIGN_TOP_RIGHT, 0, 2);

    lblLunarInfo = lv_label_create(card);
    lv_label_set_text(lblLunarInfo, "Năm Bính Ngọ");
    CydTheme::applyTextFont(lblLunarInfo, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblLunarInfo, LV_ALIGN_TOP_RIGHT, 0, 18);

    // 6. Horizontal Weekly Calendar Ribbon (Days Mon-Sun)
    lv_obj_t* ribbon = lv_obj_create(card);
    lv_obj_set_size(ribbon, 210, 48);
    lv_obj_align(ribbon, LV_ALIGN_BOTTOM_MID, 0, 4);
    lv_obj_set_style_bg_opa(ribbon, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(ribbon, 0, 0);
    lv_obj_set_style_pad_all(ribbon, 0, 0);
    lv_obj_set_layout(ribbon, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(ribbon, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ribbon, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ribbon, LV_OBJ_FLAG_SCROLLABLE);

    const char* wdayNames[7] = {"T2", "T3", "T4", "T5", "T6", "T7", "CN"};

    for (int i = 0; i < 7; i++) {
        // Individual day cell
        ribbonDayContainers[i] = lv_obj_create(ribbon);
        lv_obj_set_size(ribbonDayContainers[i], 26, 42);
        lv_obj_set_style_radius(ribbonDayContainers[i], 6, 0);
        lv_obj_set_style_bg_opa(ribbonDayContainers[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(ribbonDayContainers[i], 0, 0);
        lv_obj_set_style_pad_all(ribbonDayContainers[i], 0, 0);
        lv_obj_clear_flag(ribbonDayContainers[i], LV_OBJ_FLAG_SCROLLABLE);

        // Weekday Name Label (T2, T3...)
        ribbonDayNames[i] = lv_label_create(ribbonDayContainers[i]);
        lv_label_set_text(ribbonDayNames[i], wdayNames[i]);
        CydTheme::applyTextFont(ribbonDayNames[i], CydTheme::getFont12(), CydTheme::getTextMuted());
        lv_obj_align(ribbonDayNames[i], LV_ALIGN_TOP_MID, 0, 2);

        // Day Number Label
        ribbonDayNumbers[i] = lv_label_create(ribbonDayContainers[i]);
        lv_label_set_text(ribbonDayNumbers[i], "1");
        CydTheme::applyTextFont(ribbonDayNumbers[i], CydTheme::getFont14(), CydTheme::getTextSecondary());
        lv_obj_align(ribbonDayNumbers[i], LV_ALIGN_BOTTOM_MID, 0, -2);
    }
}

void HomeScreen::createWeatherCard(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 230, 131);
    lv_obj_align(card, LV_ALIGN_TOP_RIGHT, -6, 6);
    CydTheme::applyCardStyle(card);

    // Weather Icon using custom Weather Icons font
    objWeatherIconCanvas = lv_label_create(card);
    lv_obj_set_style_text_font(objWeatherIconCanvas, CydTheme::getWeatherFont24(), 0);
    lv_obj_set_style_text_color(objWeatherIconCanvas, CydTheme::getGoldColor(), 0);
    lv_label_set_text(objWeatherIconCanvas, "\uF00D"); // Default to wi-day-sunny
    lv_obj_align(objWeatherIconCanvas, LV_ALIGN_TOP_LEFT, 6, 4);

    // Dynamic temperature label
    lblWeatherTemp = lv_label_create(card);
    lv_label_set_text(lblWeatherTemp, "28°C");
    CydTheme::applyTextFont(lblWeatherTemp, CydTheme::getFont24(), CydTheme::getTextPrimary());
    lv_obj_align(lblWeatherTemp, LV_ALIGN_TOP_LEFT, 50, 4);

    // Condition text
    lblWeatherCond = lv_label_create(card);
    lv_label_set_text(lblWeatherCond, "Nắng Đẹp / Ít Mây");
    CydTheme::applyTextFont(lblWeatherCond, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblWeatherCond, LV_ALIGN_TOP_LEFT, 4, 38);

    // Right-hand 4-row weather details parameters table
    lv_obj_t* paramTable = lv_obj_create(card);
    lv_obj_set_size(paramTable, 100, 118);
    lv_obj_align(paramTable, LV_ALIGN_TOP_RIGHT, 4, -4);
    lv_obj_set_style_bg_opa(paramTable, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(paramTable, 0, 0);
    lv_obj_set_style_pad_all(paramTable, 0, 0);
    lv_obj_clear_flag(paramTable, LV_OBJ_FLAG_SCROLLABLE);

    lblWeatherFeelsLike = lv_label_create(paramTable);
    lv_label_set_text(lblWeatherFeelsLike, "Cảm giác: 30°");
    CydTheme::applyTextFont(lblWeatherFeelsLike, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblWeatherFeelsLike, LV_ALIGN_TOP_LEFT, 0, 8);

    lblWeatherParamHumid = lv_label_create(paramTable);
    lv_label_set_text(lblWeatherParamHumid, "Độ ẩm: 65%");
    CydTheme::applyTextFont(lblWeatherParamHumid, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblWeatherParamHumid, LV_ALIGN_TOP_LEFT, 0, 30);

    lblWeatherParamWind = lv_label_create(paramTable);
    lv_label_set_text(lblWeatherParamWind, "Gió: 8 km/h");
    CydTheme::applyTextFont(lblWeatherParamWind, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblWeatherParamWind, LV_ALIGN_TOP_LEFT, 0, 52);

    lblWeatherParamUv = lv_label_create(paramTable);
    lv_label_set_text(lblWeatherParamUv, "Chỉ số UV: 6");
    CydTheme::applyTextFont(lblWeatherParamUv, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblWeatherParamUv, LV_ALIGN_TOP_LEFT, 0, 74);
}

void HomeScreen::createGoldCard(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 230, 131);
    lv_obj_align(card, LV_ALIGN_BOTTOM_LEFT, 6, -6);
    CydTheme::applyCardStyle(card);

    // Title label
    lv_obj_t* lblTitle = lv_label_create(card);
    lv_label_set_text(lblTitle, "Giá Vàng SJC (x1000 đ)");
    CydTheme::applyTextFont(lblTitle, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    // Left Column: BUY
    lv_obj_t* lblBuyTitle = lv_label_create(card);
    lv_label_set_text(lblBuyTitle, "MUA VÀO");
    CydTheme::applyTextFont(lblBuyTitle, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblBuyTitle, LV_ALIGN_TOP_LEFT, 10, 28);

    lblGoldBuy = lv_label_create(card);
    lv_label_set_text(lblGoldBuy, "82,500");
    CydTheme::applyTextFont(lblGoldBuy, CydTheme::getFont20(), CydTheme::getTextPrimary());
    lv_obj_align(lblGoldBuy, LV_ALIGN_TOP_LEFT, 10, 48);

    lblGoldBuyDelta = lv_label_create(card);
    lv_label_set_text(lblGoldBuyDelta, "+500");
    CydTheme::applyTextFont(lblGoldBuyDelta, CydTheme::getFont12(), CydTheme::getSuccessColor());
    lv_obj_align(lblGoldBuyDelta, LV_ALIGN_TOP_LEFT, 10, 78);

    // Right Column: SELL
    lv_obj_t* lblSellTitle = lv_label_create(card);
    lv_label_set_text(lblSellTitle, "BÁN RA");
    CydTheme::applyTextFont(lblSellTitle, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblSellTitle, LV_ALIGN_TOP_RIGHT, -20, 28);

    lblGoldSell = lv_label_create(card);
    lv_label_set_text(lblGoldSell, "85,000");
    CydTheme::applyTextFont(lblGoldSell, CydTheme::getFont20(), CydTheme::getTextPrimary());
    lv_obj_align(lblGoldSell, LV_ALIGN_TOP_RIGHT, -20, 48);

    lblGoldSellDelta = lv_label_create(card);
    lv_label_set_text(lblGoldSellDelta, "-200");
    CydTheme::applyTextFont(lblGoldSellDelta, CydTheme::getFont12(), CydTheme::getDangerColor());
    lv_obj_align(lblGoldSellDelta, LV_ALIGN_TOP_RIGHT, -20, 78);
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
    const char* fuelNames[4] = {"RON 95-III", "E5 RON 92", "Dầu Diesel", "Dầu Hỏa"};
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
        lv_label_set_text(prices[i], "0");
        CydTheme::applyTextFont(prices[i], CydTheme::getFont12(), CydTheme::getTextPrimary());
        lv_obj_align(prices[i], LV_ALIGN_TOP_RIGHT, -56, yOffset);

        // Delta
        deltas[i] = lv_label_create(card);
        lv_label_set_text(deltas[i], "0");
        CydTheme::applyTextFont(deltas[i], CydTheme::getFont12(), CydTheme::getTextSecondary());
        lv_obj_align(deltas[i], LV_ALIGN_TOP_RIGHT, 0, yOffset);
    }

    // Save bindings
    lblFuelRon95Price = prices[0];
    lblFuelRon95Delta = deltas[0];
    lblFuelRon92Price = prices[1];
    lblFuelRon92Delta = deltas[1];
    lblFuelDieselPrice = prices[2];
    lblFuelDieselDelta = deltas[2];
    lblFuelKerosenePrice = prices[3];
    lblFuelKeroseneDelta = deltas[3];
}

// --- Dynamic Setter Updates ---

void HomeScreen::updateTime(const char* timeStr, const char* secondsStr, const char* dateStr, bool isAm) {
    if (lblClockTime) lv_label_set_text(lblClockTime, timeStr);
    if (lblClockSeconds) lv_label_set_text(lblClockSeconds, secondsStr);
    if (lblClockDate) lv_label_set_text(lblClockDate, dateStr);
    if (lblClockAmPm) lv_label_set_text(lblClockAmPm, isAm ? "SA" : "CH");
}

void HomeScreen::updateLunarCalendar(const char* lunarDayStr, const char* lunarInfoStr) {
    if (lblLunarDate) lv_label_set_text(lblLunarDate, lunarDayStr);
    if (lblLunarInfo) lv_label_set_text(lblLunarInfo, lunarInfoStr);
}

void HomeScreen::updateCalendarRibbon(int activeDayIndex, const int* dayNumbers) {
    for (int i = 0; i < 7; i++) {
        char buf[8];
        sprintf(buf, "%d", dayNumbers[i]);
        lv_label_set_text(ribbonDayNumbers[i], buf);

        if (i == activeDayIndex) {
            // Selected active day: colored container + white text
            lv_obj_set_style_bg_color(ribbonDayContainers[i], CydTheme::getAccentColor(), 0);
            lv_obj_set_style_bg_opa(ribbonDayContainers[i], LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(ribbonDayNumbers[i], CydTheme::getWhiteColor(), 0);
            lv_obj_set_style_text_color(ribbonDayNames[i], CydTheme::getTextSecondary(), 0);
        } else {
            // Idle day: transparent container + secondary text
            lv_obj_set_style_bg_opa(ribbonDayContainers[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(ribbonDayNumbers[i], CydTheme::getTextSecondary(), 0);
            lv_obj_set_style_text_color(ribbonDayNames[i], CydTheme::getTextMuted(), 0);
        }
    }
}

void HomeScreen::updateWeather(int temp, const char* condition, int feelsLike, int humidity, int windSpeed, int uvIndex) {
    char buf[32];
    
    // Custom weather icon mapping using dedicated Weather Icons font
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
    lv_label_set_text(lblWeatherCond, condition);

    sprintf(buf, "Cảm giác: %d°", feelsLike);
    lv_label_set_text(lblWeatherFeelsLike, buf);

    sprintf(buf, "Độ ẩm: %d%%", humidity);
    lv_label_set_text(lblWeatherParamHumid, buf);

    sprintf(buf, "Gió: %d km/h", windSpeed);
    lv_label_set_text(lblWeatherParamWind, buf);

    sprintf(buf, "Chỉ số UV: %d", uvIndex);
    lv_label_set_text(lblWeatherParamUv, buf);
}

void HomeScreen::updateGoldPrices(int buySJC, int sellSJC, int buyDelta, int sellDelta) {
    char buf[16];
    
    // Format values (e.g. 82.5m is represented in thousands)
    sprintf(buf, "%d,%03d", buySJC / 1000, buySJC % 1000);
    lv_label_set_text(lblGoldBuy, buf);

    sprintf(buf, "%d,%03d", sellSJC / 1000, sellSJC % 1000);
    lv_label_set_text(lblGoldSell, buf);

    // Delta buy
    if (buyDelta >= 0) {
        sprintf(buf, "+%d", buyDelta);
        lv_label_set_text(lblGoldBuyDelta, buf);
        lv_obj_set_style_text_color(lblGoldBuyDelta, CydTheme::getSuccessColor(), 0);
    } else {
        sprintf(buf, "%d", buyDelta);
        lv_label_set_text(lblGoldBuyDelta, buf);
        lv_obj_set_style_text_color(lblGoldBuyDelta, CydTheme::getDangerColor(), 0);
    }

    // Delta sell
    if (sellDelta >= 0) {
        sprintf(buf, "+%d", sellDelta);
        lv_label_set_text(lblGoldSellDelta, buf);
        lv_obj_set_style_text_color(lblGoldSellDelta, CydTheme::getSuccessColor(), 0);
    } else {
        sprintf(buf, "%d", sellDelta);
        lv_label_set_text(lblGoldSellDelta, buf);
        lv_obj_set_style_text_color(lblGoldSellDelta, CydTheme::getDangerColor(), 0);
    }
}

void HomeScreen::updateFuelPrices(int ron95, int ron92, int diesel, int kerosene, int ron95Delta, int ron92Delta, int dieselDelta, int keroseneDelta) {
    char priceBuf[16];
    char deltaBuf[16];
    
    auto applyRow = [](lv_obj_t* lblPrice, lv_obj_t* lblDelta, int price, int delta) {
        char pBuf[16];
        char dBuf[16];
        
        sprintf(pBuf, "%d", price);
        lv_label_set_text(lblPrice, pBuf);

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
    applyRow(lblFuelKerosenePrice, lblFuelKeroseneDelta, kerosene, keroseneDelta);
}