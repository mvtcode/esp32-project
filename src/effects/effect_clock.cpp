#include "effects.h"
#include "safe_draw.h"
#include "../wifi_app.h"
#include "../bt_audio.h"

void effect_clock_render(const int32_t *left, const int32_t *right, size_t n) {
    char time_str[32];
    wifi_app_get_time_str(time_str, sizeof(time_str));

    WeatherData w = wifi_app_get_weather();

    // 1. Digital Clock (large centered font)
    gfx.setFont(u8g2_font_7x14_tf);
    int tw = gfx.getStrWidth(time_str);
    gfx.drawStr((SCREEN_W - tw) / 2, 18, time_str);

    // Separator line
    gfx.drawHLine(4, 24, SCREEN_W - 8);

    // 2. Weather & Status
    gfx.setFont(u8g2_font_6x10_tf);
    char wstr[32];
    snprintf(wstr, sizeof(wstr), "%.1f C  %d%% Hum", w.temp_c, w.humidity);
    gfx.drawStr((SCREEN_W - gfx.getStrWidth(wstr)) / 2, 36, wstr);

    // 3. Audio Mode info bar
    gfx.setFont(u8g2_font_04b_03_tr);
    const char *status_str = bt_audio_is_connected() ? "[BT] Connected" : "[MIC] Stereo Live";
    gfx.drawStr((SCREEN_W - gfx.getStrWidth(status_str)) / 2, 48, status_str);

    // 4. Subtle live mini-spectrum / VU bar at the bottom
    int32_t peak_l = 0;
    int32_t peak_r = 0;
    for (size_t i = 0; i < n; i++) {
        int32_t al = left[i] < 0 ? -left[i] : left[i];
        int32_t ar = right[i] < 0 ? -right[i] : right[i];
        if (al > peak_l) peak_l = al;
        if (ar > peak_r) peak_r = ar;
    }
    int bar_l = (int)((float)peak_l * 56.0f / 32768.0f);
    int bar_r = (int)((float)peak_r * 56.0f / 32768.0f);
    if (bar_l > 56) bar_l = 56;
    if (bar_r > 56) bar_r = 56;

    // Dual bottom audio level bars
    gfx.drawBox(64 - bar_l, 58, bar_l, 3);
    gfx.drawBox(64, 58, bar_r, 3);
}
