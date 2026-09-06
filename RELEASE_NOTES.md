# ESP32 CYD 3.5" Smart Dashboard & Audio Player - Release v1.0.0

🎉 **Bản phát hành chính thức đầu tiên (Production Release)** dành cho mạch **ESP32-3248S035 (CYD 3.5" TFT ST7796 + Cảm ứng điện trở XPT2046)**.

Phiên bản này nâng cấp toàn diện từ giao diện mẫu sang hệ thống thời gian thực hoàn chỉnh với đầy đủ các tính năng thông minh, giao diện mượt mà và tối ưu hóa phần cứng vượt trội.

---

## ✨ Điểm Nổi Bật & Tính Năng Chính

### 1. 🕒 Đồng Hồ NTP & Lịch Âm Việt Nam
- **Đồng bộ thời gian thực**: Kết nối NTP Server (`pool.ntp.org`), chuẩn múi giờ GMT+7, cập nhật từng giây không giật lag.
- **Lịch Âm chính xác**: Tích hợp thuật toán thiên văn Hồ Ngọc Đức tính toán chuẩn xác ngày, tháng, năm âm lịch.
- **Thông tin Vạn Niên**: Hiển thị Can Chi năm (Ất Tỵ, Giáp Thìn...), Can Chi ngày, Ngày Hoàng Đạo / Hắc Đạo và các ngày lễ truyền thống Việt Nam.

### 2. ⛅ Bảng Tin Thời Tiết & Tài Chính Trực Tuyến
- **Thời tiết thời gian thực (Open-Meteo API)**: Hỗ trợ 20 tỉnh/thành phố lớn tại Việt Nam, cập nhật nhiệt độ, độ ẩm, sức gió, chỉ số UV và biểu tượng thời tiết động.
- **Thị trường Tài chính (VNExpress API)**: Cập nhật biến động giá vàng SJC / Nhẫn 9999 (Mua/Bán) và giá xăng dầu (RON 95-III, E5 RON 92-II, Dầu Diesel).
- **Tiết kiệm tài nguyên**: Tác vụ mạng ngầm chỉ kích hoạt khi xem màn hình Home và tự ngắt khi rời tab để nhường 100% tài nguyên cho âm nhạc và đồ họa.

### 3. 🎵 Trình Phát Nhạc MP3 / WAV Cao Cấp (Thẻ Nhớ SD)
- **Tự động quét thẻ nhớ**: Hỗ trợ thư mục nhạc trên thẻ nhớ microSD (VSPI bus riêng biệt).
- **Bảo vệ toàn vẹn file (Magic Bytes)**: Kiểm tra cấu trúc byte đầu (ID3v2, Frame sync word `0xFFE0`, RIFF WAVE) loại trừ 100% rủi ro file hỏng hoặc file giả mạo đổi đuôi gây treo chip.
- **Xuất âm thanh chất lượng**: Tận dụng DAC nội bộ của ESP32 (GPIO 26) qua bộ đệm I2S DMA đa tầng, phát âm thanh mượt mà, không tiếng bụp khi chuyển bài.
- **Giao diện nghe nhạc sống động**: Sóng âm thanh Visualizer động (Spectrum Wave), thanh tiến trình seek track, chế độ Shuffle và Repeat.

### 4. ⚙️ Trung Tâm Cài Đặt & Quản Lý Thông Minh
- **Quản trị mạng WiFi**: Quét danh sách mạng xung quanh, nhập mật khẩu kết nối trực tiếp trên màn hình cảm ứng bàn phím QWERTY.
- **Tự phục hồi kết nối (Self-Healing)**: Tự động thử kết nối lại mạng không chặn (non-blocking) chu kỳ 15 giây khi bị rớt mạng.
- **Quản lý thẻ nhớ**: Xem thông số dung lượng, định dạng thẻ (FAT32) trực tiếp từ menu cài đặt.
- **Tùy chỉnh hệ thống**: Điều chỉnh độ sáng màn hình (PWM), âm lượng khởi động, thời gian chờ tắt màn hình tiết kiệm điện.

---

## 🛠️ Tối Ưu Hệ Thống & Tuân Thủ Quy Chuẩn GEMINI.md

- 🛡️ **Chống mòn bộ nhớ Flash NVS (Rule 7)**:
  - Tích hợp cơ chế **Debounce (600ms)** và **Dirty Check**: Khi kéo thanh trượt độ sáng hay âm lượng, hệ thống phản hồi tức thì về mặt thị giác nhưng hoãn ghi NVS cho đến khi người dùng dừng thao tác, bảo vệ tuổi thọ chip Flash.
- 📐 **Quản lý phần cứng tập trung (Rule 3 & Rule 11)**:
  - Toàn bộ định nghĩa chân GPIO được tập trung tại file `include/pin_config.h`.
  - Đồng bộ toàn bộ các file header sang chuẩn `#pragma once`.
- ⚡ **Tối ưu FPS & Zero Memory Leak (Rule 2 & Rule 4)**:
  - Render GUI trên LVGL 8.4.0 với Dark Theme cao cấp, tách biệt luồng xử lý trên 2 nhân FreeRTOS (Core 0 cho Audio & Network, Core 1 cho UI).
  - Tích hợp **Developer HUD** overlay trực tiếp theo dõi: FPS, Free RAM Heap, CPU Load và Cường độ WiFi (dBm).
  - Hệ thống macro Logging tập trung (`LOG_I`, `LOG_D`...), tắt sạch ở chế độ Production giúp tăng FPS tối đa.

---

## 📦 Danh Mục File Đính Kèm (Release Assets)

| File | Offset / Địa chỉ | Dung lượng | Mô tả chức năng |
| :--- | :---: | :---: | :--- |
| **`merged_firmware_0x00.bin`** | **`0x00000000` (`0x00`)** | **~1.69 MB** | **File gộp ALL-IN-ONE khuyên dùng**, nạp từ `0x00` cho mọi board mới |
| `firmware.bin` | `0x10000` | ~1.62 MB | Mã nguồn chương trình ứng dụng chính |
| `bootloader.bin` | `0x1000` | ~17.5 KB | ESP32 Second Stage Bootloader |
| `partitions.bin` | `0x8000` | ~3 KB | Bảng phân vùng bộ nhớ Flash (`min_spiffs.csv`) |
| `boot_app0.bin` | `0xe000` | 8 KB | Dữ liệu cấu hình OTA boot ban đầu |

---

## 🚀 Hướng Dẫn Nạp Firmware Nhanh

Chi tiết hướng dẫn kèm hình ảnh có trong tài liệu [**`FLASH_GUIDE.md`**](FLASH_GUIDE.md).

### Cách 1: Nạp qua Trình Duyệt Web (Tiện lợi nhất - Không cần cài đặt)
1. Dùng trình duyệt **Google Chrome** hoặc **Microsoft Edge**.
2. Truy cập: [Espressif Web Flasher](https://espressif.github.io/esptool-js/) hoặc [Adafruit WebSerial ESPTool](https://adafruit.github.io/Adafruit_WebSerial_ESPTool/).
3. Cắm cáp kết nối ESP32 với máy tính -> bấm **Connect** -> chọn cổng COM.
4. Chọn file **`merged_firmware_0x00.bin`** tại địa chỉ **`0x0`** và bấm **Program**.

### Cách 2: Nạp bằng công cụ `esptool.py` (CLI)
```powershell
esptool.py --chip esp32 --port COMx --baud 921600 write_flash 0x00 merged_firmware_0x00.bin
```

### Cách 3: Nạp bằng phần mềm ESP Flash Download Tool (Chính hãng Espressif)
- Chọn chip: **ESP32** | WorkMode: **Develop**.
- Tích chọn dòng 1: Đường dẫn file `merged_firmware_0x00.bin`, địa chỉ: `0x0`.
- SPI SPEED: `40MHz`, SPI MODE: `DIO`, FLASH SIZE: `32Mbit`.
- Chọn cổng COM, Baud `921600` và bấm **START**.

---

## 👨‍💻 Tác Giả & Liên Hệ Hỗ Trợ

- **Tác giả:** **Mạc Tân**
- **Facebook:** [Mạc Văn Tân](https://www.facebook.com/mvt.hp.star/)
- **Hotline / Zalo hỗ trợ:** **0964 335 688**
- **Mã nguồn:** [GitHub - mvtcode/esp32-project](https://github.com/mvtcode/esp32-project)

⭐ _Nếu thấy dự án hữu ích, đừng quên bấm **Star** trên GitHub để ủng hộ tác giả phát triển thêm các bản cập nhật tiếp theo!_
