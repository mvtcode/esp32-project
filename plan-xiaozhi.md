# Plan: Tích hợp XiaoZhi AI Voice Mode vào ESP32 Sound Visualizer

## Tổng quan

Thêm `AUDIO_MODE_XIAOZHI` (mode thứ 4) vào firmware `esp32-project`. Thiết bị kết nối lên XiaoZhi AI backend qua WebSocket với **2 giai đoạn bắt buộc**:

1. **Activation Screen** — Lần đầu dùng: gọi HTTP `/check-version`, server trả về mã 6 chữ số, hiển thị lên OLED để người dùng nhập vào ứng dụng điện thoại/web xác nhận, sau đó thiết bị được đăng ký.
2. **AI Chat Screen** — Sau khi activated: WebSocket mở, stream audio 2 chiều, hiển thị UI mắt robot `effect_xiaozhi` animate theo trạng thái.

**Tái sử dụng WiFi Config**: Mode XiaoZhi dùng chung hoàn toàn hệ thống quản lý WiFi (`wifi_app.cpp` & `WiFiManager`) của mode Weather & Clock. Không cần cài đặt WiFi riêng! Nếu chưa từng có WiFi hoặc đổi mạng, thiết bị sẽ mở AP Captive Portal dùng chung để cấu hình cả WiFi lẫn URL máy chủ XiaoZhi.

