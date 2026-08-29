#include "display_service.h"
#include <WiFi.h>
#include "vietnamese_helper.h"
#include "Verdana_Vietnamese10pt.h"
#include "Verdana_Vietnamese12pt.h"
#include "lunar_calendar.h"

// Vẽ biểu tượng sóng WiFi 5 vạch
template<typename T>
static void drawWifiIcon(T &target, int x, int y, bool connected, int8_t rssi = -60) {
    int numBars = 5;
    int barWidth = 3;
    int spacing = 2;
    int maxH = 13;

    int activeBars = 0;
    if (connected) {
        if (rssi >= -55) activeBars = 5;
        else if (rssi >= -67) activeBars = 4;
        else if (rssi >= -75) activeBars = 3;
        else if (rssi >= -85) activeBars = 2;
        else activeBars = 1;
    }

    for (int b = 0; b < numBars; b++) {
        int barH = 3 + b * 2; // Chiều cao vạch 1..5: 3, 5, 7, 9, 11px
        int bx = x + b * (barWidth + spacing);
        int by = y + (maxH - barH);

        uint16_t color = (connected && b < activeBars) ? TFT_GREEN : 0x39E7; // Xanh lá cây hoặc xám mờ
        if (!connected && b == 0) color = TFT_RED; // Mất kết nối: vạch 1 đỏ

        target.fillRect(bx, by, barWidth, barH, color);
    }
}

LGFX DisplayService::lcd;
LGFX_Sprite DisplayService::clockCanvas(&DisplayService::lcd);
char DisplayService::toast_msg[64] = "";
uint16_t DisplayService::toast_bg_color = TFT_DARKGREEN;
uint16_t DisplayService::toast_text_color = TFT_WHITE;
uint32_t DisplayService::toast_expiry_time = 0;
bool DisplayService::screen_on = true;
DisplayMode DisplayService::current_mode = DISPLAY_MODE_STANDBY_CLOCK;
uint32_t DisplayService::last_clock_render_time = 0;

static const char* DAY_NAMES_VI[] = {
    "Chủ Nhật", "Thứ Hai", "Thứ Ba", "Thứ Tư", "Thứ Năm", "Thứ Sáu", "Thứ Bảy"
};

const char* DisplayService::getDayOfWeekStr(int wday) {
    if (wday >= 0 && wday < 7) {
        return DAY_NAMES_VI[wday];
    }
    return "N/A";
}

// Tính số ngày trong tháng (month: 0-11, year: ví dụ 2026)
static int getDaysInMonth(int month, int year) {
    if (month == 1) { // Tháng 2
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) return 29;
        return 28;
    }
    if (month == 3 || month == 5 || month == 8 || month == 10) return 30; // 4, 6, 9, 11
    return 31;
}

// Tính thứ của ngày 1 trong tháng (0 = Thứ 2, 1 = Thứ 3, ..., 6 = Chủ Nhật)
static int getFirstDayOfWeekMon(int mday, int wday) {
    int curMonBased = (wday + 6) % 7;
    int firstMonBased = (curMonBased - ((mday - 1) % 7) + 7) % 7;
    return firstMonBased;
}

void DisplayService::init() {
    lcd.init();
    lcd.setRotation(1); // Xoay ngang 320x240
    lcd.fillScreen(TFT_BLACK);
    lcd.setAttribute(lgfx::v1::attribute_t::utf8_switch, false);
    screen_on = true;
    current_mode = DISPLAY_MODE_STANDBY_CLOCK; // Mặc định vào Chế độ Đồng Hồ & Lịch

    // Khởi tạo bộ đệm kép Double-Buffer Sprite trong PSRAM 8MB để chống giật nhấp nháy 100%
    clockCanvas.setColorDepth(16);
    clockCanvas.setPsram(true);
    clockCanvas.createSprite(320, 240);
    clockCanvas.setAttribute(lgfx::v1::attribute_t::utf8_switch, false);

    Serial.println("[DisplayService] ST7789 Display & PSRAM Double-Buffer Canvas Initialized");
}

void DisplayService::setScreenOn(bool on) {
    screen_on = on;
    if (on) {
        lcd.setBrightness(255);
    } else {
        lcd.setBrightness(0);
    }
}

void DisplayService::setMode(DisplayMode mode) {
    current_mode = mode;
    lcd.fillScreen(TFT_BLACK);
}

void DisplayService::showMessage(int x, int y, const char *msg, uint16_t color) {
    if (!screen_on) return;
    lcd.setTextColor(color, TFT_BLACK);
    lcd.setTextSize(2);
    lcd.setCursor(x, y);
    lcd.print(msg);
}

