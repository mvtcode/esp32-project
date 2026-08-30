# ESP32 All-in-One Multi-Mode Sound Visualizer, MP3 Player, Bluetooth Speaker & Retro Game Console 🎵📻⏰🕹️

Dự án thiết bị giải trí đa phương tiện và đồng hồ để bàn đa năng tích hợp **5 Chế Độ Hoạt Động Độc Lập**: **Sound Visualizer Stereo (Microphone)**, **Loa Bluetooth Hi-Fi (A2DP Sink + DAC PCM5102A)**, **Đồng Hồ Thời Tiết & Âm Lịch Việt Nam (NTP & Open-Meteo)**, **Máy Nghe Nhạc MP3 Thẻ Nhớ (MicroSD + DAC)** và **Máy Chơi Game Retro 8-in-1 (Retro Game Console)** trên vi điều khiển **ESP32 Dev Module** kết hợp màn hình **OLED 1.3" (SH1106 / SSD1306)**.

---

## 🌟 Tính năng nổi bật & 5 Chế độ hoạt động

Hệ thống quản lý tài nguyên thông minh (100% Resource Isolation), tự động tắt tiến trình và giải phóng bộ nhớ phần cứng khi chuyển chế độ:

### 1. 🎤 Chế độ MICROPHONE (Sound Visualizer Stereo)
- **Thu âm Stereo 24-bit:** Thu nhận âm thanh thời gian thực từ cặp micro MEMS **INMP441** qua giao tiếp I²S (16 kHz).
- **Kiến trúc FreeRTOS đa nhân:** Core 0 đọc buffer I²S DMA liên tục, Core 1 xử lý AGC tự động, biến đổi FFT và hiển thị **68 hiệu ứng visualizer** đỉnh cao.
- **Tiết kiệm năng lượng & chống nhiễu:** Tắt hoàn toàn WiFi và Bluetooth khi ở chế độ Micro để khử nhiễu sóng 2.4GHz RF vào đường thu âm.

### 2. 📱 Chế độ BLUETOOTH (Loa Bluetooth Hi-Fi & Visualizer)
- **A2DP Sink & DAC PCM5102A:** Nhận luồng âm thanh không dây chất lượng cao từ điện thoại / máy tính, giải mã SBC và xuất ra DAC **PCM5102A** (I²S Stereo).
- **Đồng bộ âm lượng 2 chiều (AVRCP 1.4 Absolute Volume):** Xoay núm EC11 trên ESP32 làm thanh âm lượng trên điện thoại trượt theo đồng thời, và ngược lại.
- **Điều khiển phát nhạc:** Nút **BACK** gửi lệnh Bluetooth AVRCP Play/Pause trực tiếp đến Spotify, YouTube, Apple Music, ZingMP3,...
- **Re-Pairing nhanh:** Nhấn giữ nút **BOOT** (3s) để xóa ghép đôi cũ và chuyển ngay sang chế độ chờ kết nối thiết bị mới.
- **Visualizer theo nhạc:** 68 hiệu ứng sóng âm/phổ tần số nhảy múa đồng bộ theo luồng nhạc Bluetooth đang phát.

### 3. ⏰ Chế độ CLOCK & WEATHER (Đồng hồ, Thời tiết & Âm Lịch Việt Nam)
- **WiFi Non-blocking:** Chuyển sang chế độ đồng hồ tức thì mà không gây đơ lag màn hình.
- **Đồng hồ số lớn (Hardware RTC):** Đếm thời gian bằng bộ đếm thạch anh phần cứng ESP32 với tốc độ 30 FPS mượt mà.
- **Đồng bộ giờ NTP:** Tự động lấy giờ chuẩn quốc tế theo chu kỳ **1 giờ / lần** từ các máy chủ NTP (`pool.ntp.org`, `time.google.com`, `time.cloudflare.com`).
- **Dự báo thời tiết:** Tự động cập nhật nhiệt độ (°C) và độ ẩm (%) từ Open-Meteo API theo chu kỳ **10 phút / lần**.
- **Lịch Âm Việt Nam & Can Chi:** Tích hợp thuật toán thiên văn Hồ Ngọc Đức tính toán chính xác ngày, tháng âm lịch và tên năm Can Chi (ví dụ: `AL: 08/07 (Bính Ngọ)`).
- **Web Config Portal & ElegantOTA:** Nhấn nút **BOOT** để phát WiFi AP `MVT-Audio-Setup` cấu hình mạng; hỗ trợ nạp firmware từ xa qua giao diện web ElegantOTA.

