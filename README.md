# ESP32-S3 Super Mini - OLED 1.3" Test Case

Dự án này là bộ code mẫu để kiểm tra khả năng hoạt động của board **ESP32-S3 Super Mini** với màn hình **OLED 1.3 inch** (độ phân giải 128x64, chip điều khiển SH1106) sử dụng giao tiếp I2C.

## 🛠 Thông số kỹ thuật
- **Microcontroller**: ESP32-S3 Super Mini (Lolin S3 Mini clone).
- **Màn hình**: OLED 1.3 inch.
- **Độ phân giải**: 128x64 pixels.
- **Driver Chip**: SH1106.
- **Giao tiếp**: I2C (Software I2C để tăng tính ổn định).

## 🔌 Sơ đồ đấu nối (Pin Wiring)

Kết nối màn hình OLED với ESP32-S3 Super Mini theo sơ đồ sau:

| OLED 1.3" (I2C) | ESP32-S3 Super Mini | Màu dây (Gợi ý) |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Đỏ |
| **GND** | **GND** | Đen |
| **SCL** | **GPIO 9** | Vàng |
| **SDA** | **GPIO 8** | Xanh lá |

> [!IMPORTANT]
> **Lưu ý về chân I2C:** Trên dòng S3 Super Mini, mặc dù có thể dùng nhiều chân khác nhau, nhưng GPIO 8/9 là cặp chân mặc định ổn định nhất cho I2C.

## 🚀 Hướng dẫn chạy ứng dụng

Dự án được quản lý bằng **PlatformIO**.

### 1. Chuẩn bị môi trường
- Cài đặt VS Code và extension **PlatformIO IDE**.
- Chipset sử dụng: `espressif32`, Board: `lolin_s3_mini`.

### 2. Biên dịch và Nạp Code
Mở terminal trong thư mục dự án và chạy các lệnh sau:

```bash
# Biên dịch dự án
pio run

# Nạp code lên ESP32
pio run --target upload

# Hoặc dùng script tiện ích có sẵn
./upload.sh
```

### 3. Theo dõi Serial Monitor
Sau khi nạp code thành công, bạn có thể kiểm tra log debug:
```bash
pio device monitor
# Hoặc
./monitor.sh
```

## 📝 Cấu trúc mã nguồn
- `platformio.ini`: Chứa cấu hình board và khai báo thư viện `U8g2`.
- `src/main.cpp`: Mã nguồn chính sử dụng Software I2C và thư viện U8g2 để hiển thị animation test.

## 💡 Xử lý sự cố
Nếu màn hình bị nhiễu (garbage) hoặc hiển thị không đúng:
1. Kiểm tra lại các đầu cắm trên breadboard (đảm bảo tiếp xúc tốt).
2. Code hiện tại đang dùng `U8G2_SH1106_128X64_NONAME_F_SW_I2C`. Nếu màn hình bị lệch, bạn có thể thử đổi sang `VHR` hoặc `SSD1306` trong `main.cpp`.

---
*Dự án được phát triển và hỗ trợ bởi Antigravity AI.*
