# ESP32-S3 Sound Visualizer – Project Requirements

## 1. Tổng quan dự án

Dự án xây dựng một thiết bị **hiển thị sóng âm thanh (Sound Visualizer)** theo thời gian thực trên màn hình OLED, sử dụng hai microphone stereo (kênh trái/phải). Thiết bị cho phép quan sát trực quan dạng sóng âm, hỗ trợ nhiều chế độ hiển thị và tự động hiệu chỉnh biên độ.

---

## 2. Phần cứng (Hardware)

| Thành phần | Model / Thông số | GPIO đã xác nhận | Ghi chú |
|---|---|---|---|
| Vi điều khiển | **ESP32-S3 Super Mini** | — | Board nhỏ gọn, tích hợp Wi-Fi & BT |
| Màn hình | **OLED 1.3 inch SH1106** (128×64) | SDA=**GPIO8**, SCL=**GPIO9** | I²C Software mode (U8g2 SW_I2C) — ✅ Đã hoạt động |
| Microphone L | **INMP441** – kênh Left | SD chia sẻ, L/R=GND | Giao tiếp I²S stereo |
| Microphone R | **INMP441** – kênh Right | SD chia sẻ, L/R=3V3 | Giao tiếp I²S stereo |

### 2.1 Phân tích GPIO – Xung đột & Phân bổ

#### GPIO đã sử dụng (I²C OLED — đã xác nhận)

| Chức năng | GPIO | Ghi chú |
|---|---|---|
| I²C SDA (OLED) | **GPIO 8** | Xác nhận từ `main.cpp` — ✅ Hoạt động |
| I²C SCL (OLED) | **GPIO 9** | Strapping pin, nhưng ổn với SW_I2C ở runtime |

> ⚠️ **Lưu ý GPIO 9:** Đây là strapping pin trên ESP32-S3, ảnh hưởng chế độ boot khi có điện trở kéo bên ngoài. Với Software I²C và **không có pull-up ngoài mạnh**, hệ thống boot bình thường. Tuy nhiên nếu có vấn đề boot, cần chuyển SCL sang GPIO khác (ví dụ GPIO 7).

#### GPIO còn khả dụng (an toàn cho I²S)

Các GPIO an toàn trên ESP32-S3 Super Mini (tránh 0, 19, 20, 26–32, 45–48):

| GPIO | Trạng thái | Ghi chú |
|---|---|---|
| 4, 5, 6, 7 | ✅ Khả dụng | Ưu tiên cho I²S |
| 10, 11, 12, 13 | ✅ Khả dụng | Dự phòng |
| 14, 15, 16, 17 | ✅ Khả dụng | Dự phòng |
| 8, 9 | ❌ Đã dùng | I²C OLED |
| 19, 20 | ❌ Tránh | USB-C native |

### 2.2 Kết nối INMP441 (I²S Stereo)

Hai INMP441 **chia sẻ cùng 1 dây SD** — phân kênh tự động qua WS clock:
- Mic Left (L/R = GND): phát dữ liệu ở **slot trái** (WS = LOW)
- Mic Right (L/R = 3V3): phát dữ liệu ở **slot phải** (WS = HIGH)

Chỉ cần **3 GPIO** cho toàn bộ I²S stereo:

| Chân INMP441 | ESP32-S3 GPIO | Mô tả |
|---|---|---|
| SCK (cả 2 mic) | **GPIO 4** | I²S Bit Clock |
| WS (cả 2 mic) | **GPIO 5** | I²S Word Select (LRCLK) |
| SD (cả 2 mic) | **GPIO 6** | Dây data chung — stereo interleaved |
| L/R – Mic L | GND | Mic Left phát ở slot WS=LOW |
| L/R – Mic R | 3V3 | Mic Right phát ở slot WS=HIGH |
| VDD (cả 2) | 3V3 | Nguồn |
| GND (cả 2) | GND | Nối đất |

> **GPIO 4, 5, 6 được chọn** vì không xung đột với I²C (GPIO 8, 9) và không phải strapping/USB pin.

---

## 3. Phần mềm (Software)

### 3.1 Kiến trúc tổng thể

```
[INMP441 L] ──┐
               ├── I²S Driver (ESP-IDF / Arduino) ──> [Sample Buffer L/R]
[INMP441 R] ──┘                                              │
                                                             ▼
                                                    [Signal Processing]
                                                    - Auto Gain Control (AGC)
                                                    - (Tương lai) FFT
                                                             │
                                                             ▼
                                                    [Render Engine]
                                                    - Mode: Waveform
                                                    - Mode: Spectrum (tương lai)
                                                             │
                                                             ▼
                                                    [OLED Display Driver]
                                                    - Kênh L: nửa trên (32px)
                                                    - Kênh R: nửa dưới (32px)
```

