# Plan: ESP32 VU Meter V2 — Nâng cấp Bluetooth + PCM5102A DAC + Encoder EC11 + WiFi

> **Tác giả:** Mạc Tân  
> **Ngày:** 2026-08-18  
> **Phiên bản hiện tại:** ESP32-S3 Super Mini — 65 chế độ visualizer OLED (Phase 7 hoàn thành)  
> **Phiên bản mới:** ESP32-WROOM — BT A2DP + DAC PCM5102A + Encoder EC11 + WiFi (OTA/NTP/Weather)  
> **Pin mapping:** Phương án A (CONFIRMED ✅)

---

## 1. Tổng Quan Requirement (Đã Xác Nhận)

### Priority thứ tự implement

| #   | Tính năng                  | Phase | Ghi chú                     |
| --- | -------------------------- | ----- | --------------------------- |
| 1   | **INMP441** mic stereo     | 8     | Core feature, migrate trước |
| 2   | **Bluetooth A2DP** + AVRCP | 11    | Main new feature            |
| 3   | **PCM5102A** DAC output    | 13    | Thứ yếu, fault-tolerant     |
| 4   | **WiFi** (AP config + OTA) | 15    | Phase cuối                  |
| 5   | **NTP + Weather** display  | 16    | Bonus — phụ thuộc WiFi      |

---

### Requirement Table

| Hạng mục                     | Nội dung                                                                  |
| ---------------------------- | ------------------------------------------------------------------------- |
| **Vi điều khiển mới**        | ESP32-WROOM (thay ESP32-S3 Super Mini)                                    |
| **Nguồn audio**              | 2 mic INMP441 (MIC mode) **hoặc** Bluetooth A2DP Sink (BT mode)           |
| **DAC output**               | PCM5102A (GY-PCM5102) qua I2S — **thứ yếu**, fault-tolerant               |
| **OLED Visualizer**          | ✅ Cả 2 mode đều có visualizer — **BT audio → FFT → OLED**                |
| **65 effects**               | ✅ **Giữ nguyên toàn bộ 65 effects**                                      |
| **Encoder EC11**             | Xoay = điều chỉnh volume BT output                                        |
| **Button PUSH (encoder)**    | Switch mode: MIC ↔ BT                                                     |
| **Button PLUS / CON (dưới)** | Chuyển chế độ hiển thị OLED (thay GPIO 0 cũ)                              |
| **Button BACK / BAK (trên)** | Play/Pause (AVRCP sync với thiết bị phát)                                 |
| **Volume sync**              | ✅ Đồng bộ 2 chiều với điện thoại/PC (AVRCP Absolute Volume)              |
| **Play/Pause sync**          | ✅ Đồng bộ với thiết bị phát (AVRCP)                                      |
| **Nhớ mode**                 | ✅ Lưu mode (MIC/BT) vào NVS — sau power cycle giữ nguyên                 |
| **Nhớ device BT**            | ✅ Lưu địa chỉ thiết bị BT đã kết nối — tự reconnect khi bật lại          |
| **Chuyển mode mượt**         | ✅ Giải phóng tài nguyên mode cũ, không crash                             |
| **Tên Bluetooth**            | `MVT VU METER V2`                                                         |
| **WiFi AP mode**             | ✅ Captive portal để cấu hình WiFi credentials (phase cuối)               |
| **WiFi STA + OTA**           | ✅ Kết nối router, update firmware qua browser                            |
| **Màn hình Clock/Weather**   | ✅ Mode hiển thị giờ + thời tiết trên OLED (khi có WiFi)                  |
| **Button BOOT (GPIO 0)**     | Short press = Reset WiFi → vào Captive Portal; Long press = BT re-pairing |

---

## 2. Fault-Tolerant Matrix (Đã Xác Nhận)