**Tham chiếu nguồn**:
- Activation flow: [`xiaozhi-esp32/main/application.cc`](file:///d:/projects/xiaozhi-esp32/main/application.cc) — `ActivationTask()` + `ShowActivationCode()`
- OTA/Activation HTTP: [`xiaozhi-esp32/main/ota.cc`](file:///d:/projects/xiaozhi-esp32/main/ota.cc) — `CheckVersion()` + `Activate()`
- WebSocket protocol: [`xiaozhi-esp32/docs/websocket.md`](file:///d:/projects/xiaozhi-esp32/docs/websocket.md)
- Client Python: [`py-xiaozhi/src/protocols/websocket_protocol.py`](file:///d:/projects/py-xiaozhi/src/protocols/websocket_protocol.py)

---

## Kiến trúc tổng thể

```
┌───────────────────────────────────────────────────────────────────┐
│                         ESP32-WROOM                               │
│                                                                   │
│  ┌──────────┐     ┌──────────────────────────────────────────┐   │
│  │  OLED    │◀────│          XIAOZHI MODE                    │   │
│  │ 128x64   │     │                                          │   │
│  │          │     │  Phase 0: WiFi Check / Portal            │   │
│  │ [WIFI AP]│     │  ┌──────────────┐ (Dùng chung với Clock) │   │
│  │          │     │  │ wifi_app     │ (MVT-Audio-Setup)      │   │
│  │          │     │  └──────┬───────┘                        │   │
│  │          │     │         ▼ WiFi OK                        │   │
│  │          │     │  Phase A: Activation Screen              │   │
│  │ [######] │     │  ┌──────────────┐                        │   │
│  │ [123456] │     │  │ xz_act_task  │─── HTTP POST ─────────▶│   │
│  │ [待确认] │     │  └──────┬───────┘                        │   │
│  │          │     │         ▼ Activated                      │   │
│  │          │     │  Phase B: AI Chat Screen                 │   │
│  │ (^‿^)(^) │     │  ┌─────────┐  ┌──────────┐  ┌────────┐  │   │
│  │ ~~smile~ │     │  │xz_ws    │  │xz_mic    │  │xz_play │  │   │
│  └──────────┘     │  │_task    │  │_task     │  │_task   │  │   │
│                   │  └────┬────┘  └──────────┘  └────────┘  │   │
│                   └───────┼──────────────────────────────────┘   │
└───────────────────────────┼───────────────────────────────────────┘
                            │ WiFi (Shared STA config)
                ┌───────────┴────────────────┐
                │     XiaoZhi Backend        │
                │                            │
                │  GET/POST /check-version   │ ← Activation Phase
                │  POST /activate            │
                │                            │
                │  wss://...                 │ ← Chat Phase
                │  (WebSocket + Opus audio)  │
                └────────────────────────────┘
```

**Mode cycle (Button PUSH):** `MIC → BT → CLOCK → XIAOZHI → MIC`

---

## Bố cục Điều khiển ở Chế độ XiaoZhi (Controls Mapping)

| Phần cứng | Thao tác | Chức năng ở Chế độ XiaoZhi |
|-----------|----------|----------------------------|
| **Biến trở xoay EC11** | Xoay Trái / Phải | Tăng / Giảm âm lượng loa (TTS / DAC output) |
| **Button PUSH (GPIO 4)** | Bấm nhả | Chuyển Mode âm thanh: `MIC → BT → CLOCK → XIAOZHI → MIC` |
| **Button CONFIRM (GPIO 14)** | **Bấm nhả (Short)** | **Đổi giao diện khuôn mặt AI (Face Themes)**: Mắt Anime XiaoZhi ⇄ Mắt AI Bot ⇄ Mắt Mèo Neko ⇄ Subtitle Text |
| | **Bấm giữ (Long >1s)** | **Đổi chế độ đối thoại (Chat Mode)**: Bấm để nói (Manual PTT) ⇄ Đối thoại tự động liên tục (Auto Continuous Chat) |
| **Button BACK (GPIO 13)** | Bấm nhả | **Push-to-Talk / Interrupt**: Bấm để bắt đầu nghe, hoặc ngắt lời khi bot đang phát TTS |
| **Button BOOT (GPIO 0)** | Bấm giữ (>2s) | **Mở Web Setup AP Portal** (`MVT-Audio-Setup`) để cấu hình lại WiFi hoặc Server URL |

---

## Cơ chế tái sử dụng WiFi & Cấu hình tập trung

1. **Dùng chung kết nối WiFi**:
   - Khi vào `AUDIO_MODE_XIAOZHI`, gọi `wifi_app_init()`.
   - Hệ thống tự động kết nối theo SSID / Password đã lưu từ trước (chung với mode Clock & Weather).
   - Nếu chưa có WiFi hoặc không kết nối được: tự động bật AP `MVT-Audio-Setup` và Captive Portal để người dùng kết nối điện thoại vào nhập WiFi.
2. **Cấu hình bổ sung cho XiaoZhi qua Web Portal**:
   - Thêm các trường cài đặt XiaoZhi trực tiếp trên trang Web Captive Portal của `wifi_app.cpp`:
     - **XiaoZhi OTA URL** (mặc định: `https://api.tenclass.net/xiaozhi/ota/`)
     - **XiaoZhi Token** (nếu dùng server riêng)
   - Lưu trữ an toàn trong NVS Flash.
3. **Cơ chế re-config nhanh**:
   - Khi đang ở mode XiaoZhi, bấm giữ nút **BOOT (GPIO 0)** để bật lại Web Setup AP portal bất kỳ lúc nào nếu muốn đổi WiFi hoặc đổi Server URL.

---

## Phase A: Activation Flow (lần đầu / chưa activated)

### Luồng HTTP (từ `ota.cc` + `application.cc`)

```
ESP32                             XiaoZhi Server
  │                                     │
  │── POST /check-version ─────────────▶│
  │   Headers:                          │
  │     Device-Id: <MAC>                │
  │     Client-Id: <UUID from NVS>      │
  │     Activation-Version: 1           │
  │                                     │
  │◀── 200 OK ──────────────────────────│
  │   {                                 │
  │     "activation": {                 │
  │       "code": "123456",  ← 6 digits │
  │       "message": "请在App中输入验证码",│
  │       "challenge": "<token>",       │
  │       "timeout_ms": 30000           │
  │     }                               │
  │   }                                 │
  │                                     │
  │  [Hiển thị code 6 số trên OLED]     │
  │  [Người dùng nhập vào App/Web]      │
  │                                     │
  │── POST /activate ──────────────────▶│
  │   { "challenge": "...", ... }       │
  │                                     │
  │◀── 202 (pending) / 200 (done) ──────│
  │  Retry mỗi 3s tối đa 10 lần        │
  │                                     │
  │  [Activation xong → Phase B]        │
```

### Màn hình OLED — Screen A: Activation

> **Đây là screen riêng biệt, KHÔNG dùng `effect_xiaozhi` animation.**

```
┌────────────────────────────┐
│  XIAOZHI ACTIVATION        │  ← Dòng status (font nhỏ)
│                            │
│  ╔══════════════════╗      │
│  ║   1  2  3  4  5  6║    │  ← 6 chữ số TO, font lớn nhất
│  ╚══════════════════╝      │
│                            │
│  Nhập code vào App         │  ← Message từ server
│  Chờ xác nhận... [###   ]  │  ← Spinner/progress bar
└────────────────────────────┘
```

**Trạng thái hiển thị trong Phase A:**

| Sub-state | OLED hiển thị |
|-----------|--------------|
| `XZ_ACT_WIFI_WAIT` | `Đang kết nối WiFi...` / `AP: MVT-Audio-Setup` |
| `XZ_ACT_FETCHING` | `Đang kết nối Server...` (spinner) |
| `XZ_ACT_CODE_WAIT` | Code 6 số to + message + timeout countdown |
| `XZ_ACT_TIMEOUT` | `Hết thời gian. Đang thử lại...` |
| `XZ_ACT_RETRY` | `Đang thử lại... (n/10)` |
| `XZ_ACT_DONE` | ✓ `Kích hoạt thành công!` (1.5s rồi sang Phase B) |
| `XZ_ACT_ERROR` | `Lỗi kết nối` + error code |

---

## Phase B: AI Chat Flow (sau khi activated)

### Luồng WebSocket (từ `websocket.md`)

```
ESP32                             XiaoZhi Server
  │                                     │
  │── Upgrade HTTP → WebSocket ────────▶│
  │   Headers:                          │
  │     Authorization: Bearer <token>   │
  │     Protocol-Version: 1             │
  │     Device-Id: <MAC>                │
  │     Client-Id: <UUID>               │
  │                                     │
  │── JSON: hello ─────────────────────▶│
  │   { "type": "hello",                │
  │     "version": 1,                   │
  │     "transport": "websocket",       │
  │     "audio_params": {               │
  │       "format": "opus",             │
  │       "sample_rate": 16000,         │
  │       "channels": 1,               │
  │       "frame_duration": 60 } }      │
  │                                     │
  │◀── JSON: hello ─────────────────────│
  │   { "type": "hello",                │
  │     "transport": "websocket",       │
  │     "session_id": "xxx",            │
  │     "audio_params": {               │
  │       "sample_rate": 24000 } }      │
  │                                     │
  │── JSON: listen/start ──────────────▶│
  │── Binary: Opus 16kHz mic ──────────▶│  (streaming)
  │                                     │
  │◀── JSON: stt { text: "..." } ───────│
  │◀── JSON: tts { state: "start" } ────│
  │◀── Binary: Opus 24kHz TTS ──────────│  (streaming)
  │◀── JSON: tts { state: "stop" } ─────│
```

### Màn hình OLED — Screen B: AI Chat

> **Dùng `effect_xiaozhi` với state injection theo XiaozhiState.**

```
┌──────────────────────────────┐
│  ●●● (dots connecting...)    │  ← XZ_CONNECTING
│  (O)(O)                      │
│  ~~~smile~~~                 │
├──────────────────────────────┤
│  (O)(O)  đang nghe...        │  ← XZ_LISTENING
│  [mic pulse]                 │  ← Mắt đảo theo stereo mic
│  ~~~mouth pulse~~~           │
├──────────────────────────────┤
│  (^‿^)(^‿^)  đang nói...    │  ← XZ_SPEAKING
│  ===open mouth animate===    │  ← happy_crescent eyes
│  [TTS energy driven]         │
├──────────────────────────────┤
│  (O)(O)   chờ lệnh...       │  ← XZ_IDLE
│  ~~~resting smile~~~         │
└──────────────────────────────┘
```

**State machine Screen B:**

| XiaozhiState | Mắt | Miệng | Blink |
|--------------|-----|-------|-------|
| `XZ_CONNECTING` | Normal, nháy nhanh (25 tick) | Resting đóng | 25 tick |
| `XZ_LISTENING` | Đảo theo stereo mic energy | Pulse nhẹ mic energy | 85 tick |
| `XZ_SPEAKING` | `happy_crescent` ^_^ | Mở theo `tts_energy` (0-11px) | 200 tick |
| `XZ_IDLE` | Normal | Resting smile | 85 tick |
| `XZ_ERROR` | Wink (>) | Flat line | — |

---

## XiaoZhi State Machine đầy đủ

```
[enter AUDIO_MODE_XIAOZHI]
         │
         ▼
   [Kiểm tra WiFi] ──(Chưa có)──▶ Bật AP Setup Portal (MVT-Audio-Setup)
         │                               │
         │ (Đã kết nối)                  │ (User nhập WiFi xong)
         ▼                               ▼
   XZ_ACT_FETCHING ──── HTTP /check-version ────▶
         │                                         │ 200 OK + activation.code
         │◀── retry (err) ──────────────────────────│
         │                                         │ no activation needed
         ▼                                         │
   XZ_ACT_CODE_WAIT ◀────────────────────────────  │
         │                                         │ (no activation obj)
         │  [user confirms in App]                 │
         ▼                                         ▼
   XZ_ACT_DONE ──────────────────────────── XZ_IDLE
         │                                         │
         └──────────────────────────────────────▶  │
                                                   │ [Button BACK / auto]
                                                   ▼
                                           XZ_CONNECTING
                                                   │
                                          [WS hello/ack]
                                                   ▼
                                           XZ_LISTENING ──▶ [mic stream]
                                                   │
                                        [tts/start msg]
                                                   ▼
                                           XZ_SPEAKING ◀── [TTS binary]
                                                   │
                                         [tts/stop msg]
                                                   │
                                                   ▼
                                           XZ_LISTENING (auto) / XZ_IDLE
                                                   │
                              [mode switch / error / timeout]
                                                   │
                                                   ▼
                                           [cleanup → exit mode]
```

---

## Các file cần tạo mới

### `src/xiaozhi_config.h`

```cpp
#pragma once

// OTA/Activation HTTP endpoint
#define XZ_OTA_DEFAULT_URL    "https://api.tenclass.net/xiaozhi/ota/"
#define XZ_NVS_KEY_OTA_URL    "xz_ota_url"

// WebSocket (sau khi server trả về websocket config)
#define XZ_WS_DEFAULT_URL     "wss://api.tenclass.net/xiaozhi/v1/"
#define XZ_NVS_KEY_WS_URL     "xz_ws_url"
#define XZ_NVS_KEY_TOKEN      "xz_tok"
#define XZ_NVS_KEY_CLIENT_ID  "xz_cid"
#define XZ_NVS_KEY_ACTIVATED  "xz_act"    // bool: đã activation hay chưa

// Audio params
#define XZ_SAMPLE_RATE_MIC    16000
#define XZ_SAMPLE_RATE_TTS    24000
#define XZ_FRAME_DURATION_MS  60
#define XZ_OPUS_COMPLEXITY    5

// DAC I2S (PCM5102A — fault-tolerant)
#define XZ_DAC_BCK_PIN        18
#define XZ_DAC_LCK_PIN        19
#define XZ_DAC_DIN_PIN        23

// Timeouts
#define XZ_ACT_TIMEOUT_MS     30000
#define XZ_ACT_MAX_RETRY      10
#define XZ_CONNECT_TIMEOUT_MS 10000
#define XZ_ERROR_RECOVERY_MS  3000
```

### `src/xiaozhi_mode.h`

```cpp
#pragma once
#include <Arduino.h>

// Activation sub-states
enum XiaozhiActState {
    XZ_ACT_IDLE      = 0,  // Chưa bắt đầu
    XZ_ACT_WIFI_WAIT = 1,  // Đang đợi WiFi hoặc đang ở AP Portal
    XZ_ACT_FETCH     = 2,  // Đang gọi HTTP /check-version
    XZ_ACT_WAIT      = 3,  // Đang hiển thị code 6 số, chờ user xác nhận
    XZ_ACT_DONE      = 4,  // Activation thành công
    XZ_ACT_ERROR     = 5,  // Lỗi kết nối
};

// Chat sub-states (sau activation)
enum XiaozhiState {
    XZ_IDLE       = 0,
    XZ_CONNECTING = 1,
    XZ_LISTENING  = 2,
    XZ_SPEAKING   = 3,
    XZ_ERROR      = 4,
};

// Screen selector để display biết vẽ gì
enum XiaozhiScreen {
    XZ_SCREEN_ACTIVATION = 0,  // Screen A: 6-digit code
    XZ_SCREEN_CHAT       = 1,  // Screen B: eye animation
};

void         xiaozhi_init();
void         xiaozhi_start();
void         xiaozhi_stop();
void         xiaozhi_loop();

// Chat controls
void         xiaozhi_start_listen();
void         xiaozhi_stop_listen();

// Query for display
XiaozhiScreen   xiaozhi_get_screen();
XiaozhiActState xiaozhi_get_act_state();
const char*     xiaozhi_get_act_code();        // "123456"
const char*     xiaozhi_get_act_message();     // Server message string
int             xiaozhi_get_act_timeout_sec(); // Countdown giây còn lại
XiaozhiState    xiaozhi_get_state();
float           xiaozhi_get_tts_energy();      // 0.0-1.0
const char*     xiaozhi_get_emotion();
```

### `src/xiaozhi_mode.cpp`

**Task khi AUDIO_MODE_XIAOZHI active:**

```
Phase A:
  xz_act_task  (Core 1, P3):
    1. Kiểm tra wifi_app_is_connected(). Nếu chưa, chờ WiFi ready (hoặc user config qua AP)
    2. POST /check-version → lấy activation code
    3. Nếu có code → set screen = XZ_SCREEN_ACTIVATION, lưu code, countdown
    4. Loop POST /activate (retry ≤10, delay 3s)
    5. Khi 200 OK → lưu NVS "xz_act"=true, lưu WS URL từ response
    6. set screen = XZ_SCREEN_CHAT, trigger Phase B

Phase B (chạy sau Phase A):
  xz_ws_task   (Core 1, P3):  WebSocket I/O + JSON dispatch
  xz_mic_task  (Core 0, P2):  I2S capture → Opus encode → WS binary send
  xz_play_task (Core 1, P2):  WS binary → Opus decode → I2S DAC output
```

**JSON message handlers (Phase B):**

| JSON type | Hành động |
|-----------|----------|
| `tts.state = start` | state → XZ_SPEAKING |
| `tts.state = stop`  | state → XZ_LISTENING / XZ_IDLE |
| `stt`               | `display_toast(text)` |
| `llm`               | lưu emotion |
| `system.reboot`     | `ESP.restart()` |

---

## Các file cần sửa đổi

### 1. `src/i2s_mic.h` — Thêm `AUDIO_MODE_XIAOZHI`

```diff
 enum AudioMode {
     AUDIO_MODE_MIC     = 0,
     AUDIO_MODE_BT      = 1,
     AUDIO_MODE_CLOCK   = 2,
+    AUDIO_MODE_XIAOZHI = 3,
 };
```

### 2. `src/display.h` — Thêm prototype render activation screen

```diff
+/** Render XiaoZhi activation screen (6-digit code display / WiFi status). */
+void display_draw_xiaozhi_activation(const char *code, const char *message,
+                                     int timeout_sec, int act_sub_state);
+
+/** Render XiaoZhi AI chat screen (eye/mouth animation). */
+void display_draw_xiaozhi_chat(XiaozhiState state, float tts_energy);
```

### 3. `src/display.cpp` — Implement 2 hàm mới

#### `display_draw_xiaozhi_activation()` — Screen A
```
Layout OLED 128×64:
  Row 0 (y=0-9):   "XIAOZHI" | sub-state label    (font 6x10)
  Row 1 (y=12-35): Code 6 số — mỗi digit 18px wide (font 10x20 hoặc u8g2_font_fur20_tf)
  Row 2 (y=38-50): message từ server               (font 6x10, truncate nếu dài)
  Row 3 (y=52-63): Spinner dots "..." hoặc countdown "29s"
```

**Khi `XZ_ACT_WIFI_WAIT`:** Hiển thị `WiFi kết nối...` hoặc `AP: MVT-Audio-Setup`.  
**Khi `XZ_ACT_WAIT`:** Hiển thị code, nhấp nháy từng chữ số mỗi 500ms.  
**Khi `XZ_ACT_FETCH`:** Chỉ hiển thị spinner `⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏` hoặc `[###   ]`.  
**Khi `XZ_ACT_DONE`:** `✓ Đã kích hoạt!` (1.5s).  
**Khi `XZ_ACT_ERROR`:** `ERR: <message>` + icon X.

#### `display_draw_xiaozhi_chat()` — Screen B
Gọi lại `effect_xiaozhi_set_state()` + `effect_xiaozhi_render()` với silence frames khi không có audio.

### 4. `src/main.cpp` — Thêm XIAOZHI vào switch + loop

```diff
+#include "xiaozhi_mode.h"

+// Button CONFIRM (GPIO 14) -> Đổi Face Theme (Short) / Đổi Conversation Mode (Long)
+    if (button_pressed(BTN_PLUS)) {
+        if (s_current_mode == AUDIO_MODE_XIAOZHI) {
+            xiaozhi_next_face_theme();
+            LOG_D("XZ", "Next Face Theme");
+        } else {
+            display_next_mode();
+            nvs_save_display_mode((uint8_t)display_get_mode());
+        }
+    }
+    if (button_long_pressed(BTN_PLUS)) {
+        if (s_current_mode == AUDIO_MODE_XIAOZHI) {
+            bool is_auto = !xiaozhi_is_auto_mode();
+            xiaozhi_set_auto_mode(is_auto);
+            display_toast(is_auto ? "MODE: AUTO CHAT" : "MODE: MANUAL PTT");
+        } else {
+            bool auto_cycle = !display_get_auto_cycle();
+            display_set_auto_cycle(auto_cycle);
+            nvs_save_auto_cycle(auto_cycle);
+        }
+    }

 // Button PUSH → MIC → BT → CLOCK → XIAOZHI → MIC
     if      (s_current_mode == AUDIO_MODE_MIC)     next_mode = AUDIO_MODE_BT;
     else if (s_current_mode == AUDIO_MODE_BT)      next_mode = AUDIO_MODE_CLOCK;
-    else                                            next_mode = AUDIO_MODE_MIC;
+    else if (s_current_mode == AUDIO_MODE_CLOCK)   next_mode = AUDIO_MODE_XIAOZHI;
+    else                                            next_mode = AUDIO_MODE_MIC;

+// Button BACK → Push-to-talk (chỉ khi đang ở Screen B / IDLE/LISTENING)
+    else if (s_current_mode == AUDIO_MODE_XIAOZHI) {
+        if (xiaozhi_get_screen() == XZ_SCREEN_CHAT) {
+            if (xiaozhi_get_state() == XZ_SPEAKING)  xiaozhi_abort_speaking(); // Ngắt lời bot
+            else if (xiaozhi_get_state() == XZ_LISTENING) xiaozhi_stop_listen();
+            else                                     xiaozhi_start_listen();
+        }
+    }

+// Rotary Encoder EC11 -> Điều chỉnh âm lượng loa ở cả BT và XiaoZhi mode
+    if (s_current_mode == AUDIO_MODE_BT || s_current_mode == AUDIO_MODE_XIAOZHI) {
+        int32_t enc_delta = encoder_get_delta();
+        if (enc_delta != 0) {
+            if (s_current_mode == AUDIO_MODE_BT) {
+                bt_audio_adjust_volume(enc_delta);
+                display_show_volume(bt_audio_get_volume());
+            } else {
+                xiaozhi_adjust_volume(enc_delta);
+                display_show_volume(xiaozhi_get_volume());
+            }
+        }
+    }

+// Button BOOT (GPIO 0) → Giữ nút mở Web Setup Portal
+    else if (s_current_mode == AUDIO_MODE_XIAOZHI) {
+        if (button_pressed(BTN_BOOT) || button_long_pressed(BTN_BOOT)) {
+            LOG_I("BTN", "XIAOZHI Mode: BOOT pressed -> Launching WiFi Web Setup AP...");
+            wifi_app_start_ap_portal();
+        }
+    }
```

```diff
 // switch_audio_mode() — thêm case XIAOZHI
+else if (target_mode == AUDIO_MODE_XIAOZHI) {
+    s_mic_task_active = false;
+    delay(150);
+    i2s_mic_deinit();
+    encoder_set_enabled(false);
+    bt_audio_stop();
+    if (s_audio_queue) xQueueReset(s_audio_queue);
+    beat_detector_reset();
+    // Sử dụng chung wifi_app_init() với Clock mode
+    wifi_app_init();       // non-blocking, fault-tolerant
+    xiaozhi_start();
+    // Force display mode to XIAOZHI eye effect
+    display_set_mode(MODE_XIAOZHI, false);
+    display_set_audio_mode(AUDIO_MODE_XIAOZHI, false, false);
+    s_current_mode = AUDIO_MODE_XIAOZHI;
+    nvs_save_audio_mode(AUDIO_MODE_XIAOZHI);
+    display_toast("MODE: XIAOZHI AI");
+}
+// Rời XIAOZHI → cleanup
+if (s_current_mode == AUDIO_MODE_XIAOZHI) {
+    xiaozhi_stop();
+}
```

```diff
 // loop() — thêm XIAOZHI render path
+if (s_current_mode == AUDIO_MODE_XIAOZHI) {
+    wifi_app_loop(true);
+    xiaozhi_loop();
+    // Render theo screen hiện tại
+    if (xiaozhi_get_screen() == XZ_SCREEN_ACTIVATION) {
+        display_draw_xiaozhi_activation(
+            xiaozhi_get_act_code(),
+            xiaozhi_get_act_message(),
+            xiaozhi_get_act_timeout_sec(),
+            (int)xiaozhi_get_act_state()
+        );
+    } else {
+        display_draw_xiaozhi_chat(xiaozhi_get_state(), xiaozhi_get_tts_energy());
+    }
+}
```

### 5. `src/effects/effect_xiaozhi.cpp` — Thêm state injection

```diff
+#include "xiaozhi_mode.h"
+static XiaozhiState s_xz_state      = XZ_IDLE;
+static float        s_xz_tts_energy = 0.0f;

+void effect_xiaozhi_set_state(XiaozhiState state, float tts_energy) {
+    s_xz_state      = state;
+    s_xz_tts_energy = tts_energy;
+}

 // Trong effect_xiaozhi_render():
-bool is_happy = (avg > 0.30f && ...);
+bool  is_happy     = false;
+float target_open  = 0.0f;
+int   blink_period = 85;
+switch (s_xz_state) {
+    case XZ_SPEAKING:
+        is_happy = true; target_open = s_xz_tts_energy * 11.0f; blink_period = 200; break;
+    case XZ_LISTENING:
+        target_open = avg * 11.0f + s_beat_pulse * 4.0f; break;
+    case XZ_CONNECTING:
+        blink_period = 25; target_open = 0.0f; break;
+    case XZ_ERROR:
+        wink_l = true; target_open = 0.0f; break;
+    default:
+        is_happy   = (avg > 0.30f && (treble > 0.22f || s_beat_pulse > 0.45f));
+        target_open = avg * 11.0f + s_beat_pulse * 4.0f; break;
+}
```

### 6. `src/effects/effects.h` — Thêm prototype

```diff
 void effect_xiaozhi_render(const int32_t *left, const int32_t *right, size_t n);
 void effect_xiaozhi_on_enter();
 void effect_xiaozhi_on_exit();
+enum XiaozhiState : int;
+void effect_xiaozhi_set_state(XiaozhiState state, float tts_energy);
```

### 7. `src/nvs_storage.h/.cpp` — Thêm NVS keys

```cpp
void nvs_save_xz_ota_url(const char *url);
bool nvs_load_xz_ota_url(char *out, size_t max_len);
void nvs_save_xz_ws_url(const char *url);
bool nvs_load_xz_ws_url(char *out, size_t max_len);
void nvs_save_xz_token(const char *token);
bool nvs_load_xz_token(char *out, size_t max_len);
void nvs_save_xz_client_id(const char *cid);
bool nvs_load_xz_client_id(char *out, size_t max_len);
void nvs_save_xz_activated(bool activated);
bool nvs_load_xz_activated();
```

### 8. `src/wifi_app.cpp` — WiFiManager portal config mở rộng

```cpp
// Thêm custom parameter cho XiaoZhi trên cùng Web Portal cài WiFi
WiFiManagerParameter xz_url_param("xz_ota", "XiaoZhi OTA Server URL",
                                   nvs_loaded_ota_url, 128);
wifiManager.addParameter(&xz_url_param);
// Sau khi người dùng ấn Save:
nvs_save_xz_ota_url(xz_url_param.getValue());
```

### 9. `platformio.ini` — Thêm thư viện

```ini
lib_deps =
    ; ... (giữ nguyên)
    links2004/arduinoWebSockets@^2.4.1
    pschatzmann/arduino-libopus@^1.0.0
```

---

## Cấu trúc file hoàn chỉnh sau thêm

```
src/
├── main.cpp                   [MODIFY] — mode cycle, switch, loop, button
├── i2s_mic.h                  [MODIFY] — thêm AUDIO_MODE_XIAOZHI
├── display.h                  [MODIFY] — thêm 2 hàm draw mới
├── display.cpp                [MODIFY] — implement activation + chat screen
├── nvs_storage.h/.cpp         [MODIFY] — thêm xz_* NVS keys
├── wifi_app.cpp               [MODIFY] — WiFiManager XiaoZhi OTA URL param
├── xiaozhi_config.h           [NEW]    — compile-time constants
├── xiaozhi_mode.h             [NEW]    — public interface, enums
├── xiaozhi_mode.cpp           [NEW]    — activation task + WebSocket + Opus
└── effects/
    ├── effects.h              [MODIFY] — thêm effect_xiaozhi_set_state
    └── effect_xiaozhi.cpp     [MODIFY] — state injection + expression override
```

---

## Fault Tolerance

| Scenario | Hành vi |
|----------|---------|
| WiFi chưa cấu hình / mất mạng | Hiển thị thông báo, tự mở AP portal `MVT-Audio-Setup` để kết nối lại |
| HTTP /check-version lỗi | Retry ≤ 10 lần, exponential backoff |
| Activation timeout 30s | Server trả `activation.timeout_ms`, hiển thị countdown → retry |
| Đã activated NVS | Bỏ qua Phase A, vào Phase B trực tiếp |
| PCM5102A không có | `xz_dac_init()` fail → log warning, mất audio output |
| INMP441 không có | Bỏ qua mic capture |
| WebSocket bị ngắt | Auto-reconnect 5 lần, hiển thị `XZ: RECONNECTING...` |
| Server timeout WS > 10s | `XZ_ERROR`, wink face, tự về IDLE sau 3s |
| Opus decode fail | Drop frame, log, tiếp tục |
| Rời mode về MIC/BT/CLOCK | `xiaozhi_stop()` kill tất cả tasks, close WS, free Opus |
| Xóa NVS (reset) | Activation lại từ đầu — re-enter Phase A |

---

## Thứ tự triển khai (6 phases)

### Phase 1 — Foundation
- [ ] Tạo `xiaozhi_config.h`, `xiaozhi_mode.h` (enums + interfaces)
- [ ] Sửa `i2s_mic.h`: thêm `AUDIO_MODE_XIAOZHI`
- [ ] Sửa `nvs_storage`: thêm XiaoZhi NVS keys
- [ ] Thêm `arduinoWebSockets` + `arduino-libopus` vào `platformio.ini`
- [ ] Verify build pass

### Phase 2 — Activation Screen (Screen A) & WiFi Shared Setup
- [ ] Tạo `xiaozhi_mode.cpp` với `xz_act_task`
- [ ] Tích hợp kiểm tra `wifi_app_is_connected()` và fallback mở Portal AP nếu cần
- [ ] Implement HTTP client: POST `/check-version`, parse activation JSON
- [ ] Implement `display_draw_xiaozhi_activation()` trong `display.cpp`
  - Layout: code 6 số font lớn + message + spinner + countdown
  - Nhấp nháy từng digit khi `XZ_ACT_WAIT`
- [ ] Implement retry loop: POST `/activate` (≤10 lần, 3s)
- [ ] Lưu NVS `xz_act=true` khi activation thành công
- [ ] Test: hiển thị đúng code, xác nhận thành công/timeout

### Phase 3 — WebSocket Chat (Screen B)
- [ ] `xz_ws_task`: connect WS, gửi hello, nhận JSON + binary
- [ ] JSON dispatcher: `tts/start`, `tts/stop`, `stt`, `llm`, `system`
- [ ] State machine: IDLE → CONNECTING → LISTENING → SPEAKING
- [ ] Test: WebSocket connect, hello handshake, receive TTS

### Phase 4 — Audio pipeline
- [ ] `xz_mic_task`: INMP441 16kHz → Opus encode → WS binary
- [ ] `xz_play_task`: WS binary → Opus decode → PCM5102A 24kHz
- [ ] Skip DAC gracefully nếu hardware không có
- [ ] Test: full conversation loop

### Phase 5 — OLED UI integration (Screen B eyes)
- [ ] Sửa `effect_xiaozhi.cpp`: thêm `effect_xiaozhi_set_state()`
- [ ] Implement `display_draw_xiaozhi_chat()` gọi effect
- [ ] Inject state từ `loop()` → effect
- [ ] Test: eyes animate đúng per-state

### Phase 6 — Polish & Validation
- [ ] Graceful switch: MIC↔XZ, BT↔XZ, CLOCK↔XZ
- [ ] Button BACK = Push-to-talk đúng trong Screen B
- [ ] Button BOOT = Mở AP Portal cấu hình lại WiFi / Server URL
- [ ] NVS skip activation khi đã activated trước đó
- [ ] `ESP.getFreeHeap()` monitor — không leak
- [ ] OLED FPS ≥ 25 khi stream audio
- [ ] Serial log sạch trong mọi state

---

## Tham chiếu

| File | Vai trò |
|------|---------|
| [`xiaozhi-esp32/main/application.cc:713`](file:///d:/projects/xiaozhi-esp32/main/application.cc#L713) | `ShowActivationCode()` — logic hiển thị + phát âm |
| [`xiaozhi-esp32/main/application.cc:431`](file:///d:/projects/xiaozhi-esp32/main/application.cc#L431) | `CheckNewVersion()` — activation loop |
| [`xiaozhi-esp32/main/ota.cc:77`](file:///d:/projects/xiaozhi-esp32/main/ota.cc#L77) | `CheckVersion()` — HTTP + JSON parse |
| [`xiaozhi-esp32/main/ota.cc:458`](file:///d:/projects/xiaozhi-esp32/main/ota.cc#L458) | `Activate()` — POST /activate |
| [`xiaozhi-esp32/docs/websocket.md`](file:///d:/projects/xiaozhi-esp32/docs/websocket.md) | Giao thức WebSocket đầy đủ |
| [`py-xiaozhi/src/protocols/websocket_protocol.py`](file:///d:/projects/py-xiaozhi/src/protocols/websocket_protocol.py) | Client Python reference |
| [`src/wifi_app.cpp`](file:///d:/projects/esp32-project/src/wifi_app.cpp) | WiFiManager Captive Portal & WiFi STA management |
| [`src/effects/effect_xiaozhi.cpp`](file:///d:/projects/esp32-project/src/effects/effect_xiaozhi.cpp) | OLED eye/mouth animation |
| [`src/main.cpp`](file:///d:/projects/esp32-project/src/main.cpp) | Main loop + switch_audio_mode() |