void DisplayService::showToast(const char *msg, uint16_t bgColor, uint16_t textColor, uint32_t durationMs) {
    strncpy(toast_msg, msg, sizeof(toast_msg) - 1);
    toast_msg[sizeof(toast_msg) - 1] = '\0';
    toast_bg_color = bgColor;
    toast_text_color = textColor;
    toast_expiry_time = millis() + durationMs;
}

void DisplayService::render(const CameraFrame &frame, const FaceDetectionResult &aiResult, float displayFps, UploadStatus uploadStatus, bool wifiConnected, bool uploadEnabled) {
    if (!screen_on) return;

    lcd.startWrite();

    // 1. Vẽ khung hình Camera toàn màn hình (Full Screen không bị che)
    if (frame.isValid() && frame.buffer != nullptr) {
        lcd.pushImage(0, 0, frame.width, frame.height, (uint16_t *)frame.buffer);
    }

    // 2. Vẽ Bounding Box phát hiện khuôn mặt
    for (const auto &face : aiResult.faces) {
        int x = max(0, min(319, face.x1));
        int y = max(0, min(239, face.y1));
        int w = min(face.width(), 320 - x);
        int h = min(face.height(), 240 - y);

        uint16_t box_color = TFT_YELLOW;
        lcd.drawRect(x, y, w, h, box_color);
        lcd.drawRect(x + 1, y + 1, max(1, w - 2), max(1, h - 2), box_color);

        // Nhãn phát hiện trong suốt, tinh tế
        char label[32];
        snprintf(label, sizeof(label), "Face: %.0f%%", face.score * 100.0f);
        int tag_y = (y >= 12) ? (y - 10) : (y + h + 2);

        lcd.setTextSize(1);
        lcd.setTextColor(TFT_YELLOW);
        lcd.setCursor(x + 2, tag_y);
        lcd.print(label);
    }

    // 3. Top Info HUD
    lcd.setFont(&fonts::Font0);
    lcd.setTextSize(1);

    // FPS & AI State
    lcd.setTextColor(TFT_CYAN);
    char fpsStr[24];
    snprintf(fpsStr, sizeof(fpsStr), "FPS: %.1f", displayFps);
    lcd.drawString(fpsStr, 6, 6);

    // Dấu gạch đứng |
    lcd.setTextColor(0x7BEF);
    lcd.drawString("|", 74, 6);

    // Up:
    lcd.setTextColor(TFT_WHITE);
    lcd.drawString("Up:", 86, 6);

    // Icon tròn: xanh lá cây là ON, xám là OFF (vàng khi gửi, đỏ khi lỗi)
    uint16_t circleColor = uploadEnabled ? TFT_GREEN : 0x7BEF;
    if (uploadEnabled) {
        if (uploadStatus == UPLOAD_IN_PROGRESS) circleColor = TFT_YELLOW;
        else if (uploadStatus == UPLOAD_FAILED) circleColor = TFT_RED;
    }
    lcd.fillCircle(112, 10, 4, circleColor);
    lcd.drawCircle(112, 10, 4, TFT_WHITE);

    // WiFi 5 vạch ở góc bên phải
    int camRssi = wifiConnected ? WiFi.RSSI() : -100;
    drawWifiIcon(lcd, 290, 4, wifiConnected, camRssi);

    // 4. Toast Notification
    if (millis() < toast_expiry_time) {
        int toast_w = 240;
        int toast_h = 36;
        int toast_x = (320 - toast_w) / 2;
        int toast_y = 102;

        lcd.fillRoundRect(toast_x, toast_y, toast_w, toast_h, 6, toast_bg_color);
        lcd.drawRoundRect(toast_x, toast_y, toast_w, toast_h, 6, TFT_WHITE);

        lcd.setFont(&Verdana_Vietnamese10pt);
        lcd.setTextSize(1);
        lcd.setTextColor(toast_text_color, toast_bg_color);
        String cToast = utf8ToCustom(toast_msg);
        int text_w = lcd.textWidth(cToast);
        int cur_x = toast_x + max(6, (toast_w - text_w) / 2);
        lcd.drawString(cToast, cur_x, toast_y + 10);
    }

    lcd.endWrite();
}

