#pragma once

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Vietnamese Montserrat text fonts
LV_FONT_DECLARE(vi_font_montserrat_12);
LV_FONT_DECLARE(vi_font_montserrat_14);
LV_FONT_DECLARE(vi_font_montserrat_16);
LV_FONT_DECLARE(vi_font_montserrat_20);
LV_FONT_DECLARE(vi_font_montserrat_24);

// Font Awesome icon fonts
LV_FONT_DECLARE(vi_icon_font_20);   // Tab bar & smaller icons
LV_FONT_DECLARE(vi_icon_font_24);   // Button & media icons

#ifdef __cplusplus
}
#endif

// ---- Icon codepoint defines (UTF-8 encoded FontAwesome 4.7) ----

// Tab Bar Icons (dùng vi_icon_font_20)
#define ICON_TRACKPAD     "\xEF\x89\x85"  // U+F245 fa-mouse-pointer
#define ICON_KEYBOARD     "\xEF\x84\x9C"  // U+F11C fa-keyboard-o
#define ICON_NUMPAD       "\xEF\x80\x8A"  // U+F00A fa-th (numpad grid)
#define ICON_AUDIO        "\xEF\x80\x81"  // U+F001 fa-music
#define ICON_SETTINGS     "\xEF\x80\x93"  // U+F013 fa-cog

// Media & System Icons (dùng vi_icon_font_24)
#define ICON_PLAY         "\xEF\x81\x8B"  // U+F04B fa-play
#define ICON_PAUSE        "\xEF\x81\x8C"  // U+F04C fa-pause
#define ICON_STOP         "\xEF\x81\x8D"  // U+F04D fa-stop
#define ICON_PREV         "\xEF\x81\x88"  // U+F048 fa-step-backward
#define ICON_NEXT         "\xEF\x81\x91"  // U+F051 fa-step-forward
#define ICON_VOL_UP       "\xEF\x80\xA8"  // U+F028 fa-volume-up
#define ICON_VOL_DOWN     "\xEF\x80\xA7"  // U+F027 fa-volume-down
#define ICON_MUTE         "\xEF\x80\xA6"  // U+F026 fa-volume-off
#define ICON_CALC         "\xEF\x87\xAC"  // U+F1EC fa-calculator
#define ICON_LOCK         "\xEF\x80\xA3"  // U+F023 fa-lock
#define ICON_CAMERA       "\xEF\x80\xB0"  // U+F030 fa-camera
#define ICON_TASKS        "\xEF\x82\xAE"  // U+F0AE fa-tasks

// Trackpad & Navigation Icons
#define ICON_ARROW_UP     "\xEF\x81\xA2"  // U+F062 fa-arrow-up
#define ICON_ARROW_DOWN   "\xEF\x81\xA3"  // U+F063 fa-arrow-down
#define ICON_MOUSE        "\xEF\x89\x85"  // U+F245 fa-mouse-pointer
#define ICON_ARROWS       "\xEF\x81\x87"  // U+F047 fa-arrows

// Settings Icons
#define ICON_WINDOWS      "\xEF\x85\xBA"  // U+F17A fa-windows
#define ICON_APPLE        "\xEF\x85\xB9"  // U+F179 fa-apple
#define ICON_SUN          "\xEF\x86\x85"  // U+F185 fa-sun-o
#define ICON_HDD          "\xEF\x82\xA0"  // U+F0A0 fa-hdd-o
#define ICON_CHIP         "\xEF\x8B\x9B"  // U+F2DB fa-microchip
#define ICON_CLOCK        "\xEF\x80\x97"  // U+F017 fa-clock-o
#define ICON_DOWNLOAD     "\xEF\x80\x99"  // U+F019 fa-download
#define ICON_REFRESH      "\xEF\x80\xA1"  // U+F021 fa-refresh
#define ICON_CUBE         "\xEF\x86\xB2"  // U+F1B2 fa-cube