| Thiếu module                | Ảnh hưởng                            | Hệ thống vẫn hoạt động                     |
| --------------------------- | ------------------------------------ | ------------------------------------------ |
| Không có PCM5102A DAC       | Không có audio output                | ✅ OLED visualizer BT vẫn chạy             |
| Không có 2 mic INMP441      | Không có MIC mode audio              | ✅ BT mode vẫn hoạt động bình thường       |
| Không có encoder EC11       | Không điều chỉnh được volume         | ✅ MIC mode vẫn chạy bình thường           |
| Lỗi I2S init (bất kỳ)       | Chỉ log warning                      | ✅ Không halt, OLED không bị ảnh hưởng     |
| Không có WiFi / chưa config | Không có OTA, không có clock/weather | ✅ MIC + BT + DAC vẫn hoạt động 100%       |
| WiFi mất kết nối            | NTP/Weather không update             | ✅ Hiển thị giờ từ RTC nội, weather cached |
| API weather lỗi             | Không update thời tiết               | ✅ Hiển thị dữ liệu cache hoặc dấu "--"    |

---

## 3. Phân Tích Kỹ Thuật Quan Trọng

### 3.1 — ESP32-WROOM vs ESP32-S3: Khác biệt mấu chốt

| Thông số  | ESP32-S3 Super Mini         | ESP32-WROOM (mới)      |
| --------- | --------------------------- | ---------------------- |
| CPU       | Dual-core LX7, 240MHz       | Dual-core LX6, 240MHz  |
| Bluetooth | ❌ BLE only (không có A2DP) | ✅ BT Classic + BLE    |
| RAM       | 512KB + PSRAM option        | 520KB (không có PSRAM) |
| USB       | Native USB CDC              | Qua CH340/CP2102       |
| GPIO      | 45 chân                     | 34 chân dùng được      |

> **Lý do bắt buộc dùng WROOM:** Bluetooth Classic A2DP Sink chỉ chạy trên ESP32 (LX6), không chạy trên S3.

> ⚠️ **Phải xóa flag `-DBOARD_HAS_PSRAM`** trong platformio.ini khi migrate sang WROOM.

---

### 3.2 — RAM Budget (Quan trọng nhất khi giữ 65 effects)

| Thành phần             | RAM ước tính           |
| ---------------------- | ---------------------- |
| BT Classic A2DP stack  | ~150–200KB             |
| FreeRTOS + tasks       | ~30KB                  |
| Audio buffer (BT PCM)  | ~8KB                   |
| OLED framebuffer       | ~2KB                   |
| FFT buffer             | ~4KB                   |
| 65 effects code (IRAM) | ~10–20KB               |
| Stack tasks            | ~16KB                  |
| **Tổng ước tính**      | **~220–280KB / 520KB** |

→ **Khả thi**, nhưng cần cẩn thận với stack size và buffer allocation.

---

### 3.3 — I2S Port Allocation

| Port        | Chức năng             | Trạng thái                  |
| ----------- | --------------------- | --------------------------- |
| **I2S0**    | INMP441 mic input     | Active khi MIC mode         |
| **I2S1**    | PCM5102A DAC output   | Active khi BT mode + có DAC |
| **BT A2DP** | Nhận PCM từ Bluetooth | Dùng internal BT stack      |

**Flow BT mode:**

```
BT stack → bt_audio_callback() → [audio_queue] → FFT/visualizer (OLED)
                                               → I2S1 output (PCM5102A) — nếu có
```

---

### 3.4 — Chuyển Mode Mượt (Resource Release)

Yêu cầu: Không crash khi switch MIC ↔ BT.

**Quy trình switch mode:**

```
1. Dừng mic_task (suspend FreeRTOS task hoặc set flag)
2. Deinit I2S0 (mic port) — nếu chuyển sang BT
3. Flush audio queue
4. Init BT A2DP sink
5. Init I2S1 (DAC) — nếu có hardware
6. Lưu mode mới vào NVS
```

**Ngược lại (BT → MIC):**

```
1. Disconnect BT / stop A2DP
2. Deinit I2S1 (DAC)
3. Flush audio queue
4. Init I2S0 (mic)
5. Resume mic_task
6. Lưu mode mới vào NVS
```

---

### 3.5 — NVS Storage (Nhớ Mode + BT Device)

Dùng ESP32 NVS (Non-Volatile Storage) để lưu:

- `"audio_mode"` → `0` (MIC) hoặc `1` (BT)
- `"bt_mac"` → MAC address 6 bytes của thiết bị BT đã pair lần cuối

Khi khởi động:

