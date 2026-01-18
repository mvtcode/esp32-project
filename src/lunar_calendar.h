#ifndef LUNAR_CALENDAR_H
#define LUNAR_CALENDAR_H

#include <stdint.h>

/**
 * Chuyển đổi ngày dương lịch sang âm lịch Việt Nam
 * Sử dụng thuật toán Hồ Ngọc Đức (tối ưu cho ESP32)
 * 
 * @param solarYear Năm dương lịch
 * @param solarMonth Tháng dương lịch (1-12)
 * @param solarDay Ngày dương lịch (1-31)
 * @param lunarDay [OUT] Ngày âm lịch
 * @param lunarMonth [OUT] Tháng âm lịch
 * @param lunarYear [OUT] Năm âm lịch
 */
void solarToLunar(int solarYear, int solarMonth, int solarDay, 
                  int &lunarDay, int &lunarMonth, int &lunarYear);

#endif // LUNAR_CALENDAR_H
