# ESP32-S3 Dual-Channel Sound Visualizer 🎵📊

Dự án thiết bị **hiển thị sóng âm thanh (Sound Visualizer)** thời gian thực trên màn hình OLED 1.3", thu âm từ 2 microphone INMP441 (I²S Stereo), xử lý tín hiệu đa luồng trên 2 nhân vi điều khiển **ESP32-S3 Super Mini** và hỗ trợ nhiều chế độ hiển thị trực quan.

---

## 🌟 Tính năng nổi bật

- **Thu âm Stereo I²S 24-bit:** Sử dụng 2 microphone MEMS **INMP441** (kênh Trái / Phải) chia sẻ chung đường dữ liệu (interleaved), tần số lấy mẫu 16 kHz.
- **Kiến trúc đa nhân FreeRTOS (Dual-Core):**
  - **Core 0 (`mic_task`):** Đọc buffer I²S liên tục, không gây nghẽn luồng xử lý.
  - **Core 1 (`loop`):** Xử lý tín hiệu, Auto Gain Control (AGC), FFT và xuất dữ liệu ra màn hình OLED.
- **Tự động điều chỉnh độ lợi (Auto Gain Control - AGC):** Tự động khuếch đại âm thanh nhỏ và nén nhanh âm thanh lớn (attack/release) riêng biệt cho từng kênh L/R để luôn hiển thị rõ nét mà không bị tràn khung hình.
- **65 chế độ hiển thị đa dạng:** Chuyển đổi linh hoạt giữa 65 chế độ trực quan hóa âm thanh đỉnh cao: sóng âm, phổ tần số, đồng hồ kim vẫy cơ học (Analog VU), visualizer vòng tròn Stereo (Circle MVT), visualizer trái tim (MVT Heart), visualizer không gian mạng kết hợp chữ "MVT" xoay động (MVT Fusion & MVT Cyber), băng Cassette cổ điển (MVT Cassette), đường hầm 3D (MVT Tunnel), quỹ đạo nguyên tử (MVT Orbit), mưa ma trận (MVT Matrix), thác nước địa hình 3D (MVT Terrain), trái tim ma trận (Heart Matrix), đôi trái tim kết nối sóng âm (Twin Hearts), bàn DJ ảo (MVT DJ Deck), loa thùng bass đập (MVT Speaker), tai nghe chụp đầu studio (MVT Headphone), mạng nhện 3D (MVT Spiderweb), thành phố Synthwave 80s (MVT Synthwave), radar/sonar quét 360° (MVT Radar), lõi Arc Reactor phóng sét (MVT Reactor), hố đen vũ trụ hút bụi sao (MVT Blackhole), đua xe đêm Cyber Highway (MVT Highway), loa biểu tượng phát sóng MVT Sound, sóng âm xoay 360 độ 20s/vòng (MVT Rotate), 3 tia laser bay va đập cạnh (MVT Bounce), dàn laser sân khấu quét 2 vị trí 1/3 & 2/3 (MVT Laser), vũ công chibi nhảy theo nhịp nhạc (MVT Dancer), tâng bóng vật lý trên dải sóng âm (MVT Juggle), máy đo góc pha Audio Vectorscope (MVT Vectorscope), hoa văn bánh răng Spirograph (MVT Spirograph), đa giác biến hình Superformula (MVT Superform), hệ động lực hỗn loạn cánh bướm Lorenz 3D (MVT Lorenz 3D), vòng tròn sóng năng lượng Polar Wave (MVT Polar Wave), đĩa rung cộng hưởng sóng Chladni Cymatics (MVT Chladni), siêu lập phương không gian 4D Tesseract (MVT Tesseract), thác phổ SDR Waterfall đối xứng (MVT Waterfall), chữ 3D bay xuyên không gian Warp Tunnel (MVT Warp Text), hạt ma thuật ghép chữ động Kinetic Particle (MVT Particle), bước nhảy không gian 3D Warp Starfield (MVT Starfield), quả cầu 3D xung động (MVT Sphere 3D), vòng xuyến bánh donut 3D (MVT Torus 3D), chuỗi xoắn kép sinh học (MVT DNA Helix), khối lập phương lồng nhau phát nổ 3D (MVT Cubes 3D), dải ngân hà xoắn ốc 3D (MVT Galaxy 3D), kim cương tinh thể đa diện phóng tia năng lượng (MVT Crystal 3D), đường hầm ống sóng âm 3D (MVT Cylinder), trái tim tham số 3D gợn sóng âm (MVT Heart 3D), chữ MVT 3D khối đặc xoay đa trục (MVT Text 3D), hệ Mặt Trời 3D (Solar System), vụ nổ siêu tân tinh (MVT Supernova), bão sấm sét Cyber (Thunderstorm), buồng lái phi thuyền Sci-Fi (Cyber Cockpit), game arcade bắn quái không gian (Space Invaders), chim bay Flappy Beat, Pac-Man ngoạm sóng âm (Pac-Beat), khủng long vượt xương rồng T-Rex (Dino Runner), robot Xiaozhi AI biểu cảm, trợ lý robot AI Robot, quả cầu Plasma ma thuật, sân khấu đèn LED Zigzag EDM, và mèo máy Cyber Neko.
- **Chuyển chế độ linh hoạt & Auto-Cycle:** 
  - **Nhấn nhả (Short-press)** nút **BOOT (GPIO 0)** để chuyển chế độ thủ công.
  - **Nhấn giữ (Long-press)** nút BOOT (> 1 giây) để bật/tắt chế độ **Auto-Cycle** (tự động chuyển hiệu ứng mỗi 20 giây, kèm thông báo ON/OFF).
