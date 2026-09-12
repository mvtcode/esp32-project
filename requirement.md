# 📄 BẢNG YÊU CẦU DỰ ÁN (PROJECT REQUIREMENTS SPECIFICATION)

## 📌 TÊN DỰ ÁN: ESP32-S3 SMART BLE HID CONTROLLER (AIR TRACKPAD & MACRO DECK)

---

## 1. 🎯 Tổng Quan Dự Án (Project Overview)

Biến bo mạch **ESP32-S3 tích hợp màn hình cảm ứng điện dung 3.5 inch** thành thiết bị ngoại vi không dây cao cấp kết nối với Máy tính / Laptop / Tablet qua chuẩn **Bluetooth Low Energy (BLE) Human Interface Device (HID)**.

Thiết bị đóng vai trò là một **HID Composite Device** (vừa là Chuột, vừa là Bàn phím, vừa là Bộ điều khiển đa phương tiện - Consumer Control) mà không cần cài đặt driver trên máy tính chủ (Driverless, tương thích Windows, macOS, Linux, Android, iOS).

---

## 2. ⚙️ Phần Cứng & Môi Trường Hoạt Động (Hardware Specifications)

| Thành phần              | Thông số kỹ thuật                          | Ghi chú tích hợp                                     |
| :---------------------- | :----------------------------------------- | :--------------------------------------------------- |
| **Vi điều khiển (MCU)** | ESP32-S3 Dual-Core Xtensa LX7 @ 240MHz     | 16MB Flash (Quad/Octal), 8MB Octal PSRAM             |
| **Màn hình hiển thị**   | 3.5" IPS LCD độ phân giải **320x480**      | Giao tiếp QSPI tốc độ cao (AXS15231B / ST7796)       |
| **Cảm ứng (Touch)**     | Cảm ứng điện dung (Capacitive Touch)       | Giao tiếp I2C, **chỉ 1 điểm chạm** (Single-touch only - giới hạn AXS15231B) |
| **Bluetooth**           | BLE 5.0 (Bluetooth Low Energy)             | Công suất phát tối đa, độ trễ thấp (< 15ms)          |
| **Âm thanh (Audio)**    | Loa onboard qua IC khuếch đại NS4168 (I2S) | Phát âm thanh phản hồi xúc giác (Audio Haptic Click) |
| **Lưu trữ**             | Flash NVS & Thẻ nhớ MicroSD (SD_MMC / SPI) | Lưu cài đặt người dùng, macro tuỳ biến               |
| **Định hướng màn hình** | **Dọc (Portrait 320 x 480 px)**            | Màn hình vật lý native Portrait; LVGL `hor_res=320, ver_res=480` |

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
  - Hướng dẫn nhanh người dùng: _"Vào Cài đặt Bluetooth trên máy tính để ghép nối"_.
  - Trạng thái: `Đang chờ kết nối... (Advertising)`.
- **Giao diện khi ghép nối thành công (Connected State)**:
  - Biểu tượng Bluetooth chuyển sang màu xanh dương sáng kèm thông báo _"Đã kết nối thành công!"_.
  - Hiển thị tên máy tính/thiết bị đã kết nối (nếu lấy được qua BLE GATT).
  - Tự động chuyển mượt mà vào **Màn hình chính (Main UI)** sau 1.5 giây hoặc cho phép bấm trực tiếp để vào ngay.
- **Cơ chế mất kết nối (Auto Reconnect & Fallback)**:
  - Khi máy tính tắt hoặc mất sóng, lập tức quay lại màn hình chờ kết nối, không crash và sẵn sàng nhận kết nối lại tự động.

---

### 4.2. Màn Hình Chính & Thanh Điều Hướng (Main Dashboard & Navigation)