### 4. 🎵 Chế độ MP3 PLAYER (Máy phát nhạc từ thẻ nhớ MicroSD)
- **Phát nhạc MP3 chất lượng cao:** Đọc trực tiếp các file nhạc `.mp3` từ thẻ nhớ MicroSD qua giao tiếp SPI tốc độ cao và giải mã bằng thư viện `ESP8266Audio` xuất ra DAC PCM5102A.
- **3 Màn hình giao diện linh hoạt:**
  - **s1 - Player chính:** Hiển thị tên bài hát cuộn chữ (Marquee), thanh thời gian Progress Bar, thời lượng thực tế (`00:00 / 00:00`), số thứ tự bài (`01/20`), icon trạng thái phát (Play/Pause) và mức âm lượng.
  - **s2 - Visualizer:** Màn hình toàn cảnh 68 hiệu ứng sóng âm/phổ tần số nhảy múa theo luồng nhạc MP3 đang phát.
  - **s3 - Playlist Menu:** Menu danh sách bài hát cuộn mượt mà có thanh cuộn (Scrollbar) bên phải và icon nốt nhạc chỉ bài đang phát; chọn bài bằng núm xoay EC11.
- **Ghi nhớ vị trí phát:** Tự động lưu chỉ số bài hát đang nghe vào NVS Flash để phát tiếp khi quay lại chế độ.

### 5. 🕹️ Chế độ GAME CONSOLE (Máy chơi game Retro 8-in-1)
- **Game Engine chuyên biệt:** Tốc độ khung hình mượt mà **35 - 40 FPS** trên màn hình OLED.
- **Menu chọn game trực quan:** Danh sách trò chơi dạng thanh cuộn (Scrollbar), xoay núm EC11 để duyệt và bấm phím để bắt đầu chơi.
- **Bộ sưu tập 8 trò chơi kinh điển:**
  1. **Tetris (Xếp hình cổ điển):** Xoay khối gạch, thả gạch và xóa hàng ghi điểm.
  2. **Sky Fighter (Chiến cơ không chiến):** Điều khiển phi thuyền né đạn, nhặt nâng cấp đạn và đối đầu Boss ngoài không gian.
  3. **Highway Racer (Đua xe Outrun):** Lái xe tốc độ cao trên đường cao tốc né chướng ngại vật.
  4. **Pong (Bóng bàn cổ điển):** Đỡ bóng phản xạ tốc độ cao.
  5. **Brick Breaker (Phá gạch Arkanoid):** Điều khiển thanh trượt đỡ bóng phá vỡ các khối gạch.
  6. **Snake (Rắn săn mồi):** Điều khiển rắn ăn mồi và tăng dần độ dài thân.
  7. **Space Invaders (Bắn quái vật không gian):** Tiêu diệt hạm đội quái vật không gian đang đổ bộ.
  8. **Flappy Bird (Chim bay vượt chướng ngại):** Nhấn phím giữ nhịp cho chim bay luồn qua các cột ống.

---

## 🎮 Hệ thống điều khiển & Phím bấm

| Phím / Núm xoay | Chân GPIO | Chế độ | Thao tác | Chức năng |
| :--- | :---: | :--- | :--- | :--- |
| **PUSH** | **GPIO 4** | Mọi chế độ | Nhấn nhanh | Chuyển tuần hoàn 5 chế độ: `MIC` $\to$ `BT` $\to$ `CLOCK` $\to$ `MP3` $\to$ `GAME` $\to$ `MIC`. |
| **ROTARY ENCODER** | **GPIO 32 / 33** | **BT** | Xoay núm | Tăng / Giảm âm lượng (đồng bộ 2 chiều với điện thoại). |
| | | **MP3 (s1/s2)** | Xoay núm | Tăng / Giảm âm lượng nhạc MP3. |
| | | **MP3 (s3)** | Xoay núm | Cuộn lên / xuống danh sách bài hát trong Playlist. |
| | | **GAME** | Xoay núm | Di chuyển phi thuyền / thanh trượt / tay lái / chọn game trong Menu. |
| **BACK** | **GPIO 13** | **BT** | Nhấn nhanh | **Play / Pause** bài hát trên điện thoại qua Bluetooth AVRCP. |
| | | **MP3 (s1/s2)** | Nhấn nhanh | **Play / Pause** bài hát MP3. |
| | | **MP3 (s1/s2)** | Nhấn giữ (>= 600ms) | Lùi về bài hát trước (**Previous Track**). |
| | | **MP3 (s3)** | Nhấn nhanh | Quay lại màn hình phát trước đó. |
| | | **GAME** | Nhấn nhanh | Quay lại Menu Game / Thoát màn chơi. |
| **PLUS** | **GPIO 14** | **MIC / BT / CLOCK** | Nhấn nhanh | Chuyển hiệu ứng hiển thị tiếp theo (trong 68 hiệu ứng Visualizer). |
| | | **MIC / BT / CLOCK** | Nhấn giữ (> 1s) | Bật / Tắt chế độ **Auto-Cycle** (tự đổi hiệu ứng mỗi 20 giây). |
| | | **MP3 (s1/s2)** | Nhấn nhanh (< 1s) | Mở danh sách **Playlist Menu (s3)**. |
| | | **MP3 (s1/s2)** | Nhấn giữ (1s) | Chuyển sang bài tiếp theo (**Next Track**). |
| | | **MP3 (s1/s2)** | Nhấn giữ (3s) | Chuyển đổi qua lại giữa màn hình **Player (s1)** và **Visualizer (s2)**. |
| | | **MP3 (s3)** | Nhấn nhanh | Chọn và phát bài hát đang bôi đen trong Playlist. |
| | | **GAME** | Nhấn / Giữ | Bắn đạn / Bật nhảy / Xoay khối gạch / Chọn game trong Menu. |
| **BOOT** | **GPIO 0** | **BT** | Nhấn giữ (> 3s) | Xóa thiết bị nhớ và kích hoạt chế độ **BT Re-Pairing**. |
| | | **CLOCK** | Nhấn nhanh / Giữ | Mở WiFi AP cấu hình mạng (`MVT-Audio-Setup`). |