- **Hiệu năng cao, độ trễ thấp:** Tốc độ làm mới mượt mà (≥ 25–30 FPS), độ trễ âm thanh < 10ms (nhờ queue RTOS non-blocking). Tối ưu hóa thuật toán tính toán FFT dùng chung (Centralized Audio Analysis) giúp tiết kiệm tài nguyên CPU.

---

## 🖥️ 65 Chế độ hiển thị (Display Modes)

|  Mode  | Tên chế độ        | Mô tả                                                                                                      |
| :----: | :---------------- | ---------------------------------------------------------------------------------------------------------- |
| **0**  | **WAVEFORM**      | Dạng sóng 2 kênh stereo độc lập (Kênh L: nửa trên, Kênh R: nửa dưới, có đường phân cách).                  |
| **1**  | **MIRROR**        | Sóng âm đối xứng từ tâm màn hình ra 2 phía (Mono mix L+R).                                                 |
| **2**  | **SPECTRUM**      | Phổ tần số âm thanh (FFT 128 điểm → 64 cột tần số thời gian thực).                                         |
| **3**  | **LISSAJOUS**     | Đồ thị pha X=Left, Y=Right trực quan hóa độ lệch pha và trường stereo (stereo image).                      |
| **4**  | **VU METER**      | Đồng hồ đo cường độ âm lượng (2 cột dọc L/R) kèm tính năng giữ đỉnh (**Peak Hold**) và decay mượt mà.      |
| **5**  | **ANALOG VU**     | Đồng hồ VU kim vẫy cơ học (2 mặt L/R, thang đo chuẩn) + 2 cuộn băng cối quay tối giản nối dây đỉnh.        |
| **6**  | **CIRCLE MVT**    | Visualizer hình tròn tỏa 40 dải phổ Stereo x1.5, chữ **"MVT"** lớn ở tâm + vòng năng lượng đập theo bass.  |
| **7**  | **MVT HEART**     | Trái tim ❤️ ở trung tâm đập theo nhịp bass, chữ **"MVT"** ở dưới, 2 dải sóng âm đối xứng L & R 2 bên.      |
| **8**  | **MVT FUSION**    | Vòng tròn tâm có chữ **"MVT" xoay 10s/vòng** (tăng tốc theo bass), tia phổ tỏa đỉnh/đáy, 2 cánh sóng L/R.  |
| **9**  | **MVT CYBER**     | Cột bar ngang 2 bên nháy bắn vào tâm, vòng tròn tâm chữ **"MVT" xoay 10s/vòng** + 3 vòng radar công nghệ.  |
| **10** | **MVT CASSETTE**  | Băng Cassette cổ điển với 2 bánh xe cuộn băng quay tròn + cửa sổ sóng âm thời gian thực ở giữa.            |
| **11** | **MVT TUNNEL**    | Đường hầm không gian 3D phóng nhanh ra phía trước + bung nở theo nhịp Bass giật mạnh.                      |
| **12** | **MVT ORBIT**     | Quỹ đạo nguyên tử 3D với 3 vòng elip nghiêng + các hạt electron quay quanh hạt nhân "MVT".                 |
| **13** | **MVT MATRIX**    | Cơn mưa kỹ thuật số Matrix 16 cột rơi toàn màn hình theo 16 dải tần số FFT.                                |
| **14** | **MVT TERRAIN**   | Thác nước phổ tần số 3D (3D Spectrum Waterfall Mesh) cuộn trôi liên tục theo thời gian.                    |
| **15** | **HEART MATRIX**  | Mưa ma trận kỹ thuật số rơi xung quanh **trái tim rỗng có chữ "MVT" phóng to** theo cường độ âm thanh.     |
| **16** | **TWIN HEARTS**   | 2 trái tim đặc 2 bên (L & R) đập phập phồng độc lập + **dây kết nối ở giữa là sóng âm Waveform**.          |
| **17** | **MVT DJ DECK**   | Bàn DJ Mixer 2 mâm đĩa xoay vinyl có kim đọc + thanh fader gạt theo kênh L/R + mini oscilloscope.          |
| **18** | **MVT SPEAKER**   | Cặp loa thùng Hi-Fi với màng loa Woofer phập phồng cực mạnh theo Bass và phát ra các vòng sóng âm.         |
| **19** | **MVT HEADPHONE** | Tai nghe studio over-ear ở giữa + 2 bên tai nghe bắn ra dải sóng phổ tần số Stereo sang 2 mép màn hình.    |
| **20** | **MVT SPIDERWEB** | Mạng nhện hình học 3D 8 góc, các tầng mạng nhện co giãn dợn sóng theo tần số và bung nở theo bass.         |
| **21** | **MVT SYNTHWAVE** | Phong cách 80s với mặt trời neon rung nhịp + mặt đất lưới 3D trôi tới + dãy skyline FFT ở chân trời.       |
| **22** | **MVT RADAR**     | Màn hình Radar/Sonar quét 360 độ, phát hiện các "mục tiêu" tần số âm thanh và để lại vệt quét sáng.        |
| **23** | **MVT REACTOR**   | Lõi lò phản ứng Arc Reactor với các vành cơ khí xoay ngược chiều và phóng tia chớp năng lượng khi âm lớn.  |
| **24** | **MVT BLACKHOLE** | Hố đen vũ trụ với đĩa bồi tụ xoay tròn hút bụi sao vào tâm và bắn tia plasma phản lực khi có Bass.         |
| **25** | **MVT HIGHWAY**   | Đua xe đêm Synthwave với góc nhìn đuôi xe lắc lư theo stereo + đường cao tốc 3D và cột đèn dải tần số.     |
| **26** | **MVT SOUND**     | Loa phóng thanh Iconic Megaphone + sóng âm thanh bung nở hình cánh quạt + luồng hạt bắn rực rỡ.            |
| **27** | **MVT ROTATE**    | Sóng âm Waveform với 2 điểm neo xoay đều bám sát biên độ màn hình theo chu kỳ **20s/vòng**.                |
| **28** | **MVT BOUNCE**    | 3 tia line dài 10-15px bay tự do va đập nảy 4 cạnh màn hình + vệt đuôi sau lưng + tăng tốc theo nhạc.      |
| **29** | **MVT LASER**     | Dàn đèn laser sân khấu 2 tia (1/3 và 2/3 đỉnh) quét chùm tia đan chéo + đốm sáng sàn theo nhịp nhạc.       |
| **30** | **MVT DANCER**    | Vũ công chibi nhảy cực sung: gật đầu headbob, nhún squat theo bass và quẩy tay theo 2 kênh stereo L/R.     |
| **31** | **MVT JUGGLE**    | Tâng bóng vật lý trên dải sóng âm thời gian thực: bóng nảy cao theo độ mạnh của sóng và đổi hướng dốc.     |
| **32** | **MVT VECTORSCOPE** | Máy đo góc pha âm thanh Goniometer xoay 45° chuẩn phòng thu, lưới trục M/S (+M, ±S) & trace vector Stereo.  |
| **33** | **MVT SPIROGRAPH** | Hoa văn bánh răng toán học Hypotrochoid nở rộ theo Bass/Treble, biến đổi số cánh và xoay mượt mà.         |
| **34** | **MVT SUPERFORM** | Đa giác biến hình sinh học Superformula uốn lượn liên tục + sóng âm Stereo rung động chạy dọc viền.        |
| **35** | **MVT LORENZ 3D** | Hệ động lực hỗn loạn cánh bướm Lorenz 3D phóng to x1.5, vệt quỹ đạo 96 điểm xoay 3D theo nhịp nhạc.       |
| **36** | **MVT POLAR WAVE** | Vòng tròn sóng năng lượng Arc Reactor, vòng sóng âm tròn bao quanh lõi năng lượng phát xung nhịp.           |
| **37** | **MVT CHLADNI**   | Hiện tượng Cymatics đĩa rung Chladni, các hạt vật chất tự động dồn về đường nút sóng theo tần số âm thanh. |
| **38** | **MVT TESSERACT** | Khung dây siêu lập phương 4 chiều (4D Hypercube) nhào lộn xoay trong không gian 4D và chiếu phối cảnh 2D.  |
| **39** | **MVT WATERFALL** | Thác phổ Spectrogram SDR đối xứng cánh bướm (Stereo Butterfly): Bass cực đại ở tâm giữa màn hình, dạt về 2 bên. |
| **40** | **MVT WARP TEXT** | Chữ 3D (`L,O,V,E,M,A,C,T,A,N`, `LOVE`, `MAC TAN`) bay vút từ tâm xuyên đường hầm vũ trụ 3D Hyperspace Warp. |
| **41** | **MVT PARTICLE**  | 96 hạt ma thuật Kinetic tự do bay lượn rồi tự động gom tụ ghép thành từng chữ `LOVE` $\to$ `MAC` $\to$ `TAN` $\to$ ❤️ rồi nổ tung theo Bass. |
| **42** | **MVT STARFIELD** | Không gian sao 3D 75 hạt bay từ tâm vô cực; khi có beat/bass giật mạnh, sao kéo dài thành **vệt laser Warp Speed**. |
| **43** | **MVT SPHERE 3D** | Quả cầu khung dây 3D xoay 2 trục $(X,Y)$; các vòng kinh/vĩ tuyến rung sóng và phập phồng co giãn theo âm thanh. |
| **44** | **MVT TORUS 3D**  | Vòng xuyến bánh Donut 3D xoay lộn vòng trong không gian 3D; thân ống donut gợn sóng nhấp nhô theo waveform live. |
| **45** | **MVT DNA HELIX** | Chuỗi xoắn kép 3D DNA xoay quanh trục; các thanh liên kết base-pairs nảy nhịp theo dải phổ âm thanh (Bass $\to$ Treble). |
| **46** | **MVT CUBES 3D**  | 2 khối lập phương wireframe 3D lồng nhau xoay ngược chiều; bung nổ mở rộng (Explode) khi đánh bass cực đại. |
| **47** | **MVT GALAXY 3D** | Dải ngân hà xoắn ốc 3D hạ thấp tâm, tia năng lượng cực 3D (Relativistic Jets) phóng cao theo bass.        |
| **48** | **MVT CRYSTAL 3D**| Khối 20 mặt Icosahedron 3D xoay tự do; phóng ra các tia sáng năng lượng từ 12 đỉnh khi có âm thanh lớn. |
| **49** | **MVT CYLINDER**  | Đường hầm ống trụ tròn 3D nhìn chiều sâu; bề mặt lưới biến dạng uốn lượn theo dữ liệu waveform trực tiếp. |
| **50** | **MVT HEART 3D**  | Trái tim tham số không gian 3D (Parametric Heart 3D Mesh) xoay 3 trục; thân trái tim gợn sóng nhấp nhô theo waveform live và đập theo nhịp bass. |
| **51** | **MVT TEXT 3D**   | Khối chữ "MVT" 3D extruded wireframe xoay lộn vòng không gian 3D; độ dày và đỉnh biến dạng theo âm nhạc.  |
| **52** | **SOLAR SYSTEM**  | Hệ Mặt Trời 3D: Mặt trời trung tâm phát tia nhật hoa theo bass + 4 hành tinh quay quỹ đạo nghiêng.        |
| **53** | **MVT SUPERNOVA** | Vụ nổ siêu tân tinh: Sóng xung kích vũ trụ giãn nở chậm rãi, 40 hạt plasma trôi dạt và mây tinh vân xoay.  |
| **54** | **THUNDERSTORM**  | Bão sấm sét Cyber: Tia sét fractal phóng giật từ trên xuống theo beat, mưa rơi Stereo và sóng biển dưới đáy. |
| **55** | **CYBER COCKPIT** | Buồng lái phi thuyền vũ trụ HUD: Radar Stereo 2 bên, lò phản ứng trung tâm và sao 3D bay tốc độ ánh sáng. |
| **56** | **SPACE INVADERS**| Game Arcade bắn quái không gian: Phi thuyền di chuyển tránh đạn theo Stereo, bắn chùm laser phá tàu mẹ khi có bass. |
| **57** | **FLAPPY BEAT**   | Game chim bay Flappy Beat: Chú chim vỗ cánh theo âm lượng RMS, luồn lách qua các cột chướng ngại vật phổ FFT. |
| **58** | **PAC-BEAT**      | Game Pac-Man: Pac-Man há miệng đớp sóng âm Waveform, ăn chấm năng lượng và săn ma khi nhạc bùng nổ.        |
| **59** | **DINO RUNNER**   | Game khủng long T-Rex pixel tự động nhảy qua các bụi cây xương rồng theo từng nhịp trống bass kick.         |
| **60** | **XIAOZHI AI**    | Khuôn mặt robot thông minh Xiaozhi: Đôi mắt OLED biểu cảm (chớp mắt, nhìn quanh) + miệng nói sóng âm live. |
| **61** | **AI ROBOT**      | Người bạn robot AI: Đầu nhún nhảy theo nhạc, 2 tai là 2 cột phổ tần số Stereo và anten phát sóng radio.    |
| **62** | **PLASMA BALL**   | Quả cầu Plasma ma thuật: Lõi điện cực trung tâm phóng các nhánh tia sét uốn lượn va chạm bề mặt quả cầu.  |
| **63** | **LED ZIGZAG**    | Sân khấu LED Zigzag EDM: Dàn đèn chữ V đa tầng nhấp nháy đuổi theo nhịp điệu sân khấu sôi động.           |
| **64** | **CYBER NEKO**    | Mèo máy tương lai Cyber Neko: Đôi tai cử động theo Stereo, miệng phát sóng âm nói chuyện và đuôi ngoe nguẩy. |