### 3.2 Thu âm (I²S Input)

- Cấu hình I²S ở chế độ **stereo**, đọc đồng thời cả hai kênh L và R.
- Sample rate: **16 kHz** (có thể tăng lên 44.1 kHz nếu hiệu năng cho phép).
- Bit depth: **32-bit** (INMP441 trả về 24-bit left-justified trong word 32-bit).
- Buffer đọc liên tục bằng task FreeRTOS hoặc interrupt.

### 3.3 Xử lý tín hiệu

#### Giai đoạn 1 – Hiển thị cơ bản (Waveform)
- Lấy mẫu từ buffer I²S → chuẩn hóa về dải `[-1.0, 1.0]`.
- Map giá trị sang tọa độ pixel Y trên từng nửa màn hình.
- Vẽ dạng sóng liên tục (scrolling waveform hoặc full-screen per frame).

#### Giai đoạn 2 – Hiệu chỉnh biên độ (Auto Gain Control)
- Tính **RMS** (Root Mean Square) hoặc **peak** của từng frame.
- Nếu tín hiệu quá nhỏ: tăng hệ số khuếch đại (gain) từ từ (slow attack).
- Nếu tín hiệu quá to (gần tràn biên độ): giảm gain ngay lập tức (fast attack, slow release).
- Giới hạn cứng (hard clip) để tránh overflow pixel.
- Hiển thị indicator gain nếu cần (tùy chọn).

#### Giai đoạn 3 – Chế độ hiển thị bổ sung
- **Tổng số chế độ:** 32 chế độ hiển thị âm thanh Stereo OLED đa dạng (Mode 0 đến Mode 31):
  1. `MODE_WAVEFORM` (Dạng sóng stereo 2 kênh L/R độc lập).
  2. `MODE_MIRROR` (Sóng âm đối xứng tâm).
  3. `MODE_SPECTRUM` (Phổ tần số FFT 64 cột).
  4. `MODE_LISSAJOUS` (Đồ thị pha X-Y trường stereo).
  5. `MODE_VU_METER` (Đồng hồ đo âm lượng stereo Peak Hold).
  6. `MODE_ANALOG` (Đồng hồ VU kim vẫy cơ học dB và quán tính vật lý).
  7. `MODE_CIRCLE` (Visualizer vòng tròn stereo tỏa tia, chữ "MVT" ở tâm).
  8. `MODE_HEART` (Visualizer trái tim trung tâm nhịp đập, chữ "MVT" ở dưới, 2 dải sóng 2 bên).
  9. `MODE_FUSION` (Visualizer kết hợp: Vòng tròn trung tâm chữ "MVT" xoay kim đồng hồ 10s/vòng + tia phổ đỉnh/đáy + 2 cánh sóng L/R có hạt giữ đỉnh).
  10. `MODE_CYBER` (Cyber Matrix: Bar ngang 2 bên bắn vào tâm, vòng tròn tâm chữ "MVT" xoay 10s/vòng + 3 vòng radar công nghệ).
  11. `MODE_CASSETTE` (Băng Cassette cổ điển với 2 bánh xe cuộn băng quay tròn + cửa sổ sóng âm thời gian thực ở giữa).
  12. `MODE_TUNNEL` (Đường hầm không gian 3D Warp Drive phóng nhanh ra phía trước + bung nở theo nhịp Bass).
  13. `MODE_ORBIT` (Quỹ đạo nguyên tử 3D với 3 vòng elip nghiêng + các hạt electron quay quanh hạt nhân "MVT").
  14. `MODE_MATRIX` (Cơn mưa kỹ thuật số Matrix 16 cột rơi toàn màn hình theo 16 dải tần số FFT).
  15. `MODE_TERRAIN` (Thác nước phổ tần số 3D Waterfall Mesh cuộn trôi liên tục theo thời gian).
  16. `MODE_RAIN_HEART` (Trái tim ma trận: Mưa số kỹ thuật số + trái tim rỗng có chữ "MVT" phóng to theo cường độ âm thanh).
  17. `MODE_TWIN_HEARTS` (Đôi trái tim: 2 trái tim đặc 2 bên đập độc lập theo kênh L/R + dây kết nối ở giữa là sóng âm Waveform).
  18. `MODE_DJ_DECK` (Cyber DJ Mixer: Dual vinyl turntables + crossfader + mini wave).
  19. `MODE_SPEAKER` (Hi-Fi Bouncing Speaker: Loa bass nhún nhảy + vòng sóng âm tỏa).
  20. `MODE_HEADPHONE` (Studio Headphone: Tai nghe chụp đầu + dải phổ stereo 2 bên).
  21. `MODE_SPIDERWEB` (Cyber Spider Web: Mạng nhện 3D co giãn dợn sóng theo tần số).
  22. `MODE_SYNTHWAVE` (Synthwave 80s: Mặt trời neon + lưới 3D + skyline FFT chân trời).
  23. `MODE_RADAR` (Cyber Radar/Sonar: Quét 360 độ + khóa mục tiêu tần số âm).
  24. `MODE_REACTOR` (Arc Reactor Core: Vành bánh răng xoay đảo chiều + phóng tia sét năng lượng).
  25. `MODE_BLACKHOLE` (Cosmic Black Hole: Hố đen vũ trụ hút bụi sao xoay tròn + tia plasma phản lực).
  26. `MODE_HIGHWAY` (Cyber Night Highway: Đua xe đêm Synthwave + đường cao tốc 3D).
  27. `MODE_SOUND_ICON` (Iconic Megaphone: Loa phát sóng âm quạt góc + luồng hạt âm thanh).
  28. `MODE_ROTATE_WAVE` (Rotating Wave: Sóng âm 2 điểm xoay đều bám biên độ màn hình chu kỳ 20s/vòng).
  29. `MODE_BOUNCE_LINES` (Bounce Lines: 10-15px flying lines bouncing off screen borders + audio speed boost).
  30. `MODE_STAGE_LASER` (Stage Lasers: Dàn đèn laser sân khấu 2 tia quét đỉnh đan chéo + đốm sáng sàn).
  31. `MODE_DANCER` (Rhythm Dancer: Vũ công nhảy cực sung, headbobbing, squat nhún và quẩy tay theo beat).
  32. `MODE_BALL_JUGGLE` (Waveform Juggle: Tâng bóng vật lý trên dải sóng âm nảy cao theo biên độ).
