# 📄 BẢNG YÊU CẦU DỰ ÁN (PROJECT REQUIREMENTS SPECIFICATION)

## 📌 TÊN DỰ ÁN: ESP32-S3 SMART BLE HID CONTROLLER (AIR TRACKPAD & MACRO DECK)

---

## 1. 🎯 Tổng Quan Dự Án (Project Overview)
Biến bo mạch **ESP32-S3 tích hợp màn hình cảm ứng điện dung 3.5 inch** thành thiết bị ngoại vi không dây cao cấp kết nối với Máy tính / Laptop / Tablet qua chuẩn **Bluetooth Low Energy (BLE) Human Interface Device (HID)**.

Thiết bị đóng vai trò là một **HID Composite Device** (vừa là Chuột, vừa là Bàn phím, vừa là Bộ điều khiển đa phương tiện - Consumer Control) mà không cần cài đặt driver trên máy tính chủ (Driverless, tương thích Windows, macOS, Linux, Android, iOS).

---

## 2. ⚙️ Phần Cứng & Môi Trường Hoạt Động (Hardware Specifications)

| Thành phần | Thông số kỹ thuật | Ghi chú tích hợp |
| :--- | :--- | :--- |
| **Vi điều khiển (MCU)** | ESP32-S3 Dual-Core Xtensa LX7 @ 240MHz | 16MB Flash (Quad/Octal), 8MB Octal PSRAM |
| **Màn hình hiển thị** | 3.5" IPS LCD độ phân giải **320x480** (Landscape 480x320) | Giao tiếp QSPI tốc độ cao (AXS15231B / ST7796) |
| **Cảm ứng (Touch)** | Cảm ứng điện dung (Capacitive Touch) | Giao tiếp I2C, nhận diện đa điểm (Multi-touch) |
| **Bluetooth** | BLE 5.0 (Bluetooth Low Energy) | Công suất phát tối đa, độ trễ thấp (< 15ms) |
| **Âm thanh (Audio)** | Loa onboard qua IC khuếch đại NS4168 (I2S) | Phát âm thanh phản hồi xúc giác (Audio Haptic Click) |
| **Lưu trữ** | Flash NVS & Thẻ nhớ MicroSD (SD_MMC / SPI) | Lưu cài đặt người dùng, macro tuỳ biến |
| **Định hướng màn hình**| **Ngang (Landscape 480 x 320 px)** | Tối ưu không gian cho bàn di chuột và cụm phím |

---

## 3. 🧩 Kiến Trúc Phần Mềm & Phân Luồng (Architecture & Concurrency)

Tuân thủ nghiêm ngặt các quy chuẩn kỹ thuật trong `GEMINI.md`:
- **Dual-Core FreeRTOS Partitioning**:
  - **Core 0**: Chạy BLE HID Stack (NimBLE / ESP32 BLE HID) đảm bảo tốc độ gửi báo cáo HID ổn định, không giật lag.
  - **Core 1**: Chạy UI Framework (LVGL / GFX PSRAM Canvas), đọc dữ liệu cảm ứng I2C (sample rate ~40–50Hz), xử lý cử chỉ và quản lý trạng thái.
- **Non-blocking Execution**: Không sử dụng `delay()` trong luồng chính; FreeRTOS task có `vTaskDelay()`.
- **Cấu hình GPIO tập trung**: Mọi pinout định nghĩa tại `include/pin_config.h`.
- **Flash Endurance**: Cài đặt (âm lượng, độ nhạy chuột, chế độ Win/Mac) lưu vào NVS có kiểm tra Dirty Check và Debounce (trì hoãn 500ms - 1s).

---

## 4. 📱 Đặc Tả Chi Tiết Giao Diện & Tính Năng (Detailed Features)

### 4.1. Màn Hình Khởi Động & Ghép Nối (Pairing & Connection Screen)
- **Giao diện khi chưa kết nối (Advertising State)**:
  - Hiển thị logo Bluetooth với hiệu ứng nhịp thở (Pulsing Animation).
  - Tên thiết bị BLE quảng bá rõ ràng (ví dụ: `ESP32-S3 Smart TouchPad`).
  - Hướng dẫn nhanh người dùng: *"Vào Cài đặt Bluetooth trên máy tính để ghép nối"*.
  - Trạng thái: `Đang chờ kết nối... (Advertising)`.