> 💡 **Cách chuyển chế độ:** Nhấn (click) nút **BOOT** trên board ESP32-S3 Super Mini để chuyển tuần hoàn giữa 65 chế độ. Tên chế độ mới sẽ tự động hiển thị overlay.
> 💡 **Auto-Cycle Mode:** **Nhấn giữ nút BOOT** > 1 giây để bật/tắt chế độ tự động chuyển. Mạch sẽ tự động đổi hiệu ứng mỗi 20 giây. Khi đang bật Auto-Cycle, nếu bạn bấm nút thủ công, mạch sẽ chuyển ngay sang hiệu ứng mới và đếm lại 20 giây từ đầu.
---

## 🛠 Phần cứng (Hardware Components)

| Linh kiện            | Thông số / Model       | Giao tiếp                 | Ghi chú                                         |
| -------------------- | ---------------------- | ------------------------- | ----------------------------------------------- |
| **Vi điều khiển**    | ESP32-S3 Super Mini    | —                         | Dual-core Xtensa LX7, 240MHz, 4MB Flash, Type-C |
| **Màn hình**         | OLED 1.3 inch (SH1106) | I²C (800 kHz)             | Độ phân giải 128×64 pixels, monochrome          |
| **Microphone Left**  | INMP441 (Kênh Trái)    | I²S Stereo (Slot WS=LOW)  | L/R nối GND                                     |
| **Microphone Right** | INMP441 (Kênh Phải)    | I²S Stereo (Slot WS=HIGH) | L/R nối 3V3                                     |

