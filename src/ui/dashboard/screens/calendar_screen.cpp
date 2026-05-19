#include "calendar_screen.h"
#include "../cyd_theme.h"
#include <stdio.h>

CalendarScreen::CalendarScreen(lv_obj_t* parent) {
    // 1. Create root screen container
    rootContainer = lv_obj_create(parent);
    lv_obj_set_size(rootContainer, 480, 282);
    lv_obj_set_style_bg_opa(rootContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rootContainer, 0, 0);
    lv_obj_set_style_pad_all(rootContainer, 0, 0);
    lv_obj_clear_flag(rootContainer, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Build left and right layout panes
    createCalendarPane(rootContainer);
    createTimelinePane(rootContainer);
}

void CalendarScreen::createCalendarPane(lv_obj_t* parent) {
    lv_obj_t* leftCard = lv_obj_create(parent);
    lv_obj_set_size(leftCard, 230, 270);
    lv_obj_align(leftCard, LV_ALIGN_TOP_LEFT, 6, 6);
    CydTheme::applyCardStyle(leftCard);

    // 1. Header with Month / Year & toggle buttons
    lblMonthYear = lv_label_create(leftCard);
    lv_label_set_text(lblMonthYear, "Tháng 05, 2026");
    CydTheme::applyTextFont(lblMonthYear, CydTheme::getFont14(), CydTheme::getTextPrimary());
    lv_obj_align(lblMonthYear, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t* btnPrev = lv_btn_create(leftCard);
    lv_obj_set_size(btnPrev, 24, 20);
    lv_obj_align(btnPrev, LV_ALIGN_TOP_LEFT, 0, -2);
    CydTheme::applyButtonStyle(btnPrev, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_t* lblPrev = lv_label_create(btnPrev);
    lv_label_set_text(lblPrev, "<");
    lv_obj_center(lblPrev);

    lv_obj_t* btnNext = lv_btn_create(leftCard);
    lv_obj_set_size(btnNext, 24, 20);
    lv_obj_align(btnNext, LV_ALIGN_TOP_RIGHT, 0, -2);
    CydTheme::applyButtonStyle(btnNext, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_t* lblNext = lv_label_create(btnNext);
    lv_label_set_text(lblNext, ">");
    lv_obj_center(lblNext);

    // 2. Day Header Row (T2, T3... CN)
    lv_obj_t* dayHeader = lv_obj_create(leftCard);
    lv_obj_set_size(dayHeader, 210, 20);
    lv_obj_align(dayHeader, LV_ALIGN_TOP_MID, 0, 22);
    lv_obj_set_style_bg_opa(dayHeader, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(dayHeader, 0, 0);
    lv_obj_set_style_pad_all(dayHeader, 0, 0);
    lv_obj_clear_flag(dayHeader, LV_OBJ_FLAG_SCROLLABLE);

    const char* daysName[7] = {"Hai", "Ba", "Tư", "Năm", "Sáu", "Bảy", "CN"};
    for (int i = 0; i < 7; i++) {
        lv_obj_t* lblDay = lv_label_create(dayHeader);
        lv_label_set_text(lblDay, daysName[i]);
        CydTheme::applyTextFont(lblDay, CydTheme::getFont12(), CydTheme::getTextMuted());
        lv_obj_align(lblDay, LV_ALIGN_LEFT_MID, i * 30 + 4, 0);
    }

    // 3. 42 Days grid matrix container (Flex/Grid style alignment)
    calendarGrid = lv_obj_create(leftCard);
    lv_obj_set_size(calendarGrid, 210, 134);
    lv_obj_align(calendarGrid, LV_ALIGN_TOP_MID, 0, 42);
    lv_obj_set_style_bg_opa(calendarGrid, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(calendarGrid, 0, 0);
    lv_obj_set_style_pad_all(calendarGrid, 0, 0);
    lv_obj_clear_flag(calendarGrid, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 42; i++) {
        int row = i / 7;
        int col = i % 7;
        int xPos = col * 30;
        int yPos = row * 22;

        // Individual cell container
        cellsContainer[i] = lv_obj_create(calendarGrid);
        lv_obj_set_size(cellsContainer[i], 26, 20);
        lv_obj_set_pos(cellsContainer[i], xPos + 2, yPos);
        lv_obj_set_style_radius(cellsContainer[i], 4, 0);
        lv_obj_set_style_bg_opa(cellsContainer[i], LV_OPA_TRANSP, 0);
        lv_obj_set_style_border_width(cellsContainer[i], 0, 0);
        lv_obj_set_style_pad_all(cellsContainer[i], 0, 0);
        lv_obj_clear_flag(cellsContainer[i], LV_OBJ_FLAG_SCROLLABLE);

        // Day Number Label
        cellNumbers[i] = lv_label_create(cellsContainer[i]);
        lv_label_set_text(cellNumbers[i], "");
        CydTheme::applyTextFont(cellNumbers[i], CydTheme::getFont12(), CydTheme::getTextSecondary());
        lv_obj_align(cellNumbers[i], LV_ALIGN_TOP_MID, 0, 0);

        // Under-date Event dot indication box
        cellDotsBox[i] = lv_obj_create(cellsContainer[i]);
        lv_obj_set_size(cellDotsBox[i], 4, 4);
        lv_obj_align(cellDotsBox[i], LV_ALIGN_BOTTOM_MID, 0, -1);
        lv_obj_set_style_radius(cellDotsBox[i], LV_RADIUS_CIRCLE, 0);
        lv_obj_set_style_border_width(cellDotsBox[i], 0, 0);
        lv_obj_add_flag(cellDotsBox[i], LV_OBJ_FLAG_HIDDEN); // hidden by default
    }

    // 4. Cloud Calendars Sync Status (Footer)
    lv_obj_t* syncFooter = lv_obj_create(leftCard);
    lv_obj_set_size(syncFooter, 210, 48);
    lv_obj_align(syncFooter, LV_ALIGN_BOTTOM_MID, 0, 4);
    lv_obj_set_style_bg_color(syncFooter, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_bg_opa(syncFooter, LV_OPA_30, 0);
    lv_obj_set_style_border_width(syncFooter, 0, 0);
    lv_obj_set_style_radius(syncFooter, 8, 0);
    lv_obj_set_style_pad_all(syncFooter, 4, 0);
    lv_obj_clear_flag(syncFooter, LV_OBJ_FLAG_SCROLLABLE);

    // Google row
    swGoogle = lv_switch_create(syncFooter);
    lv_obj_set_size(swGoogle, 26, 14);
    lv_obj_align(swGoogle, LV_ALIGN_TOP_LEFT, 4, 4);
    CydTheme::applySliderStyle(swGoogle, CydTheme::getAccentGlowColor());
    
    lblGoogleStatus = lv_label_create(syncFooter);
    lv_label_set_text(lblGoogleStatus, "Google Calendar: Off");
    CydTheme::applyTextFont(lblGoogleStatus, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblGoogleStatus, LV_ALIGN_TOP_LEFT, 36, 4);

    // Apple row
    swApple = lv_switch_create(syncFooter);
    lv_obj_set_size(swApple, 26, 14);
    lv_obj_align(swApple, LV_ALIGN_BOTTOM_LEFT, 4, -4);
    CydTheme::applySliderStyle(swApple, CydTheme::getBlueColor());
    
    lblAppleStatus = lv_label_create(syncFooter);
    lv_label_set_text(lblAppleStatus, "Apple Calendar: Off");
    CydTheme::applyTextFont(lblAppleStatus, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblAppleStatus, LV_ALIGN_BOTTOM_LEFT, 36, -4);
}

void CalendarScreen::createTimelinePane(lv_obj_t* parent) {
    lv_obj_t* rightCard = lv_obj_create(parent);
    lv_obj_set_size(rightCard, 230, 270);
    lv_obj_align(rightCard, LV_ALIGN_TOP_RIGHT, -6, 6);
    CydTheme::applyCardStyle(rightCard);

    // 1. Right Header: Current active date text details
    lblEventDayNum = lv_label_create(rightCard);
    lv_label_set_text(lblEventDayNum, "15");
    CydTheme::applyTextFont(lblEventDayNum, CydTheme::getFont24(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblEventDayNum, LV_ALIGN_TOP_LEFT, 0, 0);

    lblEventDayName = lv_label_create(rightCard);
    lv_label_set_text(lblEventDayName, "Thứ Năm");
    CydTheme::applyTextFont(lblEventDayName, CydTheme::getFont14(), CydTheme::getTextPrimary());
    lv_obj_align(lblEventDayName, LV_ALIGN_TOP_LEFT, 36, 2);

    lblEventDate = lv_label_create(rightCard);
    lv_label_set_text(lblEventDate, "15 Tháng 05, 2025");
    CydTheme::applyTextFont(lblEventDate, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblEventDate, LV_ALIGN_TOP_LEFT, 36, 18);

    // 2. Events scrollable timeline lists
    eventsScrollContainer = lv_obj_create(rightCard);
    lv_obj_set_size(eventsScrollContainer, 210, 210);
    lv_obj_align(eventsScrollContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(eventsScrollContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(eventsScrollContainer, 0, 0);
    lv_obj_set_style_pad_all(eventsScrollContainer, 0, 0);
    lv_obj_set_layout(eventsScrollContainer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(eventsScrollContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(eventsScrollContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(eventsScrollContainer, 6, 0);
    lv_obj_clear_flag(eventsScrollContainer, LV_OBJ_FLAG_SCROLL_ELASTIC);
}

// --- Dynamic Setter Updates ---

void CalendarScreen::updateMonthYearHeader(const char* monthYearStr) {
    if (lblMonthYear) lv_label_set_text(lblMonthYear, monthYearStr);
}

void CalendarScreen::updateCalendarDays(int startDayOfWeek, int daysInMonth, int activeDay, const uint32_t* dotColorsMatrix) {
    // Fill previous empty cells
    for (int i = 0; i < startDayOfWeek; i++) {
        lv_label_set_text(cellNumbers[i], "");
        lv_obj_set_style_bg_opa(cellsContainer[i], LV_OPA_TRANSP, 0);
        lv_obj_add_flag(cellDotsBox[i], LV_OBJ_FLAG_HIDDEN);
    }

    // Fill days
    for (int day = 1; day <= daysInMonth; day++) {
        int cellIdx = startDayOfWeek + day - 1;
        if (cellIdx >= 42) break;

        char buf[8];
        sprintf(buf, "%d", day);
        lv_label_set_text(cellNumbers[cellIdx], buf);

        // Highlight selected day
        if (day == activeDay) {
            lv_obj_set_style_bg_color(cellsContainer[cellIdx], CydTheme::getAccentColor(), 0);
            lv_obj_set_style_bg_opa(cellsContainer[cellIdx], LV_OPA_COVER, 0);
            lv_obj_set_style_text_color(cellNumbers[cellIdx], CydTheme::getWhiteColor(), 0);
        } else {
            lv_obj_set_style_bg_opa(cellsContainer[cellIdx], LV_OPA_TRANSP, 0);
            lv_obj_set_style_text_color(cellNumbers[cellIdx], CydTheme::getTextSecondary(), 0);
        }

        // Draw event scheduled dot indicator
        uint32_t dotColor = dotColorsMatrix[cellIdx];
        if (dotColor != 0) {
            lv_obj_clear_flag(cellDotsBox[cellIdx], LV_OBJ_FLAG_HIDDEN);
            lv_obj_set_style_bg_color(cellDotsBox[cellIdx], lv_color_hex(dotColor), 0);
            lv_obj_set_style_bg_opa(cellDotsBox[cellIdx], LV_OPA_COVER, 0);
        } else {
            lv_obj_add_flag(cellDotsBox[cellIdx], LV_OBJ_FLAG_HIDDEN);
        }
    }

    // Fill post empty cells
    int totalFilled = startDayOfWeek + daysInMonth;
    for (int i = totalFilled; i < 42; i++) {
        lv_label_set_text(cellNumbers[i], "");
        lv_obj_set_style_bg_opa(cellsContainer[i], LV_OPA_TRANSP, 0);
        lv_obj_add_flag(cellDotsBox[i], LV_OBJ_FLAG_HIDDEN);
    }
}

void CalendarScreen::updateSyncStatus(bool googleConnected, bool appleConnected) {
    if (googleConnected) {
        lv_obj_add_state(swGoogle, LV_STATE_CHECKED);
        lv_label_set_text(lblGoogleStatus, "Google Calendar: Live");
    } else {
        lv_obj_clear_state(swGoogle, LV_STATE_CHECKED);
        lv_label_set_text(lblGoogleStatus, "Google Calendar: Off");
    }

    if (appleConnected) {
        lv_obj_add_state(swApple, LV_STATE_CHECKED);
        lv_label_set_text(lblAppleStatus, "Apple Calendar: Live");
    } else {
        lv_obj_clear_state(swApple, LV_STATE_CHECKED);
        lv_label_set_text(lblAppleStatus, "Apple Calendar: Off");
    }
}

void CalendarScreen::clearEvents() {
    lv_obj_clean(eventsScrollContainer);
}

void CalendarScreen::addEvent(const CalendarEvent& event) {
    // 1. Create main timeline card item
    lv_obj_t* item = lv_obj_create(eventsScrollContainer);
    lv_obj_set_size(item, 206, 46);
    lv_obj_set_style_bg_color(item, CydTheme::getCardColor(), 0);
    lv_obj_set_style_bg_opa(item, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(item, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_border_width(item, 1, 0);
    lv_obj_set_style_radius(item, 6, 0);
    lv_obj_set_style_pad_all(item, 0, 0);
    lv_obj_clear_flag(item, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Category left side indicator bar
    lv_obj_t* categoryBar = lv_obj_create(item);
    lv_obj_set_size(categoryBar, 4, 46);
    lv_obj_align(categoryBar, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_bg_color(categoryBar, event.color, 0);
    lv_obj_set_style_bg_opa(categoryBar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(categoryBar, 0, 0);
    lv_obj_set_style_radius(categoryBar, 0, 0);

    // 3. Time label slot on the left side
    lv_obj_t* lblTime = lv_label_create(item);
    lv_label_set_text(lblTime, event.timeStr);
    CydTheme::applyTextFont(lblTime, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblTime, LV_ALIGN_LEFT_MID, 10, 0);
    
    // Support two-line times by reducing vertical alignment
    lv_obj_set_style_text_align(lblTime, LV_TEXT_ALIGN_CENTER, 0);

    // 4. Content pane for text info on the right side
    lv_obj_t* textPane = lv_obj_create(item);
    lv_obj_set_size(textPane, 142, 44);
    lv_obj_align(textPane, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_set_style_bg_opa(textPane, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(textPane, 0, 0);
    lv_obj_set_style_pad_all(textPane, 2, 0);
    lv_obj_clear_flag(textPane, LV_OBJ_FLAG_SCROLLABLE);

    // Song/Activity Title
    lv_obj_t* lblTitle = lv_label_create(textPane);
    lv_label_set_text(lblTitle, event.title);
    CydTheme::applyTextFont(lblTitle, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_align(lblTitle, LV_ALIGN_TOP_LEFT, 0, 2);
    lv_label_set_long_mode(lblTitle, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lblTitle, 138);

    // Room Location Details
    lv_obj_t* lblLoc = lv_label_create(textPane);
    lv_label_set_text(lblLoc, event.location);
    CydTheme::applyTextFont(lblLoc, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblLoc, LV_ALIGN_BOTTOM_LEFT, 0, -2);
    lv_label_set_long_mode(lblLoc, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lblLoc, 90);

    // Duration Capsule badge (Top right)
    lv_obj_t* durationBadge = lv_obj_create(textPane);
    lv_obj_set_size(durationBadge, 42, 16);
    lv_obj_align(durationBadge, LV_ALIGN_BOTTOM_RIGHT, -2, -2);
    lv_obj_set_style_bg_color(durationBadge, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_bg_opa(durationBadge, LV_OPA_COVER, 0);
    lv_obj_set_style_radius(durationBadge, 4, 0);
    lv_obj_set_style_border_width(durationBadge, 0, 0);
    lv_obj_set_style_pad_all(durationBadge, 0, 0);
    lv_obj_clear_flag(durationBadge, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lblDur = lv_label_create(durationBadge);
    lv_label_set_text(lblDur, event.durationStr);
    CydTheme::applyTextFont(lblDur, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_center(lblDur);
}