- **Thanh Tab điều hướng (Tab Bar - Cao ~38px)** nằm trên cùng màn hình:
  - **Toàn bộ 5 tab chỉ hiển thị ICON** (không có text label), sử dụng LVGL built-in symbols hoặc icon font riêng:
    1. `LV_SYMBOL_HOME` hoặc icon trackpad tương đương → **TouchPad**
    2. `LV_SYMBOL_KEYBOARD` hoặc icon shortcuts → **Dev Shortcuts**
    3. `LV_SYMBOL_LIST` hoặc icon numpad → **Numpad**
    4. `LV_SYMBOL_AUDIO` hoặc icon nhạc → **Media Control**
    5. `LV_SYMBOL_SETTINGS` → **Settings**
  - Tap vào icon để chuyển tab; **tắt swipe gesture** (`lv_obj_clear_flag(_tabview, LV_OBJ_FLAG_GESTURE_BUBBLE)` và `lv_tabview` không dùng swipe) để tránh chuyển tab không mong muốn khi kéo trackpad hoặc kéo slider.
  - Tab active: icon highlight màu `COLOR_ACTIVE_BLUE`; inactive: icon màu `COLOR_TEXT_MUTED`.

---

### 4.3. Chi Tiết Các Tab Tính Năng (Feature Tabs)

#### 🔹 TAB 1: MOUSE & TRACKPAD (Bàn di chuột thông minh)

- **Khu vực TouchPad trung tâm (Trackpad Canvas)**:
  - Vùng cảm ứng rộng lớn chiếm ≈ 75% diện tích màn hình (phần còn lại là cụm nút bấm dưới).
  - Chuyển động vi sai (Delta X, Delta Y) truyền đến máy tính với thuật toán làm mượt (Smoothing & Ballistics) giúp di chuột chính xác.
  - **Giới hạn phần cứng**: Chip cảm ứng **AXS15231B chỉ hỗ trợ 1 điểm chạm** tại một thời điểm (single-touch), không có multi-touch. Mọi gesture đều phải được thiết kế cho **1 ngón tay**.
  - Hỗ trợ cử chỉ single-touch (Gestures):
    - **Chạm ngắn 1 ngón (Single Tap < 200ms)**: Chuột trái (Left Click).
    - **Giữ lâu 1 ngón (Long Press ≥ 500ms, không di chuyển)**: Chuột phải (Right Click) — *thay thế Two-Finger Tap do không hỗ trợ multi-touch*.
    - **Chạm-Giữ-Kéo (Tap & Hold Drag)**: Chạm nhanh rồi giữ lâu và kéo — kéo bôi đen văn bản / kéo cửa sổ (drag mode).
    - **Scroll**: Dùng cụm nút **Scroll Up / Scroll Down** riêng trên Tab Trackpad thay cho Edge Scroll 2 ngón.
- **Cụm nút bấm phía dưới (Bottom Buttons)**:
  - Nút **Left Click** (Chuột trái - hiển thị bằng icon).
  - Nút **Right Click** (Chuột phải - hiển thị bằng icon).
  - Nút **Scroll ↑** và **Scroll ↓** (cuộn trang - hiển thị bằng icon mũi tên).
  - Có hiệu ứng đổi màu khi nhấn và phát âm thanh click I2S (haptic audio click).

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
  > _Tự động hoán đổi giữa mã phím `Ctrl` và `Cmd (GUI)` dựa theo chế độ hệ điều hành đang chọn trên Status Bar._

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

#### 🔹 TAB 5: SETTINGS (Cài đặt & Hệ thống)

> **Ngôn ngữ hiển thị**: Tab Settings sử dụng **Tiếng Việt hoàn toàn** với font Montserrat hỗ trợ dấu tiếng Việt. Các tab còn lại (1–4) dùng icon không có text.

- **Cấu hình Trải nghiệm Điều khiển**:
  - Thanh trượt `lv_slider` chỉnh **Độ nhạy con trỏ chuột** (_Phạm vi: 1.0x – 3.0x, bước 0.1_); giá trị lưu vào NVS sau Debounce 500ms.
    - ⚠️ **Chống Swipe không mong muốn**: Slider phải `lv_obj_add_flag(slider, LV_OBJ_FLAG_CLICK_FOCUSABLE)` và container tab phải tắt gesture bubble để kéo slider không kích hoạt chuyển tab.
  - Toggle switch **Đảo chiều cuộn trang** (_Cuộn tự nhiên / Tiêu chuẩn_).
  - Toggle switch **Hệ điều hành** (_Windows `Ctrl` / macOS `Cmd`_); lưu NVS.