- **Giao diện khi ghép nối thành công (Connected State)**:
  - Biểu tượng Bluetooth chuyển sang màu xanh dương sáng kèm thông báo *"Đã kết nối thành công!"*.
  - Hiển thị tên máy tính/thiết bị đã kết nối (nếu lấy được qua BLE GATT).
  - Tự động chuyển mượt mà vào **Màn hình chính (Main UI)** sau 1.5 giây hoặc cho phép bấm trực tiếp để vào ngay.
- **Cơ chế mất kết nối (Auto Reconnect & Fallback)**:
  - Khi máy tính tắt hoặc mất sóng, lập tức quay lại màn hình chờ kết nối, không crash và sẵn sàng nhận kết nối lại tự động.

---

### 4.2. Màn Hình Chính & Thanh Điều Hướng (Main Dashboard & Navigation)
- **Thanh trạng thái trên cùng (Top Status Bar - Cao ~32px)**:
  - Biểu tượng BLE: Hiển thị trạng thái kết nối và cường độ sóng.
  - Chế độ hệ điều hành: `[ WIN ]` hoặc `[ MAC ]` (chạm để chuyển đổi tức thì).
  - Phản hồi âm thanh: Biểu tượng Loa (Bật/Tắt âm thanh click).
  - Thanh Tab chuyển đổi tính năng mượt mà (Swipe hoặc Tap):
    1. 🖱️ **TouchPad** (Chuột & Cử chỉ)
    2. ⌨️ **Dev Shortcuts** (Phím tắt lập trình)
    3. 🔢 **Numpad** (Bàn phím số)
    4. 🎵 **Media Control** (Đa phương tiện & Hệ thống)
    5. ⚙️ **Settings** (Cài đặt & Giám sát)

---

### 4.3. Chi Tiết Các Tab Tính Năng (Feature Tabs)

#### 🔹 TAB 1: MOUSE & TRACKPAD (Bàn di chuột thông minh)
- **Khu vực TouchPad trung tâm (Trackpad Canvas)**:
  - Vùng cảm ứng rộng lớn chiếm 75% diện tích màn hình.
  - Chuyển động vi sai (Delta X, Delta Y) truyền đến máy tính với thuật toán làm mượt (Smoothing & Ballistics/Gia tốc con trỏ) giúp di chuột chuẩn xác.
  - Hỗ trợ cử chỉ cảm ứng thông minh (Gestures):
    - **Chạm 1 ngón (Single Tap)**: Chuột trái (Left Click).
    - **Chạm 2 ngón (Two-Finger Tap)**: Chuột phải (Right Click).
    - **Giữ và kéo (Tap & Hold Drag)**: Kéo bôi đen văn bản / kéo cửa sổ.
    - **Vuốt mép phải (Edge Scroll)**: Cuộn trang lên/xuống (Scroll Wheel).
- **Cụm nút bấm phía dưới (Bottom Physical-like Buttons)**:
  - Nút **Left Click** (Chuột trái): Diện tích lớn bên trái.
  - Nút **Right Click** (Chuột phải): Bên phải.
  - Có hiệu ứng đổi màu khi nhấn và phát âm thanh bíp cơ học (haptic audio click).

#### 🔹 TAB 2: DEV SHORTCUTS (Cụm phím tắt Lập trình viên & Văn phòng)
Bố trí lưới nút bấm (Grid Layout 3x3 hoặc 4x2) với các macro thường dùng:
- **Select All** (`Ctrl + A` trên Win / `Cmd + A` trên Mac)
- **Copy** (`Ctrl + C` / `Cmd + C`)
- **Paste** (`Ctrl + V` / `Cmd + V`)
- **Cut** (`Ctrl + X` / `Cmd + X`)
- **Undo** (`Ctrl + Z` / `Cmd + Z`)
- **Redo** (`Ctrl + Y` hoặc `Ctrl + Shift + Z`)
- **Save All** (`Ctrl + S` / `Cmd + S`)
- **Home / End**: Nhảy về đầu dòng / cuối dòng.
- **Find / Replace** (`Ctrl + F` / `Ctrl + H`).
> *Tự động hoán đổi giữa mã phím `Ctrl` và `Cmd (GUI)` dựa theo chế độ hệ điều hành đang chọn trên Status Bar.*