- Đọc `"audio_mode"` → khởi tạo đúng mode
- Nếu mode BT: đọc `"bt_mac"` → tự động reconnect

---

## 4. Pin Mapping Chính Thức (Phương Án A — CONFIRMED ✅)

> Module EC11 header: `CON | SDA | SCL | PSH | TRA | TRB | BAK | GND | VCC`

### ✅ Kiểm tra Button List — ĐỦ

| Tên             | Chân module | GPIO   | Short Press                 | Long Press (>1s)         |
| --------------- | ----------- | ------ | --------------------------- | ------------------------ |
| Encoder xoay    | TRA + TRB   | 32+33  | Volume +/- (ISR)            | —                        |
| **Button PUSH** | PSH         | **4**  | Switch mode MIC ↔ BT        | —                        |
| **Button BACK** | BAK         | **13** | Play/Pause BT (AVRCP)       | —                        |
| **Button PLUS** | CON         | **14** | Next OLED display mode      | Toggle Auto-Cycle ON/OFF |
| **Button BOOT** | trên board  | **0**  | Reset WiFi → Captive Portal | BT Re-pairing (>3s)      |

> 💡 **"Button PLUS"** = chân **CON** trên module EC11 (button phía dưới)  
> → Kế thừa nguyên vẹn chức năng cũa GPIO 0:  
> → **Short press** = Next OLED mode (như hiện tại)  
> → **Long press** = Toggle Auto-Cycle ON/OFF (như hiện tại)

> ⚠️ **Không dùng GPIO 0 nữa** — trên ESP32-WROOM, GPIO 0 là strapping pin (boot mode) gây phiền khi debug. Dùng GPIO 14 sạch hơn.

---

### Bảng Pin Mapping Đầy Đủ

> ✅ Tất cả GPIO dưới đây **không phải strapping pin** → an toàn 100%

| Chức năng              | Chân module | GPIO WROOM | Ghi chú                                         |
| ---------------------- | ----------- | ---------- | ----------------------------------------------- |
| **OLED SDA**           | SDA         | **21**     | I2C hardware default                            |
| **OLED SCL**           | SCL         | **22**     | I2C hardware default                            |
| **INMP441 SCK** (BCLK) | SCK         | **26**     | I2S0 Serial Clock                               |
| **INMP441 WS** (LRCLK) | WS          | **25**     | I2S0 Word Select                                |
| **INMP441 SD** (DATA)  | SD          | **27**     | I2S0 Serial Data                                |
| **PCM5102A BCK**       | —           | **18**     | I2S1 output                                     |
| **PCM5102A LCK**       | —           | **19**     | I2S1 output                                     |
| **PCM5102A DIN**       | —           | **23**     | I2S1 output                                     |
| **Encoder CLK**        | TRA         | **32**     | Interrupt, có pull-up nội                       |
| **Encoder DT**         | TRB         | **33**     | Interrupt, có pull-up nội                       |
| **Button PUSH**        | PSH         | **4**      | Pull-up, interrupt                              |
| **Button BACK**        | BAK         | **13**     | Pull-up, interrupt                              |
| **Button PLUS**        | CON         | **14**     | Pull-up, interrupt (thay GPIO 0)                |
| **Button BOOT**        | trên board  | **0**      | Short press: WiFi reset; Long press: BT re-pair |
| **VCC module**         | VCC         | 3.3V       | —                                               |
| **GND module**         | GND         | GND        | —                                               |

> ⚠️ **Lưu ý quan trọng:** Phương án A cũ có **GPIO 12 cho PCM5102A LCK** — **SAI!**  
> GPIO 12 là strapping pin (MTDI) trên ESP32, nếu HIGH lúc boot → lỗi flash voltage.  
> Đã sửa thành GPIO 18/19/23 cho PCM5102A.

---

## 5. Kiến Trúc Hệ Thống Mới

