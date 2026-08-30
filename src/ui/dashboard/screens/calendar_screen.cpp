#include "calendar_screen.h"
#include "../cyd_theme.h"
#include "log.h"
#include <stdio.h>
#include <time.h>

static void btn_nav_event_cb(lv_event_t* e) {
    CalendarScreen* screen = (CalendarScreen*)lv_event_get_user_data(e);
    lv_obj_t* target = lv_event_get_current_target(e);
    if (!screen) return;

    if (target == screen->btnPrevMonth) {
        LOG_D("Calendar", "Clicked: Prev Month");
        screen->onPrevMonth();
    } else if (target == screen->btnNextMonth) {
        LOG_D("Calendar", "Clicked: Next Month");
        screen->onNextMonth();
    } else if (target == screen->btnPrevYear) {
        LOG_D("Calendar", "Clicked: Prev Year");
        screen->onPrevYear();
    } else if (target == screen->btnNextYear) {
        LOG_D("Calendar", "Clicked: Next Year");
        screen->onNextYear();
    } else if (target == screen->btnToday) {
        LOG_D("Calendar", "Clicked: Today");
        screen->onTodayClick();
    }
}

static void cell_click_event_cb(lv_event_t* e) {
    CalendarScreen* screen = (CalendarScreen*)lv_event_get_user_data(e);
    int cellIndex = (int)(intptr_t)lv_obj_get_user_data(lv_event_get_current_target(e));
    if (screen) {
        LOG_D("Calendar", "Clicked cell index: %d", cellIndex);
        screen->onCellClick(cellIndex);
    }
}

CalendarScreen::CalendarScreen(lv_obj_t* parent) :
    viewYear(2026),
    viewMonth(8),
    selectedDay(30),
    realTodayYear(2026),
    realTodayMonth(8),
    realTodayDay(30)
{
    // 1. Create root screen container (480 x 282)
    rootContainer = lv_obj_create(parent);
    lv_obj_set_size(rootContainer, 480, 282);
    lv_obj_set_style_bg_opa(rootContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rootContainer, 0, 0);
    lv_obj_set_style_pad_all(rootContainer, 0, 0);
    lv_obj_clear_flag(rootContainer, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Build left and right layout panes
    createCalendarPane(rootContainer);
    createDetailPane(rootContainer);

    // 3. Initial refresh
    refreshCalendar();
}

CalendarScreen::~CalendarScreen() {
    if (rootContainer) {
        lv_obj_del(rootContainer);
        rootContainer = nullptr;
    }
}

int CalendarScreen::getDaysInMonth(int year, int month) {
    if (month == 2) {
        bool isLeap = (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0));
        return isLeap ? 29 : 28;
    }
    if (month == 4 || month == 6 || month == 9 || month == 11) return 30;
    return 31;
}

int CalendarScreen::getFirstDayOfWeek(int year, int month) {
    struct tm timeinfo = {0};
    timeinfo.tm_year = year - 1900;
    timeinfo.tm_mon = month - 1;
    timeinfo.tm_mday = 1;
    mktime(&timeinfo);
    // tm_wday: 0 = Sun, 1 = Mon, ..., 6 = Sat -> Convert to 0 = Mon, 1 = Tue, ..., 6 = Sun
    int w = timeinfo.tm_wday;
    return (w == 0) ? 6 : (w - 1);
}

