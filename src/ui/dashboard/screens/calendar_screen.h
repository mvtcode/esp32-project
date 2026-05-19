#ifndef CALENDAR_SCREEN_H
#define CALENDAR_SCREEN_H

#include <lvgl.h>

struct CalendarEvent {
    const char* timeStr;
    const char* title;
    const char* location;
    const char* durationStr;
    lv_color_t color;
};

class CalendarScreen {
public:
    CalendarScreen(lv_obj_t* parent);
    ~CalendarScreen() {
        if (rootContainer) lv_obj_del(rootContainer);
    }

    // Setters for dynamic updates
    void updateMonthYearHeader(const char* monthYearStr);
    void updateCalendarDays(int startDayOfWeek, int daysInMonth, int activeDay, const uint32_t* dotColorsMatrix); 
    // dotColorsMatrix is 42-length array, 0 means no dot, hex color otherwise.
    
    void updateSyncStatus(bool googleConnected, bool appleConnected);
    void clearEvents();
    void addEvent(const CalendarEvent& event);

    lv_obj_t* getRoot() { return rootContainer; }

private:
    lv_obj_t* rootContainer;

    // Monthly Calendar widgets
    lv_obj_t* lblMonthYear;
    lv_obj_t* calendarGrid;
    lv_obj_t* cellsContainer[42];
    lv_obj_t* cellNumbers[42];
    lv_obj_t* cellDotsBox[42];

    // Connection switches
    lv_obj_t* swGoogle;
    lv_obj_t* swApple;
    lv_obj_t* lblGoogleStatus;
    lv_obj_t* lblAppleStatus;

    // Daily Timeline widgets
    lv_obj_t* lblEventDayNum;
    lv_obj_t* lblEventDayName;
    lv_obj_t* lblEventDate;
    lv_obj_t* eventsScrollContainer;

    // Helper functions
    void createCalendarPane(lv_obj_t* parent);
    void createTimelinePane(lv_obj_t* parent);
};

#endif // CALENDAR_SCREEN_H