- **Chuyển chế độ:** Nhấn nút **BOOT (GPIO 0)** trên mạch để chuyển tuần hoàn giữa 32 chế độ. Nhãn chế độ hiển thị overlay 1.5s.

### 3.4 Hiển thị OLED

- **Phân vùng màn hình (128×64):**
  - Hàng `0–29`  : Waveform kênh **LEFT**
  - Hàng `30–31` : Đường phân cách (separator line) — tùy chọn
  - Hàng `32–63` : Waveform kênh **RIGHT**
- Tốc độ làm mới màn hình: **≥ 20 FPS** (mục tiêu).
- Dùng double-buffering (frame buffer trong RAM) để tránh nhấp nháy.
- Thư viện gợi ý: **U8g2** hoặc **Adafruit SSD1306/SH1106**.

---

## 4. Các pha phát triển (Development Phases)

| Pha | Mục tiêu | Trạng thái |
|---|---|---|
| **Phase 1** | Cấu hình I²S, đọc dữ liệu từ 2 mic INMP441 | ✅ Done |
| **Phase 2** | Vẽ waveform đơn giản lên OLED (2 kênh) | ✅ Done |
| **Phase 3** | Implement Auto Gain Control (AGC) | ✅ Done — per-channel, attack/release |
| **Phase 4** | Thêm chế độ hiển thị (FFT, VU Meter, Lissajous) | ✅ Done — 5 modes ban đầu |
| **Phase 5** | Nút điều hướng chuyển chế độ + UI cải thiện | ✅ Done — BOOT button (GPIO0), mode label overlay 1.5s |
| **Phase 6** | Thêm trọn bộ visualizer sáng tạo (32 modes đỉnh cao) | ✅ Done — Tổng cộng 32 modes hoàn chỉnh |

---

## 5. Ràng buộc & yêu cầu phi chức năng

- **Độ trễ (Latency):** Độ trễ từ âm thanh đến hiển thị < 100 ms.
- **Ổn định:** Không bị crash do buffer overflow; AGC không gây méo tín hiệu.
- **Kích thước:** Code phải vừa với Flash của ESP32-S3 Super Mini.
- **Năng lượng:** Ưu tiên tiêu thụ điện thấp khi có thể.

---

## 6. Môi trường phát triển

- **Framework:** Arduino-ESP32 hoặc ESP-IDF (ưu tiên Arduino cho giai đoạn đầu).
- **IDE:** VS Code + PlatformIO.
- **Ngôn ngữ:** C/C++.
- **Board target:** `esp32s3` (ESP32-S3 Super Mini).

---

*Cập nhật lần cuối: 2026-08-15 — Tất cả 6 phase đã hoàn thành. 32 chế độ hiển thị. RAM 9.3%, Flash 33.1%*

