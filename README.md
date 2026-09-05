# 🚀 Tổng Hợp Các Dự Án ESP32 & IoT Ecosystem

Kho mã nguồn **ESP32 & Embedded Systems** đa dạng dự án thực tế, từ **Edge AI, Thị giác máy tính (Computer Vision), Âm thanh & Trực quan hóa sóng nhạc (Audio Visualizer), Màn hình cảm ứng GUI LVGL, Đồng hồ thông minh LED Matrix đến Sinh trắc học**.

Nhánh `main` này đóng vai trò là **Cổng thư mục trung tâm (Directory Hub)**. Mỗi dự án hoàn chỉnh với sơ đồ phần cứng, mã nguồn và tài liệu chi tiết được phát triển trên từng **nhánh (branch) chuyên biệt**.

---

## 📑 Mục Lục Các Nhóm Dự Án

- [1. 🤖 Nhóm Camera AI & Thị giác máy tính](#1--nhóm-camera-ai--thị-giác-máy-tính)
- [2. 🎵 Nhóm Âm thanh, Visualizer & Xử lý giọng nói AI](#2--nhóm-âm-thanh-visualizer--xử-lý-giọng-nói-ai)
- [3. 🕒 Nhóm Đồng hồ thông minh & Màn hình hiển thị LED](#3--nhóm-đồng-hồ-thông-minh--màn-hình-hiển-thị-led)
- [4. 🖥️ Nhóm Màn hình cảm ứng & Bảng điều khiển (Smart Dashboard)](#4-️-nhóm-màn-hình-cảm-ứng--bảng-điều-khiển-smart-dashboard)
- [5. 🔐 Nhóm Sinh trắc học & Dự án phần cứng khác](#5--nhóm-sinh-trắc-học--dự-án-phần-cứng-khác)
- [🛠️ Hướng dẫn sử dụng & Chuyển đổi nhánh (Git Workflow)](#️-hướng-dẫn-sử-dụng--chuyển-đổi-nhánh-git-workflow)
- [⚡ Các lệnh PlatformIO cơ bản](#-các-lệnh-platformio-cơ-bản)

---

## 1. 🤖 Nhóm Camera AI & Thị giác máy tính

| Nhánh (Branch) | Vi điều khiển & Phần cứng | Mô tả & Tính năng nổi bật |
| :--- | :--- | :--- |
| [**`ESP32-S3cam-N16R8-FaceDetection`**](https://github.com/mvtcode/esp32-project/tree/ESP32-S3cam-N16R8-FaceDetection) | • ESP32-S3 N16R8 (16MB Flash, 8MB OPI PSRAM)<br>• Camera OV3660 / OV2640<br>• LCD ST7789 240x320 (LovyanGFX)<br>• Đèn LED RGB GPIO 48 | • **Edge AI Nhận diện khuôn mặt** thời gian thực bằng mô hình ESP-WHO (`human_face_detect`) trên Core 0.<br>• Hiển thị HUD, Bounding Boxes, OSD, thước đo FPS mượt mà.<br>• Tự động crop khuôn mặt và tải lên **Google Drive** qua Google Apps Script Webhook (kèm queue an toàn 1 file pending & cooldown).<br>• **WiFi Captive Portal** hiện đại lưu trong LittleFS (`data/index.html`), cấu hình camera, độ trễ và webhook qua trình duyệt.<br>• Nút BOOT đa chức năng: Click bật/tắt upload, giữ 1s tắt màn hình, giữ 3s factory reset. |
| [**`ESP32-S3cam-N16R8-FaceDetection-draft`**](https://github.com/mvtcode/esp32-project/tree/ESP32-S3cam-N16R8-FaceDetection-draft) | • ESP32-S3 N16R8<br>• Camera Module | • Nhánh thử nghiệm và tinh chỉnh thuật toán phát hiện khuôn mặt trước khi sáp nhập vào bản chính thức. |
| [**`ESP32-S3-N16R8-Webcam`**](https://github.com/mvtcode/esp32-project/tree/ESP32-S3-N16R8-Webcam) | • ESP32-S3 N16R8<br>• Camera Module | • Biến ESP32-S3 thành **IP Webcam không dây** truyền hình ảnh (MJPEG Stream) qua mạng nội bộ hoặc trình duyệt web. |
| [**`esp32s3cam-TFT240x320`**](https://github.com/mvtcode/esp32-project/tree/esp32s3cam-TFT240x320) | • ESP32-S3<br>• Camera OV5640 / OV2640<br>• TFT ST7789 240x320 (TFT_eSPI) | • Truyền dữ liệu luồng video trực tiếp từ camera lên màn hình màu ST7789 với định dạng RGB565 và bộ đệm DRAM tối ưu, không yêu cầu PSRAM. |

```bash
# Chuyển sang dự án Camera AI Face Detection
git checkout ESP32-S3cam-N16R8-FaceDetection
```

---

## 2. 🎵 Nhóm Âm thanh, Visualizer & Xử lý giọng nói AI

| Nhánh (Branch) | Vi điều khiển & Phần cứng | Mô tả & Tính năng nổi bật |
| :--- | :--- | :--- |
| [**`esp32-sound-visualization`**](https://github.com/mvtcode/esp32-project/tree/esp32-sound-visualization) | • ESP32 DevKit<br>• 2x Mic MEMS INMP441 (I2S Stereo)<br>• DAC PCM5102A / PCM5100A (I2S)<br>• OLED 1.3" (SH1106 / SSD1306)<br>• Khe thẻ nhớ MicroSD (SPI)<br>• Rotary Encoder EC11 + Nút bấm | • **Trực quan hóa âm thanh với 68 chế độ Visualizer**: Waveform 2 kênh, Spectrum FFT 64-band, Đồng hồ VU kim vẫy cơ học (Analog VU), 3D Tunnel, Lissajous, Oscilloscope kỹ thuật số có Trigger kép, Chromatic Tuner đo nốt nhạc/cents guitar, các hiệu ứng hình học 3D/4D (Lorenz Attractor, Tesseract 4D, Chladni Cymatics, DNA Helix...).<br>• **4 Chế độ hoạt động đỉnh cao**:<br>&nbsp;&nbsp;1. *MIC Visualizer*: Bắt âm môi trường bằng cặp mic stereo.<br>&nbsp;&nbsp;2. *Bluetooth Speaker (A2DP Sink)*: Nghe nhạc chất lượng cao từ điện thoại qua DAC I2S rời.<br>&nbsp;&nbsp;3. *SD MP3 Player*: Phát nhạc thẻ nhớ, Playlist Menu, tua bài, phát lặp/ngẫu nhiên.<br>&nbsp;&nbsp;4. *Internet Clock & Weather*: Đồng bộ giờ NTP, thời tiết Open-Meteo, WiFi AP `MVT-Audio-Setup`.<br>• Tích hợp **Retro Mini Games**: Flappy Beat, Space Invaders, Pac-Beat, Dino Runner. |
| [**`esp32-sound-visualization-xiaozhi`**](https://github.com/mvtcode/esp32-project/tree/esp32-sound-visualization-xiaozhi) | • ESP32 DevKit<br>• OLED 1.3"<br>• Cặp Mic I2S INMP441<br>• Bluetooth Audio | • Tích hợp **Trợ lý trí tuệ nhân tạo Xiaozhi (Xiaozhi AI Client)**, nhận diện giọng nói, hiển thị biểu cảm khuôn mặt AI động (Xiaozhi Face), kết hợp nhịp điệu Beat Detector và Âm lịch. |
| [**`esp32s3-super-mini-oled-1.3-sound-visualization`**](https://github.com/mvtcode/esp32-project/tree/esp32s3-super-mini-oled-1.3-sound-visualization) | • ESP32-S3 Super Mini siêu nhỏ gọn<br>• Cặp Mic INMP441 (I2S Stereo)<br>• OLED 1.3" SH1106 (I2C 800kHz) | • Tối ưu hóa riêng cho bo mạch nhỏ **ESP32-S3 Super Mini** với 65 hiệu ứng Visualizer.<br>• Phân luồng FreeRTOS 2 nhân (Core 0 đọc Mic, Core 1 xử lý tín hiệu & Render).<br>• Bộ đệm bảo vệ chống tràn biên SafeDraw và chế độ **Auto-Cycle** tự động chuyển hiệu ứng mỗi 20s. |
| [**`esp32-s3-voice-command`**](https://github.com/mvtcode/esp32-project/tree/esp32-s3-voice-command) | • ESP32-S3 N16R8<br>• Mic INMP441 (16kHz Mono)<br>• OLED 1.3" (U8g2)<br>• Cụm Relay điều khiển | • **Nhận diện giọng nói ngoại tuyến (Offline Voice Recognition)** hoàn toàn bằng thư viện **ESP-SR** của Espressif (không cần Internet hay cloud).<br>• WakeNet phát hiện từ khóa đánh thức ("Hi ESP") + MultiNet nhận diện câu lệnh điều khiển đèn, relay, thiết bị điện.<br>• Dual Core: Core 0 đọc I2S/UI, Core 1 chạy mạng nơ-ron AI Inference. |
| [**`bluetooth-speaker`**](https://github.com/mvtcode/esp32-project/tree/bluetooth-speaker) | • ESP32<br>• I2S DAC Audio<br>• Thẻ nhớ SD<br>• OLED Display | • Loa thông minh đa năng 3 trong 1: Bluetooth Audio Sink (A2DP), máy phát nhạc MP3 từ thẻ SD, và phát đài Internet Radio qua WiFi.<br>• Quản lý trạng thái bằng NVS Flash, cơ chế phục hồi sau sự cố (Crash Recovery), chế độ Deep Sleep tiết kiệm pin. |
| [**`bluetooth-speaker-extent-file`**](https://github.com/mvtcode/esp32-project/tree/bluetooth-speaker-extent-file) | • ESP32<br>• Mạch giải mã âm thanh | • Phiên bản mở rộng hỗ trợ nhiều định dạng tệp tin âm thanh và tối ưu bộ đệm phát trực tuyến. |

```bash
# Chuyển sang dự án Visualizer 68 hiệu ứng + Bluetooth + MP3
git checkout esp32-sound-visualization
```

---

## 3. 🕒 Nhóm Đồng hồ thông minh & Màn hình hiển thị LED

| Nhánh (Branch) | Vi điều khiển & Phần cứng | Mô tả & Tính năng nổi bật |
| :--- | :--- | :--- |
| [**`clock-esp32s3-6p5`**](https://github.com/mvtcode/esp32-project/tree/clock-esp32s3-6p5) *(v3.1.0)* | • ESP32-S3 N16R8 (Octal PSRAM)<br>• **6 Panel LED Matrix HUB75 (192×96 px)**<br>• Cảm biến AHT10<br>• Module RTC DS1302 | • **Đồng hồ thông minh cao cấp v3.1.0** với lưới 6 panel LED HUB75 (3 cột × 2 hàng).<br>• 5 hàng hiển thị dữ liệu đồng thời: Đồng hồ số 24px lớn với hiệu ứng sliding colon, Lịch Âm - Dương, Dự báo thời tiết Open-Meteo & chỉ số bức xạ UV ngoài trời, Nhiệt độ/độ ẩm phòng (AHT10), Dòng chữ chạy tin tức Marquee font Verdana tiếng Việt có dấu.<br>• **Captive Portal Web Server**: Cấu hình WiFi, tự động đồng bộ timestamp từ điện thoại vào RTC khi bấm lưu, hiển thị IP căn giữa khi khởi động.<br>• Hỗ trợ chế độ ngủ ban đêm (Smart Sleep Mode) giảm độ sáng / tắt panel theo giờ hẹn. |
| [**`clock`**](https://github.com/mvtcode/esp32-project/tree/clock) | • ESP32<br>• 2 hoặc 3 Panel LED HUB75<br>• AHT10 & RTC DS1302 | • Phiên bản đồng hồ LED Matrix thông minh thế hệ trước (v2.x) với FreeRTOS đa nhiệm, hiển thị giờ NTP, thời tiết, âm lịch và web cấu hình. |
| [**`feature/clock-v2.2.x`**](https://github.com/mvtcode/esp32-project/tree/feature/clock-v2.2.x) / [**`fix/clock-2.0.x`**](https://github.com/mvtcode/esp32-project/tree/fix/clock-2.0.x) | • ESP32<br>• LED Matrix HUB75 | • Các nhánh phát triển tính năng nâng cao chế độ kết nối WiFi APSTA và sửa lỗi cho dòng Clock v2. |
| [**`esp32c3SuperMini-Clock`**](https://github.com/mvtcode/esp32-project/tree/esp32c3SuperMini-Clock) | • ESP32-C3 Super Mini<br>• Màn hình tròn GC9A01 LCD 1.28" (240x240) | • **Đồng hồ kim Analog phong cách Rolex** sang trọng trên màn hình tròn GC9A01.<br>• Tự động đồng bộ thời gian thực qua WiFi NTP (múi giờ GMT+7), cơ chế tự kết nối lại và chế độ demo thời gian khi mất mạng. |
| [**`clock-esp8266-01S`**](https://github.com/mvtcode/esp32-project/tree/clock-esp8266-01S) | • ESP-01S (ESP8266 siêu nhỏ)<br>• Ma trận 4 cụm MAX7219 Dot Matrix | • Đồng hồ số LED Ma trận sử dụng vi điều khiển siêu nhỏ ESP-01S, tận dụng chân TX/RX điều khiển MAX7219 qua thư viện MD_Parola/MD_MAX72XX.<br>• Đồng bộ giờ NTP, web cấu hình captive portal, hiệu ứng chữ chạy hiển thị địa chỉ IP và chớp tắt dấu hai chấm. |
| [**`p5Matrix`**](https://github.com/mvtcode/esp32-project/tree/p5Matrix) | • ESP32<br>• Panel LED P5 HUB75 (64x32 px) | • Điều khiển LED ma trận P5 bằng cơ chế I2S DMA tốc độ cao, demo dải màu chuyển sắc Rainbow, các khối hình đồ họa và chữ chạy. |

```bash
# Chuyển sang dự án Smart Clock 6 Panel HUB75 v3.1.0
git checkout clock-esp32s3-6p5
```

---

## 4. 🖥️ Nhóm Màn hình cảm ứng & Bảng điều khiển (Smart Dashboard)

| Nhánh (Branch) | Vi điều khiển & Phần cứng | Mô tả & Tính năng nổi bật |
| :--- | :--- | :--- |
| [**`esp32-kit3.5`**](https://github.com/mvtcode/esp32-project/tree/esp32-kit3.5) | • Bo mạch **ESP32-3248S035** (CYD 3.5" TFT ST7796 480x320 + Cảm ứng XPT2046)<br>• Thẻ nhớ MicroSD, DAC Audio, Quang trở LDR, RGB LED | • **Bảng điều khiển thông minh Smart Dashboard (LVGL v8)** mượt mà 30+ FPS.<br>• **Màn hình Home**: Đồng hồ số lớn, Lịch Âm Can Chi, Dự báo thời tiết, cập nhật **Giá vàng SJC/PNJ** và **Giá xăng dầu** trực tiếp từ Internet.<br>• **Lịch vạn niên**: Ma trận lịch tháng, xem ngày hoàng đạo, sự kiện.<br>• **Trình phát nhạc MP3**: Quét thư viện thẻ nhớ SD, Visualizer sóng âm thanh, tua bài.<br>• **Cài đặt & Giám sát**: Cấu hình mạng WiFi, format thẻ nhớ, độ sáng PWM, Dev HUD hiển thị RAM, CPU load, FPS thực tế và WiFi dBm. |
| [**`esp32wroom-lcd3.5-lvgl`**](https://github.com/mvtcode/esp32-project/tree/esp32wroom-lcd3.5-lvgl) | • ESP32-WROOM CYD 3.5"<br>• Cảm ứng XPT2046 | • Nền tảng tích hợp thư viện đồ họa LVGL cùng bộ biểu tượng Material Icon Font trên màn hình cảm ứng điện trở 3.5 inch. |
| [**`esp32wroom-lcd3.5`**](https://github.com/mvtcode/esp32-project/tree/esp32wroom-lcd3.5) | • ESP32-WROOM CYD 3.5"<br>• Cảm biến DHT11 & LDR<br>• Thẻ nhớ SD & I2S DAC | • Bản thử nghiệm toàn diện phần cứng CYD: Đo nhiệt độ/độ ẩm phòng (DHT11), đo cường độ sáng (LDR), hiệu ứng nháy LED RGB 7 màu, phát nhạc MP3 từ thẻ nhớ và vẽ thử nghiệm vùng cảm ứng. |
| [**`esp32c3supermini-oled0.91`**](https://github.com/mvtcode/esp32-project/tree/esp32c3supermini-oled0.91) / [**`esp32s3supermini-oled.91`**](https://github.com/mvtcode/esp32-project/tree/esp32s3supermini-oled.91) | • ESP32-C3 / S3 Super Mini<br>• OLED 0.91" (128x32 I2C SSD1306) | • Trình diễn đồ họa trên màn hình nhỏ gọn: Đồng hồ đếm thời gian, đo nhiệt độ mô phỏng, thanh tiến trình, biểu đồ cột và giám sát tài nguyên chip (Free RAM Heap, Uptime). |
| [**`esp32s3-super-mini-oled-1.3`**](https://github.com/mvtcode/esp32-project/tree/esp32s3-super-mini-oled-1.3) / [**`esp32-lvgl-oled1.3`**](https://github.com/mvtcode/esp32-project/tree/esp32-lvgl-oled1.3) | • ESP32 / ESP32-S3<br>• OLED 1.3" (128x64 SH1106 / SSD1306) | • Thử nghiệm giao diện đồ họa đơn sắc và tích hợp thư viện đồ họa LVGL trên màn hình OLED 1.3 inch. |

```bash
# Chuyển sang dự án Smart Dashboard CYD 3.5" TFT LVGL v8
git checkout esp32-kit3.5
```

---

## 5. 🔐 Nhóm Sinh trắc học & Dự án phần cứng khác

| Nhánh (Branch) | Vi điều khiển & Phần cứng | Mô tả & Tính năng nổi bật |
| :--- | :--- | :--- |
| [**`finger`**](https://github.com/mvtcode/esp32-project/tree/finger) & [**`finger_enroll`**](https://github.com/mvtcode/esp32-project/tree/finger_enroll) | • ESP32 DevKit<br>• Module cảm biến vân tay UART **TZM1026** (hoặc tương đương) | • Giao tiếp truyền nhận khung dữ liệu HEX chuẩn 8-byte qua Serial UART.<br>• Đăng ký vân tay mới theo nhiều lần chạm, so khớp nhận diện 1:1 và 1:N, kiểm tra số lượng người dùng trong cơ sở dữ liệu, phân quyền quản trị và cơ chế xóa bộ nhớ an toàn. |
| [**`arduinoNano-ws2812-24Led`**](https://github.com/mvtcode/esp32-project/tree/arduinoNano-ws2812-24Led) | • Arduino Nano (ATmega328P)<br>• Vòng 24 LED RGB WS2812 | • Điều khiển vòng tròn 24 bóng LED RGB bằng thư viện FastLED với hiệu ứng cầu vồng (Rainbow) chuyển động mượt mà, tối ưu bộ nhớ vi điều khiển 8-bit. Có video demo đi kèm. |
| [**`esp32s3-super-mini-test`**](https://github.com/mvtcode/esp32-project/tree/esp32s3-super-mini-test) | • ESP32-S3 Super Mini | • Firmware kiểm tra nhanh phần cứng board, kết nối WiFi và chân GPIO. |

```bash
# Chuyển sang dự án Cảm biến vân tay
git checkout finger
```

---

## 🛠️ Hướng dẫn sử dụng & Chuyển đổi nhánh (Git Workflow)

### 1. Xem danh sách tất cả các nhánh trong kho mã nguồn:

```bash
git branch -a
```

### 2. Chuyển sang một nhánh dự án cụ thể để nạp code:

```bash
# Ví dụ: Muốn nạp code cho dự án Nhận diện khuôn mặt AI
git checkout ESP32-S3cam-N16R8-FaceDetection

# Hoặc muốn nạp dự án Âm thanh Visualizer 68 hiệu ứng
git checkout esp32-sound-visualization

# Hoặc dự án Smart Dashboard màn hình cảm ứng 3.5"
git checkout esp32-kit3.5
```

> [!TIP]
> Mỗi nhánh đều có file `README.md` riêng biệt hướng dẫn chi tiết sơ đồ đấu nối dây (pinout wiring), danh sách linh kiện cần chuẩn bị và cấu hình `platformio.ini` tương ứng.

---

## ⚡ Các lệnh PlatformIO cơ bản

Dự án được xây dựng và quản lý thống nhất bằng **PlatformIO CLI / VS Code Extension**:

### Biên dịch dự án:
```bash
pio run
```

### Nạp code vào vi điều khiển:
```bash
pio run --target upload
```

### Nạp dữ liệu hệ thống tệp (LittleFS / SPIFFS) nếu dự án có thư mục `data/`:
```bash
pio run --target uploadfs
```

### Mở Serial Monitor theo dõi log:
```bash
pio device monitor
```

### Biên dịch + Nạp + Mở Serial Monitor trong một lệnh:
```bash
pio run --target upload && pio device monitor
```

### Dọn dẹp các tệp build tạm:
```bash
pio run --target clean
```

---

## 👨‍💻 Tác Giả & Bản Quyền

- **Tác giả:** **[Mạc Tân](https://www.facebook.com/mvt.hp.star/)**
- **Hotline / Zalo:** [0964 335 688](tel:0964335688)
- **Mã nguồn:** Mã nguồn mở phục vụ nghiên cứu, học tập và ứng dụng thực tế trên các dòng vi điều khiển ESP32 / ESP8266 / Arduino.

⭐ *Nếu thấy kho dự án hữu ích, hãy tặng 1 Star trên GitHub để ủng hộ tác giả nhé!*