void CalendarScreen::createCalendarPane(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 318, 270);
    lv_obj_align(card, LV_ALIGN_TOP_LEFT, 6, 6);
    CydTheme::applyCardStyle(card);

    // --- 1. Header with Prev/Next buttons & Month/Year title ---
    btnPrevYear = lv_btn_create(card);
    lv_obj_set_size(btnPrevYear, 28, 26);
    lv_obj_align(btnPrevYear, LV_ALIGN_TOP_LEFT, 0, 0);
    CydTheme::applyButtonStyle(btnPrevYear, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_set_style_pad_all(btnPrevYear, 0, 0);
    lv_obj_set_ext_click_area(btnPrevYear, 6);
    lv_obj_t* lblPY = lv_label_create(btnPrevYear);
    lv_label_set_text(lblPY, "<<");
    lv_obj_clear_flag(lblPY, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblPY);
    lv_obj_add_event_cb(btnPrevYear, btn_nav_event_cb, LV_EVENT_CLICKED, this);

    btnPrevMonth = lv_btn_create(card);
    lv_obj_set_size(btnPrevMonth, 26, 26);
    lv_obj_align(btnPrevMonth, LV_ALIGN_TOP_LEFT, 30, 0);
    CydTheme::applyButtonStyle(btnPrevMonth, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_set_style_pad_all(btnPrevMonth, 0, 0);
    lv_obj_set_ext_click_area(btnPrevMonth, 6);
    lv_obj_t* lblPM = lv_label_create(btnPrevMonth);
    lv_label_set_text(lblPM, "<");
    lv_obj_clear_flag(lblPM, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblPM);
    lv_obj_add_event_cb(btnPrevMonth, btn_nav_event_cb, LV_EVENT_CLICKED, this);

    lblMonthYearTitle = lv_label_create(card);
    lv_obj_set_width(lblMonthYearTitle, 114);
    lv_label_set_text(lblMonthYearTitle, "Tháng 08, 2026");
    CydTheme::applyTextFont(lblMonthYearTitle, CydTheme::getFont14(), CydTheme::getTextPrimary());
    lv_obj_set_style_text_align(lblMonthYearTitle, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_clear_flag(lblMonthYearTitle, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(lblMonthYearTitle, LV_ALIGN_TOP_LEFT, 58, 3);

    btnNextMonth = lv_btn_create(card);
    lv_obj_set_size(btnNextMonth, 26, 26);
    lv_obj_align(btnNextMonth, LV_ALIGN_TOP_LEFT, 174, 0);
    CydTheme::applyButtonStyle(btnNextMonth, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_set_style_pad_all(btnNextMonth, 0, 0);
    lv_obj_set_ext_click_area(btnNextMonth, 6);
    lv_obj_t* lblNM = lv_label_create(btnNextMonth);
    lv_label_set_text(lblNM, ">");
    lv_obj_clear_flag(lblNM, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblNM);
    lv_obj_add_event_cb(btnNextMonth, btn_nav_event_cb, LV_EVENT_CLICKED, this);

    btnNextYear = lv_btn_create(card);
    lv_obj_set_size(btnNextYear, 28, 26);
    lv_obj_align(btnNextYear, LV_ALIGN_TOP_LEFT, 202, 0);
    CydTheme::applyButtonStyle(btnNextYear, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_set_style_pad_all(btnNextYear, 0, 0);
    lv_obj_set_ext_click_area(btnNextYear, 6);
    lv_obj_t* lblNY = lv_label_create(btnNextYear);
    lv_label_set_text(lblNY, ">>");
    lv_obj_clear_flag(lblNY, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblNY);
    lv_obj_add_event_cb(btnNextYear, btn_nav_event_cb, LV_EVENT_CLICKED, this);

    btnToday = lv_btn_create(card);
    lv_obj_set_size(btnToday, 64, 26);
    lv_obj_align(btnToday, LV_ALIGN_TOP_RIGHT, 0, 0);
    CydTheme::applyButtonStyle(btnToday, CydTheme::getAccentGlowColor(), lv_color_white());
    lv_obj_set_style_pad_all(btnToday, 0, 0);
    lv_obj_set_ext_click_area(btnToday, 6);
    lv_obj_t* lblToday = lv_label_create(btnToday);
    lv_label_set_text(lblToday, "Hôm nay");
    CydTheme::applyTextFont(lblToday, CydTheme::getFont12(), lv_color_white());
    lv_obj_clear_flag(lblToday, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblToday);
    lv_obj_add_event_cb(btnToday, btn_nav_event_cb, LV_EVENT_CLICKED, this);

    // --- 2. Weekday Header (T2 .. CN) ---
    const char* daysName[7] = {"T2", "T3", "T4", "T5", "T6", "T7", "CN"};
    for (int i = 0; i < 7; i++) {
        lv_obj_t* lblDay = lv_label_create(card);
        lv_obj_set_width(lblDay, 41);
        lv_label_set_text(lblDay, daysName[i]);
        if (i == 6) { // CN
            CydTheme::applyTextFont(lblDay, CydTheme::getFont12(), CydTheme::getDangerColor());
        } else if (i == 5) { // T7
            CydTheme::applyTextFont(lblDay, CydTheme::getFont12(), CydTheme::getGoldColor());
        } else {
            CydTheme::applyTextFont(lblDay, CydTheme::getFont12(), CydTheme::getTextMuted());
        }
        lv_obj_set_style_text_align(lblDay, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_clear_flag(lblDay, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_align(lblDay, LV_ALIGN_TOP_LEFT, i * 43, 30);
    }

    // --- 3. 42 Days Matrix (7 cols x 6 rows) ---
    // Sử dụng 1 lv_label duy nhất cho mỗi ô thay vì lồng 3 lv_obj
    for (int i = 0; i < 42; i++) {
        int row = i / 7;
        int col = i % 7;
        int xPos = col * 43;
        int yPos = 48 + row * 34;

        cellLabels[i] = lv_label_create(card);
        lv_obj_set_size(cellLabels[i], 41, 32);
        lv_obj_set_pos(cellLabels[i], xPos, yPos);
        lv_obj_set_style_radius(cellLabels[i], 6, 0);
        lv_obj_set_style_pad_top(cellLabels[i], 2, 0);
        lv_obj_set_style_pad_hor(cellLabels[i], 0, 0);
        lv_obj_set_style_text_align(cellLabels[i], LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_style_text_line_space(cellLabels[i], -1, 0);
        CydTheme::applyTextFont(cellLabels[i], CydTheme::getFont12(), CydTheme::getTextPrimary());
        lv_obj_clear_flag(cellLabels[i], LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(cellLabels[i], LV_OBJ_FLAG_CLICKABLE);
        lv_obj_set_user_data(cellLabels[i], (void*)(intptr_t)i);
        lv_obj_add_event_cb(cellLabels[i], cell_click_event_cb, LV_EVENT_CLICKED, this);
    }
}

void CalendarScreen::createDetailPane(lv_obj_t* parent) {
    lv_obj_t* card = lv_obj_create(parent);
    lv_obj_set_size(card, 144, 270);
    lv_obj_align(card, LV_ALIGN_TOP_RIGHT, -6, 6);
    CydTheme::applyCardStyle(card);

    // Title
    lv_obj_t* lblTitle = lv_label_create(card);
    lv_label_set_text(lblTitle, "CHI TIẾT NGÀY");
    CydTheme::applyTextFont(lblTitle, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblTitle, LV_ALIGN_TOP_MID, 0, 0);

    // Big Solar Day
    lblDetailSolarDay = lv_label_create(card);
    lv_label_set_text(lblDetailSolarDay, "30");
    CydTheme::applyTextFont(lblDetailSolarDay, CydTheme::getFont40(), CydTheme::getTextPrimary());
    lv_obj_align(lblDetailSolarDay, LV_ALIGN_TOP_MID, 0, 16);

    // Weekday name
    lblDetailSolarWeekDay = lv_label_create(card);
    lv_label_set_text(lblDetailSolarWeekDay, "Chủ Nhật");
    CydTheme::applyTextFont(lblDetailSolarWeekDay, CydTheme::getFont14(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblDetailSolarWeekDay, LV_ALIGN_TOP_MID, 0, 58);

    // Solar Month & Year
    lblDetailSolarMonthYear = lv_label_create(card);
    lv_label_set_text(lblDetailSolarMonthYear, "30/08/2026");
    CydTheme::applyTextFont(lblDetailSolarMonthYear, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDetailSolarMonthYear, LV_ALIGN_TOP_MID, 0, 76);

    // Divider
    lv_obj_t* line = lv_obj_create(card);
    lv_obj_set_size(line, 120, 1);
    lv_obj_align(line, LV_ALIGN_TOP_MID, 0, 96);
    lv_obj_set_style_bg_color(line, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_border_width(line, 0, 0);

    // Lunar Date Header
    lv_obj_t* lblLunarH = lv_label_create(card);
    lv_label_set_text(lblLunarH, "ÂM LỊCH");
    CydTheme::applyTextFont(lblLunarH, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblLunarH, LV_ALIGN_TOP_MID, 0, 102);

    // Lunar Day / Month
    lblDetailLunarDayMonth = lv_label_create(card);
    lv_label_set_text(lblDetailLunarDayMonth, "18/07 ÂL");
    CydTheme::applyTextFont(lblDetailLunarDayMonth, CydTheme::getFont20(), CydTheme::getGoldColor());
    lv_obj_align(lblDetailLunarDayMonth, LV_ALIGN_TOP_MID, 0, 118);

    // Can Chi Day
    lblDetailLunarCanChi = lv_label_create(card);
    lv_label_set_text(lblDetailLunarCanChi, "Ngày Bính Tý");
    CydTheme::applyTextFont(lblDetailLunarCanChi, CydTheme::getFont12(), CydTheme::getTextPrimary());
    lv_obj_align(lblDetailLunarCanChi, LV_ALIGN_TOP_MID, 0, 144);

    // Can Chi Year
    lblDetailLunarYear = lv_label_create(card);
    lv_label_set_text(lblDetailLunarYear, "Năm Ất Tỵ");
    CydTheme::applyTextFont(lblDetailLunarYear, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblDetailLunarYear, LV_ALIGN_TOP_MID, 0, 162);

    // Holiday / Special Event Badge
    boxHolidayBadge = lv_obj_create(card);
    lv_obj_set_size(boxHolidayBadge, 126, 42);
    lv_obj_align(boxHolidayBadge, LV_ALIGN_BOTTOM_MID, 0, -2);
    lv_obj_set_style_bg_color(boxHolidayBadge, lv_color_make(30, 41, 59), 0);
    lv_obj_set_style_bg_opa(boxHolidayBadge, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(boxHolidayBadge, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_border_width(boxHolidayBadge, 1, 0);
    lv_obj_set_style_radius(boxHolidayBadge, 6, 0);
    lv_obj_set_style_pad_all(boxHolidayBadge, 2, 0);
    lv_obj_clear_flag(boxHolidayBadge, LV_OBJ_FLAG_SCROLLABLE);

    lblDetailHoliday = lv_label_create(boxHolidayBadge);
    lv_obj_set_width(lblDetailHoliday, 120);
    lv_label_set_text(lblDetailHoliday, "Ngày Hoàng Đạo");
    CydTheme::applyTextFont(lblDetailHoliday, CydTheme::getFont12(), CydTheme::getSuccessColor());
    lv_obj_set_style_text_align(lblDetailHoliday, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(lblDetailHoliday);
}

void CalendarScreen::setToday(int year, int month, int day) {
    if (year <= 0 || month <= 0 || day <= 0) return;

    bool changed = (realTodayYear != year || realTodayMonth != month || realTodayDay != day);
    if (!changed) return;

    bool wasInitial = (realTodayYear == 0);
    realTodayYear = year;
    realTodayMonth = month;
    realTodayDay = day;

    if (wasInitial) {
        viewYear = year;
        viewMonth = month;
        selectedDay = day;
        refreshCalendar();
    } else {
        // If user is currently looking at the current month, refresh highlighting
        if (viewYear == realTodayYear && viewMonth == realTodayMonth) {
            refreshCalendar();
        }
    }
}

void CalendarScreen::setViewMonth(int year, int month) {
    viewYear = year;
    viewMonth = month;
    int maxDays = getDaysInMonth(viewYear, viewMonth);
    if (selectedDay > maxDays) selectedDay = maxDays;
    refreshCalendar();
}

void CalendarScreen::selectDay(int day) {
    selectedDay = day;
    refreshCalendar();
}

void CalendarScreen::onPrevMonth() {
    viewMonth--;
    if (viewMonth < 1) {
        viewMonth = 12;
        viewYear--;
    }
    int maxDays = getDaysInMonth(viewYear, viewMonth);
    if (selectedDay > maxDays) selectedDay = maxDays;
    refreshCalendar();
}

void CalendarScreen::onNextMonth() {
    viewMonth++;
    if (viewMonth > 12) {
        viewMonth = 1;
        viewYear++;
    }
    int maxDays = getDaysInMonth(viewYear, viewMonth);
    if (selectedDay > maxDays) selectedDay = maxDays;
    refreshCalendar();
}

void CalendarScreen::onPrevYear() {
    viewYear--;
    int maxDays = getDaysInMonth(viewYear, viewMonth);
    if (selectedDay > maxDays) selectedDay = maxDays;
    refreshCalendar();
}

void CalendarScreen::onNextYear() {
    viewYear++;
    int maxDays = getDaysInMonth(viewYear, viewMonth);
    if (selectedDay > maxDays) selectedDay = maxDays;
    refreshCalendar();
}

void CalendarScreen::onTodayClick() {
    if (realTodayYear > 0) {
        viewYear = realTodayYear;
        viewMonth = realTodayMonth;
        selectedDay = realTodayDay;
    } else {
        viewYear = 2026;
        viewMonth = 8;
        selectedDay = 30;
    }
    refreshCalendar();
}

void CalendarScreen::onCellClick(int cellIndex) {
    if (cellIndex < 0 || cellIndex >= 42) return;
    CellData cd = cellData[cellIndex];
    if (!cd.isCurrentMonth) {
        // Switch to clicked month
        viewYear = cd.year;
        viewMonth = cd.month;
    }
    selectedDay = cd.day;
    refreshCalendar();
}

void CalendarScreen::refreshCalendar() {
    char buf[64];

    // 1. Update Title: "Tháng MM, YYYY"
    sprintf(buf, "Tháng %02d, %d", viewMonth, viewYear);
    lv_label_set_text(lblMonthYearTitle, buf);

    // 2. Compute 42 Days Matrix
    int startDayOfWeek = getFirstDayOfWeek(viewYear, viewMonth);
    int daysInCurMonth = getDaysInMonth(viewYear, viewMonth);

    int prevMonth = viewMonth - 1;
    int prevYear = viewYear;
    if (prevMonth < 1) {
        prevMonth = 12;
        prevYear--;
    }
    int daysInPrevMonth = getDaysInMonth(prevYear, prevMonth);

    int nextMonth = viewMonth + 1;
    int nextYear = viewYear;
    if (nextMonth > 12) {
        nextMonth = 1;
        nextYear++;
    }

    for (int i = 0; i < 42; i++) {
        int cellYear, cellMonth, cellDay;
        bool isCurMonth = false;

        if (i < startDayOfWeek) {
            // Previous month
            cellDay = daysInPrevMonth - startDayOfWeek + 1 + i;
            cellMonth = prevMonth;
            cellYear = prevYear;
        } else if (i >= startDayOfWeek + daysInCurMonth) {
            // Next month
            cellDay = i - (startDayOfWeek + daysInCurMonth) + 1;
            cellMonth = nextMonth;
            cellYear = nextYear;
        } else {
            // Current month
            cellDay = i - startDayOfWeek + 1;
            cellMonth = viewMonth;
            cellYear = viewYear;
            isCurMonth = true;
        }

        cellData[i].year = cellYear;
        cellData[i].month = cellMonth;
        cellData[i].day = cellDay;
        cellData[i].isCurrentMonth = isCurMonth;

        // Calculate lunar date
        LunarDate ld = getDetailedLunarDate(cellYear, cellMonth, cellDay);

        // Solar + Lunar text kết hợp 2 dòng: dòng 1 dương lịch, dòng 2 âm lịch
        char lunarStr[16];
        if (ld.day == 1) {
            snprintf(lunarStr, sizeof(lunarStr), "1/%d", ld.month);
        } else if (ld.day == 15) {
            snprintf(lunarStr, sizeof(lunarStr), "15*");
        } else {
            snprintf(lunarStr, sizeof(lunarStr), "%d", ld.day);
        }

        snprintf(buf, sizeof(buf), "%d\n%s", cellDay, lunarStr);
        lv_label_set_text(cellLabels[i], buf);

        // Color & Styling
        bool isSelected = (isCurMonth && cellDay == selectedDay);
        bool isToday = (cellYear == realTodayYear && cellMonth == realTodayMonth && cellDay == realTodayDay);

        if (isSelected) {
            lv_obj_set_style_bg_opa(cellLabels[i], LV_OPA_COVER, 0);
            lv_obj_set_style_bg_color(cellLabels[i], lv_color_make(24, 60, 120), 0);
            lv_obj_set_style_border_color(cellLabels[i], CydTheme::getAccentGlowColor(), 0);
            lv_obj_set_style_border_width(cellLabels[i], 1, 0);
            lv_obj_set_style_text_color(cellLabels[i], lv_color_white(), 0);
        } else if (isToday) {
            lv_obj_set_style_bg_opa(cellLabels[i], LV_OPA_30, 0);
            lv_obj_set_style_bg_color(cellLabels[i], CydTheme::getAccentGlowColor(), 0);
            lv_obj_set_style_border_color(cellLabels[i], CydTheme::getAccentGlowColor(), 0);
            lv_obj_set_style_border_width(cellLabels[i], 1, 0);
            lv_obj_set_style_text_color(cellLabels[i], CydTheme::getAccentGlowColor(), 0);
        } else {
            lv_obj_set_style_bg_opa(cellLabels[i], LV_OPA_TRANSP, 0);
            lv_obj_set_style_border_width(cellLabels[i], 0, 0);

            if (!isCurMonth) {
                lv_obj_set_style_text_color(cellLabels[i], CydTheme::getTextMuted(), 0);
            } else {
                int col = i % 7;
                if (col == 6) { // CN
                    lv_obj_set_style_text_color(cellLabels[i], CydTheme::getDangerColor(), 0);
                } else if (col == 5) { // T7
                    lv_obj_set_style_text_color(cellLabels[i], CydTheme::getGoldColor(), 0);
                } else {
                    lv_obj_set_style_text_color(cellLabels[i], CydTheme::getTextPrimary(), 0);
                }
            }
        }
    }

    // 3. Update Right Detail Card
    updateDetailCard();
}

void CalendarScreen::updateDetailCard() {
    char buf[64];

    // Day number
    sprintf(buf, "%d", selectedDay);
    lv_label_set_text(lblDetailSolarDay, buf);

    // Weekday name
    struct tm timeinfo = {0};
    timeinfo.tm_year = viewYear - 1900;
    timeinfo.tm_mon = viewMonth - 1;
    timeinfo.tm_mday = selectedDay;
    mktime(&timeinfo);

    const char* weekNames[7] = {"Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"};
    int wday = timeinfo.tm_wday;
    if (wday >= 0 && wday < 7) {
        lv_label_set_text(lblDetailSolarWeekDay, weekNames[wday]);
        if (wday == 0) {
            CydTheme::applyTextFont(lblDetailSolarWeekDay, CydTheme::getFont14(), CydTheme::getDangerColor());
        } else {
            CydTheme::applyTextFont(lblDetailSolarWeekDay, CydTheme::getFont14(), CydTheme::getAccentGlowColor());
        }
    }

    // Month & Year
    sprintf(buf, "%02d/%02d/%d", selectedDay, viewMonth, viewYear);
    lv_label_set_text(lblDetailSolarMonthYear, buf);

    // Lunar Date
    LunarDate ld = getDetailedLunarDate(viewYear, viewMonth, selectedDay);
    sprintf(buf, "%d/%02d ÂL", ld.day, ld.month);
    lv_label_set_text(lblDetailLunarDayMonth, buf);

    // Can Chi Day
    if (ld.dayName && strlen(ld.dayName) > 0) {
        sprintf(buf, "Ngày %s", ld.dayName);
    } else {
        sprintf(buf, "Ngày Hoàng Đạo");
    }
    lv_label_set_text(lblDetailLunarCanChi, buf);

    // Can Chi Year
    if (ld.yearName && strlen(ld.yearName) > 0) {
        sprintf(buf, "Năm %s", ld.yearName);
    } else {
        sprintf(buf, "Năm Bính Thìn");
    }
    lv_label_set_text(lblDetailLunarYear, buf);

    // Holiday / Special Event
    if (ld.holiday && strlen(ld.holiday) > 0) {
        lv_label_set_text(lblDetailHoliday, ld.holiday);
        CydTheme::applyTextFont(lblDetailHoliday, CydTheme::getFont12(), CydTheme::getDangerColor());
        lv_obj_set_style_border_color(boxHolidayBadge, CydTheme::getDangerColor(), 0);
    } else if (ld.day == 15) {
        lv_label_set_text(lblDetailHoliday, "Ngày Rằm");
        CydTheme::applyTextFont(lblDetailHoliday, CydTheme::getFont12(), CydTheme::getGoldColor());
        lv_obj_set_style_border_color(boxHolidayBadge, CydTheme::getGoldColor(), 0);
    } else if (ld.day == 1) {
        lv_label_set_text(lblDetailHoliday, "Mùng 1 Đầu Tháng");
        CydTheme::applyTextFont(lblDetailHoliday, CydTheme::getFont12(), CydTheme::getGoldColor());
        lv_obj_set_style_border_color(boxHolidayBadge, CydTheme::getGoldColor(), 0);
    } else {
        lv_label_set_text(lblDetailHoliday, "Ngày Hoàng Đạo");
        CydTheme::applyTextFont(lblDetailHoliday, CydTheme::getFont12(), CydTheme::getSuccessColor());
        lv_obj_set_style_border_color(boxHolidayBadge, CydTheme::getCardBorderColor(), 0);
    }
}