- **Cấu hình Hiển thị & Âm thanh**:
  - Thanh trượt `lv_slider` chỉnh **Độ sáng màn hình** (_10% – 100% qua `analogWrite(PIN_TFT_BL, val)`_); lưu NVS Debounce 500ms.
    - ⚠️ **Chống Swipe không mong muốn**: Áp dụng tương tự slider độ nhạy.
  - Toggle switch **Âm thanh phản hồi khi bấm** (_Bật / Tắt tiếng click I2S_); lưu NVS.
  - Dropdown `lv_dropdown` **Thời gian chờ tắt màn hình** (_30 giây / 1 phút / 3 phút / Luôn sáng_); lưu NVS.
- **Giám sát Phần cứng (Hardware Diagnostics)**:
  - Tên thiết bị Bluetooth và địa chỉ MAC thực (lấy từ `NimBLEDevice::getAddress()`).
  - Dung lượng RAM & PSRAM còn trống theo thời gian thực (cập nhật mỗi 2 giây).
  - Tình trạng thẻ nhớ MicroSD (có/không có thẻ + dung lượng nếu có).
  - Phiên bản Firmware & nút bấm **"Khôi phục cài đặt gốc"** (xoá NVS namespace và reboot).

---

## 4.4. ⚠️ Vấn Đề Kỹ Thuật Cần Xử Lý (Known Issues & Constraints)

### Bug: Swipe/Drag Slider Không Mong Muốn

- **Mô tả**: Khi người dùng kéo ngón tay trên **Trackpad** hoặc kéo **slider** trong Tab Settings, LVGL truyền sự kiện gesture lên `lv_tabview` gây chuyển tab ngoài ý muốn; đồng thời kéo trên trackpad có thể vô tình tương tác với slider.
- **Nguyên nhân**: `lv_tabview` mặc định cho phép swipe gesture để chuyển tab; sự kiện không được consume tại lớp con mà bubble lên cha.
- **Yêu cầu Fix**:
  1. **Tắt hoàn toàn swipe gesture trên tabview**: Sau khi tạo `_tabview`, gọi:
     ```cpp
     lv_obj_clear_flag(lv_tabview_get_content(_tabview), LV_OBJ_FLAG_SCROLLABLE);
     ```
     và trên từng tab page:
     ```cpp
     lv_obj_clear_flag(_tabTouchpad, LV_OBJ_FLAG_SCROLLABLE);
     // tương tự cho các tab khác
     ```
  2. **Isolate slider events**: Slider trong Tab Settings phải `consume` event và không cho bubble:
     ```cpp
     lv_obj_add_event_cb(slider, onSliderEvent, LV_EVENT_PRESSING, this);
     // Trong callback: lv_event_stop_bubbling(e);
     ```
  3. **Chuyển tab chỉ bằng Tap**: Chỉ dùng `lv_tabview_set_act()` trong callback `LV_EVENT_CLICKED` của từng button tab bar, không dùng swipe.

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

- **Giai đoạn 1**: Cấu hình BLE HID Composite Service (Mouse + Keyboard + Consumer Control) và test truyền nhận lệnh chuẩn với PC qua Serial. ✅ _Hoàn thành_
- **Giai đoạn 2**: Xây dựng UI Framework với LVGL v8 ở chế độ **Portrait 320×480**; tích hợp driver cảm ứng AXS15231B I2C và nhúng bộ font Montserrat Tiếng Việt (`vi_font_montserrat_xx`) từ nhánh `esp32-kit3.5`. ✅ _Hoàn thành cơ bản_
- **Giai đoạn 3** _(Đang thực hiện)_:
  - Fix bug swipe/drag không mong muốn trên tabview và slider.
  - Thay thế text label trên Tab Bar bằng **Icon** (LVGL symbol hoặc custom icon font).
  - Hoàn thiện Tab TouchPad: thuật toán lọc nhiễu (Smoothing + Ballistics), nhận diện 2-finger tap, Tap & Hold drag, Edge scroll.
  - Bổ sung phím còn thiếu: Tab Numpad (Delete, %, Esc/Clear), Tab Shortcuts (Home/End), Tab Media (Lock Screen, Screenshot, Task Manager).
- **Giai đoạn 4**: Xây dựng Tab Settings Tiếng Việt hoàn chỉnh, tích hợp âm thanh click I2S từ UI, slider độ nhạy/độ sáng chống swipe, NVS persistence, Screen Saver, kiểm tra thực tế trên Windows/macOS.
