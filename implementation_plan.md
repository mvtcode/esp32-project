# Plan: Icon Font cho ESP32-S3 BLE HID Controller

## Phân Tích Branch `esp32-kit3.5`

### Font có sẵn trong `esp32-kit3.5`

| File | Loại | Range | Mục đích gốc |
|---|---|---|---|
| `vi_font_montserrat_12/14/16/20/24.c` | Text + LVGL Symbols | ASCII + Vietnamese + LVGL built-in | Text UI |
| `vi_weather_font_24.c` | Icon-only | `0xF000–0xF050` (Weather Icons) | Icon thời tiết |
| `weathericons.ttf` | Nguồn TTF | Weather Icons v2 | Nguồn generate |

### Approach của `esp32-kit3.5`

- **Tab bar icons**: Dùng **LVGL built-in symbols** (`LV_SYMBOL_HOME`, `LV_SYMBOL_LIST`, `LV_SYMBOL_AUDIO`, `LV_SYMBOL_SETTINGS`) được nhúng sẵn trong `vi_font_montserrat_20` (LVGL tự nhúng symbol range khi generate font)
- **Weather icons**: Font riêng `vi_weather_font_24` từ `weathericons.ttf`
- **Pattern dùng**: `lv_label_set_text(label, "\uF00D")` — string Unicode trực tiếp

---

## Phân Tích Nhu Cầu Icon — Dự Án HID Controller

### Nhóm 1: Tab Bar (5 icons)

| Tab | LVGL Symbol sẵn có | Unicode | Ghi chú |
|---|---|---|---|
| TouchPad | `LV_SYMBOL_HOME` | `\u0060` (built-in) | Tạm dùng, tương đối |
| Dev Shortcuts | `LV_SYMBOL_KEYBOARD` | `\u00a5` (built-in) | ✅ Phù hợp |
| Numpad | `LV_SYMBOL_LIST` | `\u00a9` (built-in) | Tạm dùng |
| Media | `LV_SYMBOL_AUDIO` | `\u00c1` (built-in) | ✅ Phù hợp |
| Settings | `LV_SYMBOL_SETTINGS` | `\u00ca` (built-in) | ✅ Phù hợp |

> [!NOTE]
> LVGL built-in symbols đã được nhúng sẵn trong font `vi_font_montserrat_20`. Không cần font riêng cho tab bar nếu dùng LVGL symbols. Tuy nhiên chất lượng thấp và lựa chọn hạn chế.

### Nhóm 2: Trackpad Tab — Button Icons

| Nút | Icon cần | FA codepoint |
|---|---|---|
| Left Click | Mouse left button | `\uF245` (fa-mouse-pointer) |
| Right Click | Mouse right button | `\uF245` + variant |
| Scroll Up | Arrow up | `\uF062` (fa-arrow-up) |
| Scroll Down | Arrow down | `\uF063` (fa-arrow-down) |
| Drag indicator | Move/arrows | `\uF047` (fa-arrows) |

### Nhóm 3: Media Tab — Button Icons

| Nút | Icon | FA codepoint |
|---|---|---|
| Play/Pause | `\uF04B` / `\uF04C` | fa-play / fa-pause |
| Next Track | `\uF051` | fa-step-forward |
| Prev Track | `\uF048` | fa-step-backward |
| Stop | `\uF04D` | fa-stop |
| Volume Up | `\uF028` | fa-volume-up |
| Volume Down | `\uF027` | fa-volume-down |
| Mute | `\uF026` | fa-volume-off |
| Lock Screen | `\uF023` | fa-lock |
| Screenshot | `\uF030` | fa-camera |
| Task Manager | `\uF0AE` | fa-tasks |

### Nhóm 4: Settings Tab — Label Icons

| Nhãn | Icon | FA codepoint |
|---|---|---|
| Độ nhạy chuột | `\uF245` | fa-mouse-pointer |
| Độ sáng | `\uF185` | fa-sun-o |
| Âm thanh | `\uF028` | fa-volume-up |
| Screen timeout | `\uF017` | fa-clock-o |
| OS Mode (Win) | `\uF17A` | fa-windows |
| OS Mode (Mac) | `\uF179` | fa-apple |
| RAM | `\uF2DB` | fa-microchip |
| SD Card | `\uF0A0` | fa-hdd-o |
| Firmware | `\uF019` | fa-download |
| Reset Factory | `\uF1B2` | fa-cube (warning) |

---

## Kế Hoạch Triển Khai

### Bước 1: Lựa Chọn Nguồn Font

**Quyết định**: Dùng **Font Awesome 4.7** (Free, phổ biến, đầy đủ icon cần thiết, `.ttf` nhỏ gọn)

