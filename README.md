# ESP32 Smart Dashboard & MP3 Player (CYD 3.5")

Dự án Đồng hồ thông minh, Bảng tin tức & Trình phát nhạc MP3 cao cấp chạy trên bo mạch **ESP32 Cheap Yellow Display (CYD) 3.5 inch** (Mã bo: **ESP32-3248S035**). Dự án sử dụng framework **Arduino (PlatformIO)** kết hợp với thư viện đồ họa **LVGL v8.3.11** và kiến trúc đa nhiệm **FreeRTOS**.

---

## 🌟 Tổng quan & Các cập nhật mới nhất (Recent Updates)

Dự án đã được nâng cấp toàn diện từ giao diện mẫu (Mock UI) sang **hệ thống thời gian thực hoàn chỉnh (Production-ready System)** với các cải tiến nổi bật:

### 1. Nâng cấp Tính năng Thực tế (Real Features)
- 🕒 **Đồng hồ NTP & Lịch Âm Việt Nam**: Đồng bộ thời gian chuẩn GMT+7 qua NTP Server (`pool.ntp.org`), tích hợp thuật toán Hồ Ngọc Đức tính toán chính xác ngày, tháng, năm âm lịch và Can Chi (Giáp Thìn, Ất Tỵ,...).
- ⛅ **Thời tiết Trực tuyến (Open-Meteo API)**: Dự báo thời tiết tự động theo Tỉnh/Thành phố được chọn (Hà Nội, TP.HCM, Đà Nẵng,...), hiển thị nhiệt độ, độ ẩm, sức gió và biểu tượng thời tiết động.
- 📈 **Thị trường Tài chính (VNExpress API)**: Cập nhật biến động **Giá vàng SJC / Nhẫn 9999** (Mua/Bán) và **Giá xăng dầu** (RON 95-III, E5 RON 92-II, Dầu Diesel).
- 🎵 **Trình phát nhạc MP3 từ Thẻ nhớ SD**:
  - Quét và phát trực tiếp các file `.mp3`, `.wav` từ thẻ MicroSD.
  - Bộ điều khiển đầy đủ: Phát/Tạm dừng, Chuyển bài, Lặp bài, Phát ngẫu nhiên (Shuffle).
  - Hỗ trợ **Thanh tìm kiếm nhanh bài hát** và **Thanh tua bài hát (Seek Bar)** mượt mà.
  - Hiệu ứng sóng âm sống động (Visualizer wave animation).
- ⚙️ **Trung tâm Cài đặt 6 Mục (Settings Center)**:
  - **Thiết bị**: Thông tin phần cứng, Firmware, Uptime, IP/MAC, RAM Free Heap, nút Khởi động lại & Khôi phục cài đặt gốc.
  - **WiFi Manager**: Quét mạng bất đồng bộ, bàn phím ảo LVGL nhập mật khẩu, tự động lưu và kết nối lại.
  - **Quản lý Thẻ nhớ (SD Tool)**: Kiểm tra thông số thẻ nhớ, đo dung lượng Total/Used/Free và tích hợp công cụ Định dạng (Format FAT32).
  - **Vị trí & Đồng bộ**: Chọn Tỉnh/TP, tùy chỉnh chu kỳ đồng bộ dữ liệu (15p, 30p, 1h, 2h), nút "Đồng bộ ngay".
  - **Màn hình & Nguồn**: Điều chỉnh độ sáng PWM (LEDC), thời gian tự động tắt màn hình (Sleep Timeout), tự động sáng theo cảm biến quang trở (LDR).
  - **Hệ thống & Âm thanh**: Chỉnh âm lượng mặc định, bật/tắt âm thanh phản hồi chạm và kích hoạt Developer HUD.