```
┌─────────────────────────────────────────────────────────────────────┐
│                          ESP32-WROOM                                │
│                                                                     │
│  ┌──────────────┐  I2S0 in  ┌────────────────────────┐             │
│  │ INMP441 L+R  │ ─────────▶│  mic_task (Core 0)     │             │
│  └──────────────┘           └────────────┬───────────┘             │
│         [Fault: init fail → skip, no halt]          │ Queue        │
│                                                     ▼             │
│  ┌──────────────┐  BT A2DP  ┌────────────────────────┐             │
│  │  Phone / PC  │◀─────────▶│  bt_audio_task (Core 0)│             │
│  └──────────────┘  AVRCP    └────────────┬───────────┘             │
│         Vol sync 2 chiều                 │ Queue                   │
│                                          ▼                         │
│                              ┌────────────────────────┐             │
│                              │   main loop (Core 1)   │             │
│                              │  ┌──────────────────┐  │             │
│                              │  │  Audio Source Mux│  │             │
│                              │  │  MIC mode / BT   │  │             │
│                              │  └────────┬─────────┘  │             │
│                              │           │             │             │
│                              │  ┌────────▼─────────┐  │             │
│                              │  │  AGC + FFT + RMS  │  │             │
│                              │  └────────┬─────────┘  │             │
│                              │           │             │             │
│                              │  ┌────────▼─────────┐  │             │
│                              │  │  65 Effects OLED  │  │             │
│                              │  └──────────────────┘  │             │
│                              └────────────┬───────────┘             │
│                                           │                         │
│                         ┌─────────────────┼──────────────┐          │
│                         ▼                 ▼              ▼          │
│                    ┌─────────┐   ┌──────────────┐  ┌─────────┐     │
│                    │  OLED   │   │  PCM5102A    │  │  AVRCP  │     │
│                    │ (U8g2)  │   │ DAC (I2S1)   │  │ Vol/PP  │     │
│                    └─────────┘   │[Fault-tolerant]  └─────────┘     │
│                                  └──────────────┘                   │
│                                                                     │
│  Inputs:                                                            │
│  [Encoder CLK/DT] → Volume (AVRCP sync)                            │
│  [Encoder Push]   → Switch MIC/BT (graceful resource release)       │
│  [Button CONFIRM] → Cycle OLED display mode                         │
│  [Button BACK]    → Play/Pause AVRCP                                │
└─────────────────────────────────────────────────────────────────────┘

NVS Storage:
  "audio_mode" → 0 (MIC) | 1 (BT)
  "bt_mac"     → last paired device MAC (auto-reconnect)
```

---

## 6. Kế Hoạch Implement

### Phase 8 — Migration ESP32-S3 → ESP32-WROOM

**Mục tiêu:** 65 effects cũ chạy được trên WROOM với pin mới

- [x] Cập nhật `platformio.ini`:
  - `board = esp32dev`
  - Xóa `-DBOARD_HAS_PSRAM`
  - Thêm BT lib deps
  - Cập nhật partition table → `min_spiffs.csv` để có đủ app space
- [x] Remap GPIO trong `i2s_mic.h` và `display.h`
- [x] Kiểm tra tương thích U8g2 + arduinoFFT trên LX6
- [x] Build và test: OLED + mic + 65 effects OK (Compiled 100% SUCCESS)

### Phase 9 — Encoder EC11 + Multi-Button System

**Mục tiêu:** Tất cả button và encoder hoạt động đúng chức năng

#### Thực trạng hiện tại

- `button.h/.cpp` chỉ quản lý **1 button** (GPIO 0), có sẵn `button_pressed()` và `button_long_pressed()`
- Cần nâng lên hỗ trợ **5 button độc lập** (GPIO 4, 13, 14, 0, + encoder)

#### Thiết kế mới `button.h/.cpp`

```cpp
// Enum các button
enum BtnId { BTN_PUSH=0, BTN_BACK, BTN_PLUS, BTN_BOOT, BTN_COUNT };

// Mỗi button có trạng thái riêng
void buttons_init();            // Init tất cả button
void buttons_update();          // Gọi trong loop()
bool button_pressed(BtnId);     // Short press
bool button_long_pressed(BtnId); // Long press
```

- [x] Nâng cấp `button.h/.cpp` → multi-button (array `BtnState[BTN_COUNT]`)
- [x] Giữ nguyên logic debounce + long press (>1s) của code cũ
- [x] **BTN_PLUS (GPIO 14)**:
  - Short press → `display_next_mode()` — **giữ y nguyên chức năng hiện tại**
  - Long press → `display_set_auto_cycle(!current)` — **giữ y nguyên chức năng hiện tại**
