# Kế Hoạch Triển Khai Kết Nối Chuột Bluetooth Với ESP32-WROOM

Tài liệu này mô tả chi tiết phương án kỹ thuật và kế hoạch triển khai kết nối **Chuột Bluetooth (BLE HID Mouse)** sử dụng vi điều khiển **ESP32-WROOM**. Hệ thống sẽ **tắt hoàn toàn WiFi** để giải phóng tối đa tài nguyên RAM/Heap, chống xung đột thời gian chia sẻ băng tần vô tuyến (RF Coexistence) và đảm bảo độ trễ thấp nhất cho tín hiệu chuột.

---

## 1. Mục Tiêu & Phạm Vi

1. **Tối ưu tài nguyên hệ thống**:
   - Tắt hoàn toàn WiFi (`WiFi.mode(WIFI_OFF)`), không khởi tạo stack mạng IP/WiFi.
   - Sử dụng thư viện **NimBLE-Arduino** thay cho Bluedroid mặc định để tiết kiệm ~50% RAM heap và giảm dung lượng Flash.
2. **Phase 1: BLE Scanner Service**:
   - Quét các thiết bị BLE phát sóng xung quanh trong phạm vi phủ sóng.
   - Nhận diện thiết bị ngoại vi HID (Human Interface Device qua UUID `0x1812`).
   - Xuất thông tin ra Serial Monitor: Tên thiết bị (Name), Địa chỉ MAC (BLE Address), RSSI, cờ HID Service.
3. **Phase 2: BLE Mouse Client (Hard-code MAC & Đọc dữ liệu chuột)**:
   - Cấu hình địa chỉ MAC cố định của chuột đã quét được.
   - Thiết lập kết nối bảo mật (BLE Security / Bonding / Just Works) theo chuẩn HOGP (HID Over GATT Profile).
   - Đọc thông tin thiết bị (Device Information `0x180A`) và Mức pin (Battery Service `0x180F`).
   - Đăng ký nhận thông báo (Subscribe Notification) trên Characteristic HID Report (`0x2A4D`).
   - Giải mã (parse) gói tin HID theo thời gian thực: Trạng thái nút bấm (Trái, Phải, Giữa), tọa độ di chuyển ($\Delta X, \Delta Y$), con lăn cuộn (Wheel).
   - Cơ chế tự phục hồi kết nối (Auto-reconnect non-blocking) khi chuột sleep hoặc tắt nguồn.

---

## 2. Kiến Trúc & Cấu Trúc File Đề Xuất

Tuân thủ nghiêm ngặt các quy tắc trong `AGENTS.md`:

```
esp32-project/
├── platformio.ini              # Thêm thư viện NimBLE-Arduino, tối ưu build flags
├── include/
│   ├── log.h                   # Quản lý logging chuẩn (LOG_I, LOG_D, LOG_E, LOG_W)
│   ├── pin_config.h            # Khai báo LED, nút bấm
│   └── mouse_config.h          # [NEW] Cấu hình chế độ (Scan vs Connect) & MAC chuột
└── src/
    ├── services/
    │   ├── ble_scanner.h       # [NEW] Class BleScanner quản lý việc quét thiết bị
    │   ├── ble_scanner.cpp     # [NEW] Triển khai quét và lọc thiết bị HID
    │   ├── ble_mouse_client.h  # [NEW] Class BleMouseClient quản lý kết nối chuột & parse HID
    │   └── ble_mouse_client.cpp# [NEW] Triển khai kết nối, bảo mật, parse gói tin
    └── main.cpp                # Điều phối vòng đời ứng dụng (tắt WiFi, khởi chạy BLE)
```

---

## 3. Chi Tiết Kỹ Thuật

### 3.1. Cấu hình PlatformIO (`platformio.ini`)
- Thêm phụ thuộc thư viện:
  ```ini
  lib_deps = 
      h2zero/NimBLE-Arduino@^1.4.2
  ```