### 2. Tối ưu Hiệu năng & Ổn định Hệ thống
- 🚀 **Tối ưu FPS & Render GUI**: Tối ưu chu kỳ vẽ `lv_timer_handler()` và cơ chế cập nhật UI theo sự kiện / timer tách biệt, không render liên tục các widget tĩnh, loại bỏ giật lag màn hình.
- 🧠 **Khắc phục rò rỉ bộ nhớ (Zero Memory Leak)**: Tối ưu dung lượng buffer LVGL, giải phóng tài nguyên chuỗi và modal sau khi đóng popup.
- 📊 **Developer HUD**: Khung thông số hiệu năng overlay trực tiếp trên màn hình theo dõi **FPS, RAM Free Heap, CPU Load, Cường độ sóng WiFi (dBm)**.
- 💾 **Lưu trữ Cấu hình Toàn cục (NVS Flash)**: Toàn bộ cấu hình mạng, tỉnh thành, độ sáng, âm lượng được tự động lưu vào Flash thông qua `Preferences`.
- 🔕 **Quản lý Log linh hoạt**: Hỗ trợ macro `LOG_D`, `LOG_I`, `LOG_W`, `LOG_E` tập trung qua `include/log.h`. Tắt log dễ dàng trong `platformio.ini` cho bản build thương mại để tiết kiệm RAM và tăng tốc tối đa.

---

## 📱 Chi tiết các màn hình (Screens & Features)

```
        ┌────────────────────────────────────────────────────────┐
        │                        TOP BAR                         │
        │ [Clock]    [Special Event]    [WiFi Status]  [MP3 Mini]│
        └────────────────────────────────────────────────────────┘
                                    │
        ┌───────────────┬───────────┴───┬───────────────┬──────────────┐
        ▼               ▼               ▼               ▼              ▼
 ┌─────────────┐ ┌─────────────┐ ┌─────────────┐ ┌──────────────┐ ┌────────────┐
 │  HOME TAB   │ │CALENDAR TAB │ │ PLAYER TAB  │ │ SETTINGS TAB │ │  DEV HUD   │
 ├─────────────┤ ├─────────────┤ ├─────────────┤ ├──────────────┤ ├────────────┤
 │• Đồng hồ lớn│ │• Ma trận    │ │• Thư viện SD│ │• 1. Thiết bị │ │• Realtime  │
 │• Lịch Âm    │ │  Lịch tháng │ │• Tìm kiếm   │ │• 2. WiFi     │ │  FPS       │
 │• Thời tiết  │ │• Âm/Dương   │ │• Điều khiển │ │• 3. Thẻ SD   │ │• Free Heap │
 │• Giá vàng   │ │• Hoàng đạo  │ │• Tua bài    │ │• 4. Đồng bộ  │ │• CPU Usage │
 │• Giá xăng   │ │• Sự kiện    │ │• Visualizer │ │• 5. Màn hình │ │• WiFi dBm  │
 └─────────────┘ └─────────────┘ └─────────────┘ │• 6. Hệ thống │ └────────────┘
                                                 └──────────────┘
```

---

## 🛠 Sơ đồ chân phần cứng (Wiring Diagram)

