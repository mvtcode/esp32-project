# ESP32 Multi-Mode Sound Visualizer & Bluetooth Speaker Clock 🎵📻⏰

Dự án thiết bị đa năng tích hợp **3 Chế Độ Hoạt Động Độc Lập**: **Sound Visualizer Stereo (Microphone)**, **Loa Bluetooth Hi-Fi (A2DP Sink + DAC PCM5102A)** và **Đồng Hồ Thời Tiết & Âm Lịch Việt Nam (NTP & Open-Meteo)** trên vi điều khiển **ESP32 Dev Module** kết hợp màn hình **OLED 1.3" (SH1106 / SSD1306)**.

---

## 🌟 Tính năng nổi bật

### 1. 🔀 3 Chế độ hoạt động độc lập (100% Resource Isolation)
Hệ thống quản lý tài nguyên thông minh, tự động tắt tiến trình và giải phóng phần cứng khi chuyển chế độ:
- **🎤 Chế độ MICROPHONE (Sound Visualizer):**
  - Tắt Bluetooth và WiFi $\to$ tiết kiệm năng lượng, chống nhiễu 2.4GHz RF.
  - Thu âm Stereo 24-bit từ cặp micro MEMS **INMP441** qua giao tiếp I²S (16 kHz).
  - Kiến trúc FreeRTOS đa nhân: Core 0 đọc buffer I²S, Core 1 xử lý AGC, FFT và hiển thị 65 hiệu ứng visualizer trực quan đỉnh cao.
- **📱 Chế độ BLUETOOTH (Loa Bluetooth & Visualizer):**
  - Tắt WiFi, giải phóng I²S Micro $\to$ dành 100% tài nguyên cho Bluetooth A2DP Sink.
  - Nhận luồng âm thanh từ điện thoại / máy tính, giải mã SBC và xuất ra DAC **PCM5102A** qua I²S chất lượng cao, đồng thời hiển thị 65 hiệu ứng Visualizer theo luồng nhạc.
  - **Đồng bộ âm lượng 2 chiều (AVRCP 1.4 Absolute Volume):** Xoay núm trên ESP32 làm thanh âm lượng điện thoại/máy tính trượt theo đồng thời, và ngược lại.
  - **Phím điều khiển Play/Pause:** Nút BACK gửi lệnh Bluetooth AVRCP trực tiếp đến Spotify, YouTube, ZingMP3,...
  - **Khóa núm xoay an toàn:** Module xoay EC11 chỉ kích hoạt ở chế độ Bluetooth, khóa hoàn toàn ở chế độ khác để tránh chạm nhầm.
- **⏰ Chế độ CLOCK & WEATHER (Đồng hồ, Thời tiết & Âm Lịch):**
  - Tắt Bluetooth, deinit Micro. Bật WiFi bất đồng bộ (Non-blocking, chuyển chế độ 0ms, không lag/đơ màn hình).
  - **Đồng hồ số lớn (Hardware RTC):** Đếm thời gian bằng bộ đếm thạch anh phần cứng ESP32 với tốc độ 30 FPS mượt mà (không bị nghẽn mạng hay nhảy cóc giây).
  - **Đồng bộ giờ NTP:** Tự động lấy giờ chuẩn quốc tế theo chu kỳ **1 giờ / lần** từ `pool.ntp.org`, `time.google.com`, `time.cloudflare.com`.
  - **Dự báo thời tiết:** Tự động cập nhật nhiệt độ (°C) và độ ẩm (%) từ Open-Meteo API theo chu kỳ **10 phút / lần**.
  - **Lịch Âm Việt Nam & Can Chi:** Tích hợp thuật toán thiên văn Hồ Ngọc Đức tính toán chính xác ngày, tháng âm lịch và tên năm Can Chi (ví dụ: `AL: 08/07 (Bính Ngọ)`).
  - **Giao diện OLED tối ưu:** Header hiển thị Thứ & Ngày Dương Lịch cùng biểu tượng 5 vạch sóng WiFi (hiệu ứng quét sóng khi đang kết nối).
  - **Web Config Portal & ElegantOTA:** Trang cấu hình WiFi và hỗ trợ nạp firmware từ xa qua mạng (OTA).

---

### 2. 💾 Lưu trữ cấu hình tự động (NVS Flash Persistence)
- Tự động ghi nhớ Chế độ hoạt động (MIC / BT / CLOCK), Hiệu ứng hiển thị, Trạng thái Auto-Cycle, Mức âm lượng và Địa chỉ MAC thiết bị Bluetooth đã ghép đôi.
- Khi khởi động lại hoặc mất nguồn bật lại, thiết bị tự động phục hồi đúng chế độ và tự động kết nối lại thiết bị Bluetooth gần nhất.