### 3.2. Cấu hình Tập trung (`include/mouse_config.h`)
- Cung cấp cờ chuyển đổi chế độ linh hoạt giữa **Phase 1** (Quét tìm MAC) và **Phase 2** (Kết nối chuột):
  ```cpp
  #pragma once

  // Chế độ hoạt động:
  // 1 = Chạy Phase 1 (BLE Scanner) để tìm MAC của chuột
  // 2 = Chạy Phase 2 (BLE Mouse Client) để kết nối và nhận dữ liệu chuột
  #define BLE_APP_MODE 1

  // Địa chỉ MAC chuột (điền vào sau khi hoàn thành Phase 1)
  #define TARGET_MOUSE_MAC "xx:xx:xx:xx:xx:xx"

  // Chu kỳ quét Phase 1 (giây)
  #define BLE_SCAN_DURATION_SEC 8

  // Chu kỳ tự động thử kết nối lại Phase 2 khi rớt chuột (ms)
  #define BLE_RECONNECT_INTERVAL_MS 5000
  ```

### 3.3. Phase 1: `BleScanner` (`src/services/ble_scanner.h` & `.cpp`)
- **Khởi tạo**: `NimBLEDevice::init("")`, tạo `NimBLEScan*`.
- **Cấu hình Scan**:
  - Chế độ Active Scan (`setActiveScan(true)`).
  - Tần số quét cao (`setInterval(100)`, `setWindow(99)`).
  - Đăng ký `NimBLEAdvertisedDeviceCallbacks` để lọc và hiển thị ngay khi phát hiện thiết bị.
- **Nhận diện thiết bị**:
  - Kiểm tra xem Service UUID có chứa `0x1812` (HID Service) hay không để đánh dấu `[HID DEVICE]`.
  - Log ra Serial bằng `LOG_I` với format rõ ràng, dễ nhìn.
- **Giải phóng**: Tự động dọn dẹp bộ nhớ kết quả quét (`clearResults()`).

### 3.4. Phase 2: `BleMouseClient` (`src/services/ble_mouse_client.h` & `.cpp`)
- **Bảo mật BLE (Security & Bonding)**:
  ```cpp
  NimBLEDevice::setSecurityAuth(true, true, true); // Bonding, MITM, Secure Connections
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  ```
- **Kết nối (Client Lifecycle)**:
  - Tạo `NimBLEClient*`, thiết lập `NimBLEClientCallbacks` (`onConnect`, `onDisconnect`).
  - Khi `onDisconnect`: cập nhật cờ trạng thái `_isConnected = false` và kích hoạt hẹn giờ non-blocking để reconnect.
- **Khám phá Dịch vụ & Thuộc tính (GATT Discovery)**:
  - Lấy thông tin nhà sản xuất / Model từ `0x180A` (Device Information).
  - Đọc % dung lượng pin từ `0x180F` (Battery Service -> `0x2A19`).
  - Lấy Service `0x1812` (HID Service) và tìm Characteristic `0x2A4D` (HID Report).
- **Đăng ký Notify & Giải mã HID Report**:
  - Gọi `pReportChar->subscribe(true, onNotifyCallback)`.
  - Hàm `onNotifyCallback` giải mã buffer theo chuẩn HID Mouse 3/4/5 bytes:
    - Byte 0: `Button Left (bit 0)`, `Button Right (bit 1)`, `Button Middle (bit 2)`.
    - Byte 1: $\Delta X$ (ép kiểu `int8_t`).
    - Byte 2: $\Delta Y$ (ép kiểu `int8_t`).
    - Byte 3: `Scroll Wheel` (ép kiểu `int8_t`).
  - Ghi log qua `LOG_I`:
    `[BLEMouse] Buttons: [L:0 R:0 M:0] | dX: +5 | dY: -3 | Wheel: 0 | Pin: 90%`

### 3.5. Luồng Chính (`src/main.cpp`)
- Vô hiệu hóa WiFi:
  ```cpp
  WiFi.mode(WIFI_OFF);
  btStop(); // Tắt radio classic nếu còn sót
  ```
