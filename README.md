# ESP-01S WiFi Clock (MAX7219 LED Matrix)

Dự án firmware chuyên biệt cho board **ESP-01S (ESP8266)** điều khiển đồng hồ LED Matrix 32x8 (MAX7219) tự động đồng bộ thời gian qua Internet (NTP) và cấu hình WiFi qua Captive Portal.

---

## 📌 Sơ đồ kết nối phần cứng (GPIO Mapping)

Do ESP-01S có số lượng chân GPIO giới hạn (chỉ có 4 chân hữu dụng), dự án được chia làm 2 giai đoạn (Phase) với sơ đồ chân khác nhau được cấu hình động qua PlatformIO:

### Phase 1: Thử nghiệm & Debug Serial
> [!NOTE]
> Giai đoạn này ưu tiên sử dụng cổng Serial (TX/RX) để debug trạng thái hệ thống.

| Linh kiện | Chân ESP-01S | Chức năng | Trạng thái điện áp |
| --- | --- | --- | --- |
| **Config Button** | `GPIO0` | Nhấn giữ > 3s để reset WiFi/Config | Kéo trở Pull-up lên 3.3V |
| **Status LED** | `GPIO2` | Đèn nháy báo trạng thái hệ thống | Active LOW (Led ON ở mức LOW) |
| **Serial Debug** | `GPIO1 (TX)` / `GPIO3 (RX)` | Log thông tin ra Serial Monitor | 115200 Baud |

---

### Phase 2: Chạy chính thức với LED Matrix (Mặc định)
> [!WARNING]
> Do sử dụng GPIO1 (TX) và GPIO3 (RX) làm chân Data/Clock cho LED Matrix, cổng Serial sẽ bị vô hiệu hóa hoàn toàn trong giai đoạn này để tránh xung đột tín hiệu.

| Linh kiện / LED Matrix | Chân ESP-01S | Chức năng | Ghi chú |
| --- | --- | --- | --- |
| **DIN (Data In)** | `GPIO1 (TX)` | Đường truyền dữ liệu LED | Kết nối chân DIN của MAX7219 |
| **CLK (Clock)** | `GPIO3 (RX)` | Xung nhịp đồng bộ LED | Kết nối chân CLK của MAX7219 |
| **CS (Chip Select)** | `GPIO0` | Chọn chip điều khiển | Kết nối chân CS của MAX7219 |
| **Config Button** | `GPIO2` | Nhấn giữ > 3s để reset WiFi/Config | Kéo trở Pull-up lên 3.3V |

---

## 📂 Cấu trúc mã nguồn (Modular Architecture)

Mã nguồn được thiết kế theo dạng module độc lập để dễ dàng mở rộng và bảo trì:

* **[main.cpp](src/main.cpp)**: Entrypoint chính của ứng dụng cho Phase 2 (Đồng hồ LED Matrix).
* **[main-phase1.cpp](src/main-phase1.cpp)**: Entrypoint chính cho Phase 1 (Debug qua Serial).
* **[config_manager.h](src/config_manager.h) / [.cpp](src/config_manager.cpp)**: Quản lý mount LittleFS, lưu trữ thông tin cấu hình WiFi và các tham số phụ (`timezone`, `brightness`, `is24h`) dạng JSON.
* **[wifi_controller.h](src/wifi_controller.h) / [.cpp](src/wifi_controller.cpp)**: Quản lý kết nối WiFi, tích hợp Portal cấu hình của `WiFiManager` và cơ chế tự động kết nối lại ngầm.
* **[ntp_manager.h](src/ntp_manager.h) / [.cpp](src/ntp_manager.cpp)**: Đồng bộ giờ Internet qua giao thức NTP, quản lý múi giờ và xuất chuỗi thời gian định dạng chuẩn.
* **[button_handler.h](src/button_handler.h) / [.cpp](src/button_handler.cpp)**: Xử lý nút bấm thông minh (nhấn giữ 3 giây để xóa cấu hình và tự động restart).
* **[led_indicator.h](src/led_indicator.h) / [.cpp](src/led_indicator.cpp)**: Điều khiển LED trạng thái bằng `Ticker` bất đồng bộ (chỉ hoạt động trong Phase 1).
* **[debug.h](src/debug.h)**: Cung cấp các macro `DEBUG_PRINT`, `DEBUG_PRINTLN`, `DEBUG_PRINTF` để tắt/bật Serial logs linh hoạt.

---

## 🚀 Hướng dẫn Biên dịch & Nạp Code

Dự án được cấu hình sẵn 2 môi trường (Environment) tương ứng với 2 giai đoạn trong file `platformio.ini`:

### 1. Nạp chương trình Phase 1 (Cơ bản - Debug)
Biên dịch `main-phase1.cpp`, mở Serial log ở baudrate 115200:
```bash
# Biên dịch và nạp code
pio run -e esp01s_phase1 --target upload

# Mở cổng giám sát Serial
pio device monitor -e esp01s_phase1
```

### 2. Nạp chương trình Phase 2 (Đồng hồ LED Matrix - Chính thức)
Biên dịch `main.cpp` kết hợp điều khiển LED Matrix MAX7219 qua thư viện `MD_Parola`:
```bash
# Biên dịch và nạp code
pio run -e esp01s_phase2 --target upload
```

---

## ⚙️ Hướng dẫn Cấu hình WiFi & Hệ thống

1. Khi thiết bị bật nguồn lần đầu hoặc không kết nối được WiFi cũ, nó sẽ tự động phát ra một Access Point có tên là: **`ESPClock-Setup`**.
2. Dùng điện thoại kết nối vào WiFi này, Captive Portal sẽ tự động hiện ra (hoặc truy cập IP `192.168.4.1`).
3. Nhập các thông tin cấu hình:
   * **WiFi SSID & Password**
   * **Timezone**: Múi giờ của bạn (ví dụ: `7` đối với Việt Nam).
   * **Brightness**: Độ sáng màn hình LED từ `0` đến `15`.
   * **24h Format**: Định dạng giờ (`1` là 24h, `0` là 12h).
4. Nhấn **Save**. Thiết bị sẽ tự lưu cấu hình vào phân vùng LittleFS, kết nối WiFi, đồng bộ giờ từ NTP và bắt đầu hiển thị.