---

## 🔌 Sơ đồ đấu nối (Pinout & Wiring)

### 1. Màn hình OLED 1.3" (SH1106 I²C)

| Chân OLED | ESP32-S3 Pin | Màu dây gợi ý | Chức năng  |
| :-------- | :----------- | :------------ | :--------- |
| **VCC**   | **3.3V**     | Đỏ            | Nguồn 3.3V |
| **GND**   | **GND**      | Đen           | Nối đất    |
| **SCL**   | **GPIO 9**   | Vàng          | I²C Clock  |
| **SDA**   | **GPIO 8**   | Xanh lá       | I²C Data   |

### 2. Hai Microphone INMP441 (I²S Stereo)

Hai microphone **chia sẻ chung 3 đường SCK, WS, SD** với ESP32-S3, phân biệt kênh bằng chân **L/R**:

| Chân INMP441        | ESP32-S3 Pin | Ghi chú                                   |
| :------------------ | :----------- | :---------------------------------------- |
| **SCK** (cả 2 mic)  | **GPIO 4**   | I²S Bit Clock (BCLK)                      |
| **WS** (cả 2 mic)   | **GPIO 5**   | I²S Word Select / LRCLK                   |
| **SD** (cả 2 mic)   | **GPIO 6**   | I²S Serial Data (chung đường data stereo) |
| **VDD** (cả 2 mic)  | **3.3V**     | Nguồn 3.3V                                |
| **GND** (cả 2 mic)  | **GND**      | Nối đất                                   |
| **L/R** (Mic Left)  | **GND**      | Kênh Trái (phát ở slot WS = LOW)          |
| **L/R** (Mic Right) | **3.3V**     | Kênh Phải (phát ở slot WS = HIGH)         |