- Tùy vào `BLE_APP_MODE` trong `include/mouse_config.h`:
  - Nếu `BLE_APP_MODE == 1`: Khởi động `BleScanner::start()`.
  - Nếu `BLE_APP_MODE == 2`: Khởi động `BleMouseClient::begin()`, trong `loop()` gọi `BleMouseClient::update()` định kỳ non-blocking.
- Vẫn duy trì nhấp nháy LED built-in báo hiệu hệ thống sống và `vTaskDelay(pdMS_TO_TICKS(10))` chống watchdog timer.

---

## 4. Kế Hoạch Thực Hiện Từng Bước (Implementation Steps)

| Bước | Nội dung công việc | Output / Kết quả |
|:---|:---|:---|
| **Bước 1** | Cập nhật `platformio.ini` thêm thư viện `h2zero/NimBLE-Arduino@^1.4.2`. | Thư viện được tải và sẵn sàng biên dịch |
| **Bước 2** | Tạo file `include/mouse_config.h` để quản lý chế độ và MAC chuột. | File cấu hình tập trung |
| **Bước 3** | Viết module `BleScanner` (`ble_scanner.h`, `ble_scanner.cpp`). | Module quét BLE độc lập chuẩn RAII |
| **Bước 4** | Cập nhật `src/main.cpp`: Tắt WiFi, chạy Phase 1 Scan BLE. Biên dịch kiểm tra với `pio run`. | Code build thành công, người dùng nạp chạy Phase 1 |
| **Bước 5** | [User Action] Người dùng bật chuột ở chế độ Pairing, nạp firmware và xem log để lấy MAC chuột. | Xác định chính xác địa chỉ MAC của chuột |
| **Bước 6** | Viết module `BleMouseClient` (`ble_mouse_client.h`, `ble_mouse_client.cpp`). | Module kết nối chuột, xử lý bảo mật & parse HID |
| **Bước 7** | Cập nhật MAC chuột vào `mouse_config.h`, chuyển `BLE_APP_MODE = 2`. Cập nhật `src/main.cpp`. Biên dịch kiểm tra với `pio run`. | Code build thành công cho Phase 2 |
| **Bước 8** | [User Action] Người dùng nạp firmware Phase 2, rê chuột, click chuột và kiểm tra log phản hồi. | Chuột hoạt động mượt mà, log hiển thị real-time |

---

## 5. Kế Hoạch Kiểm Tra (Verification Plan)

### Kiểm tra Tự động (Compile / Static Analysis)
- Chạy lệnh `pio run` sau mỗi bước chỉnh sửa code để đảm bảo không có lỗi cú pháp, không cảnh báo ngầm và tương thích thư viện.

### Kiểm tra Thực tế trên Phần Cứng (Manual Testing của Người dùng)
- **Phase 1**:
  - Người dùng mở Serial Monitor bằng `./monitor.sh` hoặc `pio device monitor`.
  - Quan sát danh sách thiết bị quét được, tìm thiết bị có tên chuột (ví dụ: `Logitech Pebble`, `BT Mouse`...) hoặc có gắn cờ `[HID DEVICE]`.
  - Copy địa chỉ MAC (ví dụ: `eb:34:21:aa:bb:cc`).
- **Phase 2**:
  - Sau khi nạp Phase 2, chuột sẽ kết nối tự động với ESP32 (đèn trên chuột chuyển từ nhấp nháy sang sáng đứng hoặc tắt báo đã paired).
  - Rê chuột trên bàn -> Log hiển thị giá trị `dX`, `dY` thay đổi tương ứng.
  - Nhấp chuột trái, phải, con lăn -> Log hiển thị `L:1`, `R:1`, `M:1`.
  - Cuộn bánh xe chuột -> Log hiển thị giá trị `Wheel`.
  - Tắt công tắc chuột và bật lại -> ESP32 tự động reconnect mà không cần bấm nút reset ESP32.