- Source: `FontAwesome.otf` / `fontawesome-webfont.ttf` (FA 4.7.0)
- Tại sao FA4 thay vì FA6: FA4 có file TTF đơn, nhỏ hơn, tương thích tốt với `lv_font_conv`
- `weathericons.ttf` từ `esp32-kit3.5` **không phù hợp** — đây là Weather Icons chuyên biệt, thiếu hầu hết icon UI cần dùng

### Bước 2: Xác Định Range Cần Generate

Chỉ nhúng các glyph thực sự dùng để tối ưu Flash:

```
Cần nhúng các codepoints sau từ Font Awesome 4.7:
0xF017  (clock-o)       → Screen timeout
0xF019  (download)      → Firmware info
0xF023  (lock)          → Lock screen
0xF026  (volume-off)    → Mute
0xF027  (volume-down)   → Volume down  
0xF028  (volume-up)     → Volume up
0xF030  (camera)        → Screenshot
0xF047  (arrows)        → Drag indicator
0xF048  (step-backward) → Prev track
0xF04B  (play)          → Play
0xF04C  (pause)         → Pause
0xF04D  (stop)          → Stop
0xF051  (step-forward)  → Next track
0xF062  (arrow-up)      → Scroll Up
0xF063  (arrow-down)    → Scroll Down
0xF0A0  (hdd-o)         → SD Card
0xF0AE  (tasks)         → Task Manager
0xF17A  (windows)       → Windows OS icon
0xF179  (apple)         → macOS icon
0xF185  (sun-o)         → Brightness
0xF1B2  (cube)          → Reset Factory
0xF245  (mouse-pointer) → Mouse icons
0xF2DB  (microchip)     → RAM/chip
```

**Ước tính Flash**: ~23 glyph × 24px × 4bpp ≈ **~40–60 KB** (chấp nhận được)

### Bước 3: Generate Font bằng `lv_font_conv`

```bash
# Cài đặt (nếu chưa có)
npm install -g lv_font_conv

# Generate icon font 24px 4bpp
lv_font_conv \
  --font FontAwesome.ttf \
  -r 0xF017,0xF019,0xF023,0xF026,0xF027,0xF028,0xF030,0xF047,0xF048,0xF04B,0xF04C,0xF04D,0xF051,0xF062,0xF063,0xF0A0,0xF0AE,0xF17A,0xF179,0xF185,0xF1B2,0xF245,0xF2DB \
  --size 24 \
  --bpp 4 \
  --format lvgl \
  --lv-include lvgl.h \
  --lv-font-name vi_icon_font_24 \
  --no-compress \
  -o src/ui/fonts/vi_icon_font_24.c

# Generate size 20px cho tab bar (nhỏ hơn, vừa tab height 38px)
lv_font_conv \
  --font FontAwesome.ttf \
  -r 0xF017,0xF019,0xF023,0xF026,0xF027,0xF028,0xF030,0xF047,0xF048,0xF04B,0xF04C,0xF04D,0xF051,0xF062,0xF063,0xF0A0,0xF0AE,0xF17A,0xF179,0xF185,0xF1B2,0xF245,0xF2DB \
  --size 20 \
  --bpp 4 \
  --format lvgl \
  --lv-include lvgl.h \
  --lv-font-name vi_icon_font_20 \
  --no-compress \
  -o src/ui/fonts/vi_icon_font_20.c
```

### Bước 4: Cấu Trúc File Sau Khi Hoàn Thành

```
src/ui/fonts/
├── vi_fonts.h              ← Cập nhật thêm LV_FONT_DECLARE mới
├── vi_icon_font_20.c       ← [NEW] FA icons 20px (cho tab bar)
├── vi_icon_font_24.c       ← [NEW] FA icons 24px (cho buttons)
├── vi_font_montserrat_12.c ← Copy từ esp32-kit3.5 (chưa có)
├── vi_font_montserrat_14.c ← Copy từ esp32-kit3.5 (chưa có)
├── vi_font_montserrat_16.c ← Copy từ esp32-kit3.5 (chưa có)
├── vi_font_montserrat_20.c ← Copy từ esp32-kit3.5 (chưa có)
└── vi_font_montserrat_24.c ← Copy từ esp32-kit3.5 (chưa có)
```

> [!IMPORTANT]
> Hiện tại `src/ui/fonts/` chỉ có `vi_fonts.h` (header khai báo) nhưng **chưa có file .c nào**! Cần copy 5 file font `.c` từ branch `esp32-kit3.5` vào trước khi build.

### Bước 5: Cập Nhật `vi_fonts.h`

