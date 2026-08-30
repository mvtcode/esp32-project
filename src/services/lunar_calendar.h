#ifndef LUNAR_CALENDAR_H
#define LUNAR_CALENDAR_H

#include <stdint.h>
#include <Arduino.h>

struct LunarDate {
    int day;
    int month;
    int year;
    bool isLeap;
    const char* yearName;   // Ví dụ: "Ất Tỵ", "Giáp Thìn"
    const char* dayName;    // Can Chi ngày: "Giáp Tý", "Bính Thìn"
    const char* holiday;    // Ngày lễ nếu có: "Tết Nguyên Đán", "Giỗ Tổ Hùng Vương"...
};

/**
 * Chuyển đổi ngày dương lịch sang âm lịch Việt Nam
 * Sử dụng thuật toán thiên văn Hồ Ngọc Đức (tối ưu hóa cho ESP32)
 */
void solarToLunar(int solarYear, int solarMonth, int solarDay, 
                  int &lunarDay, int &lunarMonth, int &lunarYear);

/**
 * Lấy đầy đủ thông tin Âm lịch, Can Chi và Ngày lễ
 */
LunarDate getDetailedLunarDate(int solarYear, int solarMonth, int solarDay);

/**
 * Lấy tên năm Can Chi (Ví dụ: Ất Tỵ, Bính Ngọ...)
 */
const char* getLunarYearName(int lunarYear);

/**
 * Lấy tên ngày Can Chi
 */
const char* getLunarDayName(int solarYear, int solarMonth, int solarDay);

/**
 * Kiểm tra ngày lễ truyền thống hoặc quốc tế
 */
const char* getHolidayName(int solarDay, int solarMonth, int lunarDay, int lunarMonth);

#endif // LUNAR_CALENDAR_H