---

## 🔌 Sơ đồ đấu nối phần cứng (Pinout & Wiring)

### 1. Màn hình OLED 1.3" (SH1106 / SSD1306 I²C)
| Chân OLED | ESP32 Pin | Chức năng |
| :--- | :---: | :--- |
| **VCC** | **3.3V** | Nguồn 3.3V |
| **GND** | **GND** | Nối đất |
| **SCL** | **GPIO 22** | I²C Clock |
| **SDA** | **GPIO 21** | I²C Data |

### 2. Cặp Microphone MEMS INMP441 (I²S Stereo Input)
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

### 4. Module đọc thẻ nhớ MicroSD (SPI Interface)
| Chân MicroSD Module | ESP32 Pin | Ghi chú |
| :--- | :---: | :--- |
| **CS** | **GPIO 5** | Chip Select |
| **SCK / CLK** | **GPIO 16** | SPI Clock |
| **MOSI** | **GPIO 17** | Master Out Slave In |
| **MISO** | **GPIO 34** | Master In Slave Out (Input only) |
| **VCC** | **5V / 3.3V** | Tùy loại module thẻ nhớ |
| **GND** | **GND** | Nối đất |

### 5. Núm xoay mã hóa EC11 (Rotary Encoder) & Phím điều khiển
| Linh kiện | ESP32 Pin | Ghi chú |
| :--- | :---: | :--- |
| **Encoder CLK (A)** | **GPIO 32** | Tín hiệu xung A (Internal Pullup) |
| **Encoder DT (B)** | **GPIO 33** | Tín hiệu xung B (Internal Pullup) |
| **Nút PUSH (Núm xoay)** | **GPIO 4** | Nút chuyển Mode (Active LOW) |
| **Nút PLUS (Confirm)** | **GPIO 14** | Nút chuyển Effect / Chọn bài / Thao tác Game (Active LOW) |
| **Nút BACK** | **GPIO 13** | Nút Play/Pause / Lùi bài / Thoát Game (Active LOW) |
| **Nút BOOT** | **GPIO 0** | Nút BOOT trên board ESP32 (Active LOW) |

---

## 🖥️ Danh sách 68 Chế độ Visualizer

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
| **65** | **BEAT METER** | Đo nhịp điệu BPM thời gian thực + 3 dải phổ năng lượng Bass/Mid/Treble + Vòng xung năng lượng. |
| **66** | **OSCILLOSCOPE** | Máy hiện sóng hiện đại với Trigger đồng bộ dạng sóng kép + Lưới chia Graticule chuẩn kỹ thuật. |
| **67** | **CHROMATIC TUNER**| Bộ dò cao độ nhạc cụ (Guitar/Ukulele/Vocal) tự động tính nốt nhạc & độ lệch Cents thời gian thực. |

---

## 🚀 Hướng dẫn biên dịch & Nạp code

### 1. Chuẩn bị môi trường
- Cài đặt **Visual Studio Code** cùng tiện ích mở rộng **PlatformIO IDE**.
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
1. Khi chuyển sang chế độ **CLOCK & WEATHER** lần đầu (hoặc nhấn nút **BOOT** ở chế độ Clock):
   - ESP32 sẽ phát WiFi Access Point: `MVT-Audio-Setup`.
2. Dùng điện thoại kết nối vào WiFi `MVT-Audio-Setup`, giao diện cấu hình sẽ tự động hiển thị (hoặc truy cập `192.168.4.1`).
3. Chọn WiFi gia đình và nhập mật khẩu $\to$ Bấm **Save**.
4. ESP32 sẽ kết nối WiFi, tự động đồng bộ giờ NTP chuẩn xác và cập nhật thời tiết Open-Meteo.

---

## 📄 License & Tác Giả

- **License:** MIT License.
- **Tác giả:** **Power by [Mạc Tân](https://www.facebook.com/mvt.hp.star/)** | Mobile: [0964 335 688](tel:0964335688)

⭐ *Nếu bạn thấy dự án hữu ích, hãy tặng 1 Star trên GitHub nhé!*