- [x] **BTN_PUSH (GPIO 4)**:
  - Short press → switch mode MIC ↔ BT (Phase 12)
- [x] **BTN_BACK (GPIO 13)**:
  - Short press → AVRCP Play/Pause (Phase 11)
- [x] **BTN_BOOT (GPIO 0)**:
  - Short press (<3s) → WiFi reset + restart (Phase 15)
  - Long press (>3s) → BT re-pairing (Phase 11)
- [x] Tạo `encoder.h/.cpp` (ISR-based):
  - Interrupt trên TRA (GPIO 32) + TRB (GPIO 33) xác định chiều
  - Output: delta (+1/-1) vào AVRCP volume (Phase 11) (hook cho Phase 10)
- [x] Test tất cả button + encoder (Compiled 100% SUCCESS)

### Phase 10 — NVS Storage

**Mục tiêu:** Nhớ mode và BT device sau power cycle

- [x] Tạo `nvs_storage.h/.cpp`:
  - `nvs_save_audio_mode(mode)` / `nvs_load_audio_mode()`
  - `nvs_save_bt_mac(mac)` / `nvs_load_bt_mac()` / `nvs_erase_bt_mac()`
  - `nvs_save_volume(vol)` / `nvs_load_volume()`
  - `nvs_save_display_mode()` / `nvs_save_auto_cycle()`
- [x] Tích hợp vào switch mode logic và boot setup
- [x] Test: tắt bật nguồn giữ nguyên mode và settings (Compiled 100% SUCCESS)

### Phase 11 — Bluetooth A2DP Sink + AVRCP

**Mục tiêu:** ESP32 hoạt động như loa BT với volume sync 2 chiều

- [x] Thêm `pschatzmann/ESP32-A2DP` vào lib_deps
- [x] Tạo `bt_audio.h/.cpp`:
  - Init A2DP Sink: tên `MVT VU METER V2`
  - Callback nhận PCM → audio_queue cho visualizer
  - Tự reconnect: load MAC từ NVS → connect
  - Lưu MAC khi kết nối mới → NVS
- [x] Implement AVRCP:
  - Encoder delta → `avrc_set_volume()` → sync lên phone/PC
  - Nhận `avrc_volume_changed` từ thiết bị → update hiển thị
  - BACK button → `avrc_play_pause()`
  - Sync trạng thái play/pause lên OLED
- [x] Test: Build & Link 100% SUCCESS

### Phase 12 — Graceful Mode Switch

**Mục tiêu:** Switch MIC ↔ BT không crash, mượt mà

- [x] Implement `switch_audio_mode(AUDIO_MODE_BT)`:
  1. Suspend mic_task
  2. Deinit I2S0 (mic)
  3. Flush audio queue
  4. Init BT A2DP
  5. Save mode → NVS
- [x] Implement `switch_audio_mode(AUDIO_MODE_MIC)`:
  1. Stop A2DP / disconnect BT
  2. Flush audio queue
  3. Init I2S0 (mic)
  4. Resume mic_task
  5. Save mode → NVS
- [x] Test: Build & Link 100% SUCCESS

### Phase 13 — DAC PCM5102A Output (Fault-tolerant)

**Mục tiêu:** BT audio phát ra loa qua DAC (nếu có)

- [x] Cấu hình I2S output (BCK: GPIO 18, LCK: GPIO 19, DIN: GPIO 23)
- [x] Pipe: BT PCM data qua A2DP I2S pipeline trực tiếp tới PCM5102A
- [x] Khi MIC mode → deinit/bypass DAC output
- [x] Fault-tolerant: có/không có PCM5102A phần cứng → hệ thống không crash, OLED visualizer vẫn chạy mượt mà

### Phase 14 — OLED UI hoàn chỉnh

**Mục tiêu:** UI hiển thị đầy đủ thông tin

- [x] Status bar & Toast overlay:
  - Thông báo chuyển mode: `MODE: BLUETOOTH` / `MODE: MICROPHONE`
  - Thanh popup volume: hiển thị % và thanh tiến trình đồ họa khi xoay encoder rồi tự ẩn sau 2 giây
  - Trạng thái play/pause và kết nối Bluetooth