### 3. Nút bấm chuyển chế độ

- Sử dụng nút **BOOT (GPIO 0)** tích hợp sẵn trên board ESP32-S3 Super Mini (Active LOW, có sẵn internal pull-up, không cần hàn thêm linh kiện).

---

## 🏗 Kiến trúc hệ thống (Software Architecture)

```text
[INMP441 Left ] ──┐ (I2S Stereo)
                  ├──> [Core 0: mic_task] ──> [FreeRTOS Queue]
[INMP441 Right] ──┘                                 │
                                                    ▼
[BOOT Button  ] ──────────────────────────> [Core 1: loop()]
                                                    │
                                                    ▼
                                            [Signal Processing]
                                            - Per-channel AGC
                                            - FFT Spectrum Analysis
                                            - Peak & RMS Detection
                                            - Dynamic Display Router
                                                    │
                                                    ▼
                                            [SafeDraw Clipping Layer]
                                                    │
                                                    ▼
                                            [OLED Driver (U8g2)]
                                            (128x64 SH1106 @ 800kHz)
```

---

## 📂 Cấu trúc thư mục (Project Structure)

```text
esp32-project/
├── platformio.ini       # Cấu hình PlatformIO, board lolin_s3_mini & thư viện
├── requirement.md       # Tài liệu yêu cầu kỹ thuật & tiến độ các phase
├── README.md            # Tài liệu hướng dẫn sử dụng dự án
├── monitor.sh           # Script mở Serial Monitor tiện lợi
├── upload.sh            # Script build & upload firmware
└── src/
    ├── main.cpp         # Khởi tạo hệ thống, FreeRTOS queue & dual-core task
    ├── i2s_mic.h/.cpp   # Driver I2S đọc dữ liệu stereo từ cặp mic INMP441
    ├── display.h/.cpp   # Render engine U8g2, AGC, FFT và điều phối chế độ
    ├── button.h/.cpp    # Xử lý nút bấm BOOT (GPIO 0) với debounce chống rung
    └── effects/         # Mô-đun 65 hiệu ứng visualizer chuyên biệt
        ├── safe_draw.h/.cpp        # Lớp đồ họa bảo vệ chống tràn biên màn hình
        ├── effects.h/.cpp          # Bảng tra cứu điều phối VisualEffect
        ├── effect_common.h/.cpp    # FFT & AGC buffer dùng chung
        ├── effect_waveform.cpp     # Mode 0: Waveform Stereo
        ├── effect_circle.cpp       # Mode 6: Circle MVT Stereo 40-band
        ├── effect_chladni.cpp      # Mode 37: MVT Chladni
        ├── effect_tesseract.cpp    # Mode 38: MVT Tesseract
        └── effect_waterfall.cpp    # Mode 39: MVT Waterfall
```