Mạch **ESP32-3248S035 (CYD 3.5" TFT ST7796 + XPT2046)** có sơ đồ kết nối phần cứng như sau:

### 1. Màn hình TFT LCD (Driver: ST7796, 480x320 px)
- **TFT_MISO**: GPIO 12
- **TFT_MOSI**: GPIO 13
- **TFT_SCLK**: GPIO 14
- **TFT_CS**: GPIO 15
- **TFT_DC**: GPIO 2
- **TFT_RST**: -1 (Dùng chung chân EN/RESET của ESP32)
- **TFT_BL**: GPIO 27 (Backlight - Điều khiển xung PWM)

### 2. Cảm ứng điện trở (Resistive Touch - XPT2046)
- **TOUCH_CS**: GPIO 33
- **Bus SPI**: Dùng chung đường HSPI với màn hình TFT (MOSI=13, MISO=12, SCLK=14).

### 3. Khay thẻ nhớ MicroSD (VSPI riêng biệt)
- **SD_CS**: GPIO 5
- **SD_MOSI**: GPIO 23
- **SD_MISO**: GPIO 19
- **SD_SCLK**: GPIO 18

### 4. Âm thanh & Cảm biến ngoại vi
- **Audio Output (Speaker)**: GPIO 26 (Sử dụng DAC nội bộ của ESP32 hoặc mở rộng I2S)
- **LDR (Quang trở cảm biến ánh sáng)**: GPIO 34 (Analog ADC)
- **RGB LED Onboard**: GPIO 21

> [!WARNING]
> **Cảnh báo lỗi phần cứng board CYD 3.5":**
> Trên bo mạch CYD 3.5 inch, chip Flash ngoài **W25Q32** bị đấu chung chân CS với Flash nội bộ của ESP32. **Tuyệt đối không khởi tạo hay truy cập chip W25Q32 này** vì sẽ gây xung đột bus SPI và crash hệ điều hành. Toàn bộ tính năng lưu trữ dữ liệu mở rộng được định tuyến an toàn qua **Thẻ nhớ MicroSD** (VSPI).

---

## 🏗 Kiến trúc Hệ thống & Đa nhiệm (FreeRTOS)

Dự án tận dụng cả 2 nhân của ESP32 để đảm bảo giao diện luôn mượt mà 30+ FPS trong khi vẫn xử lý dữ liệu mạng và phát nhạc:

- **Core 1 (App / GUI Task)**:
  - Vòng lặp LVGL `lv_timer_handler()` và điều khiển quét cảm ứng Touchpad.
  - Render giao diện, Dev HUD, TopBar và hiệu ứng chuyển cảnh.
- **Core 0 (System & Background Tasks)**:
  - `WifiService`: Quản lý kết nối, tự động reconnect và quét mạng.
  - `TimeService` & `LunarCalendar`: Đồng bộ NTP và thuật toán Lịch Âm.
  - `WeatherService` & `MarketService`: Xử lý HTTP Request lấy giá vàng, xăng dầu, thời tiết.
  - `AudioPlayerService`: Tác vụ nền giải mã MP3 từ thẻ SD qua thư viện `ESP8266Audio`, đảm bảo âm thanh liền mạch, không bị khựng giật khi thao tác vuốt chạm.

---

## 📂 Cấu trúc Thư mục Dự án

```
esp32-project/
├── include/
│   ├── log.h                      # Hệ thống quản lý Log tập trung (LOG_D, LOG_I, ...)
│   └── lv_conf.h                  # File cấu hình thư viện đồ họa LVGL
├── src/
│   ├── main.cpp                   # Điểm khởi chạy chính & Setup FreeRTOS Tasks
│   ├── services/                  # Các module dịch vụ nền (Core 0)
│   │   ├── audio_player_service.* # Dịch vụ phát nhạc MP3 từ thẻ SD
│   │   ├── backlight_manager.*    # Điều khiển độ sáng màn hình qua PWM
│   │   ├── config_manager.*       # Quản lý lưu trữ cài đặt NVS Flash (Preferences)
│   │   ├── lunar_calendar.*       # Thuật toán tính Lịch Âm & Can Chi
│   │   ├── market_service.*       # API Giá vàng & Giá xăng dầu
│   │   ├── storage_service.*      # Quản lý & Định dạng Thẻ nhớ SD
│   │   ├── system_telemetry.*     # Đo đạc RAM, CPU, WiFi RSSI
│   │   ├── time_service.*         # Đồng bộ giờ NTP
│   │   ├── weather_service.*      # API Thời tiết Open-Meteo
│   │   └── wifi_service.*         # Dịch vụ quản lý kết nối WiFi
│   └── ui/
│       └── dashboard/             # Giao diện người dùng LVGL
│           ├── cyd_theme.h        # Theme màu sắc, typography & styling
│           ├── dashboard_ui.*     # Bộ điều phối Dashboard, TopBar & Navigation
│           ├── dev_hud.*          # Khung HUD giám sát hiệu năng thực tế
│           ├── fonts/             # Font chữ tiếng Việt (Montserrat) & Weather Icons
│           └── screens/           # Các màn hình chính
│               ├── home_screen.*      # Màn hình chính (Đồng hồ, Thời tiết, Giá vàng)
│               ├── calendar_screen.*  # Màn hình Lịch Vạn Niên
│               ├── player_screen.*    # Trình phát nhạc MP3
│               └── settings_screen.*  # Trung tâm cài đặt 6 mục
├── platformio.ini                 # Cấu hình dự án PlatformIO
├── requirement.md                 # Tài liệu đặc tả kỹ thuật chi tiết
└── README.md                      # Hướng dẫn dự án
```

---

## 🚀 Hướng dẫn Cài đặt & Sử dụng

### 1. Yêu cầu môi trường
- Cài đặt [Visual Studio Code](https://code.visualstudio.com/) cùng tiện ích mở rộng **PlatformIO IDE** (hoặc cài đặt PlatformIO Core qua Python CLI).

### 2. Các lệnh PlatformIO cơ bản

#### Build dự án:
```bash
pio run
```

#### Nạp code lên ESP32:
```bash
pio run --target upload
```

#### Mở Serial Monitor:
```bash
pio device monitor
```

#### Build + Upload + Monitor trong 1 lệnh:
```bash
pio run --target upload && pio device monitor
```

#### Xóa các file build tạm:
```bash
pio run --target clean
```

---

## 🔧 Cấu hình trong `platformio.ini`

### Tùy chọn Bật/Tắt Log (Debug / Production)

Mở file `platformio.ini` để cấu hình chế độ Log:

```ini
build_flags = 
    ; --- Logging control: Bỏ comment dòng dưới để BẬT LOG (DEV), comment lại để TẮT LOG (PRODUCTION) ---
    ; -DENABLE_SERIAL_LOG
    -DCORE_DEBUG_LEVEL=0
    -DUSER_SETUP_LOADED=1
    -DUSE_HSPI_PORT=1
    -DST7796_DRIVER=1
    -DTFT_WIDTH=320
    -DTFT_HEIGHT=480
    -DTFT_MISO=12
    -DTFT_MOSI=13
    -DTFT_SCLK=14
    -DTFT_CS=15
    -DTFT_DC=2
    -DTFT_RST=-1
    -DTFT_BL=27
    -DTOUCH_CS=33
    -DSPI_FREQUENCY=55000000
    -DSPI_TOUCH_FREQUENCY=2500000
    -DLV_CONF_INCLUDE_SIMPLE
    -I include
```

> [!TIP]
> Khi sử dụng thông thường, hãy tắt `-DENABLE_SERIAL_LOG` để thiết bị đạt tốc độ khung hình (FPS) tối đa và giảm thiểu việc chiếm dụng bộ nhớ RAM.

---

## ❓ Xử lý Sự cố thường gặp (Troubleshooting)

1. **Không nhận thẻ nhớ SD**:
   - Kiểm tra định dạng thẻ nhớ (khuyến nghị chuẩn FAT32, dung lượng thẻ <= 32GB).
   - Truy cập vào **Cài đặt -> Thẻ nhớ** trên màn hình để kiểm tra trạng thái hoặc dùng tính năng **Định dạng thẻ**.
2. **Không phát được âm thanh**:
   - Kiểm tra file âm thanh phải đúng định dạng `.mp3` hoặc `.wav` (tần số lấy mẫu 44.1kHz hoặc 22.05kHz chuẩn).
   - Kiểm tra mức âm lượng trong Cài đặt hoặc trên màn hình Player.
3. **Lỗi Upload code**:
   - Nhấn giữ nút **BOOT** trên ESP32 trong lúc phần mềm hiển thị `Connecting...`.
   - Đảm bảo đã cài driver USB UART (CH340 / CP2102).