- [x] BT Visualizer: trích xuất PCM từ Bluetooth stream → 128 samples → FFT → toàn bộ 65 visualizer effects hoạt động mượt mà như MIC mode

### Phase 15 — WiFi: Captive Portal + OTA

**Mục tiêu:** Cấu hình WiFi không cần nạp lại code + OTA update qua web

- [x] Thêm `tzapu/WiFiManager` & `ayushsharma82/ElegantOTA` vào lib_deps
- [x] Tạo `wifi_app.h/.cpp`:
  - Khởi động Captive Portal khi chưa cấu hình WiFi: SSID `MVT-VU-METER-SETUP`
  - Auto-timeout non-blocking (nếu không cấu hình sau 60s -> boot offline)
  - Async Web Server + ElegantOTA endpoint: `http://<IP>/update`
- [x] BOOT button handler:
  - Nhấn nhanh BOOT button (GPIO 0) -> Xóa cấu hình WiFi & restart vào AP Captive Portal
- [x] Tối ưu hóa Bluetooth / WiFi coexistence:
  - Khi BT đang stream nhạc -> suspend WiFi background tasks để đảm bảo 100% throughput mượt mà cho audio
- [x] Fault-tolerant: Mất WiFi / Không có WiFi -> hệ thống vẫn chạy bình thường không block audio

### Phase 16 — NTP Clock + Weather Display Mode

**Mục tiêu:** Hiển thị đồng hồ số lớn + thời tiết thời gian thực khi nghỉ nhạc

- [x] Thêm `NTPClient` & `ArduinoJson` vào lib_deps
- [x] Tạo `effect_clock.cpp` (Mode 65: CLOCK & WEATHER):
  - Digital Clock font lớn `HH:MM:SS` (đồng bộ NTP pool.ntp.org GMT+7)
  - Thông tin thời tiết: Nhiệt độ, độ ẩm & trạng thái mây
  - Status bar: Hiển thị trạng thái Bluetooth/Mic và IP
  - Dual Mini-VU level meter ở đáy màn hình
- [x] Tự động update thời tiết định kỳ 30 phút (khi không stream BT)
- [x] Build & Link: 100% SUCCESS, binary flash image `firmware.bin` tạo thành công!

---

## 7. Thư Viện Cần Thêm / Thay Đổi

| Thư viện                      | Thay đổi                  | Phase | Lý do                       |
| ----------------------------- | ------------------------- | ----- | --------------------------- |
| `olikraus/U8g2@^2.35.19`      | Giữ nguyên                | 8     | OLED driver                 |
| `kosme/arduinoFFT@^2.0.1`     | Giữ nguyên                | 8     | FFT                         |
| `pschatzmann/ESP32-A2DP`      | **Thêm mới**              | 11    | BT A2DP Sink + AVRCP        |
| ESP32 NVS                     | Built-in                  | 10    | Nhớ mode + BT MAC           |
| `ayushsharma22/ElegantOTA`    | **Thêm mới (phase cuối)** | 15    | OTA firmware update qua web |
| ESP32 WiFi + WebServer        | Built-in                  | 15    | AP config portal + Web UI   |
| `arduino-libraries/NTPClient` | **Thêm mới (phase cuối)** | 16    | Đồng bộ giờ qua NTP         |
| HTTP Client (ArduinoJson)     | **Thêm mới (phase cuối)** | 16    | Gọi OpenWeatherMap API      |

---

### RAM & Flash Budget (Tính toán đầy đủ cho WiFi)

| Thành phần                | RAM                  | Flash (app)    |
| ------------------------- | -------------------- | -------------- |
| FreeRTOS + OS overhead    | ~30 KB               | —              |
| BT Classic A2DP stack     | ~180 KB              | ~300 KB        |
| WiFi stack (STA+AP)       | ~60 KB               | ~150 KB        |
| Audio buffer + FFT        | ~12 KB               | —              |
| 65 Effects code           | ~5 KB IRAM           | ~200 KB        |
| OLED framebuffer + U8g2   | ~3 KB                | ~50 KB         |
| HTTP client + ArduinoJson | ~8 KB                | ~30 KB         |
| Tasks stack (tất cả)      | ~20 KB               | —              |
| **Tổng ước tính**         | **~318 KB / 520 KB** | **~730 KB**    |
| **Dư để an toàn**         | **~200 KB** ✅       | **+3.2 MB** ✅ |

