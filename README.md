# ESP32 LED Matrix Clock

Đồng hồ LED Matrix P5 64x32 thông minh với ESP32, hiển thị thời gian, ngày tháng, âm lịch, thông tin thời tiết và môi trường trong nhà với hiệu ứng gradient đầy màu sắc.

![Version](https://img.shields.io/badge/version-2.1.0-blue)
![Platform](https://img.shields.io/badge/platform-ESP32-green)
![License](https://img.shields.io/badge/license-MIT-orange)

## 🎬 Demo

Video demo trên tiktok:
[https://vt.tiktok.com/ZSaMTVyBm/
![Tiktok](video-thumb.jpg)](https://vt.tiktok.com/ZSaMTVyBm/)

## ✨ Tính Năng

### 🕐 Hiển Thị Đồng Hồ

- **Giờ:Phút** với font chữ lớn (size 2) và hiệu ứng gradient rainbow
- **Giây** hiển thị ở góc phải với font nhỏ (size 1)
- Dấu hai chấm `:` với hiệu ứng chuyển động động (ping-pong animation)
- Tự động đồng bộ thời gian qua NTP (Network Time Protocol)
- **Backup Thời Gian Thực (RTC)**: Tích hợp module DS1302 giúp giữ giờ chính xác ngay cả khi mất điện hoặc mất WiFi. Tự động đồng bộ từ NTP sang RTC mỗi giờ.
- Múi giờ GMT+7 (Việt Nam)

### 📅 Hiển Thị Luân Phiên (Mỗi 5 giây)

Luân phiên hiển thị 4 chế độ:

1. **Thứ và Ngày/Tháng**: "Thứ 2, 18/01" hoặc "CNhật, 18/01"
2. **Thời Tiết Ngoài Trời**: Biểu tượng cây (🌲) kèm nhiết độ/độ ẩm từ API.
3. **Môi Trường Trong Nhà**: Biểu tượng nhà (🏠) kèm nhiệt độ/độ ẩm thực tế từ cảm biến AHT10.
4. **Âm Lịch**: "AL18/12/26" (ngày/tháng/năm âm lịch)

### 🌈 Hiệu Ứng & Giao Diện

- Gradient màu rainbow tự động chuyển đổi giữa các chế độ.
- Biểu tượng (Icons) tùy chỉnh thay thế cho các nhãn văn bản (O:, I:).
- Spacing tối ưu, dấu chấm thập phân chỉ chiếm 1 pixel giúp hiển thị gọn gàng.
- **Background Tasks**: Sử dụng FreeRTOS Tasks để đọc cảm biến và lấy dữ liệu thời tiết ở background, đảm bảo màn hình LED không bao giờ bị nháy (flicker).

### 🌤️ Thông Tin Thời Tiết & Cảm Biến

- **Ngoài trời**: Tự động lấy dữ liệu từ Open-Meteo API (cập nhật mỗi 10 phút).
- **Trong nhà**: Đọc trực tiếp từ cảm biến AHT10 (cập nhật mỗi 5 giây).
- Hiển thị nhiệt độ (°C) và độ ẩm (%).

### 📱 Cấu Hình Qua Web

- Giao diện web thân thiện để cấu hình WiFi.
- Chọn thành phố từ danh sách có sẵn (Hà Nội, TP.HCM, Đà Nẵng, v.v.).
- Tự động điền tọa độ GPS khi chọn thành phố.
- Lưu cấu hình vĩnh viễn vào NVS (Non-Volatile Storage).

### 🔄 Reset Về Chế Độ Cấu Hình

- Sử dụng nút **BOOT** có sẵn trên board ESP32 (GPIO 0).
- Nhấn một lần để xóa cấu hình và khởi động lại.
- Tự động chuyển sang AP Mode để cấu hình lại.

## ✅ Checklist Tính Năng Đã Hoàn Thành

### Hiển Thị & Giao Diện (v2.1.0)

- [x] Hiển thị giờ:phút (font lớn) và giây (font nhỏ)
- [x] Hiệu ứng gradient rainbow mượt mà
- [x] Thay thế "O:" và "I:" bằng Icons (Cây 🌲 và Nhà 🏠)
- [x] Fix lỗi nháy màn hình bằng FreeRTOS Tasks
- [x] Tối ưu spacing và hiển thị dấu chấm 1-pixel
- [x] Luân phiên 4 chế độ hiển thị mỗi 5 giây

### Phần Cứng & Kết Nối (v2.1.0)

- [x] Tích hợp module RTC DS1302 (Backup thời gian)
- [x] Tích hợp cảm biến AHT10 (Nhiệt độ/Độ ẩm trong nhà)
- [x] Tự động đồng bộ NTP -> RTC
- [x] Chế độ cấu hình WiFi qua Web (AP Mode)
- [x] Tự động kết nối lại (Retry) khi WiFi mất tín hiệu

## 📋 TODO - Tính Năng Sắp Triển Khai

- [ ] **Điều chỉnh độ sáng qua Web** (v2.2.x)
- [ ] **Chế độ ngủ thông minh** (v2.2.x): Tự động tắt/giảm độ sáng ban đêm
- [ ] **Hẹn giờ báo thức** (v2.3.x): Với module âm thanh
- [ ] **Loa Bluetooth** (v2.3.x): Sử dụng I2S DAC

## 🛠️ Yêu Cầu Phần Cứng

### Linh Kiện Chính

| Linh Kiện         | Mô Tả                             |
| ----------------- | --------------------------------- |
| ESP32 DevKit V1   | Board điều khiển chính            |
| LED Matrix P5     | 64x32 pixels, HUB75 interface     |
| Module RTC DS1302 | Giữ giờ khi mất điện              |
| Cảm biến AHT10    | Đo nhiệt độ/độ ẩm trong nhà (I2C) |
| Nguồn 5V/5A       | Cấp nguồn cho LED Matrix và ESP32 |

### Sơ Đồ Kết Nối

#### 1. LED Matrix (HUB75)

Xem chi tiết tại [LED_MATRIX_SETUP.md](LED_MATRIX_SETUP.md).

#### 2. Module RTC DS1302

| DS1302 Pin | ESP32 GPIO |
| ---------- | ---------- |
| VCC        | 3.3V       |
| GND        | GND        |
| CLK        | GPIO32     |
| DAT        | GPIO33     |
| RST        | GPIO2      |

#### 3. Cảm biến AHT10 (I2C)

| AHT10 Pin | ESP32 GPIO |
| --------- | ---------- |
| VCC       | 3.3V       |
| GND       | GND        |
| SDA       | GPIO21     |
| SCL       | GPIO22     |

> [!CAUTION]
> **KHÔNG** cấp nguồn cho LED Matrix từ ESP32 chân 5V. LED Matrix cần nguồn 5V/5A riêng biệt.

## 📦 Cài Đặt

### Thư Viện Cần Thiết (v2.1.0)

- ESP32-HUB75-MatrixPanel-DMA
- Adafruit GFX Library
- FastLED
- ArduinoJson
- ESPAsyncWebServer & AsyncTCP
- **RTC by Makuna** (cho DS1302)
- **Adafruit AHTX0** (cho AHT10)

## 📂 Cấu Trúc Dự Án

```
esp32-project/
├── platformio.ini           # Cấu hình PlatformIO & Libraries
├── data/                    # Giao diện Web (SPIFFS)
└── src/                     # Source code
    ├── main.cpp             # Logic chính & Hiển thị
    ├── display.cpp/h        # Khởi tạo LED Matrix & Vẽ Icons/Chữ VN
    ├── wifi_manager.cpp/h   # Kết nối WiFi & Retry logic
    ├── rtc_manager.cpp/h    # Quản lý DS1302 & Đống bộ thời gian
    ├── indoor_sensor.cpp/h  # Đọc AHT10 qua FreeRTOS Task
    ├── weather.cpp/h        # Lấy thời tiết API qua FreeRTOS Task
    ├── config_manager.cpp/h # Quản lý cấu hình NVS
    ├── web_server.cpp/h     # Web server cấu hình
    └── lunar_calendar.cpp/h # Tính toán âm lịch
```

## 📝 Lịch Sử Phiên Bản

### v2.1.0 (2026-02-08)

- ✨ Tích hợp cảm biến **AHT10** đo môi trường trong nhà.
- ✨ Tích hợp module **DS1302 RTC** backup thời gian thực.
- ✨ Chuyển sang kiến trúc **Multitasking (FreeRTOS)**: Fix hoàn toàn lỗi nháy màn hình.
- 🎨 Thay đổi giao diện: Sử dụng biểu tượng **Tree (🌲)** và **Home (🏠)**.
- 🎨 Tối ưu hiển thị số và dấu chấm thập phân.
- ⚙️ Thêm timeout cho NTP sync để tránh treo máy khi khởi động.

### v2.0.0 (2026-01-18)

- ✨ Thêm chế độ AP Mode cấu hình WiFi qua web.
- ✨ Hiển thị thời tiết API và Âm lịch.
- ✨ Nút reset cấu hình.

## 👨‍💻 Tác Giả

**Power by [Mạc Tân](https://www.facebook.com/mvt.hp.star/)** | Mobile: [0964 335 688](tel:0964335688)

⭐ Nếu dự án này hữu ích, hãy cho một star trên GitHub!