```cpp
#pragma once
#include "lvgl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Vietnamese Montserrat text fonts
LV_FONT_DECLARE(vi_font_montserrat_12);
LV_FONT_DECLARE(vi_font_montserrat_14);
LV_FONT_DECLARE(vi_font_montserrat_16);
LV_FONT_DECLARE(vi_font_montserrat_20);
LV_FONT_DECLARE(vi_font_montserrat_24);

// Font Awesome icon fonts (chỉ các glyph cần thiết)
LV_FONT_DECLARE(vi_icon_font_20);   // Tab bar icons
LV_FONT_DECLARE(vi_icon_font_24);   // Button icons, media icons

#ifdef __cplusplus
}
#endif

// ---- Icon codepoint defines ----
// Tab Bar Icons (dùng vi_icon_font_20)
#define ICON_TRACKPAD   "\xEF\x89\x85"  // U+F245 fa-mouse-pointer
#define ICON_KEYBOARD   "\xEF\x80\xAC"  // U+F02C fa-keyboard (không chuẩn FA4, dùng LV_SYMBOL_KEYBOARD)
#define ICON_NUMPAD     "\xEF\x80\x99"  // U+F019 (placeholder)
#define ICON_AUDIO      "\xEF\x80\xA8"  // U+F028 fa-volume-up
#define ICON_SETTINGS   "\xEF\x80\x93"  // U+F013 fa-cog

// Media Icons (dùng vi_icon_font_24)
#define ICON_PLAY       "\xEF\x81\x8B"  // U+F04B
#define ICON_PAUSE      "\xEF\x81\x8C"  // U+F04C
#define ICON_STOP       "\xEF\x81\x8D"  // U+F04D
#define ICON_PREV       "\xEF\x81\x88"  // U+F048
#define ICON_NEXT       "\xEF\x81\x91"  // U+F051
#define ICON_VOL_UP     "\xEF\x80\xA8"  // U+F028
#define ICON_VOL_DOWN   "\xEF\x80\xA7"  // U+F027
#define ICON_MUTE       "\xEF\x80\xA6"  // U+F026
#define ICON_LOCK       "\xEF\x80\xA3"  // U+F023
#define ICON_CAMERA     "\xEF\x80\xB0"  // U+F030
#define ICON_TASKS      "\xEF\x82\xAE"  // U+F0AE

// Scroll & Mouse Icons
#define ICON_ARROW_UP   "\xEF\x81\xA2"  // U+F062
#define ICON_ARROW_DOWN "\xEF\x81\xA3"  // U+F063
#define ICON_MOUSE      "\xEF\x89\x85"  // U+F245

// Settings Icons  
#define ICON_WINDOWS    "\xEF\x85\xBA"  // U+F17A
#define ICON_APPLE      "\xEF\x85\xB9"  // U+F179
#define ICON_SUN        "\xEF\x86\x85"  // U+F185
#define ICON_HDD        "\xEF\x82\xA0"  // U+F0A0
#define ICON_CHIP       "\xEF\x8B\x9B"  // U+F2DB
#define ICON_CLOCK      "\xEF\x80\x97"  // U+F017
```

### Bước 6: Thay Đổi Trong `ui_manager.cpp`

Thay vì dùng LVGL built-in symbols trên tab bar, dùng custom icon:

```cpp
// Trong createMainDashboard() - tạo custom tab bar
// KHÔNG dùng lv_tabview native tab buttons (tránh swipe)
// Dùng manual button bar + lv_tabview_set_act()

const char *tabIcons[] = {ICON_TRACKPAD, ICON_KEYBOARD, ICON_NUMPAD, ICON_AUDIO, ICON_SETTINGS};
for (int i = 0; i < 5; i++) {
    lv_obj_t *btn = lv_btn_create(tabBar);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_obj_set_style_text_font(lbl, &vi_icon_font_20, 0);
    lv_label_set_text(lbl, tabIcons[i]);
    lv_obj_add_event_cb(btn, onTabBtnEvent, LV_EVENT_CLICKED, (void*)(intptr_t)i);
}
```

---

## Tóm Tắt Việc Cần Làm

| # | Việc cần làm | Trạng thái |
|---|---|---|
| 1 | Tải `FontAwesome.ttf` v4.7 | ✅ Đã hoàn thành |
| 2 | Cài / Chạy `lv_font_conv` qua npm | ✅ Đã hoàn thành |
| 3 | Đồng bộ 5 font Montserrat `.c` vào `src/ui/fonts/` | ✅ Đã hoàn thành |
| 4 | Generate `vi_icon_font_20.c` và `vi_icon_font_24.c` | ✅ Đã hoàn thành |
| 5 | Cập nhật `vi_fonts.h` (thêm declare + define macros icon) | ✅ Đã hoàn thành |
| 6 | Refactor `ui_manager.cpp`: tab bar dùng icon font, buttons dùng icon & portrait layout | ✅ Đã hoàn thành |
| 7 | Build test (`pio run`) kiểm tra cú pháp và liên kết firmware | ✅ Đã hoàn thành (SUCCESS) |