---

## 🚀 Hướng dẫn cài đặt & Chạy ứng dụng

### 1. Chuẩn bị môi trường

- Cài đặt **Visual Studio Code** cùng extension **PlatformIO IDE**.
- Hoặc sử dụng **PlatformIO Core (CLI)**.

### 2. Biên dịch và nạp firmware

Mở terminal tại thư mục dự án và thực hiện:

```bash
# 1. Biên dịch dự án
pio run

# 2. Nạp code lên ESP32-S3
pio run --target upload

# Hoặc chạy script có sẵn (Linux / macOS / Git Bash)
./upload.sh
```

### 3. Theo dõi Serial Monitor

Kiểm tra log hệ thống, FPS và trạng thái chuyển đổi chế độ:

```bash
pio device monitor

# Hoặc
./monitor.sh
```

_Baud rate mặc định: `115200`._

---

## 📊 Trạng thái phát triển (Development Status)

|    Phase    | Nội dung                                                            |  Trạng thái   |
| :---------: | :------------------------------------------------------------------ | :-----------: |
| **Phase 1** | Cấu hình I²S driver, đọc dữ liệu stereo từ 2 mic INMP441            | ✅ Hoàn thành |
| **Phase 2** | Hiển thị dạng sóng Waveform cơ bản 2 kênh lên OLED                  | ✅ Hoàn thành |
| **Phase 3** | Tích hợp Auto Gain Control (AGC) độc lập từng kênh (attack/release) | ✅ Hoàn thành |
| **Phase 4** | Thêm 4 chế độ nâng cao (Mirror, Spectrum FFT, Lissajous, VU Meter)  | ✅ Hoàn thành |
| **Phase 5** | Nút bấm chuyển chế độ (GPIO 0), Overlay UI tên chế độ & Tối ưu FPS  | ✅ Hoàn thành |
| **Phase 6** | Trọn bộ 65 chế độ visualizer sáng tạo & kiến trúc mô-đun hóa        | ✅ Hoàn thành |
| **Phase 7** | Tối ưu hóa hiệu năng (Gộp chung FFT), Non-blocking Queue & Auto-Cycle Mode (Giữ nút BOOT) | ✅ Hoàn thành |

