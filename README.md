# ESP32 LED Matrix Clock

Đồng hồ LED Matrix với 6 tấm module P5 64x32 ghép lại thành 192x128 hiển thị với ESP32-S3-N16R8, hiển thị thời gian, ngày tháng, âm lịch, thông tin thời tiết và môi trường trong nhà với hiệu ứng gradient đầy màu sắc.

Đây là bản cải tiến với 5 hàng hiển thị song song không cần luân phiên, sử dụng board ESP32-S3-N16R8.
Bản 1 led matrix 64x32 ở branch [clock](https://github.com/mvtcode/esp32-project/tree/clock).

![Version](https://img.shields.io/badge/version-3.0.0-blue)
![Platform](https://img.shields.io/badge/platform-ESP32-green)
![License](https://img.shields.io/badge/license-MIT-orange)

## 🎬 Demo

Video demo trên tiktok:
[https://vt.tiktok.com/ZSQ2DhfpF/
![Tiktok](video-thumb.jpg)](https://vt.tiktok.com/ZSQ2DhfpF/)

Video demo chế độ config: https://www.facebook.com/reel/2184050055713548

Video demo 1 tấm led matrix 64x32: https://vt.tiktok.com/ZSaMTVyBm/

## ✨ Tính Năng

### 🕐 Bố cục 5 hàng hiển thị đồng thời (v3.0.0)

Màn hình được thiết kế chia làm 5 hàng độc lập trên lưới 128x96 hiển thị song song toàn bộ thông tin:

1. **Hàng 1 (Y: 0..31): Đồng Hồ Thời Gian**
   - Định dạng `Giờ:Phút:Giây` với font chữ lớn **ClockFont 24px** (font tùy chỉnh).
   - Dấu hai chấm `:` chuyển động trượt dọc mượt mà (sliding animation).
   - Tự động đồng bộ thời gian qua NTP (Network Time Protocol) và sao lưu vào **RTC DS1302** (Backup thời gian thực khi mất điện).
2. **Hàng 2 (Y: 32..47): Lịch Dương & Lịch Âm**
   - Phía trái hiển thị Thứ và Ngày/Tháng (`T.Hai, 26/05` hoặc `CN, 26/05`).
   - Phía phải hiển thị ngày âm lịch dạng ngày/tháng (`AL:10/04`).
   - Sử dụng font **Verdana 8pt Việt Hóa** hiển thị tiếng Việt có dấu hoàn hảo.

3. **Hàng 3 (Y: 48..57): Mô Tả Thời Tiết & Chỉ Số UV**
   - Phía trái hiển thị mô tả thời tiết ngoài trời lấy từ **Open-Meteo API** (ví dụ: `Trời quang`, `Ít mây`, `Có mưa`).
   - Phía phải hiển thị chỉ số UV ngoài trời (`UV:3.5` hoặc `UV:0`).
   - Sử dụng font **Verdana 8pt Việt Hóa** sắc nét.

4. **Hàng 4 (Y: 58..73): Cảm Biến Trong Nhà & Ngoài Trời**
   - Phía trái: Nhiệt độ & Độ ẩm thực tế trong phòng đọc từ cảm biến **AHT10** kèm biểu tượng Ngôi nhà (🏠).
   - Phía phải: Nhiệt độ & Độ ẩm ngoài trời lấy từ **Open-Meteo API** kèm biểu tượng Cây thông (🌲).
   - Sử dụng font **Verdana 8pt Việt Hóa** sắc nét.

5. **Hàng 5 (Y: 74..95): Chữ chạy Marquee tự chọn**
   - Chữ chạy ngang cuộn từ phải sang trái mượt mà với font **Verdana Bold 18pt Việt Hóa** lớn, dễ đọc.
   - Nội dung chữ chạy được người dùng thay đổi trực tiếp qua giao diện Web UI.

### 🌈 Hiệu ứng & Hiển thị nâng cao

- **Anti-Jitter:** Các số giờ, phút, giây được vẽ ở vị trí cố định để chống rung/lắc chữ khi số thay đổi.
- **Gradient Rainbow:** Các vùng chữ tự động đổi sắc màu theo dải màu HSV uyển chuyển.
- **Double Buffering:** Tránh hiện tượng nháy màn hình nhờ cơ chế quét DMA của ESP32 kết hợp background tasks (FreeRTOS).
- **Phông chữ Verdana Việt Hóa:** Đã tích hợp bộ font Unicode tiếng Việt đầy đủ dấu (Verdana 8pt, 10pt, 14pt Bold).

### 🌤️ Thông Tin Thời Tiết & Cảm Biến

- **Ngoài trời**: Tự động lấy dữ liệu từ Open-Meteo API (cập nhật mỗi 10 phút).
- **Trong nhà**: Đọc trực tiếp từ cảm biến AHT10 (cập nhật mỗi 20 giây ở background).
- Hiển thị nhiệt độ (°C) và độ ẩm (%).

### 📱 Cấu Hình Qua Web

- Giao diện web thân thiện để cấu hình WiFi.
- Chọn thành phố từ danh sách có sẵn (Hà Nội, TP.HCM, Đà Nẵng, v.v.).
- Tự động điền tọa độ GPS khi chọn thành phố.
- **Điều chỉnh độ sáng màn hình** (10-100%) qua slider.
- **Cấu hình chế độ ngủ**:
  - Bật/tắt chế độ ngủ
  - Chọn giờ bắt đầu và kết thúc (hỗ trợ qua đêm, vd: 22:00 - 07:00)
  - Độ sáng khi ngủ: 0-100% (0 = tắt màn hình hoàn toàn)
- Lưu cấu hình vĩnh viễn vào NVS (Non-Volatile Storage).

### 🔄 Reset Về Chế Độ Cấu Hình

- Sử dụng nút **BOOT** có sẵn trên board ESP32 (GPIO 0).
- Nhấn một lần để xóa cấu hình và khởi động lại.
- Tự động chuyển sang AP Mode để cấu hình lại.

## ✅ Checklist Tính Năng Đã Hoàn Thành

### Lưới 128x96 & Font Việt Hóa (v3.0.0)

- [x] Ánh xạ tọa độ ảo sang chuỗi panel vật lý ghép nối tiếp (Serpentine 3x2)
- [x] Hỗ trợ hiển thị đồng thời 5 hàng thông tin độc lập không cần luân phiên
- [x] Đồng hồ số lớn bằng font tùy chỉnh ClockFont 24px
- [x] Lịch âm và lịch dương hiển thị song song
- [x] Nhiệt độ/độ ẩm cảm biến trong nhà và ngoài trời hiển thị song song kèm icon
- [x] Chữ chạy (marquee) tiếng Việt hàng 5 với font Verdana Bold 18pt
- [x] Tích hợp ô chỉnh chữ chạy tùy chọn trên Web UI và lưu vào NVS
- [x] Chuyển đổi mã Unicode tiếng Việt sang bảng mã custom 8-bit

### Hiển Thị & Giao Diện (v2.1.0)

- [x] Hiển thị giờ:phút với font chữ lớn và hiệu ứng gradient rainbow
- [x] Hiển thị giây ở góc phải
- [x] Dấu hai chấm với hiệu ứng chuyển động (ping-pong animation)
- [x] Hiển thị thứ và ngày/tháng
- [x] Hiển thị thông tin thời tiết (nhiệt độ và độ ẩm)
- [x] Hiển thị âm lịch
- [x] Luân phiên hiển thị các thông tin mỗi 5 giây
- [x] Thay thế "O:" và "I:" bằng Icons (Cây 🌲 và Nhà 🏠)
- [x] Fix lỗi nháy màn hình bằng FreeRTOS Tasks
- [x] Tối ưu spacing và hiển thị dấu chấm 1-pixel

### Kết Nối & Đồng Bộ (v2.1.0)

- [x] Tự động đồng bộ thời gian qua NTP
- [x] Kết nối WiFi với cấu hình đã lưu
- [x] Lấy dữ liệu thời tiết từ Open-Meteo API
- [x] Cập nhật thời tiết mỗi 10 phút
- [x] Tự động kết nối lại (Retry) khi WiFi mất tín hiệu

### Phần Cứng (v2.1.0)

- [x] Tích hợp module RTC DS1302 (Backup thời gian)
- [x] Tích hợp cảm biến AHT10 (Nhiệt độ/Độ ẩm trong nhà)
- [x] Tự động đồng bộ NTP -> RTC

### Cấu Hình (v2.2.0)

- [x] Chế độ AP Mode để cấu hình WiFi
- [x] Giao diện web thân thiện
- [x] Chọn thành phố từ danh sách có sẵn
- [x] Tự động điền tọa độ GPS khi chọn thành phố
- [x] Lưu cấu hình vào NVS (Non-Volatile Storage)
- [x] Nút reset phần cứng (nút BOOT)
- [x] **Điều chỉnh độ sáng qua Web** (10-100%)
- [x] **Chế độ ngủ thông minh** (hẹn giờ tắt/giảm sáng)

## 📋 TODO - Tính Năng Sắp Triển Khai

### Cải Thiện Kết Nối WiFi

- [x] **Retry Connect WiFi** (v2.0.x): Tự động thử kết nối lại WiFi khi mất kết nối hoặc kết nối thất bại, hiện tại phải nhấn phím reset để kết nối lại.
- [x] **WiFi Captive Portal** (v2.1.x): Tự động chuyển hướng đến trang cấu hình khi kết nối AP mode (không cần nhập địa chỉ IP thủ công)
- [x] **Danh sách WiFi** (v2.1.x): Hiển thị danh sách các mạng WiFi khả dụng trong trang web cấu hình để dễ dàng chọn

### Điều Chỉnh Hiển Thị

- [x] **Điều chỉnh độ sáng runtime** (v2.2.0): Cho phép thay đổi độ sáng LED Matrix qua giao diện web mà không cần upload lại code
- [x] **Chế độ ngủ thông minh** (v2.2.0):
  - Cấu hình giờ ngủ và giờ thức (ví dụ: 23:00 - 06:00)
  - Tùy chọn giảm độ sáng hoặc tắt hoàn toàn màn hình trong giờ ngủ
  - Tự động bật lại màn hình khi đến giờ thức
- [ ] **Hiển thị ngày đặc biệt** (v2.3.x):
  - Hiển thị ngày lễ đặc biệt (như Tết, Giáng sinh, v.v.)
- [ ] **Hiển thị ngày tốt xấu** (v2.3.x):
  - Hiển thị ngày tốt xấu dựa trên ngày âm lịch
- [ ] **Hiển thị ngày sinh nhật** (v2.3.x):
  - Hiển thị ngày sinh nhật của bạn, bạn bè, nyc
- [ ] **Lưu trữ và hiển thị log** (v2.4.x):
  - Lưu trữ log vào database
  - Hiển thị log qua giao diện web các thông tin: nhiệt độ, độ ẩm của ngày nào đó đã lưu.

### Tích Hợp Module Âm Thanh

- [ ] **Hẹn giờ báo thức** (v2.5.x):
  - Cấu hình nhiều báo thức qua giao diện web
  - Chọn nhạc chuông báo thức
  - Tùy chọn lặp lại theo ngày trong tuần
  - Hiển thị biểu tượng báo thức trên màn hình LED
- [ ] **Loa Bluetooth** (v2.5.x):
  - Biến đồng hồ thành loa Bluetooth
  - Kết nối với điện thoại để phát nhạc
  - Hiển thị tên bài hát đang phát trên màn hình LED (nếu có metadata)
  - Điều khiển âm lượng qua giao diện web hoặc nút bấm

### Tích hợp module tùy chọn (có thể có hoặc không)

- [x] **Module thời gian thực DS1302** (v2.1.0)
- [x] **Cảm biến nhiệt độ, độ ẩm AHT10** (v2.1.0)
- [ ] **Cảm biến chất lượng không khí BME680** (v2.6.x):
  - Hiển thị chất lượng không khí trên màn hình LED
- [ ] **Cảm biến ánh sáng BH1750** (v2.6.x):
  - Tự động điều chỉnh độ sáng LED Matrix theo ánh sáng môi trường
- [ ] **Module ESP-NOW** (v2.6.x):
  - Giao tiếp với các ESP32 khác để chia sẻ dữ liệu với các module khác.
- [ ] **Module radar** (v2.7.x):
  - Phát hiện chuyển động, người, vật thể đến gần để điều chỉnh độ sáng cho phù hợp.
- [ ] **Module GPS** (v2.7.x):
  - Lấy tọa độ GPS để hiển thị thời tiết hiện tại mà không cần phải config thủ công (điện thoại có thể được nhưng chỉ hỗ trợ web có SSL (HTTPS).
- [ ] **Làm MCP cho AI** (v3.x.x):
  - làm MCP cho AI (model context protocol)

### Tích hợp xiaozhi (dùng Esp32-s3)

- [ ] **Tích hợp xiaozhi** (v4.x.x):
  - Tích hợp xiaozhi vào esp32-s3 để làm chatbot âm thanh
  - Điều khiển thiết bị thông qua giọng nói

## 🛠️ Yêu Cầu Phần Cứng

Danh sách linh kiện - phần cứng sử dụng: [Google Sheet](https://docs.google.com/spreadsheets/d/1swM3OM9-xU4paUzBfFUdlDwbnLg4lPnLfiZTFbzTzyY/edit?usp=sharing)

### Linh Kiện Chính

| Linh Kiện         | Mô Tả                                                                   |
| ----------------- | ----------------------------------------------------------------------- |
| ESP32-S3-N16R8    | Phiên bản 16MB Flash, 8MB PSRAM (Octal SPI) chạy mượt mà nhất           |
| LED Matrix P5     | 6 panel 64x32 pixels ghép thành lưới 128x96 (3 panel ngang, 2 hàng dọc) |
| Module RTC DS1302 | Giữ giờ khi mất điện                                                    |
| Cảm biến AHT10    | Đo nhiệt độ/độ ẩm trong nhà (I2C)                                       |
| Nguồn 5V/10A+     | Cấp nguồn cho 6 panel LED Matrix và ESP32 (Dòng tải khuyên dùng >= 10A) |

### Sơ Đồ Kết Nối (ESP32-S3-N16R8)

> [!IMPORTANT]
> **Lưu ý đặc biệt cho ESP32-S3-N16R8:**
>
> - Không sử dụng các chân từ **GPIO 26 đến GPIO 32** (kết nối Flash/PSRAM nội bộ) và **GPIO 33 đến GPIO 37** (kết nối Octal PSRAM).
> - Các chân **GPIO 22, 23, 24, 25 không tồn tại** trên chip ESP32-S3.
> - Sơ đồ dưới đây đã được thiết kế tối ưu, hoàn toàn tránh xung đột và tập hợp các chân cắm cảm biến/RTC về một cụm bên trái giúp đi dây dễ dàng.

#### 1. LED Matrix (HUB75)

| HUB75 Pin | ESP32-S3 GPIO | Chức Năng     | Mô Tả                             |
| --------- | ------------- | ------------- | --------------------------------- |
| **R1**    | GPIO4         | Red Data 1    | Dữ liệu màu đỏ (nửa trên)         |
| **G1**    | GPIO5         | Green Data 1  | Dữ liệu màu xanh lá (nửa trên)    |
| **B1**    | GPIO6         | Blue Data 1   | Dữ liệu màu xanh dương (nửa trên) |
| **R2**    | GPIO7         | Red Data 2    | Dữ liệu màu đỏ (nửa dưới)         |
| **G2**    | GPIO15        | Green Data 2  | Dữ liệu màu xanh lá (nửa dưới)    |
| **B2**    | GPIO16        | Blue Data 2   | Dữ liệu màu xanh dương (nửa dưới) |
| **A**     | GPIO17        | Address A     | Quét hàng bit 0                   |
| **B**     | GPIO18        | Address B     | Quét hàng bit 1                   |
| **C**     | GPIO8         | Address C     | Quét hàng bit 2                   |
| **D**     | GPIO42        | Address D     | Quét hàng bit 3                   |
| **CLK**   | GPIO41        | Clock         | Xung nhịp đồng bộ                 |
| **LAT**   | GPIO40        | Latch         | Chốt dữ liệu                      |
| **OE**    | GPIO2         | Output Enable | Cho phép hiển thị (Active LOW)    |
| **GND**   | GND           | Ground        | Đất                               |

#### Sơ đồ lắp đặt chuỗi Panel (Serpentine Mapping):

Các panel được kết nối nối tiếp bằng cáp HUB75 theo thứ tự từ nguồn phát tín hiệu (ESP32):

```text
  [ESP32] ──► Panel 1 (Top-Left) ──► Panel 2 (Top-Right)
                                               │
                                      [Cáp nối hàng]
                                               │
                                               ▼
               Panel 4 (Mid-Left, 🔄180°) ◄── Panel 3 (Mid-Right, 🔄180°)
                       │
               [Cáp nối hàng]
                       │
                       ▼
               Panel 5 (Bot-Left) ──────────► Panel 6 (Bot-Right)
```

_Lớp `CustomMatrixPanel` trong dự án tự động chuyển đổi tọa độ ảo (0-127, 0-95) thành địa chỉ pixel vật lý phù hợp với sơ đồ đi dây này._

#### 2. Module RTC DS1302

| DS1302 Pin | ESP32-S3 GPIO | Chức Năng |
| ---------- | ------------- | --------- |
| **VCC**    | 3.3V          | Nguồn cấp |
| **GND**    | GND           | Đất       |
| **CLK**    | GPIO11        | Clock     |
| **DAT**    | GPIO12        | Data I/O  |
| **RST**    | GPIO13        | Reset     |

#### 3. Cảm biến AHT10 (I2C)

| AHT10 Pin | ESP32-S3 GPIO | Chức Năng |
| --------- | ------------- | --------- |
| **VCC**   | 3.3V          | Nguồn cấp |
| **GND**   | GND           | Đất       |
| **SDA**   | GPIO9         | I2C Data  |
| **SCL**   | GPIO10        | I2C Clock |

#### 4. Nút Reset Cấu Hình

- Sử dụng nút **BOOT** có sẵn trên board ESP32 (GPIO 0)
- Nhấn một lần để reset về chế độ cấu hình AP Mode

> [!CAUTION]
> **KHÔNG** cấp nguồn cho LED Matrix từ ESP32! Chuỗi 6 panel LED Matrix cần nguồn 5V riêng biệt với dòng điện lớn (tối thiểu 10A ở độ sáng cao). ESP32 chỉ cung cấp tín hiệu điều khiển.
> Nối chung chân GND của nguồn, LED Matrix và ESP32.

Chi tiết kết nối phần cứng: xem [LED_MATRIX_SETUP.md](LED_MATRIX_SETUP.md)

## 📦 Cài Đặt

### 1. Cài Đặt PlatformIO

#### Cách 1: PlatformIO CLI

```bash
# Cài đặt qua pip
pip install -U platformio

# Hoặc qua Homebrew (macOS)
brew install platformio
```

#### Cách 2: VS Code Extension

1. Mở VS Code
2. Vào Extensions (Ctrl+Shift+X)
3. Tìm "PlatformIO IDE"
4. Click Install

### 2. Clone Dự Án

```bash
git clone https://github.com/mvtcode/esp32-project.git
cd esp32-project
git checkout clock-esp32s3-6p5
```

### 3. Cài Đặt Dependencies

PlatformIO sẽ tự động tải các thư viện cần thiết khi build:

- ESP32-HUB75-MatrixPanel-DMA
- Adafruit GFX Library
- FastLED
- ArduinoJson
- ESPAsyncWebServer
- AsyncTCP
- **RTC by Makuna** (cho DS1302)
- **Adafruit AHTX0** (cho AHT10)

### 4. Build Project

```bash
pio run
```

### 5. Upload Code Lên ESP32

```bash
# Upload code chương trình
pio run --target upload

# Upload filesystem (SPIFFS) - chứa file index.html
pio run --target uploadfs
```

> [!TIP]
> Bạn cũng có thể sử dụng các script tiện lợi:
>
> ```bash
> ./upload.sh    # Upload code
> ./monitor.sh   # Mở Serial Monitor
> ```

### 6. Xem Serial Monitor

```bash
pio device monitor
```

## 🚀 Hướng Dẫn Sử Dụng

### Lần Đầu Sử Dụng (Chưa Có Cấu Hình)

1. **Upload code lên ESP32** (xem phần Cài Đặt)

2. **ESP32 tự động khởi động ở AP Mode**:
   - LED Matrix hiển thị:
     ```
     Cấu hình WiFi:    (màu cam, cố định)
     WiFi: Clock-2026  (màu trắng, nhấp nháy)
     Web: 192.168.4.1  (màu cyan)
     ```

3. **Kết nối WiFi từ điện thoại/máy tính**:
   - Tên WiFi: `Clock-2026`
   - Không cần mật khẩu

   ![](connect-wifi.jpg)

4. **Mở trình duyệt và truy cập**:

   ```
   http://192.168.4.1
   ```

5. **Cấu hình trên giao diện web**:
   - Nhập **SSID** (tên WiFi nhà bạn)
   - Nhập **Mật khẩu WiFi**
   - Chọn **Thành phố** từ dropdown (tọa độ GPS sẽ tự động điền)
   - Điều chỉnh **độ sáng màn hình** (10-100%)
   - Cấu hình **chế độ ngủ** (tùy chọn):
     - Bật/tắt chế độ ngủ
     - Chọn giờ bắt đầu và kết thúc
     - Chọn độ sáng khi ngủ (0-100%, 0 = tắt màn hình)
   - Click **"Lưu cấu hình"**

   ![](config.jpg)

6. **ESP32 tự động khởi động lại**:
   - Kết nối WiFi đã cấu hình
   - Đồng bộ thời gian từ NTP
   - Lấy dữ liệu thời tiết
   - Hiển thị đồng hồ

### Sử Dụng Bình Thường

Sau khi đã cấu hình, ESP32 sẽ:

- Tự động kết nối WiFi khi khởi động
- Hiển thị đồng hồ với hiệu ứng gradient
- Hiển thị song song đồng thời toàn bộ thông tin (Giờ, Lịch âm/dương, Thời tiết/UV, Cảm biến, Chữ chạy)
- Tự động cập nhật thời tiết mỗi 10 phút
- Tự động áp dụng chế độ ngủ theo giờ đã cấu hình

### Reset Về Chế Độ Cấu Hình

Nếu bạn muốn đổi WiFi hoặc cấu hình lại:

1. **Nhấn nút BOOT** trên board ESP32 (GPIO 0)
2. ESP32 sẽ:
   - Xóa toàn bộ cấu hình đã lưu
   - Khởi động lại
   - Tự động chuyển sang AP Mode
3. Làm lại các bước cấu hình như lần đầu

> [!NOTE]
> Nút BOOT là nút có sẵn trên board ESP32, thường được dùng để upload code. Nhấn một lần là đủ để kích hoạt reset.

## 📂 Cấu Trúc Dự Án

```
esp32-project/
├── platformio.ini              # Cấu hình PlatformIO & Libraries
├── data/                       # Giao diện Web (SPIFFS)
└── src/                        # Source code
    ├── main.cpp                # Logic chính & Vòng lặp hiển thị
    ├── display.cpp/h           # Khởi tạo LED Matrix & Icons/Chữ VN
    ├── wifi_manager.cpp/h      # Kết nối WiFi & Retry logic
    ├── rtc_manager.cpp/h       # Quản lý DS1302 & Đồng bộ thời gian
    ├── indoor_sensor.cpp/h     # Đọc AHT10 qua FreeRTOS Task
    ├── weather.cpp/h           # Lấy thời tiết API qua FreeRTOS Task
    ├── config_manager.cpp/h    # Quản lý cấu hình NVS
    ├── web_server.cpp/h        # Web server & Captive Portal
    ├── reset_button.cpp/h      # Nút BOOT reset cấu hình
    ├── lunar_calendar.cpp/h    # Tính toán âm lịch
    ├── ClockFont24px.h         # Font tùy chỉnh cho đồng hồ
    ├── Verdana_Bold14pt.h      # Font Verdana Bold 14pt Việt Hóa
    ├── Verdana_Bold18pt.h      # Font Verdana Bold 18pt Việt Hóa (marquee)
    ├── Verdana_Vietnamese10pt.h # Font Verdana 10pt Việt Hóa
    ├── Verdana_Vietnamese12pt.h # Font Verdana 12pt Việt Hóa
    ├── vietnamese_helper.h     # Bộ chuyển đổi Unicode → Custom 8-bit
    └── test-led.cpp            # Test panel LED riêng biệt (env:test-led)
```

## 🎨 Tùy Chỉnh

### Thay Đổi Độ Sáng

Sử dụng giao diện web để điều chỉnh độ sáng từ 10-100%.

Hoặc trong `src/display.cpp`, dòng ~63 (giá trị khởi động tạm thời khi boot):

```cpp
dma_display->setBrightness8(50); // 0-255 (chỉ áp dụng khi boot, sẽ bị ghi đè bởi config)
```

### Thay Đổi Tốc Độ Gradient

Trong `src/main.cpp`, dòng ~560:

```cpp
globalHue += 1; // Tăng giá trị này để gradient chạy nhanh hơn
```

### Thêm Thành Phố Mới

Trong `data/index.html`, thêm vào mảng `cities`:

```javascript
const cities = [
  { name: "Hà Nội", lat: 21.0285, lon: 105.8542 },
  { name: "Thành phố mới", lat: xx.xxxx, lon: xxx.xxxx },
  // ...
];
```

### Thay Đổi Tên WiFi AP Mode

Trong `src/web_server.cpp`, tìm dòng:

```cpp
WiFi.softAP("Clock-2026");
```

### Thay Đổi Thời Gian Cập Nhật Thời Tiết

Trong `src/main.cpp`, dòng 28:

```cpp
const unsigned long weatherUpdateInterval = 600000; // 10 phút (ms)
```

## 🔧 Troubleshooting

### LED Matrix không sáng

- ✅ Kiểm tra nguồn 5V cho LED Matrix
- ✅ Kiểm tra kết nối GND chung giữa ESP32 và LED Matrix
- ✅ Kiểm tra cáp HUB75 cắm đúng hướng

### Màu sắc hiển thị sai

- ✅ Kiểm tra kết nối R1, G1, B1, R2, G2, B2
- ✅ Thử thay đổi driver IC trong code (một số panel dùng FM6126A)

### Không kết nối được WiFi AP Mode

- ✅ Đảm bảo đã upload cả code và filesystem (`uploadfs`)
- ✅ Kiểm tra Serial Monitor xem có thông báo lỗi
- ✅ Reset ESP32 và thử lại

### Không lưu được cấu hình

- ✅ Kiểm tra kết nối mạng từ điện thoại đến ESP32
- ✅ Xem Serial Monitor để debug
- ✅ Đảm bảo đã nhập đúng SSID và password

### Thời gian không chính xác

- ✅ Kiểm tra kết nối WiFi
- ✅ Đảm bảo ESP32 có thể truy cập internet để đồng bộ NTP
- ✅ Kiểm tra múi giờ (GMT+7 cho Việt Nam)

### Nút Reset không hoạt động

- ✅ Sử dụng nút BOOT có sẵn trên board ESP32 (không cần nút ngoài)
- ✅ Nhấn một lần rõ ràng (có debounce 1 giây)
- ✅ Xem Serial Monitor để kiểm tra log reset

### Chế độ ngủ không hoạt động

- ✅ Kiểm tra Serial Monitor để xem log sleep mode debug
- ✅ Đảm bảo đã enable chế độ ngủ trong web config
- ✅ Kiểm tra giờ hiện tại có nằm trong khoảng thời gian ngủ không
- ✅ Verify cấu hình đã được lưu vào NVS (xem log khi boot)

## 📝 Lịch Sử Phiên Bản

### v3.0.0 (2026-05-28)

- 🚀 **Nâng cấp lên 6 panel LED Matrix** ghép thành lưới **192×96 pixels** (3 cột × 2 hàng)
- ✨ **5 hàng hiển thị song song** không cần luân phiên (Giờ / Lịch / Thời tiết+UV / Cảm biến / Marquee)
- ✨ **Font tùy chỉnh ClockFont 24px** cho đồng hồ số lớn
- ✨ **Verdana Bold 18pt** cho chữ chạy Marquee
- ✨ **Dấu hai chấm sliding animation** mượt mà giữa HH:MM:SS
- ✨ **Icon WiFi signal** (5 thanh cường độ), **icon đồng bộ thời gian** và **icon thời tiết** trên thanh trạng thái
- ✨ **Chỉ số UV ngoài trời** từ Open-Meteo API
- ✨ **NTP ↔ RTC drift detection**: Tự động cập nhật RTC nếu lệch ≥ 5 giây
- ✨ **Background WiFi reconnect** không blocking mỗi 30 giây
- ✨ **Periodic NTP sync** 60 phút/lần để đảm bảo độ chính xác
- ✨ **Captive Portal** tự động chuyển hướng khi kết nối AP mode
- 🎨 **VirtualMatrixPanel** (`CustomMatrixPanel`) với serpentine 3×2 mapping
- 🐛 Chuyển sang ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM Octal)

### v2.2.0 (2026-02-08)

- ✨ **Điều chỉnh độ sáng qua Web**: Slider 10-100% để điều chỉnh độ sáng màn hình
- ✨ **Chế độ ngủ thông minh**:
  - Bật/tắt chế độ ngủ
  - Cấu hình giờ ngủ và giờ thức (hỗ trợ qua đêm)
  - Tùy chọn độ sáng khi ngủ hoặc tắt màn hình hoàn toàn (0%)
- 🐛 Sửa lỗi brightness không được apply khi tắt sleep mode
- 📊 Thêm logging chi tiết cho toàn bộ flow config (web → API → NVS → runtime)

### v2.1.0 (2026-02-07)

- ✨ Tích hợp cảm biến **AHT10** đo môi trường trong nhà.
- ✨ Tích hợp module **DS1302 RTC** backup thời gian thực.
- ✨ Chuyển sang kiến trúc **Multitasking (FreeRTOS)**: Fix hoàn toàn lỗi nháy màn hình.
- 🎨 Thay đổi giao diện: Sử dụng biểu tượng **Tree (🌲)** và **Home (🏠)**.
- 🎨 Tối ưu hiển thị số và dấu chấm thập phân.
- ⚙️ Thêm timeout cho NTP sync để tránh treo máy khi khởi động.

### v2.0.0 (2026-01-18)

- ✨ Thêm chế độ AP Mode để cấu hình WiFi qua web
- ✨ Thêm hiển thị thông tin thời tiết
- ✨ Thêm hiển thị âm lịch
- ✨ Thêm nút reset phần cứng
- ✨ Lưu cấu hình vào NVS
- 🎨 Cải thiện hiệu ứng gradient
- 🎨 Thêm hiệu ứng chuyển động cho dấu hai chấm
- 🐛 Sửa lỗi hiển thị chữ "CNhật" với ký tự đặc biệt

### v1.0.0

- 🎉 Phiên bản đầu tiên với chức năng đồng hồ cơ bản

## 🤝 Đóng Góp

Mọi đóng góp đều được chào đón! Hãy tạo Pull Request hoặc mở Issue nếu bạn có ý tưởng cải thiện.

## 📄 License

MIT License - Tự do sử dụng và chỉnh sửa cho mục đích cá nhân và thương mại.

## 👨‍💻 Tác Giả

**Power by [Mạc Tân](https://www.facebook.com/mvt.hp.star/)** | Mobile: [0964 335 688](tel:0964335688)

---

⭐ Nếu dự án này hữu ích, hãy cho một star trên GitHub!