void DisplayService::renderStandbyClock(bool wifiConnected, const WeatherInfo &weather, float aiFps) {
    if (!screen_on) return;

    time_t nowSec = time(nullptr);
    struct tm timeinfo;
    localtime_r(&nowSec, &timeinfo);

    // Cập nhật giao diện Clock mỗi 250ms
    uint32_t nowMs = millis();
    if (nowMs - last_clock_render_time < 250) return;
    last_clock_render_time = nowMs;

    int curYear = timeinfo.tm_year + 1900;
    int curMonth = timeinfo.tm_mon; // 0 - 11
    int curDay = timeinfo.tm_mday;
    int curWday = timeinfo.tm_wday; // 0=Sun, 1=Mon...

    // ================= TOÀN BỘ VẼ TRÊN CANVAS BỘ ĐỆM KÉP (KHÔNG NHẤP NHÁY) =================
    // 1. Nền tối Gradient Dark Navy
    clockCanvas.fillScreen(0x0821);

    // 2. Header Bar (0-22px)
    clockCanvas.fillRect(0, 0, 320, 22, 0x0010);
    clockCanvas.drawFastHLine(0, 22, 320, 0x2124);

    // Chế độ đồng hồ: Thời tiết ở bên trái
    clockCanvas.setFont(&Verdana_Vietnamese10pt);
    clockCanvas.setTextSize(1);
    if (weather.is_valid) {
        clockCanvas.setTextColor(TFT_CYAN, 0x0010);
        char topWStr[80];
        // Hà Nội: có mây, 27.2°C | 96%
        snprintf(topWStr, sizeof(topWStr), "%s: %s, %.1f°C | %d%%", 
                 weather.city_name.c_str(), weather.condition_text.c_str(), 
                 weather.temperature, weather.humidity);
        clockCanvas.drawString(utf8ToCustom(topWStr), 8, 4);
    } else {
        clockCanvas.setTextColor(TFT_SILVER, 0x0010);
        clockCanvas.drawString(utf8ToCustom("Đang cập nhật thời tiết..."), 8, 4);
    }

    // WiFi 5 vạch ở góc bên phải
    int clockRssi = wifiConnected ? WiFi.RSSI() : -100;
    drawWifiIcon(clockCanvas, 290, 4, wifiConnected, clockRssi);

    // ================= KHỐI TRÁI: ĐỒNG HỒ & ÂM LỊCH (x: 4 -> 158) =================
    int lcard_x = 4;
    int lcard_y = 28;
    int lcard_w = 154;
    int lcard_h = 206;
    int lcard_cx = lcard_x + lcard_w / 2;

    clockCanvas.fillRoundRect(lcard_x, lcard_y, lcard_w, lcard_h, 8, 0x10A2);
    clockCanvas.drawRoundRect(lcard_x, lcard_y, lcard_w, lcard_h, 8, 0x2965);

    // 1. Digital Clock lớn (Căn giữa chuẩn, không bị đè viền)
    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

    clockCanvas.setFont(&fonts::Font0);
    clockCanvas.setTextSize(3);
    clockCanvas.setTextColor(TFT_WHITE, 0x10A2);
    clockCanvas.drawCenterString(timeStr, lcard_cx, lcard_y + 10);

    clockCanvas.drawFastHLine(lcard_x + 6, lcard_y + 42, lcard_w - 12, 0x2965);

    // 2. Thứ, ngày (Dương lịch to rõ ràng: "Chủ nhật, 30/08")
    char dateStr[48];
    snprintf(dateStr, sizeof(dateStr), "%s, %02d/%02d", 
             getDayOfWeekStr(curWday), curDay, curMonth + 1);

    clockCanvas.setFont(&Verdana_Vietnamese12pt);
    clockCanvas.setTextSize(1);
    clockCanvas.setTextColor(0xFFE0, 0x10A2); // Vàng kim
    clockCanvas.drawCenterString(utf8ToCustom(dateStr), lcard_cx, lcard_y + 52);

    clockCanvas.drawFastHLine(lcard_x + 6, lcard_y + 80, lcard_w - 12, 0x2124);

    // 3. Âm lịch: "ÂL: 18/07"
    int lunarDay = 0, lunarMonth = 0, lunarYear = 0;
    solarToLunar(curYear, curMonth + 1, curDay, lunarDay, lunarMonth, lunarYear);
    const char* canChiYear = getLunarYearName(lunarYear);

    char lDateStr[32];
    snprintf(lDateStr, sizeof(lDateStr), "ÂL: %02d/%02d", lunarDay, lunarMonth);

    clockCanvas.setFont(&Verdana_Vietnamese12pt);
    clockCanvas.setTextSize(1);
    clockCanvas.setTextColor(0x7FFF, 0x10A2); // Cyan nhạt
    clockCanvas.drawCenterString(utf8ToCustom(lDateStr), lcard_cx, lcard_y + 92);

    // 4. Năm Can Chi: "Năm Bính Ngọ"
    char lYearStr[40];
    snprintf(lYearStr, sizeof(lYearStr), "Năm %s", canChiYear);

    clockCanvas.setFont(&Verdana_Vietnamese12pt);
    clockCanvas.setTextSize(1);
    clockCanvas.setTextColor(0xFDE0, 0x10A2); // Vàng ấm
    clockCanvas.drawCenterString(utf8ToCustom(lYearStr), lcard_cx, lcard_y + 124);

    clockCanvas.drawFastHLine(lcard_x + 6, lcard_y + 154, lcard_w - 12, 0x2124);

    // 5. Gợi ý nút bấm
    clockCanvas.setFont(&Verdana_Vietnamese10pt);
    clockCanvas.setTextSize(1);
    clockCanvas.setTextColor(TFT_SILVER, 0x10A2);
    clockCanvas.drawCenterString(utf8ToCustom("[BOOT: Xem Camera]"), lcard_cx, lcard_y + 172);

    // ================= KHỐI PHẢI: LỊCH THÁNG (CALENDAR WIDGET) (x: 162 -> 316) =================
    int cal_x = 162;
    int cal_y = 28;
    int cal_w = 154;
    int cal_h = 206;

    clockCanvas.fillRoundRect(cal_x, cal_y, cal_w, cal_h, 8, 0x10A2);
    clockCanvas.drawRoundRect(cal_x, cal_y, cal_w, cal_h, 8, 0x2965);

    // Tiêu đề Tháng / Năm
    clockCanvas.fillRect(cal_x + 2, cal_y + 2, cal_w - 4, 22, 0x2167);
    clockCanvas.setFont(&Verdana_Vietnamese10pt);
    clockCanvas.setTextColor(TFT_WHITE, 0x2167);
    char calHeader[32];
    snprintf(calHeader, sizeof(calHeader), "THÁNG %02d / %04d", curMonth + 1, curYear);
    clockCanvas.drawCenterString(utf8ToCustom(calHeader), cal_x + cal_w / 2, cal_y + 4);

    // Header các thứ: T2 T3 T4 T5 T6 T7 CN
    const char *headers[] = {"T2", "T3", "T4", "T5", "T6", "T7", "CN"};
    int col_w = 20;
    int start_gx = cal_x + 8;
    int start_gy = cal_y + 30;

    clockCanvas.setFont(&fonts::Font0);
    clockCanvas.setTextSize(1);
    for (int i = 0; i < 7; ++i) {
        clockCanvas.setTextColor((i >= 5) ? 0xFDE0 : 0x9CD3, 0x10A2); // T7, CN màu vàng
        clockCanvas.drawString(headers[i], start_gx + i * col_w, start_gy);
    }

    clockCanvas.drawFastHLine(cal_x + 6, start_gy + 11, cal_w - 12, 0x2965);

    // Tính toán ngày trong tháng
    int daysInMonth = getDaysInMonth(curMonth, curYear);
    int firstDayW = getFirstDayOfWeekMon(curDay, curWday); // 0=T2 ... 6=CN

    int row = 0;
    int col = firstDayW;
    int grid_y_base = start_gy + 16;
    int row_h = 22;

    for (int d = 1; d <= daysInMonth; ++d) {
        int cell_x = start_gx + col * col_w;
        int cell_y = grid_y_base + row * row_h;

        // Nếu là ngày hôm nay: vẽ ô highlight màu xanh ngọc
        if (d == curDay) {
            clockCanvas.fillRoundRect(cell_x - 2, cell_y - 2, 17, 16, 3, 0x05B4); // Emerald green
            clockCanvas.setTextColor(TFT_WHITE, 0x05B4);
        } else if (col >= 5) {
            clockCanvas.setTextColor(0xFDE0, 0x10A2); // Thứ 7 & CN màu vàng nhạt
        } else {
            clockCanvas.setTextColor(0xDEDB, 0x10A2); // Ngày thường màu xám trắng
        }

        char dStr[8];
        snprintf(dStr, sizeof(dStr), "%d", d);
        clockCanvas.drawString(dStr, cell_x + (d < 10 ? 3 : 0), cell_y + 2);

        col++;
        if (col >= 7) {
            col = 0;
            row++;
        }
    }

    // 6. Toast Notification nếu có
    if (millis() < toast_expiry_time) {
        int toast_w = 230;
        int toast_h = 36;
        int toast_x = (320 - toast_w) / 2;
        int toast_y = 102;

        clockCanvas.fillRoundRect(toast_x, toast_y, toast_w, toast_h, 6, toast_bg_color);
        clockCanvas.drawRoundRect(toast_x, toast_y, toast_w, toast_h, 6, TFT_WHITE);

        clockCanvas.setFont(&Verdana_Vietnamese10pt);
        clockCanvas.setTextSize(1);
        clockCanvas.setTextColor(toast_text_color, toast_bg_color);
        String customToast = utf8ToCustom(toast_msg);
        int text_w = clockCanvas.textWidth(customToast);
        int cur_x = toast_x + max(6, (toast_w - text_w) / 2);
        clockCanvas.drawString(customToast, cur_x, toast_y + 10);
    }

    // ĐẨY TOÀN BỘ FRAME ĐÃ VẼ RA MÀN HÌNH BẰNG DMA (CHỐNG GIẬT 100%)
    clockCanvas.pushSprite(0, 0);
}
