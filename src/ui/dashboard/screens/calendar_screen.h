#ifndef CALENDAR_SCREEN_H
#define CALENDAR_SCREEN_H

#include <lvgl.h>
#include "../../../services/lunar_calendar.h"

class CalendarScreen {
public:
    CalendarScreen(lv_obj_t* parent);
    ~CalendarScreen();


    // Setters for dynamic updates
    void setToday(int year, int month, int day);
    void setViewMonth(int year, int month);
    void selectDay(int day);
    void refreshCalendar();

    // Event handlers
    void onPrevMonth();
    void onNextMonth();
    void onPrevYear();
    void onNextYear();
    void onTodayClick();
    void onCellClick(int cellIndex);

    lv_obj_t* getRoot() { return rootContainer; }

    // Header controls
    lv_obj_t* btnPrevYear;
    lv_obj_t* btnPrevMonth;
    lv_obj_t* btnNextMonth;
    lv_obj_t* btnNextYear;
    lv_obj_t* btnToday;

private:
    lv_obj_t* rootContainer;

    // Current viewing date state
    int viewYear;
    int viewMonth;
    int selectedDay;

    // Real-time "Today" state
    int realTodayYear;
    int realTodayMonth;
    int realTodayDay;

    // Header title
    lv_obj_t* lblMonthYearTitle;

    // 42 Grid Cells (Single lightweight label per cell instead of 3 nested objects)
    struct CellData {
        int year;
        int month;
        int day;
        bool isCurrentMonth;
    } cellData[42];

    lv_obj_t* cellLabels[42];


    // Right Side Detail Card
    lv_obj_t* lblDetailSolarDay;
    lv_obj_t* lblDetailSolarWeekDay;
    lv_obj_t* lblDetailSolarMonthYear;
    lv_obj_t* lblDetailLunarDayMonth;
    lv_obj_t* lblDetailLunarCanChi;
    lv_obj_t* lblDetailLunarYear;
    lv_obj_t* lblDetailHoliday;
    lv_obj_t* boxHolidayBadge;

    // Layout builders
    void createCalendarPane(lv_obj_t* parent);
    void createDetailPane(lv_obj_t* parent);
    void updateDetailCard();

    static int getDaysInMonth(int year, int month);
    static int getFirstDayOfWeek(int year, int month);
};

#endif // CALENDAR_SCREEN_H
