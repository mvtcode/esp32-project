# 🎬 ESP32-S3 2.8" Video Player (320x240 Touch + I2S DMA Codec)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Supported-orange.svg)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20ESP32-blue.svg)](https://github.com/espressif/arduino-esp32)
[![Board](https://img.shields.io/badge/Board-ESP32--S3--2.8%22%20(ES3N28P%20%2F%20ES3C28P)-green.svg)](https://github.com/mvtcode/esp32-project)
[![Display](https://img.shields.io/badge/Display-320x240%20IPS%20(ILI9341V%20%2F%20ST7789)-red.svg)]()
[![Audio Codec](https://img.shields.io/badge/Audio-ES8311%20Codec%20%2B%20FM8002E%20Amp-blueviolet.svg)]()
[![License](https://img.shields.io/badge/License-MIT-brightgreen.svg)]()

Trình phát video Motion JPEG và âm thanh I2S đồng bộ mượt mà trên phần cứng **ESP32-S3 2.8" cảm ứng (độ phân giải 320x240 - ES3C28P / ES3N28P)**. Dự án sử dụng container **AVI All-in-One** chứa cả luồng hình ảnh và âm thanh trong duy nhất 1 file, tối ưu đọc tuần tự từ thẻ nhớ MicroSD FAT32, xuất hình qua màn hình IPS 320x240 và xuất âm thanh qua chuẩn **I2S DMA** tích hợp chip **Audio Codec ES8311** và ampli công suất **FM8002E** kéo loa onboard.

---

## 🌟 Tính Năng Nổi Bật

- ⚡ **Tăng Tốc Phần Cứng Với ESP32-S3**:
  - Vi xử lý **Xtensa LX7 Dual-Core @ 240MHz** có tập lệnh vector **PIE / SIMD** giải mã Motion JPEG trong thời gian thực đạt 20 FPS ổn định.
  - Tận dụng **8MB Octal PSRAM (OPI)** làm bộ đệm streaming và frame buffer lớn, chống hiện tượng drop frame hoặc tràn RAM.
- 🎞️ **Container AVI All-in-One Tối Ưu**:
  - Tích hợp cả **Video (Motion JPEG 320x180 @ 20fps)** và **Audio (PCM WAV 16-bit Mono @ 22.05kHz)** trong 1 file duy nhất `/esp32-video/video.avi`.
  - Đọc tuần tự (sequential streaming) qua DMA, không phải nhảy seek giữa 2 file độc lập, tăng gấp đôi băng thông đọc thẻ MicroSD.
  - Tương thích 100% khi mở trực tiếp trên máy tính bằng VLC Media Player.
- 🔊 **Âm Thanh I2S Hi-Fi Qua Audio Codec ES8311 & Ampli FM8002E**:
  - **ESP32-S3 không có DAC nội**, dự án giao tiếp với chip **Everest Semiconductor ES8311 (I2C 0x18)**:
    - Cấp xung nhịp chuẩn: `MCLK` (GPIO4), `BCLK` (GPIO5), `WS/LRC` (GPIO7), `DOUT` (GPIO8).
    - Tự động bỏ MUTE phần cứng (`Reg 0x31 = 0x00`), mở bộ dao động CSM Slave (`Reg 0x00 = 0x80`), cấu hình bộ chia tần 22.05 kHz và chỉnh volume 0 dB (`Reg 0x32 = 0xBF`).
  - Tự động kích hoạt bộ khuếch đại công suất onboard **FM8002E** (`PIN_PA_ENABLE = GPIO 1`, Active LOW) xuất âm thanh rõ nét qua loa.
- 🧠 **Kiến Trúc FreeRTOS Đa Lõi (Dual-Core Architecture)**:
  - **Core 1**: Tác vụ chính `loop()` chuyên trách giải mã JPEG SIMD, render lên màn hình qua bus SPI 40 MHz và xử lý tương tác UI.
  - **Core 0**: `AudioI2sTask` độc lập đọc dữ liệu từ **RingBuffer 16KB** và đẩy sang I2S DMA, đảm bảo âm thanh không bao giờ bị giật lag khi tải video nặng.
- 🎨 **Giao Diện Cinema Mode Chuẩn 320x240**:
  - **Header (30px)**: Tiêu đề video căn lề trái ($y: 0 \rightarrow 29$).
  - **Video 16:9 (180px)**: $320 \times 180$ chính giữa màn hình ($y: 30 \rightarrow 209$), không bị che khuất.
  - **Footer (30px)**: `Current Time` (trái), `Progress Bar` (giữa), `Total Duration` (phải) ($y: 210 \rightarrow 239$).
  - Nút **▶ Play** ở tâm màn hình tự động biến mất ngay khi phát video để hiển thị trọn vẹn khung hình cinema, chỉ xuất hiện lại khi tạm dừng (Pause).
  - Tự động ẩn toàn bộ overlay sau 1 giây; chạm màn hình (**Tap-to-Wake**) để bật lại trong 1.5 giây.
- 🕹️ **Điều Khiển Linh Hoạt**:
  - Hỗ trợ cảm ứng điện dung FT6336G và nút bấm vật lý **BOOT (GPIO0)**.

---

## 🖥️ Bố Cục Giao Diện Màn Hình (Cinema Mode Layout)

```text
+-------------------------------------------------------------+ y = 0
| [Header 30px]  Tên video (ví dụ: video.avi)                 |
+-------------------------------------------------------------+ y = 30
|                                                             |
|           VÙNG HIỂN THỊ VIDEO MOTION JPEG 16:9              |
|                     (320 x 180 px)                          |
|                                                             |
|         (Nút Play ẩn khi phát, chỉ hiện khi Pause)          |
|                                                             |
+-------------------------------------------------------------+ y = 210
| [Footer 30px]  00:15  [======●-----------------]  00:57     |
+-------------------------------------------------------------+ y = 240
  x = 0                                                       x = 320
```

| Khu vực | Tọa độ $Y$ | Chiều cao | Nội dung hiển thị |
| :--- | :--- | :--- | :--- |
| **Header** | $0 \rightarrow 29$ | $30\text{ px}$ | Tên tệp video căn lề trái, font chữ mượt mà, nền cinema |
| **Video Frame** | $30 \rightarrow 209$ | $180\text{ px}$ | Video tỉ lệ chuẩn 16:9 ($320 \times 180$), không bị che bởi bất kỳ nút bấm nào |
| **Footer** | $210 \rightarrow 239$ | $30\text{ px}$ | `Thời gian hiện tại (trái)` \| `Thanh tiến độ (giữa)` \| `Tổng thời lượng (phải)` |

---

## 🔌 Sơ Đồ Chân GPIO Phần Cứng (Hardware Pinout)

Toàn bộ cấu hình chân GPIO được định nghĩa tập trung tại file [`include/pin_config.h`](include/pin_config.h):

### 1. Màn Hình LCD (ILI9341V / ST7789V - SPI Bus)
| Tên Tín Hiệu | Chân GPIO ESP32-S3 | Ghi Chú |
| :--- | :--- | :--- |
| **TFT MOSI** | `GPIO 11` | Dữ liệu SPI Master Out |
| **TFT MISO** | `GPIO 13` | Dữ liệu SPI Master In |
| **TFT SCLK** | `GPIO 12` | Xung clock SPI (40 MHz) |
| **TFT CS**   | `GPIO 10` | Chip Select màn hình |
| **TFT DC**   | `GPIO 46` | Data / Command (RS) |
| **TFT BL**   | `GPIO 45` | Đèn nền màn hình (Backlight) |
| **TFT RST**  | Tied to RST | Nối trực tiếp chân reset phần cứng |

### 2. Âm Thanh I2S Codec (ES8311 + FM8002E Amp)
| Tên Tín Hiệu | Chân GPIO ESP32-S3 | Ghi Chú |
| :--- | :--- | :--- |
| **I2S MCLK** | `GPIO 4` | Master Clock cấp cho ES8311 Codec |
| **I2S BCLK** | `GPIO 5` | Bit Clock |
| **I2S LRC**  | `GPIO 7` | Word Select (WS / Left-Right Clock) |
| **I2S DOUT** | `GPIO 8` | Data Out từ ESP32 phát sang DAC ES8311 |
| **I2S DIN**  | `GPIO 6` | Data In từ Micro MEMS |
| **I2C SDA**  | `GPIO 16` | Giao tiếp I2C cấu hình codec (Địa chỉ `0x18`) |
| **I2C SCL**  | `GPIO 15` | Giao tiếp I2C clock |
| **PA ENABLE**| `GPIO 1` | Bật bộ khuếch đại FM8002E kéo loa (**Active LOW**) |

### 3. Cảm Ứng Điện Dung (FT6336G) & Thẻ Nhớ SD
| Tên Tín Hiệu | Chân GPIO ESP32-S3 | Ghi Chú |
| :--- | :--- | :--- |
| **TOUCH SDA**| `GPIO 16` | I2C SDA (chung bus với Audio Codec) |
| **TOUCH SCL**| `GPIO 15` | I2C SCL (chung bus với Audio Codec) |
| **TOUCH INT**| `GPIO 17` | Ngắt chạm cảm ứng |
| **TOUCH RST**| `GPIO 18` | Reset cảm ứng |
| **SD CLK / CMD / D0-D3** | `GPIO 38, 40, 39, 41, 48, 47` | Khe cắm thẻ nhớ MicroSD SDIO / SPI |
| **BUTTON BOOT**| `GPIO 0` | Nút bấm cứng BOOT đa năng |
| **LED BUILTIN**| `GPIO 42` | Đèn LED WS2812B onboard |

---

## 🎬 Hướng Dẫn Chuẩn Bị Video

### Bước 1: Chuẩn bị file video và FFmpeg
1. Đặt video nguồn của bạn vào thư mục `tools/` với tên `video.mp4`.
2. Đảm bảo đã có `ffmpeg.exe` trong thư mục `tools/` (hoặc máy tính đã cài đặt sẵn FFmpeg trong PATH).

### Bước 2: Chạy script chuyển đổi tự động
Mở PowerShell tại thư mục gốc của dự án và thực thi:

```powershell
.\tools\video-to-frames.ps1
```

Script sẽ tự động:
- Cắt và scale video về chuẩn **$320 \times 180$ (Tỉ lệ 16:9)**.
- Đặt tốc độ khung hình chuẩn **20 fps**.
- Trích xuất âm thanh sang **PCM 16-bit Mono @ 22.05kHz**.
- Đóng gói đồng bộ thành 1 tệp duy nhất **`tools/out/video.avi`**.

### Bước 3: Copy vào thẻ nhớ MicroSD
1. Định dạng thẻ MicroSD sang chuẩn **FAT32**.
2. Tạo thư mục `esp32-video` và sao chép file `video.avi` vào:

```text
[Thẻ MicroSD FAT32]:/
  └── esp32-video/
        └── video.avi
```

---

## 🚀 Hướng Dẫn Biên Dịch & Nạp Code (PlatformIO)

### 1. Kiểm tra biên dịch (Build Test)
Biên dịch kiểm tra mã nguồn cho ESP32-S3:

```powershell
pio run -e esp32s3
```

### 2. Nạp Firmware (Upload)
Kết nối bo mạch ESP32-S3 qua cổng Type-C vào máy tính và chạy:

```powershell
pio run -e esp32s3 -t upload
```

### 3. Mở Serial Monitor theo dõi trạng thái
```powershell
pio device monitor
```
*(Mẹo: Có thể gộp lệnh: `pio run -e esp32s3 -t upload; pio device monitor`)*

Khi khởi động thành công, Serial Monitor sẽ hiển thị:
```text
[AudioI2sService] I2S DMA đã khởi tạo thành công (MCLK=4, BCLK=5, LRC=7, DOUT=8, Rate=22050Hz)
[AudioI2sService] Phát hiện Audio Codec ES8311 tại I2C 0x18 (Chip ID: 0x8311)
[AudioI2sService] Codec ES8311 sẵn sàng (Reg00=0x80, Reg31=0x00, Reg32=0xBF, Vol=100%)
[AudioI2sService] Đã kích hoạt PIN_PA_ENABLE (GPIO 1, Active LOW - Amp ON)
[AudioI2sService] Đã khởi tạo Audio RingBuffer 16KB
[VideoPlayer] AVI Mode (All-in-One): Frames = 1200, Duration = 60000 ms
```

---

## 🕹️ Cách Thao Tác & Điều Khiển

1. **Khởi động**: Khi bật nguồn và nhận diện thẻ nhớ, Frame 0 được nạp sẵn lên màn hình và dừng ở trạng thái **PAUSED** với nút tròn **▶ Play** ở giữa.
2. **Phát video (Play)**:
   - Nhấn nút cứng **BOOT (GPIO0)** hoặc chạm vào màn hình.
   - Video bắt đầu phát mượt mà 20 FPS, âm thanh phát ra loa onboard. Nút tròn Play lập tức biến mất để không che hình ảnh.
   - Sau **1 giây**, Header và Footer tự động biến mất (Full Cinema Mode).
3. **Hiện điều khiển khi đang xem (Tap to Wake)**:
   - Chạm nhẹ vào màn hình: Header và Footer lập tức hiển thị trong **1.5 giây** để theo dõi thời lượng/tiến độ rồi tự động ẩn lại.
4. **Tạm dừng (Pause)**:
   - Nhấn nút **BOOT (GPIO0)** hoặc chạm màn hình khi thanh điều khiển đang hiện.
   - Video lập tức tạm dừng, nút tròn **▶ Play** xuất hiện trở lại ở tâm màn hình.
5. **Hết video**:
   - Khi chạy hết tệp, video tự động tua lại từ đầu (Frame 0) và dừng ở trạng thái **PAUSED** sẵn sàng cho lần phát tiếp theo.

---

## 📂 Cấu Trúc Thư Mục Dự Án

```text
esp32-project/
├── include/
│   ├── log.h                     # Hệ thống macro logging chuẩn dự án (LOG_I, LOG_D, LOG_E)
│   └── pin_config.h              # Quản lý tập trung toàn bộ chân GPIO phần cứng
├── src/
│   ├── main.cpp                  # Khởi tạo dịch vụ và luồng loop() chính
│   ├── services/
│   │   ├── audio_i2s_service.h   # Driver âm thanh I2S DMA 16-bit & Codec ES8311
│   │   ├── audio_i2s_service.cpp # Cấu hình thanh ghi ES8311, xung MCLK/BCLK, RingBuffer 16KB
│   │   ├── storage_service.h     # Quản lý thẻ nhớ MicroSD với Mutex bảo vệ đa luồng
│   │   ├── storage_service.cpp
│   │   ├── video_player_service.h # Parser AVI, giải mã Motion JPEG SIMD & đồng bộ âm thanh
│   │   └── video_player_service.cpp
│   └── ui/
│       ├── video_ui.h            # Giao diện Cinema Mode (Header, Footer, Center Play Icon)
│       └── video_ui.cpp
├── tools/
│   ├── video-to-frames.ps1       # Script PowerShell convert MP4 -> AVI All-in-One
│   ├── video.mp4                 # Video nguồn mẫu
│   └── out/
│       └── video.avi             # Tệp video AVI thành phẩm
├── platformio.ini                # Cấu hình build flags PlatformIO (TFT_eSPI, OPI PSRAM, CDC)
└── README.md                     # Tài liệu hướng dẫn sử dụng dự án
```

---

## 📜 Giấy Phép (License)

Dự án được phát hành theo giấy phép mã nguồn mở **MIT License**.