> 💡 **Partition table:** Cần dùng `huge_app.csv` hoặc tự custom partition để app có đủ 1.5MB+  
> Mặc định `default.csv`: app 1MB — **không đủ** khi có cả BT + WiFi + 65 effects.  
> Custom partition đề xuất: App = 2MB, NVS = 0.5MB, SPIFFS = 1.5MB (cho web assets OTA)

### BT Classic + WiFi Coexistence Strategy

```
BT A2DP streaming  → WiFi SUSPEND (radio tập trung cho BT)
BT idle/disconnect → WiFi ACTIVE (OTA, NTP sync, weather fetch)
MIC mode           → WiFi ACTIVE bình thường

Không bao giờ sync WiFi nặng (OTA) khi BT đang stream audio!
```

---

## 8. Rủi Ro & Giải Pháp

| Rủi ro                                      | Mức độ         | Giải pháp                                          |
| ------------------------------------------- | -------------- | -------------------------------------------------- |
| Flash 4MB không đủ (BT + WiFi + 65 effects) | **Cao**        | Custom partition: App 2MB, NVS 0.5MB, SPIFFS 1.5MB |
| RAM thiếu khi BT + WiFi cùng active         | **Trung bình** | WiFi suspend khi BT stream; monitor heap           |
| Crash khi switch mode quá nhanh             | **Trung bình** | Mutex / semaphore bảo vệ switch logic              |
| BT audio bị drop khi WiFi OTA update        | **Cao**        | Chỉ cho phép OTA khi BT idle/disconnect            |
| AVRCP Absolute Volume không hỗ trợ          | **Trung bình** | Fallback: digital gain trong software              |
| WiFi captive portal conflict BT             | **Thấp**       | AP mode chỉ active khi cần config, tự timeout      |
| OpenWeatherMap API thay đổi / rate limit    | **Thấp**       | Cache kết quả 30 phút, fallback hiển thị "--"      |
| Auto-reconnect BT thất bại                  | **Thấp**       | Retry 3 lần → discoverable mode                    |

---

## 9. Tất Cả Câu Hỏi Đã Được Giải Đáp ✅

| Câu hỏi                       | Trả lời                                       |
| ----------------------------- | --------------------------------------------- |
| Pin mapping?                  | ✅ Phương án A (đã fix GPIO 12 → 18/19/23)    |
| Button PLUS dùng GPIO nào?    | ✅ GPIO 14 (chân CON của module EC11)         |
| Button BOOT GPIO 0 chức năng? | ✅ Short: Reset WiFi; Long: BT re-pairing     |
| Button list đủ chưa?          | ✅ Đủ — 5 controls: xoay/PSH/BACK/PLUS/BOOT   |
| 65 effects?                   | ✅ Giữ nguyên                                 |
| BT Visualizer?                | ✅ Có                                         |
| Nhớ mode + BT device?         | ✅ NVS                                        |
| DAC priority?                 | Thứ yếu, fault-tolerant                       |
| WiFi Captive Portal?          | ✅ Dùng WiFiManager (tzapu), tự động redirect |

**→ Không còn câu hỏi nào. Sẵn sàng implement Phase 8!**

---

### Phase 15 — WiFi: Captive Portal + OTA

**Mục tiêu:** Cấu hình WiFi dễ dàng + update firmware không cần cáp

#### Button BOOT (GPIO 0) — hai chức năng

```
Short press (<3s)  → Xóa WiFi credentials trong NVS
                    → Restart → vào AP Captive Portal mode
                    → OLED hiển thị: "WiFi Setup Mode"
                    → SSID: MVT-VU-METER-SETUP

Long press (>3s)   → Ngắt kết nối BT hiện tại
                    → Xóa bt_mac trong NVS
                    → Vào chế độ discoverable (pairing mới)
                    → OLED hiển thị: "BT Pairing..."
```

#### Captive Portal (dùng thư viện WiFiManager)

