# ESP32-S3 IoT Voice Command System

Hệ thống điều khiển thiết bị IoT bằng giọng nói offline, chạy trên ESP32-S3-N16R8 với AI nhận diện giọng nói thông qua ESP-SR (WakeNet + MultiNet). Hiển thị trạng thái qua OLED 1.3" và điều khiển 6 relay/LED.

---

## Mục lục

- [Tech Stack](#tech-stack)
- [Phần cứng & Sơ đồ nối dây](#phần-cứng--sơ-đồ-nối-dây)
- [Thư viện sử dụng](#thư-viện-sử-dụng)
- [Cấu trúc dự án](#cấu-trúc-dự-án)
- [Phân vùng bộ nhớ](#phân-vùng-bộ-nhớ)
- [Workflow & Cơ chế hoạt động](#workflow--cơ-chế-hoạt-động)
- [Hướng dẫn Build & Flash](#hướng-dẫn-build--flash)
- [Serial Monitor & Debug](#serial-monitor--debug)
- [Troubleshooting](#troubleshooting)

---

## Tech Stack

| Thành phần | Chi tiết |
|-----------|---------|
| **MCU** | ESP32-S3-N16R8 (Dual-core Xtensa LX7 @ 240MHz) |
| **Flash** | 16MB (OPI mode) |
| **PSRAM** | 8MB OPI PSRAM |
| **Framework** | Arduino (espressif32 @ ^6.11.0) |
| **Build System** | PlatformIO |
| **AI Engine** | ESP-SR (WakeNet9 + MultiNet6) — offline, không cần Internet |
| **Audio Input** | I2S — INMP441 MEMS Microphone (mono, 16kHz) |
| **Display** | U8g2 — SH1106 OLED 1.3" 128×64 (I2C) |
| **Debug** | USB CDC Serial @ 115200 |

---

## Phần cứng & Sơ đồ nối dây

### Danh sách linh kiện

| Linh kiện | Model | Giao tiếp |
|----------|-------|-----------|
| Vi điều khiển | ESP32-S3-DevKitC-1-N16R8 | — |
| Microphone | INMP441 | I2S |
| Màn hình | OLED 1.3" 128×64 | I2C (SH1106) |
| Nút bấm | 4× Tactile Switch | GPIO |
| Relay/LED | 6× Relay Module hoặc LED | GPIO |

### Sơ đồ nối dây

#### INMP441 Microphone (I2S)

```
INMP441       ESP32-S3
─────────     ─────────────────
VDD     ───►  3.3V
GND     ───►  GND
L/R     ───►  GND  (chọn kênh trái — mono)
WS      ───►  GPIO 42  (I2S_WS)
SCK     ───►  GPIO 41  (I2S_SCK)
SD      ───►  GPIO 2   (I2S_SD / data in)
```

#### SH1106 OLED 1.3" (I2C)

```
OLED          ESP32-S3
──────        ─────────────────
VCC     ───►  3.3V
GND     ───►  GND
SDA     ───►  GPIO 8   (I2C_SDA)
SCL     ───►  GPIO 9   (I2C_SCL)
```

> **Lưu ý:** I2C address mặc định của SH1106 là `0x3C`. U8g2 xử lý tự động.

#### Nút bấm (Active LOW — nối GND khi nhấn)

```
Nút           GPIO
──────        ──────────────────
UP      ───►  GPIO 10
DOWN    ───►  GPIO 11
ENTER   ───►  GPIO 12
BACK    ───►  GPIO 13
WAKE    ───►  GPIO 0  (boot button tích hợp)
```

#### Relay / LED Output

```
Chức năng     GPIO
──────────    ──────────────────
RELAY_1 ───►  GPIO 1
RELAY_2 ───►  GPIO 3
RELAY_3 ───►  GPIO 4
RELAY_4 ───►  GPIO 5
RELAY_5 ───►  GPIO 6
RELAY_6 ───►  GPIO 7
LED_STATUS ►  GPIO 48  (RGB LED tích hợp trên DevKit)
```

> **Lưu ý:** Relay module thường kích LOW. Nếu dùng relay kích HIGH, không cần đổi GPIO, chỉ đổi logic trong `handle_voice_command()`.

---

## Thư viện sử dụng

```
Dependency Graph
|-- U8g2 @ 2.36.18             — Driver OLED SH1106 (I2C 128×64)
|-- LovyanGFX @ 1.2.21         — GFX accelerator (dự phòng TFT mở rộng)
|-- M5Unified @ 0.2.14         — Dependency của ESP_SR_M5Unified
|-- ESP_SR_M5Unified @ 1.0.0   — Wrapper Arduino cho ESP-SR (WakeNet + MultiNet)
|-- M5GFX @ 0.2.20             — Dependency của M5Unified
|-- Wire @ 2.0.0               — I2C Arduino core
```

| Thư viện | Vai trò | Registry |
|---------|---------|---------|
| `olikraus/U8g2` | Driver OLED SH1106 — hiển thị text/graphics qua I2C | PlatformIO |
| `lovyan03/LovyanGFX` | GFX layer dự phòng (TFT trong tương lai) | PlatformIO |
| `m5stack/M5Unified` | BSP cho hardware M5Stack, required bởi ESP-SR wrapper | PlatformIO |
| `74th/ESP-SR-For-M5Unified` | Wrapper Arduino của ESP-SR: WakeNet (wake word) + MultiNet (lệnh) | GitHub |

> **ESP-SR** là thư viện AI nhận diện giọng nói của Espressif, pre-compiled sẵn trong arduino-esp32 core. Wrapper của `74th` cung cấp API đơn giản qua `feedAudio()`.

---

## Cấu trúc dự án

```
esp32-project/
├── platformio.ini          # Cấu hình PlatformIO (platform, libs, build flags)
├── partitions.csv          # Bảng phân vùng Flash tùy chỉnh (16MB)
├── scripts/
│   └── flash_srmodels.py   # Script tự động flash srmodels.bin sau upload
├── models/
│   └── srmodels.bin        # (Cần tải về) — Model AI của ESP-SR
├── src/
│   ├── main.cpp            # Entry point: setup(), loop(), GPIO control
│   ├── hardware_config.h   # Định nghĩa tất cả GPIO pins
│   ├── ui_manager.h        # API hiển thị OLED
│   ├── ui_manager.cpp      # Hiển thị U8g2 — SH1106 driver
│   ├── voice_manager.h     # API nhận diện giọng nói
│   ├── voice_manager.cpp   # I2S init + ESP-SR integration
│   └── lv_conf.h           # LVGL config (dự phòng — không active)
├── upload.sh               # Script upload nhanh (Linux/macOS)
└── monitor.sh              # Script mở Serial Monitor (Linux/macOS)
```

---

## Phân vùng bộ nhớ

Flash 16MB được chia như sau (`partitions.csv`):

| Tên | Type | Offset | Size | Mục đích |
|-----|------|--------|------|---------|
| `nvs` | data/nvs | `0x9000` | 16 KB | WiFi config, NVS key-value |
| `otadata` | data/ota | `0xD000` | 8 KB | OTA slot bookkeeping |
| `phy_init` | data/phy | `0xF000` | 4 KB | RF calibration |
| `factory` | app/factory | `0x10000` | **3 MB** | Firmware chính |
| `model` | data/spiffs | `0x310000` | **3 MB** | `srmodels.bin` (AI models) |
| `storage` | data/spiffs | `0x610000` | **~6 MB** | SPIFFS: voice vectors, config |

> **Quan trọng:** `srmodels.bin` phải được flash riêng vào partition `model` @ offset `0x310000`. Xem [Hướng dẫn flash model AI](#3-flash-model-ai-esp-sr).

---

## Workflow & Cơ chế hoạt động

### Luồng xử lý chính

```
┌─────────────────────────────────────────────────────────┐
│                     ESP32-S3 Dual Core                   │
│                                                          │
│  Core 0 (UI + I2S)          Core 1 (AI Inference)       │
│  ─────────────────           ──────────────────────      │
│  I2S read INMP441  ──────►  feedAudio() → ESP-SR        │
│  U8g2 draw OLED             WakeNet: detect wake word    │
│  Button polling             MultiNet: recognize command  │
│                                    │                     │
│                             callback(command_id)         │
│                                    │                     │
│  GPIO relay ON/OFF  ◄──────────────┘                    │
│  ui_show_message()                                       │
└─────────────────────────────────────────────────────────┘
```

### Quy trình nhận diện giọng nói

```
1. Idle
   └─► INMP441 liên tục ghi âm I2S (16kHz, mono, 32-bit)
   
2. Wake Word Detection (WakeNet)
   └─► Phát hiện "Hi ESP" (hoặc wake word tùy chọn)
   └─► LED_STATUS nhấp nháy → OLED hiện "Listening..."
   
3. Command Recognition (MultiNet)
   └─► Lắng nghe lệnh tiếp theo (timeout ~5s)
   └─► Ví dụ: "Turn on light one" → command_id = 0
   
4. Command Execution
   └─► handle_voice_command(command_id)
   └─► GPIO relay HIGH/LOW
   └─► OLED hiện "Device ON/OFF"
```

### Mapping lệnh giọng nói → GPIO

| Command ID | Lệnh mẫu (EN) | Action | GPIO |
|-----------|--------------|--------|------|
| 0 | "turn on light one" | RELAY_1 = HIGH | GPIO 1 |
| 1 | "turn on light two" | RELAY_2 = HIGH | GPIO 3 |
| 2–5 | "turn on light N" | RELAY_N = HIGH | GPIO 4–7 |
| 10 | "turn off light one" | RELAY_1 = LOW | GPIO 1 |
| 11–15 | "turn off light N" | RELAY_N = LOW | GPIO 3–7 |

> Mapping lệnh được định nghĩa trong `main.cpp` → `handle_voice_command()`. Tùy chỉnh theo model MultiNet bạn build.

---

## Hướng dẫn Build & Flash

### Yêu cầu

- Python 3.8+
- PlatformIO Core (`pip install -U platformio`)
- Git

### 1. Clone & cài đặt dependencies

```bash
git clone <repo-url>
cd esp32-project
pio pkg install
```

> **Lần đầu:** PlatformIO sẽ tự động tải toolchain Xtensa ESP32-S3, Arduino framework, và tất cả thư viện (~500MB). Cần kết nối Internet.

### 2. Build firmware

```bash
pio run
```

Kết quả thành công:
```
RAM:   [=         ]   6.4% (used 21016 bytes from 327680 bytes)
Flash: [=         ]  10.5% (used 329237 bytes from 3145728 bytes)
========================= [SUCCESS] =========================
```

### 3. Flash model AI (ESP-SR)

Model AI (`srmodels.bin`) phải được flash **riêng** vào partition `model`:

**Bước 3a:** Tải `srmodels.bin` từ ESP-SR releases:
```
https://github.com/espressif/esp-sr/releases
```
Hoặc build từ ESP-Skainet: `https://github.com/espressif/esp-skainet`

**Bước 3b:** Đặt file vào thư mục `models/`:
```bash
mkdir models
cp <path>/srmodels.bin models/srmodels.bin
```

**Bước 3c (Tự động):** Uncomment dòng trong `platformio.ini`:
```ini
extra_scripts = post:scripts/flash_srmodels.py
```
Script sẽ tự flash `srmodels.bin` sau mỗi lần `pio run --target upload`.

**Bước 3c (Thủ công):**
```bash
python -m esptool --chip esp32s3 --port <COMx> \
  --baud 921600 write_flash 0x310000 models/srmodels.bin
```

### 4. Upload firmware

```bash
# Tự động phát hiện cổng
pio run --target upload

# Hoặc chỉ định cổng (Windows)
pio run --target upload --upload-port COM3

# Script tiện lợi (Linux/macOS)
./upload.sh
```

### 5. Build + Upload + Monitor (một lệnh)

```bash
pio run --target upload && pio device monitor
```

---

## Serial Monitor & Debug

```bash
# Mở monitor (115200 baud, tự động decode exception)
pio device monitor

# Script tiện lợi
./monitor.sh
```

Output mẫu khi hoạt động đúng:
```
[00:00:00.120] I2S Microphone Initialized
[00:00:00.350] System Ready
[00:00:05.210] Received Command ID: 0
[00:00:05.215] Device ON
```

> **Monitor filters** đã được cấu hình sẵn: `default`, `time`, `esp32_exception_decoder` — lỗi crash sẽ tự động decode thành stack trace có thể đọc được.

---

## Build Flags quan trọng

| Flag | Giá trị | Mục đích |
|------|---------|---------|
| `CORE_DEBUG_LEVEL` | `3` | Log level INFO (set `0` khi production) |
| `BOARD_HAS_PSRAM` | — | Kích hoạt 8MB OPI PSRAM |
| `ARDUINO_USB_MODE` | `1` | USB CDC mode (Serial qua USB trực tiếp) |
| `ARDUINO_USB_CDC_ON_BOOT` | `1` | Serial sẵn sàng ngay từ boot |
| `I2S_SAMPLE_RATE` | `16000` | 16kHz — bắt buộc cho ESP-SR |
| `I2S_CHANNEL_NUM` | `1` | Mono — INMP441 L/R nối GND |
| `-O2` | — | Tối ưu tốc độ cho AI inference |

---

## Troubleshooting

### ❌ Lỗi build: `No module named 'intelhex'`

```bash
# Cài vào PlatformIO penv
~/.platformio/penv/Scripts/python -m pip install intelhex

# Cài vào Python hệ thống
python -m pip install intelhex
```

### ❌ Lỗi upload: Connection reset / timeout

1. Nhấn giữ **BOOT** trên board khi bắt đầu upload
2. Giảm upload speed trong `platformio.ini`:
   ```ini
   upload_speed = 460800
   ```
3. Thử cáp USB khác (cáp data, không phải cáp sạc)

### ❌ OLED không hiển thị

- Kiểm tra địa chỉ I2C: chạy I2C scanner để xác nhận `0x3C` hay `0x3D`
- Một số module SH1106 clone dùng SSD1306 — đổi class U8g2 trong `ui_manager.cpp`:
  ```cpp
  // Thay SH1106 thành SSD1306 nếu cần:
  U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(...)
  ```
- Kiểm tra kéo điện trở 4.7kΩ trên SDA/SCL (một số module không có sẵn)

### ❌ Microphone không ghi âm / âm thanh nhiễu

- Xác nhận chân **L/R** của INMP441 nối **GND** (chọn kênh trái)
- Kiểm tra nguồn 3.3V ổn định (INMP441 nhạy với nguồn)
- Thử giảm `dma_buf_len` trong `voice_manager.cpp` nếu có dropout

### ❌ ESP-SR không nhận diện lệnh

- Đảm bảo `srmodels.bin` đã được flash đúng offset `0x310000`
- Kiểm tra partition table đúng: `pio run --target upload` (partition bin được tạo tự động)
- Sample rate **phải là 16000Hz** — không thay đổi
- Khoảng cách micro tốt nhất: **30–100cm** trong môi trường ít nhiễu

### ❌ PlatformIO Core obsolete warning

```
Obsolete PIO Core v6.1.18 is used (previous was 6.1.19)
```
Gỡ bản cũ và chỉ giữ một bản:
```bash
pip install -U platformio
```
Xem: https://docs.platformio.org/en/latest/core/installation/troubleshooting.html

---

## Phát triển tiếp theo

- [ ] Implement ESP-SR WakeNet + MultiNet đầy đủ trong `voice_manager.cpp`
- [ ] Thêm giao diện menu OLED bằng U8g2 (điều hướng bằng 4 nút)
- [ ] SPIFFS: lưu trạng thái relay khi mất điện
- [ ] Thêm hỗ trợ tiếng Việt (MultiNet có model Vietnamese)
- [ ] FreeRTOS task: tách I2S read (Core 0) và AI inference (Core 1)
- [ ] WiFi OTA update firmware

---

## Tham khảo

- [ESP-SR Documentation](https://github.com/espressif/esp-sr)
- [ESP-Skainet Examples](https://github.com/espressif/esp-skainet)
- [74th ESP-SR-For-M5Unified](https://github.com/74th/ESP-SR-For-M5Unified)
- [U8g2 Wiki](https://github.com/olikraus/u8g2/wiki)
- [PlatformIO ESP32-S3 Board](https://docs.platformio.org/en/latest/boards/espressif32/esp32-s3-devkitc-1.html)
- [INMP441 Datasheet](https://invensense.tdk.com/wp-content/uploads/2015/02/INMP441.pdf)
