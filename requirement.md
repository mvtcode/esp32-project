# TÀI LIỆU YÊU CẦU DỰ ÁN (SPECIFICATION & REQUIREMENTS)

**Dự án:** Đồng hồ thông minh & Dashboard đa năng ESP32 (CYD 3.5" - ST7796 / XPT2046)

---

## 1. TỔNG QUAN VÀ MỤC TIÊU

Nâng cấp và chuyển đổi giao diện UI từ dạng hiển thị dữ liệu giả lập (Mock UI) sang hệ thống hoạt động thực tế (Production/Real-time system), tích hợp đầy đủ kết nối mạng, dịch vụ đồng bộ trực tuyến, hiển thị lịch âm dương và phát nhạc MP3 từ thẻ nhớ SD.

---

## 2. RÀNG BUỘC PHẦN CỨNG & KIẾN TRÚC HỆ THỐNG

### 2.1. Cấu hình phần cứng (Board CYD 3.5" - ESP32-3248S035)

- **MCU:** ESP32-WROOM-32 (2 Cores, 240MHz, ~320KB SRAM).
- **Màn hình:** 3.5 inch TFT LCD (ST7796 SPI, 480x320 px, SPI Bus HSPI: MOSI=13, MISO=12, SCLK=14, CS=15, DC=2, BL=27).
- **Cảm ứng:** Resistive Touch XPT2046 (CS=33, dùng chung SPI bus với LCD).
- **Thẻ nhớ MicroSD:** Giao tiếp SPI riêng biệt (VSPI: CS=5, MOSI=23, MISO=19, SCLK=18).
- **Âm thanh (Audio):**
  - _Tùy chọn Mặc định (Onboard):_ DAC nội bộ ESP32 chân GPIO 26 qua amp mini tích hợp.
  - _Tùy chọn Mở rộng:_ Hỗ trợ module I2S ngoài (như MAX98357A qua chân mở rộng nếu người dùng gắn thêm).
- **Lưu trữ cấu hình:** ESP32 NVS Flash (thư viện `Preferences`).

### 2.2. Kiến trúc Đa nhiệm & Quản lý Bộ nhớ (FreeRTOS)

- **Core 1:** Chuyên xử lý LVGL render (`lv_timer_handler()`) và đọc cảm ứng Touchpad.
- **Core 0:** Xử lý các tác vụ nền (FreeRTOS Background Tasks):
  - `WifiTask` & `NetworkTask`: Quét mạng, xử lý HTTP request (NTP, Giá vàng, Giá xăng, Thời tiết).
  - `AudioTask`: Xử lý giải mã âm thanh MP3 từ thẻ nhớ SD (đảm bảo không bị ngắt quãng âm thanh khi vuốt chạm màn hình).

---

## 3. CÁC GIAI ĐOẠN TRIỂN KHAI (PHASES)

---

### PHASE 1: QUẢN LÝ KẾT NỐI WIFI & CẤU HÌNH HỆ THỐNG (WIFI & SYSTEM SETTINGS)

**Mục tiêu:** Xây dựng trung tâm cài đặt hoàn chỉnh trên màn hình cảm ứng, cho phép cấu hình WiFi, chọn Tỉnh/TP, tần suất cập nhật dữ liệu, độ sáng màn hình, chế độ ngủ và lưu trữ toàn bộ vào NVS Flash.

1. **Quản lý mạng WiFi (WiFi Manager):**
   - **Quét WiFi (Scan):** Nút quét danh sách các mạng WiFi khả dụng kèm cột sóng (RSSI) và icon bảo mật (Khóa/Mở). Quét chạy bất đồng bộ (Non-blocking), không làm gián đoạn UI.
   - **Bàn phím ảo (Virtual Keyboard):** Mở popup bàn phím ảo (`lv_keyboard`) khi chọn SSID có mật khẩu; hỗ trợ gõ ký tự chữ, số, ký tự đặc biệt, phím xóa và nút ẩn/hiện mật khẩu.
   - **Tự động kết nối lại (Auto-reconnect):** Tự động lưu SSID & Password vào Flash (`Preferences`) và tự động kết nối lại mỗi khi khởi động.
   - **Trạng thái kết nối:** Cập nhật icon WiFi động trên thanh Top Bar (Disconnected / Connecting / Connected + cấp độ sóng).

2. **Cấu hình Vị trí & Đồng bộ Dữ liệu (Location & Sync):**
   - **Chọn Tỉnh / Thành phố thời tiết:** Dropdown/Roller danh sách các tỉnh thành phố Việt Nam (Hà Nội, TP. Hồ Chí Minh, Đà Nẵng, Hải Phòng, Cần Thơ, Nha Trang, Huế, Đà Lạt, Vũng Tàu, Quy Nhơn, Quảng Ninh,...). Tự động cập nhật dữ liệu thời tiết của vị trí được chọn khi sang Phase 2.
   - **Tần suất cập nhật (Update Interval):** Tùy chọn chu kỳ tự động cập nhật dữ liệu (Thời tiết, Giá vàng, Giá xăng): `15 phút`, `30 phút`, `1 giờ`, `2 giờ`.
   - **Nút "Đồng bộ ngay" (Sync Now):** Cho phép người dùng bấm nút kích hoạt lấy dữ liệu mới tức thì mà không cần chờ hết chu kỳ.

3. **Cấu hình Màn hình & Nguồn điện (Display & Power):**
   - **Điều chỉnh Độ sáng (Screen Brightness):** Thanh trượt Slider (10% - 100%), điều khiển trực tiếp bằng phần cứng PWM (LEDC trên chân GPIO 27 - TFT_BL).
   - **Tự động tắt/giảm sáng màn hình (Screen Sleep / Timeout):** Tùy chọn thời gian tự động giảm sáng/tắt đèn nền khi không có thao tác cảm ứng (`30 giây`, `1 phút`, `3 phút`, `5 phút`, `Luôn sáng`). Chạm vào màn hình để đánh thức lại.
   - **Tự động chỉnh sáng theo cảm biến (Auto Brightness - Tùy chọn):** Bật/tắt chế độ tự động điều chỉnh độ sáng theo quang trở LDR (GPIO 34).

4. **Âm thanh & Tùy chọn Hệ thống (Audio & System):**
   - **Âm lượng mặc định (Default Volume):** Slider chỉnh mức âm lượng khởi động cho MP3 và hệ thống (0% - 100%).
   - **Âm thanh phản hồi khi chạm (Touch Click Beep):** Bật/tắt tiếng bíp ngắn khi chạm vào màn hình cảm ứng.

5. **Thông tin Thiết bị & Bảo trì (Device Info & Maintenance):**
   - Hiển thị thông số thực tế: Địa chỉ IP, MAC Address, dung lượng RAM khả dụng (Free Heap), bộ nhớ Flash, thời gian hoạt động (Uptime).
   - **Nút "Khởi động lại" (Restart ESP32)** và **Nút "Xóa cài đặt gốc" (Factory Reset / Clear WiFi)**.

6. **Chế độ Nhà phát triển & Giám sát Hiệu năng (Development / Performance HUD):**
   - **Công tắc Bật/Tắt (Toggle Switch):** Cho phép bật hoặc tắt "Development Mode" trong mục Cài đặt hệ thống.
   - **Khung thông số nổi (Developer HUD Overlay):** Khi bật, hiển thị một thanh/khung thông số hiệu năng nhỏ gọn trên tất cả các màn hình (Home, Calendar, Player, Settings):
     - **FPS (Frame Rate):** Tốc độ khung hình render thực tế của LVGL.
     - **RAM / Free Heap:** Dung lượng RAM còn trống (KB) và % bộ nhớ đã dùng.
     - **CPU Load:** Tải xử lý CPU (% CPU Usage).
     - **WiFi Telemetry:** Cường độ sóng chi tiết (dBm) & địa chỉ IP.

7. **Lưu trữ Cấu hình Toàn cục (NVS Flash Persistence):**
   - Tất cả các giá trị cấu hình (SSID, Password, Tỉnh/TP, Tần suất sync, Độ sáng, Sleep timeout, Âm lượng, Trạng thái Dev Mode) đều được lưu trữ an toàn trong ESP32 `Preferences` để không bị mất khi mất nguồn.

8. **Quản lý Thẻ Nhớ SD (SD Card Storage & Format Tool):**
   - Khởi tạo giao tiếp phần cứng với thẻ nhớ MicroSD trên bus SPI riêng biệt (VSPI: CS=5, MOSI=23, MISO=19, SCLK=18).
   - Kiểm tra trạng thái cắm thẻ nhớ trong mục Cài đặt.
   - Hiển thị trực quan: Loại thẻ (SDSC / SDHC), Tổng dung lượng (Total), Dung lượng đã sử dụng (Used) và Dung lượng còn trống (Free).
   - **Hỗ trợ Format Thẻ Nhớ (FAT32 Format):** Nút bấm định dạng nhanh thẻ nhớ kèm hộp thoại xác nhận an toàn.

---

### PHASE 2: DỊCH VỤ DỮ LIỆU THỰC & ĐỒNG BỘ MẠNG (ONLINE SERVICES & TELEMETRY)

1. **Thời gian thực & Lịch Âm Dương (NTP & Lunar Calendar):**
   - **NTP Time Sync:** Đồng bộ giờ chính xác qua NTP Server (`pool.ntp.org` / `time.google.com`), múi giờ GMT+7 (Asia/Ho_Chi_Minh).
   - **Lịch Âm Việt Nam:** Tích hợp thuật toán Hồ Ngọc Đức để tự động tính ngày, tháng, năm âm lịch, năm Can Chi (Giáp Thìn, Ất Tỵ,...).
   - **Ngày lễ & Sự kiện:** Xác định các ngày lễ trọng đại trong năm (Tết Nguyên Đán, Giỗ Tổ Hùng Vương, 30/4 - 1/5, Quốc khánh 2/9, Giáng Sinh,...).

2. **Dữ liệu Thị trường (Market Data Service):**
   - **API:** Thu thập dữ liệu từ endpoint VNExpress (`https://gw.vnexpress.net/th?types=gia_vang_v2,gia_xang_dau`).
   - **Giá vàng:** SJC (Mua/Bán), Vàng nhẫn 9999 (Mua/Bán).
   - **Giá xăng dầu:** Xăng RON 95-III, E5 RON 92-II, Dầu Diesel.

3. **Dịch vụ Thời tiết theo Tỉnh/TP đã chọn (Weather Service):**
   - **API:** Lấy dữ liệu thời tiết thực từ Open-Meteo API.
   - **Thông số hiển thị:** Tên Tỉnh/TP, nhiệt độ (°C), độ ẩm (%), tình trạng thời tiết (Trời nắng, Có mây, Mưa rào, Giông bão,...), tốc độ gió và chỉ số UV trên HomeScreen.

---

### PHASE 3: MÀN HÌNH LỊCH TOÀN DIỆN (FULL CALENDAR SCREEN)

1. **Hiển thị Ma trận Lịch tháng (Month Grid View):**
   - Hiển thị số ngày Dương lịch (lớn), Âm lịch (nhỏ), đánh dấu ngày Rằm/Mùng 1.
2. **Bộ điều khiển:** Chuyển tháng, chọn năm, nút quay về "Hôm nay".
3. **Chi tiết ngày:** Ngày Can Chi, sự kiện/lễ.

---

### PHASE 4: TRÌNH PHÁT NHẠC MP3 TỪ THẺ NHỚ SD (SD MP3 PLAYER)

**Mục tiêu:** Xây dựng trình phát nhạc đa phương tiện hoàn chỉnh với thư viện file trên thẻ nhớ MicroSD.

1. **Quản lý & Đọc thẻ nhớ (SD Card File Manager):**
   - Khởi tạo bus SPI riêng (VSPI) cho thẻ MicroSD không gây xung đột với màn hình TFT.
   - Tự động quét các file nhạc có định dạng `.mp3`, `.wav` từ bất kỳ folder nào.
   - Hiển thị danh sách phát (Playlist) có hỗ trợ cuộn cảm ứng và chọn bài hát để phát trực tiếp.
2. **Bộ điều khiển phát nhạc (Playback Controls):**
   - Các nút chức năng: **Phát / Tạm dừng (Play/Pause)**, **Bài tiếp (Next)**, **Bài trước (Prev)**.
   - Chế độ phát: Phát tuần tự (Loop all), Lặp lại 1 bài (Loop single), Phát ngẫu nhiên (Shuffle).
   - Thanh trượt tiến độ bài hát (Progress Bar / Slider) hiển thị thời gian hiện tại (`mm:ss` hoặc `hh:mm:ss` nếu quá 1 giờ) và tổng thời lượng.
3. **Điều khiển âm lượng (Volume Control):**
   - Slider hoặc nút tăng/giảm âm lượng (0% - 100%).
   - Lưu mức âm lượng vào `Preferences` để nhớ sau khi khởi động lại.
4. **Xử lý âm thanh ổn định (Smooth Audio Decoding):**
   - Sử dụng thư viện `ESP8266Audio` (AudioGeneratorMP3 + AudioFileSourceSD + AudioOutputI2S / DAC).
   - Chạy decode trong FreeRTOS task chuyên biệt để âm thanh không bị giật/khựng (no stuttering) khi thao tác vuốt màn hình.

---

### PHASE 5 (MỞ RỘNG - OPTIONAL / LOW PRIORITY): KẾT NỐI BLUETOOTH AUDIO RECEIVER

**Mục tiêu:** Tính năng giải trí mở rộng biến ESP32 thành bộ thu âm thanh không dây (Bluetooth Speaker Sink).

1. **Bluetooth A2DP Sink (Loa Bluetooth):**
   - Biến ESP32 thành Bluetooth Audio Receiver cho phép phát nhạc trực tiếp từ điện thoại / laptop qua loa mạch CYD 3.5.
   - Quản lý tắt mở Bluetooth linh hoạt để tránh xung đột băng thông Radio với WiFi.
2. **Giao tiếp BLE (Tùy chọn cấu hình):**
   - Tùy chọn cấu hình thiết bị qua BLE trong trường hợp cần thiết.

---

## 4. TIÊU CHÍ NGHIỆM THU & ĐÁNH GIÁ (ACCEPTANCE CRITERIA)

- [ ] **WiFi & Cài đặt:** Quét được danh sách SSID, bàn phím gõ mượt mà, kết nối thành công; chọn và đổi Tỉnh/TP dễ dàng, cấu hình độ sáng/ngủ/âm lượng lưu vào Flash và áp dụng tức thì.
- [ ] **Thẻ Nhớ SD:** Nhận diện đúng thẻ MicroSD, hiển thị dung lượng Total/Used/Free chính xác, hỗ trợ định dạng (Format) an toàn.
- [ ] **Development HUD:** Bật/tắt mượt mà chế độ Development Mode trong Cài đặt; hiển thị chính xác FPS, Free Heap RAM, CPU load và WiFi telemetry nổi xuyên suốt các màn hình.
- [ ] **Data & Thời tiết:** Đồng bộ giờ chính xác qua NTP, tính đúng lịch âm & can chi, hiển thị chuẩn giá vàng/xăng dầu VNExpress và thời tiết theo Tỉnh/TP đã chọn.
- [ ] **Calendar:** Chuyển tháng mượt mà, hiển thị chuẩn cả ngày âm/dương của bất kỳ tháng nào trong năm.
- [ ] **MP3 Player:** Đọc danh sách nhạc từ thẻ nhớ SD nhanh chóng, phát nhạc mượt mà không rè/giật, nút bấm điều khiển phản hồi tức thì.
- [ ] **Hệ thống:** Không xảy ra lỗi Memory Leak (tràn RAM), không bị Crash/Watchdog Reset khi chạy đa nhiệm.
