# ESP-01S WiFi Clock Project

## 1. Mục tiêu

Xây dựng đồng hồ điện tử sử dụng:

- ESP8266 ESP-01S
- LED Matrix 32x8 (4 module 8x8 sử dụng MAX7219 / GC7219C)
- Đồng bộ thời gian qua Internet bằng NTP
- Captive Portal để cấu hình WiFi
- Lưu cấu hình vào Flash (LittleFS)

Tính năng:

- Hiển thị thời gian dạng `HH:mm`
- Tự động kết nối WiFi đã lưu
- Tự động đồng bộ thời gian qua NTP
- Hỗ trợ cấu hình WiFi bằng Captive Portal
- Có nút chuyển sang chế độ cấu hình
- Có thể mở rộng OTA trong tương lai

---

# 2. Kế hoạch phát triển

## Phase 1 - Firmware Core

Mục tiêu:

- WiFi Manager
- Captive Portal
- Lưu cấu hình
- Kết nối WiFi
- Đồng bộ NTP
- Kiểm tra độ ổn định

Trong giai đoạn này ưu tiên sử dụng UART để debug.

### GPIO Mapping

| Pin        | Chức năng           |
| ---------- | ------------------- |
| TX (GPIO1) | Serial Debug TX     |
| RX (GPIO3) | Serial Debug RX     |
| GPIO0      | Config Button       |
| GPIO2      | LED Test / Reserved |

### Các hạng mục cần hoàn thành

- WiFiManager
- Captive Portal
- Auto reconnect WiFi
- NTP Sync
- Timezone
- LittleFS
- Config load/save
- Stability Test

### Boot Flow

```text
Power On
    |
Load Config
    |
Check Config Button
    |
    +--> Config Mode
    |
Connect WiFi
    |
Sync NTP
    |
Run
```

---

## Phase 2 - LED Matrix

Sau khi firmware ổn định sẽ chuyển sang điều khiển LED Matrix.

### GPIO Mapping

| ESP-01S    | Chức năng     |
| ---------- | ------------- |
| GPIO1 (TX) | DIN           |
| GPIO3 (RX) | CLK           |
| GPIO0      | CS            |
| GPIO2      | Config Button |

### Kết nối MAX7219 / GC7219C

| MAX7219 | ESP-01S |
| ------- | ------- |
| DIN     | GPIO1   |
| CLK     | GPIO3   |
| CS      | GPIO0   |
| GND     | GND     |
| VCC     | 5V      |

### Config Button

```text
GPIO2 ---- Button ---- GND
GPIO2 ---- 10k ---- 3.3V
```

---

# 3. Yêu cầu Boot

ESP8266 yêu cầu:

| Pin   | Trạng thái Boot |
| ----- | --------------- |
| GPIO0 | HIGH            |
| GPIO2 | HIGH            |
| CH_PD | HIGH            |

Khuyến nghị:

```text
GPIO0 -> 10k -> 3.3V
GPIO2 -> 10k -> 3.3V
CH_PD -> 10k -> 3.3V
```

ESP-01S thường đã có pull-up nhưng vẫn nên kiểm tra thực tế.

---

# 4. Chế độ Flash Firmware

Khi nạp firmware:

| Pin   | Trạng thái |
| ----- | ---------- |
| GPIO0 | LOW        |
| GPIO2 | HIGH       |
| CH_PD | HIGH       |

Sau đó reset hoặc cấp nguồn lại.

---

# 5. Cấu hình lưu trữ

Sử dụng LittleFS.

Ví dụ:

```json
{
  "ssid": "HomeWifi",
  "timezone": 7,
  "brightness": 3,
  "is24h": true
}
```

---

# 6. Captive Portal

Sử dụng thư viện:

- WiFiManager

Quy trình:

```text
No Config
    |
Start AP
    |
Captive Portal
    |
Save Config
    |
Reboot
```

Tên AP mặc định:

```text
ESPClock-Setup
```

---

# 7. NTP

Server đề xuất:

```text
pool.ntp.org
time.google.com
```

Timezone:

```text
GMT+7
```

Định dạng hiển thị:

```text
HH:mm
```

Ví dụ:

```text
08:35
14:27
23:59
```

---

# 8. Hiển thị LED Matrix

Kích thước:

```text
32 x 8
```

Số module:

```text
4 x MAX7219
```

Thư viện đề xuất:

- MD_MAX72XX
- MD_Parola

Các tính năng:

- Hiển thị giờ
- Scroll text
- Animation
- Hiển thị trạng thái WiFi

---

# 9. Nguồn cấp

Không cấp nguồn LED Matrix từ ESP-01S.

Khuyến nghị:

```text
5V External Power Supply
```

Kết nối:

```text
5V ---- Matrix VCC
GND --- Matrix GND
GND --- ESP GND
```

Tất cả phải chung GND.

---

# 10. Các nâng cấp tương lai

- OTA Update
- Hiển thị ngày tháng
- Hiển thị nhiệt độ
- MQTT
- Đồng hồ đếm ngược
- Điều chỉnh độ sáng tự động bằng cảm biến ánh sáng
- Đồng bộ thời gian từ Home Assistant
- Web Configuration Page
- Hiệu ứng chuyển đổi giờ/phút
