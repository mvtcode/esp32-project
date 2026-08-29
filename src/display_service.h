#ifndef DISPLAY_SERVICE_H
#define DISPLAY_SERVICE_H

#include <Arduino.h>
#include <time.h>
#include "display.h"
#include "camera_service.h"
#include "face_detector.h"
#include "upload_types.h"
#include "weather_service.h"

enum DisplayMode {
    DISPLAY_MODE_CAMERA = 0,
    DISPLAY_MODE_STANDBY_CLOCK = 1
};

class DisplayService {
private:
    static LGFX lcd;
    static LGFX_Sprite clockCanvas;
    static char toast_msg[64];
    static uint16_t toast_bg_color;
    static uint16_t toast_text_color;
    static uint32_t toast_expiry_time;
    static bool screen_on;
    static DisplayMode current_mode;
    static uint32_t last_clock_render_time;

    static const char* getDayOfWeekStr(int wday);

public:
    static void init();
    static void setScreenOn(bool on);
    static bool isScreenOn() { return screen_on; }
    static void toggleScreen() { setScreenOn(!screen_on); }

    static void setMode(DisplayMode mode);
    static DisplayMode getMode() { return current_mode; }
    static void toggleMode() { setMode(current_mode == DISPLAY_MODE_CAMERA ? DISPLAY_MODE_STANDBY_CLOCK : DISPLAY_MODE_CAMERA); }

    static void showMessage(int x, int y, const char *msg, uint16_t color = TFT_WHITE);
    static void showToast(const char *msg, uint16_t bgColor = TFT_DARKGREEN, uint16_t textColor = TFT_WHITE, uint32_t durationMs = 2000);

    // Render Camera View
    static void render(const CameraFrame &frame, const FaceDetectionResult &aiResult, float displayFps, UploadStatus uploadStatus = UPLOAD_IDLE, bool wifiConnected = false, bool uploadEnabled = true);

    // Render Standby Clock & Weather View with Zero Flicker
    static void renderStandbyClock(bool wifiConnected, const WeatherInfo &weather, float aiFps);
};

#endif // DISPLAY_SERVICE_H