#### 🔹 TAB 3: NUMPAD (Bàn phím số & Tính toán)
Thiết kế theo chuẩn Numpad bàn phím Full-size:
- **Hàng số**: `0`, `1`, `2`, `3`, `4`, `5`, `6`, `7`, `8`, `9`
- **Dấu chấm thập phân**: `.`
- **Phép toán cơ bản**: `+`, `-`, `*`, `/`, `%`, `=`
- **Phím chức năng**:
  - `Backspace` (Xoá ký tự trước)
  - `Delete` (Xoá ký tự sau)
  - `Enter` (Xác nhận / xuống dòng)
  - `Clear / Esc` (Xoá nhanh)
- Nút bấm to rõ, khoảng cách phím hợp lý, chống bấm nhầm.

#### 🔹 TAB 4: MEDIA CONTROL (Điều khiển Đa phương tiện & Hệ thống)
Gửi các mã HID chuẩn **Consumer Control**:
- **Trình phát nhạc/video**:
  - `Play / Pause` (Phát / Tạm dừng)
  - `Next Track` (Bài kế tiếp)
  - `Previous Track` (Bài trước đó)
  - `Stop` (Dừng)
- **Điều chỉnh âm thanh hệ thống**:
  - `Volume Up` (Tăng âm lượng)
  - `Volume Down` (Giảm âm lượng)
  - `Mute` (Tắt tiếng nhanh)
- **Tiện ích hệ thống máy tính**:
  - `Lock Screen` (`Win + L` hoặc `Cmd + Ctrl + Q`)
  - `Task Manager / Mission Control`
  - Chụp ảnh màn hình (`Print Screen` / `Cmd + Shift + 4`)

#### 🔹 TAB 5: CÀI ĐẶT & HỆ THỐNG (SETTINGS & SYSTEM DASHBOARD)
- **Tùy chọn Ngôn ngữ (Language Switcher)**:
  - Hỗ trợ chuyển đổi tức thì: **[ Tiếng Việt 🇻🇳 ]** hoặc **[ English 🇬🇧 ]**.
  - Toàn bộ nhãn, thông số, trạng thái và menu được bản địa hoá Tiếng Việt chuẩn xác.
- **Cấu hình Trải nghiệm Điều khiển**:
  - Thanh trượt chỉnh **Độ nhạy con trỏ chuột** (*Độ nhạy: 1.0x - 3.0x*).
  - Tùy chọn **Đảo chiều cuộn trang** (*Cuộn tự nhiên / Tiêu chuẩn*).
  - Chọn **Hệ điều hành mặc định** (*Windows* / *macOS*).
- **Cấu hình Hiển thị & Âm thanh**:
  - Thanh trượt chỉnh **Độ sáng màn hình** (*Độ sáng: 10% - 100% qua PWM*).
  - Công tắc bật/tắt **Âm thanh phản hồi khi bấm** (*Bật / Tắt tiếng click cơ học*).
  - Thời gian **Chờ tắt màn hình** (*30 giây / 1 phút / 3 phút / Luôn sáng*).
- **Giám sát Phần cứng (Hardware Diagnostics)**:
  - Tên thiết bị Bluetooth và địa chỉ MAC.
  - Tình trạng bộ nhớ: Dung lượng RAM & PSRAM còn trống theo thời gian thực.
  - Tình trạng thẻ nhớ MicroSD (nếu có cắm thẻ).
  - Phiên bản Firmware & nút bấm *"Khôi phục cài đặt gốc (Reset Factory)"*.

---

## 5. ⚡ Yêu Cầu Phi Chức Năng (Non-Functional Requirements)

