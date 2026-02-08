# ESP32 LED Matrix Clock

Đồng hồ LED Matrix P5 64x32 thông minh với ESP32, hiển thị thời gian, ngày tháng, âm lịch, thông tin thời tiết và môi trường trong nhà với hiệu ứng gradient đầy màu sắc.

![Version](https://img.shields.io/badge/version-2.2.0-blue)
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
2. **Thời Tiết Ngoài Trời**: Biểu tượng cây (🌲) kèm nhiệt độ/độ ẩm từ API.
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

| Linh Kiện         | Mô Tả                             |
| ----------------- | --------------------------------- |
| ESP32 DevKit V1   | Board điều khiển chính            |
| LED Matrix P5     | 64x32 pixels, HUB75 interface     |
| Module RTC DS1302 | Giữ giờ khi mất điện              |
| Cảm biến AHT10    | Đo nhiệt độ/độ ẩm trong nhà (I2C) |
| Nguồn 5V/5A       | Cấp nguồn cho LED Matrix và ESP32 |

### Sơ Đồ Kết Nối

#### 1. LED Matrix (HUB75)

| HUB75 Pin | ESP32 GPIO | Chức Năng     |
| --------- | ---------- | ------------- |
| R1        | GPIO25     | Red Data 1    |
| G1        | GPIO26     | Green Data 1  |
| B1        | GPIO27     | Blue Data 1   |
| R2        | GPIO14     | Red Data 2    |
| G2        | GPIO12     | Green Data 2  |
| B2        | GPIO13     | Blue Data 2   |
| A         | GPIO23     | Address A     |
| B         | GPIO19     | Address B     |
| C         | GPIO5      | Address C     |
| D         | GPIO17     | Address D     |
| CLK       | GPIO16     | Clock         |
| LAT       | GPIO4      | Latch         |
| OE        | GPIO15     | Output Enable |
| GND       | GND        | Ground        |

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

#### 4. Nút Reset

- Sử dụng nút **BOOT** có sẵn trên board ESP32 (GPIO 0)
- Nhấn một lần để reset về chế độ cấu hình

> [!CAUTION]
> **KHÔNG** cấp nguồn cho LED Matrix từ ESP32! LED Matrix cần nguồn 5V riêng biệt với dòng điện lớn (2-4A). ESP32 chỉ cung cấp tín hiệu điều khiển.

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
git checkout clock
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
     Conf wifi:    (màu cam, cố định)
     Clock-2026    (màu trắng, nhấp nháy)
     192.168.4.1   (màu cyan)
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
- Luân phiên hiển thị: Ngày/Tháng → Thời tiết → Âm lịch (mỗi 5 giây)
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
├── platformio.ini           # Cấu hình PlatformIO & Libraries
├── data/                    # Giao diện Web (SPIFFS)
└── src/                     # Source code
    ├── main.cpp             # Logic chính & Hiển thị
    ├── display.cpp/h        # Khởi tạo LED Matrix & Vẽ Icons/Chữ VN
    ├── wifi_manager.cpp/h   # Kết nối WiFi & Retry logic
    ├── rtc_manager.cpp/h    # Quản lý DS1302 & Đồng bộ thời gian
    ├── indoor_sensor.cpp/h  # Đọc AHT10 qua FreeRTOS Task
    ├── weather.cpp/h        # Lấy thời tiết API qua FreeRTOS Task
    ├── config_manager.cpp/h # Quản lý cấu hình NVS
    ├── web_server.cpp/h     # Web server cấu hình
    └── lunar_calendar.cpp/h # Tính toán âm lịch
```

## 🎨 Tùy Chỉnh

### Thay Đổi Độ Sáng

Sử dụng giao diện web để điều chỉnh độ sáng từ 10-100%.

Hoặc trong `src/display.cpp`, dòng 34:

```cpp
dma_display->setBrightness8(50); // 0-255
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