---

## 🎮 Hệ thống điều khiển & Phím bấm

| Phím / Núm xoay | Chân GPIO | Thao tác | Chức năng |
| :--- | :---: | :--- | :--- |
| **PUSH** | **GPIO 4** | Nhấn nhanh | Chuyển đổi tuần hoàn 3 chế độ: `MIC` $\to$ `BLUETOOTH` $\to$ `CLOCK & WEATHER` $\to$ `MIC`. |
| **PLUS** | **GPIO 14** | Nhấn nhanh | Đổi chế độ hiển thị OLED tiếp theo (trong 65 hiệu ứng Visualizer). |
| | | Nhấn giữ (> 1s) | Bật / Tắt chế độ **Auto-Cycle** (tự động đổi hiệu ứng mỗi 20 giây). |
| **BACK** | **GPIO 13** | Nhấn nhanh | **Play / Pause** bài hát trên điện thoại/máy tính qua lệnh Bluetooth AVRCP (chế độ BT). |
| **BOOT** | **GPIO 0** | Nhấn nhanh | Reset cài đặt WiFi (khởi động lại vào chế độ AP cấu hình). |
| | | Nhấn giữ (> 3s) | Xóa thiết bị Bluetooth đã nhớ và kích hoạt chế độ **Re-Pairing**. |
| **ROTARY ENCODER** | **GPIO 32 / 33** | Xoay núm | Tăng / Giảm âm lượng và đồng bộ 2 chiều với điện thoại (chỉ hoạt động ở chế độ BT). |

---

## 🔌 Sơ đồ đấu nối phần cứng (Pinout & Wiring)

### 1. Màn hình OLED 1.3" (SH1106 / SSD1306 I²C)
| Chân OLED | ESP32 Pin | Chức năng |
| :--- | :---: | :--- |
| **VCC** | **3.3V** | Nguồn 3.3V |
| **GND** | **GND** | Nối đất |
| **SCL** | **GPIO 22** | I²C Clock |
| **SDA** | **GPIO 21** | I²C Data |

### 2. Hai Microphone INMP441 (I²S Stereo)
| Chân INMP441 | ESP32 Pin | Ghi chú |
| :--- | :---: | :--- |
| **SCK** (cả 2 mic) | **GPIO 26** | I²S Bit Clock (BCLK) |
| **WS** (cả 2 mic) | **GPIO 25** | I²S Word Select (LRCLK) |
| **SD** (cả 2 mic) | **GPIO 27** | I²S Serial Data (chung đường data stereo) |
| **L/R** (Mic Trái) | **GND** | Kênh Trái (phát ở slot WS = LOW) |
| **L/R** (Mic Phải) | **3.3V** | Kênh Phải (phát ở slot WS = HIGH) |

### 3. Mạch DAC giải mã âm thanh PCM5102A (I²S Output)
| Chân PCM5102A | ESP32 Pin | Ghi chú |
| :--- | :---: | :--- |
| **BCK** | **GPIO 18** | Bit Clock |
| **LCK / WS** | **GPIO 19** | Left/Right Clock |
| **DIN** | **GPIO 23** | Data In |
| **SCK** | **GND** | Nối đất để sử dụng Internal PLL của PCM5102A |

### 4. Núm xoay mã hóa EC11 (Rotary Encoder) & Phím điều khiển
| Linh kiện | ESP32 Pin | Ghi chú |
| :--- | :---: | :--- |
| **Encoder CLK (A)** | **GPIO 32** | Tín hiệu xung A (Internal Pullup) |
| **Encoder DT (B)** | **GPIO 33** | Tín hiệu xung B (Internal Pullup) |
| **Nút PUSH** | **GPIO 4** | Nút bấm chuyển Mode (Active LOW) |
| **Nút PLUS** | **GPIO 14** | Nút bấm chuyển Effect / Auto-cycle (Active LOW) |
| **Nút BACK** | **GPIO 13** | Nút bấm Play/Pause (Active LOW) |
| **Nút BOOT** | **GPIO 0** | Nút BOOT trên board ESP32 (Active LOW) |

---

## 🖥️ 65 Chế độ hiển thị Visualizer