---

## 🔧 Xử lý sự cố thường gặp (Troubleshooting)

1. **Màn hình OLED không hiển thị:**
   - Kiểm tra kỹ chân SDA (GPIO 8) và SCL (GPIO 9).
   - Đảm bảo nguồn cấp 3.3V và GND tiếp xúc tốt.
2. **Lỗi `[FATAL] I2S INIT FAIL` trên Serial Monitor:**
   - Kiểm tra lại các kết nối chân SCK (GPIO 4), WS (GPIO 5), SD (GPIO 6).
   - Đảm bảo 2 micro INMP441 đều được cấp nguồn 3.3V và GND.
3. **Kênh Left / Right bị đảo ngược:**
   - Đổi lại chân L/R trên micro: Mic Left nối GND, Mic Right nối 3.3V.
4. **Không nạp được code qua cổng Type-C:**
   - Nhấn giữ nút **BOOT**, bấm nút **RESET (RST)** một lần, sau đó thả nút **BOOT** để đưa ESP32-S3 vào chế độ ROM Bootloader trước khi nạp.

---

## 🤝 Đóng Góp

Mọi đóng góp đều được chào đón! Hãy tạo Pull Request hoặc mở Issue nếu bạn có ý tưởng cải thiện.

## 📄 License

MIT License - Tự do sử dụng và chỉnh sửa cho mục đích cá nhân và thương mại.

## 👨‍💻 Tác Giả

**Power by [Mạc Tân](https://www.facebook.com/mvt.hp.star/)** | Mobile: [0964 335 688](tel:0964335688)

---

⭐ Nếu dự án này hữu ích, hãy cho một star trên GitHub!
