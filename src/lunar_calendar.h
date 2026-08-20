#pragma once
#include <stdint.h>

/**
 * Chuyển đổi ngày dương lịch sang âm lịch Việt Nam
 * Thuật toán Hồ Ngọc Đức (tối ưu hóa cho ESP32)
 * 
 * @param solarYear Năm dương lịch (e.g. 2026)
 * @param solarMonth Tháng dương lịch (1-12)
 * @param solarDay Ngày dương lịch (1-31)
 * @param lunarDay [OUT] Ngày âm lịch
 * @param lunarMonth [OUT] Tháng âm lịch
 * @param lunarYear [OUT] Năm âm lịch
 */
void solarToLunar(int solarYear, int solarMonth, int solarDay, 
                  int &lunarDay, int &lunarMonth, int &lunarYear);

/**
 * Lấy chuỗi Can Chi của năm âm lịch (e.g. "Bính Ngọ", "Ất Tỵ")
 */
const char* getLunarYearName(int lunarYear);