1. **Hỗ Trợ Tiếng Việt Toàn Diện (Kế thừa bộ Font Montserrat từ nhánh `esp32-kit3.5`)**:
   - Tái sử dụng trực tiếp hệ thống font **Montserrat Tiếng Việt (LVGL 4-bpp Antialiased)** đã được tối ưu từ nhánh `esp32-kit3.5` (`src/ui/dashboard/fonts/`):
     - `vi_font_montserrat_12`: Chú thích nhỏ, nhãn phụ, thông số kỹ thuật.
     - `vi_font_montserrat_14`: Nhãn nút bấm, danh sách menu cài đặt, nội dung thông báo.
     - `vi_font_montserrat_16`: Tiêu đề các mục cài đặt, tiêu đề nhóm chức năng.
     - `vi_font_montserrat_20` & `vi_font_montserrat_24`: Tiêu đề lớn, chữ số Numpad, nhãn trạng thái chính.
   - Dải Unicode tích hợp đầy đủ: Ký tự ASCII (`0x20-0x7F`), Latin-1 (`0x80-0xFF`), Latin Extended-A/B (`0x100-0x1BF`), **Vietnamese Extended Latin (`0x1EA0-0x1EF9`)** và ký tự tiền tệ (`0x20A0-0x20CF`).
   - Đảm bảo hiển thị hoàn hảo 100% tiếng Việt có dấu, không lỗi ô vuông `?`, nét chữ bo tròn mịn màng đồng bộ nhận diện với hệ sinh thái dự án `esp32-kit3.5`.
   - Lưu lựa chọn ngôn ngữ vào NVS Flash để tự động khôi phục khi bật lại máy.
2. **Hiệu năng & Độ trễ (Performance & Latency)**:
   - Tần số phản hồi cảm ứng & gửi gói tin BLE đạt từ **40Hz - 60Hz**.
   - Độ trễ thao tác chuột < 15ms, không có cảm giác trễ (lag) khi di chuột.
   - Giao diện UI giữ tốc độ render ổn định >= 30 FPS.
3. **Tiết kiệm năng lượng & Chống cháy màn hình (Power & Screen Saver)**:
   - Sau 60 giây không chạm: Giảm độ sáng màn hình xuống 20%.
   - Sau thời gian cài đặt không hoạt động: Tắt màn hình (Standby Mode). Chạm vào màn hình để đánh thức lập tức mà không làm mất kết nối BLE.
4. **Phản hồi xúc giác qua âm thanh (Audio Feedback)**:
   - Sử dụng IC âm thanh I2S NS4168 phát âm click ngắn tần số cao (ví dụ: xung 2.5kHz trong 15ms) khi nhấn nút, mang lại cảm giác phản hồi cơ học chân thực.
5. **Tính ổn định & Chịu lỗi (Fault Tolerance)**:
   - Tự động khôi phục kết nối BLE khi máy tính thức dậy từ Sleep.
   - Tuân thủ Rule 10 trong `GEMINI.md`: Không crash khi mất kết nối bất ngờ hoặc khi thiết bị nhận diện chưa hoàn tất.

---

## 6. 📅 Lộ Trình Phát Triển Đề Xuất (Development Roadmap)

- **Giai đoạn 1**: Cấu hình BLE HID Composite Service (Mouse + Keyboard + Consumer Control) và test truyền nhận lệnh chuẩn với PC qua Serial.
- **Giai đoạn 2**: Xây dựng UI Framework với LVGL v8 ở chế độ Landscape 480x320; tích hợp driver cảm ứng AXS15231B I2C và nhúng bộ font Montserrat Tiếng Việt (`vi_font_montserrat_xx`) từ nhánh `esp32-kit3.5`.
- **Giai đoạn 3**: Hoàn thiện Tab Mouse (thuật toán lọc nhiễu di chuột + nhận diện tap/gestures) và Tab Numpad/Dev Shortcuts.
- **Giai đoạn 4**: Xây dựng Tab Cài Đặt song ngữ (Tiếng Việt / English), tích hợp âm thanh click I2S, thanh trượt cài đặt độ nhạy chuột, lưu NVS và kiểm tra thực tế trên Windows/macOS.
