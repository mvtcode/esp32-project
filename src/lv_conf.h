/**
 * lv_conf.h — LVGL v9 Configuration
 * IoT Voice Command System — SH1106 OLED 128x64 (monochrome)
 *
 * This file is picked up because of -DLV_CONF_INCLUDE_SIMPLE in build_flags
 * and -I src (so the compiler looks here first).
 */

#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/*====================
   COLOR SETTINGS
 *====================*/

/* Color depth: 1 (1 byte per pixel), 8, 16, or 32 — SH1106 is monochrome */
#define LV_COLOR_DEPTH 1

/*====================
   MEMORY SETTINGS
 *====================*/

/* Size of the memory available for `lv_malloc()` in bytes (≥2kB) */
#define LV_MEM_SIZE (48 * 1024U)  /* 48 KB from internal SRAM */

/* Use custom malloc/free. 0: use the built-in `lv_malloc()` */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN

/*====================
   HAL SETTINGS
 *====================*/

/* Default display refresh period (ms) */
#define LV_DEF_REFR_PERIOD  20

/* Input device read period (ms) */
#define LV_INDEV_DEF_READ_PERIOD 30

/*====================
   FONT USAGE
 *====================*/

/* Enable built-in Montserrat fonts */
#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  1   /* Primary font for 128x64 OLED */
#define LV_FONT_MONTSERRAT_14  0
#define LV_FONT_MONTSERRAT_16  0
#define LV_FONT_MONTSERRAT_18  0
#define LV_FONT_MONTSERRAT_20  0
#define LV_FONT_MONTSERRAT_22  0
#define LV_FONT_MONTSERRAT_24  0
#define LV_FONT_MONTSERRAT_26  0
#define LV_FONT_MONTSERRAT_28  0
#define LV_FONT_MONTSERRAT_30  0
#define LV_FONT_MONTSERRAT_32  0
#define LV_FONT_MONTSERRAT_34  0
#define LV_FONT_MONTSERRAT_36  0
#define LV_FONT_MONTSERRAT_38  0
#define LV_FONT_MONTSERRAT_40  0
#define LV_FONT_MONTSERRAT_42  0
#define LV_FONT_MONTSERRAT_44  0
#define LV_FONT_MONTSERRAT_46  0
#define LV_FONT_MONTSERRAT_48  0

/* Default font — used when no font is specified */
#define LV_FONT_DEFAULT &lv_font_montserrat_12

/*====================
   WIDGET USAGE
 *====================*/

#define LV_USE_LABEL    1
#define LV_USE_BTN      1
#define LV_USE_LIST     1
#define LV_USE_BAR      1
#define LV_USE_ARC      0
#define LV_USE_IMG      0
#define LV_USE_TABLE    0
#define LV_USE_CHECKBOX 0
#define LV_USE_SLIDER   0
#define LV_USE_SWITCH   0
/* TEXTAREA must be 1 — required by lv_spinbox (internal LVGL dependency) */
#define LV_USE_TEXTAREA 1
#define LV_USE_SPINBOX  0
#define LV_USE_SPINNER  0
#define LV_USE_LED      0
#define LV_USE_CHART    0
#define LV_USE_METER    0
#define LV_USE_MSGBOX   0
#define LV_USE_TABVIEW  0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN      0
#define LV_USE_SPAN     0
#define LV_USE_CALENDAR 0
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN   0
#define LV_USE_KEYBOARD 0
#define LV_USE_MENU     1   /* For device list navigation */
#define LV_USE_DROPDOWN 0

/*====================
   INPUT DEVICE
 *====================*/

/* 4 physical buttons -> KEYPAD input device */
#define LV_USE_INDEV        1

/*====================
   LOGGING
 *====================*/

#define LV_USE_LOG          1
#define LV_LOG_LEVEL        LV_LOG_LEVEL_WARN
#define LV_LOG_PRINTF       1

/*====================
   OTHERS
 *====================*/

/* Enable assert for debugging */
#define LV_USE_ASSERT_NULL          1
#define LV_USE_ASSERT_MEM           1
#define LV_USE_ASSERT_OBJ           0
#define LV_USE_ASSERT_STYLE         0

/* Garbage collector — not needed for ESP32 */
#define LV_ENABLE_GC                0

/* Large integer type (for uint32_t pixel address math) */
#define LV_USE_LARGE_COORD          0

#endif /* LV_CONF_H */