| Mode | Tên chế độ | Mô tả |
| :---: | :--- | :--- |
| **0** | **WAVEFORM** | Dạng sóng 2 kênh stereo độc lập (Kênh L: nửa trên, Kênh R: nửa dưới). |
| **1** | **MIRROR** | Sóng âm đối xứng từ tâm màn hình ra 2 phía (Mono mix L+R). |
| **2** | **SPECTRUM** | Phổ tần số âm thanh (FFT 128 điểm $\to$ 64 cột tần số thời gian thực). |
| **3** | **LISSAJOUS** | Đồ thị pha trực quan hóa trường stereo (stereo image). |
| **4** | **VU METER** | Cột đo âm lượng L/R kèm Peak Hold và decay mượt mà. |
| **5** | **ANALOG VU** | Đồng hồ VU kim vẫy cơ học (2 mặt L/R, thang đo chuẩn dB). |
| **6** | **CIRCLE MVT** | Visualizer hình tròn 40 dải phổ Stereo, chữ "MVT" ở tâm + vòng năng lượng đập theo bass. |
| **7** | **MVT HEART** | Trái tim ❤️ đập theo nhịp bass, 2 dải sóng âm đối xứng L & R 2 bên. |
| **8** | **MVT FUSION** | Vòng tròn tâm có chữ "MVT" xoay 10s/vòng (tăng tốc theo bass), tia phổ tỏa đỉnh/đáy. |
| **9** | **MVT CYBER** | Cột bar ngang 2 bên nháy bắn vào tâm, vòng tròn tâm chữ "MVT" + 3 vòng radar. |
| **10** | **MVT CASSETTE** | Băng Cassette cổ điển với 2 bánh xe cuộn băng quay tròn + cửa sổ sóng âm live. |
| **11-20** | **3D & EFFECTS** | Tunnel 3D, Orbit, Matrix, Terrain Waterfall 3D, Heart Matrix, Twin Hearts, DJ Deck, Speaker, Headphone, Spiderweb. |
| **21-30** | **SCI-FI & BEAT** | Synthwave 80s, Radar 360°, Arc Reactor, Blackhole, Cyber Highway, Sound Megaphone, Waveform Rotate, Bounce Lines, Laser Stage, Chibi Dancer. |
| **31-40** | **ADVANCED MATH** | Juggle Physics, Vectorscope 45°, Spirograph, Superformula, Lorenz Attractor 3D, Polar Wave, Chladni Cymatics, Tesseract 4D, SDR Waterfall, Warp Text 3D. |
| **41-50** | **3D SHAPES** | Kinetic Particle, Starfield Warp, Sphere 3D, Torus 3D, DNA Helix, Cubes 3D Explode, Galaxy 3D, Crystal 3D, Cylinder, Parametric Heart 3D. |
| **51-60** | **CREATIVE & GAMES** | Text 3D Extruded, Solar System, Supernova, Thunderstorm, Cyber Cockpit, Space Invaders, Flappy Beat, Pac-Beat, Dino Runner, Xiaozhi AI Face. |
| **61-64** | **ROBOT & MASCOT** | AI Robot Head, Plasma Ball, Zigzag LED EDM, Cyber Neko Cat. |

---

## 🚀 Hướng dẫn biên dịch & Nạp code

### 1. Chuẩn bị môi trường
- Cài đặt **Visual Studio Code** cùng extension **PlatformIO IDE**.
- Hoặc sử dụng **PlatformIO Core (CLI)**.

### 2. Biên dịch và nạp firmware
Mở terminal tại thư mục dự án:
```bash
# 1. Biên dịch dự án
pio run

# 2. Nạp code lên ESP32 qua cổng USB
pio run -t upload

# 3. Mở Serial Monitor theo dõi trạng thái
pio device monitor
```
*(Baud rate mặc định: `115200`)*

---

## 🌐 Cấu hình WiFi cho Chế độ Đồng Hồ (Clock & Weather)
1. Khi chuyển sang chế độ **CLOCK & WEATHER** lần đầu (hoặc nhấn nút **BOOT** để reset WiFi):
   - ESP32 sẽ phát WiFi AP: `MVT-Audio-Setup`.
2. Dùng điện thoại kết nối vào WiFi `MVT-Audio-Setup`, trình duyệt sẽ tự động mở trang cấu hình (hoặc truy cập `192.168.4.1`).
3. Chọn mạng WiFi nhà bạn và nhập mật khẩu $\to$ Bấm **Save**.
4. ESP32 sẽ kết nối WiFi, tự động đồng bộ giờ NTP và cập nhật thời tiết Open-Meteo.

---

## 📄 License & Tác Giả

- **License:** MIT License.
- **Tác giả:** **Power by [Mạc Tân](https://www.facebook.com/mvt.hp.star/)** | Mobile: [0964 335 688](tel:0964335688)

⭐ *Nếu bạn thấy dự án hữu ích, hãy tặng 1 Star trên GitHub nhé!*