```
Thiết bị khởi động (chưa có WiFi credentials):
  └→ Bật AP: "MVT-VU-METER-SETUP" (không cần password)
     Điện thoại kết nối → tự popup trang cấu hình
     Hoặc mở http://192.168.4.1
  └→ Hiển thị danh sách WiFi network xưng quanh
  └→ Chọn network + nhập mật khẩu → Save
  └→ Restart → kết nối STA mode
  └→ OLED: "WiFi: Connected ✓  192.168.1.xx"

Luồng trải nghiệm khi "Reset WiFi" (bấm BOOT):
  └→ OLED: "WiFi Reset... Restart in 3s"
  └→ Restart → AP mode → captive portal popup
```

- [ ] Thêm `tzapu/WiFiManager` vào lib_deps
- [ ] Tạo `wifi_manager.h/.cpp`:
  - Wrap WiFiManager: auto AP nếu không có credentials
  - Custom OLED callback khi vào AP mode / kết nối xong
  - Timeout 3 phút trong AP mode → tiếp tục khởi động không WiFi
- [ ] Implement BOOT button logic trong `button.h/.cpp`:
  - Short press: `nvs_erase_wifi()` → `ESP.restart()`
  - Long press (>3s): `bt_start_repairing()` → xóa NVS mac → discoverable
- [ ] **OTA Update** (ElegantOTA ở `/update`):
  - Chỉ active khi BT **không** đang stream audio
  - OLED hiển thị progress bar khi đang flash
- [ ] Fault-tolerant: không có WiFi → skip hoàn toàn, audio vẫn chạy

### Phase 16 — NTP Clock + Weather Display Mode

**Mục tiêu:** Thêm mode hiển thị đồng hồ + thời tiết trên OLED

- [ ] **NTP sync**: đồng bộ giờ khi WiFi kết nối, lưu timezone (UTC+7)
- [ ] **Weather fetch**: gọi OpenWeatherMap API mỗi 30 phút
  - Lấy: nhiệt độ, độ ẩm, icon thời tiết
  - Cache vào RAM, fallback "--" nếu lỗi
- [ ] **OLED Clock/Weather Mode** (mode đặc biệt, ngoài 65 effects):
  ```
  ┌────────────────────────────┐
  │  23:09        🌤  28°C    │
  │  Mon 18/08/2026   65% hum │
  │  ──────────────────────── │
  │  [BT] MVT VU METER V2 🔵  │
  └────────────────────────────┘
  ```

  - Giờ lớn + ngày tháng
  - Icon thời tiết (pixel art 16x16) + nhiệt độ
  - Status bar: BT/MIC mode, connection info
- [ ] Khi không có âm thanh (silence > 15s) → auto switch sang clock mode
- [ ] Khi có âm thanh → tự quay về visualizer effect
- [ ] Nếu không có WiFi → clock mode chỉ hiển thị giờ ước tính (millis-based)

---

## 10. Tóm Tắt Điểm Mấu Chốt

> ⚠️ **Xóa `-DBOARD_HAS_PSRAM`** khi migrate sang WROOM (WROOM không có PSRAM).

> ⚠️ **Custom partition table** (App=2MB, NVS=0.5MB, SPIFFS=1.5MB) — cần thiết cho BT + WiFi + 65 effects + web assets.

> ✅ **BT + WiFi coexistence**: WiFi suspend khi BT đang stream audio — không bao giờ OTA/fetch khi đang nghe nhạc BT.

> ✅ **BT Visualizer** = BT PCM callback → audio_queue → FFT → 65 effects như bình thường.

> ✅ **DAC là thứ yếu** — init fault-tolerant, không ảnh hưởng OLED.

> ✅ **NVS** lưu: audio mode + BT MAC + WiFi credentials + timezone.

> ✅ **Graceful switch** dùng suspend/resume task + deinit/init I2S có kiểm soát.

> ✅ **Fault-tolerant toàn diện**: không có bất kỳ module nào cũng không crash — chỉ mất tính năng tương ứng.

> ✅ **Button BOOT (GPIO 0)**: tận dụng button trên board WROOM — short press → WiFi reset → captive portal; long press → BT re-pairing.
