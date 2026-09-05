# 🎬 ESP32 CYD 3.5" Video Player (ST7796 + Internal DAC)

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Supported-orange.svg)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino%20ESP32-blue.svg)](https://github.com/espressif/arduino-esp32)
[![Board](https://img.shields.io/badge/Board-ESP32--3248S035%20(CYD%203.5")-green.svg)](https://github.com/mvtcode/esp32-project)
[![Display](https://img.shields.io/badge/Display-ST7796%20480x320-red.svg)]()
[![License](https://img.shields.io/badge/License-MIT-brightgreen.svg)]()

Trình phát video Motion JPEG và âm thanh WAV đồng bộ mượt mà trên phần cứng **ESP32 Cheap Yellow Display (CYD) 3.5" (ESP32-3248S035)**. Dự án sử dụng định dạng **AVI All-in-One** chứa cả luồng hình ảnh và âm thanh trong duy nhất 1 file, tối ưu đọc tuần tự từ thẻ nhớ MicroSD FAT32, xuất hình qua màn hình ST7796 480x320 (HSPI 55MHz) và xuất âm thanh qua DAC nội ESP32 (GPIO26).

---

## 🌟 Tính Năng Nổi Bật

- 🎞️ **Container AVI All-in-One**:
  - Tích hợp cả **Video (Motion JPEG 480x270 @ 20fps)** và **Audio (PCM WAV 16-bit Mono @ 22.05kHz)** trong 1 file duy nhất `/esp32-video/video.avi`.
  - Tương thích 100% với VLC Media Player và các trình phát media chuẩn trên máy tính.
  - Đọc tuần tự mượt mà trên thẻ nhớ MicroSD, không bị trễ hay nghẽn seek file riêng biệt.
- 🎨 **Giao diện Cinema Mode Hiện Đại**:
  - Phân vùng màn hình thông minh $480 \times 320$: **Header (20px)** trên cùng, **Video 16:9 (270px)** chính giữa và **Footer (30px)** dưới cùng.
  - Toàn bộ thanh tiêu đề và điều khiển nằm ngoài vùng hiển thị video, bảo toàn $100\%$ khung hình gốc, không che khuất nội dung phim.
  - **Tự động ẩn (Auto-hide)** sau 1 giây khi phát video; chạm nhẹ màn hình để **đánh thức giao diện (Tap-to-Wake)** trong 1.5 giây.
  - Biểu tượng nút **Play ▶ / Pause ❚❚** lớn với hiệu ứng mờ chính giữa màn hình khi tạm dừng.
- 🔊 **Âm Thanh I2S DAC Tự Nhiên & Chống Tiếng "Bộp"**:
  - Sử dụng DAC nội 8-bit của ESP32 xuất ra chân **GPIO26** tích hợp trên kit CYD.
  - Bộ đệm DMA I2S dung lượng lớn **371 ms** (16 buffers × 512 samples) loại bỏ triệt để hiện tượng buffer underflow (tiếng "bộp" hoặc giật cục âm thanh).
- ⏱️ **Đồng Bộ A/V Bằng Nhịp Phần Cứng (Hardware Pace Locking)**:
  - Tốc độ xả mẫu âm thanh của phần cứng I2S DAC tự nhiên làm thước đo điều tiết tốc độ khung hình ($1.0\times$), đảm bảo hình ảnh và âm thanh không bao giờ bị lệch pha hay trôi tiếng.
- 🕹️ **Điều Khiển Linh Hoạt**:
  - Hỗ trợ cả nút bấm cứng **BOOT (GPIO0)** (chống rung phím 350ms) lẫn **Cảm ứng điện trở XPT2046** trên màn hình.
  - Khởi động mặc định ở trạng thái **PAUSED** tại Frame 0; khi xem hết video tự động tua về đầu và tạm dừng chờ lệnh tiếp theo.
- 🛠️ **Công Cụ Tiện Lợi**:
  - Đi kèm script PowerShell `tools/video-to-frames.ps1` (sử dụng FFmpeg) tự động convert từ mọi video MP4 sang chuẩn AVI tối ưu cho ESP32.

---

## 🖥️ Bố Cục Giao Diện Màn Hình (Cinema Mode Layout)

```text
+-------------------------------------------------------------+ y = 0
| [Header 20px]  Tên video (ví dụ: video.avi)                 |
+-------------------------------------------------------------+ y = 20
|                                                             |
|                                                             |
|           VÙNG HIỂN THỊ VIDEO MOTION JPEG 16:9              |
|                     (480 x 270 px)                          |
|                                                             |
|                    [ ▶ Nút Play / Pause ]                   |
|                                                             |
|                                                             |
+-------------------------------------------------------------+ y = 290
| [Footer 30px]  00:15  [======●-----------------]  00:57     |
+-------------------------------------------------------------+ y = 320
  x = 0                                                       x = 480
```

| Khu vực | Tọa độ $Y$ | Chiều cao | Nội dung hiển thị |
| :--- | :--- | :--- | :--- |
| **Header** | $0 \rightarrow 19$ | $20\text{ px}$ | Tên tệp video căn lề trái, nền đen tuyền cinema |
| **Video Frame** | $20 \rightarrow 289$ | $270\text{ px}$ | Video tỉ lệ chuẩn 16:9 ($480 \times 270$), tâm nút Play/Pause tại $(240, 155)$ |
| **Footer** | $290 \rightarrow 319$ | $30\text{ px}$ | `Thời gian hiện tại (trái)` \| `Thanh tiến độ Seek bar (giữa)` \| `Tổng thời lượng (phải)` |

---

## 🔌 Sơ Đồ Chân GPIO (Hardware Pinout)

Toàn bộ cấu hình chân GPIO được quản lý tập trung tại file [`include/pin_config.h`](include/pin_config.h):

| Chức năng | Chân GPIO | Ghi chú phần cứng kit CYD 3.5" |
| :--- | :--- | :--- |
| **TFT MOSI** | `GPIO13` | Bus HSPI tốc độ cao (55 MHz) |
| **TFT MISO** | `GPIO12` | Bus HSPI |
| **TFT SCLK** | `GPIO14` | Bus HSPI |
| **TFT CS** | `GPIO15` | Chip Select màn hình ST7796 |
| **TFT DC** | `GPIO2` | Data / Command |
| **TFT BL** | `GPIO27` | Đèn nền màn hình (Backlight) |
| **TOUCH CS** | `GPIO33` | Chip Select cảm ứng XPT2046 |
| **SD MOSI** | `GPIO23` | Bus VSPI độc lập (20 MHz) |
| **SD MISO** | `GPIO19` | Bus VSPI |
| **SD SCLK** | `GPIO18` | Bus VSPI |
| **SD CS** | `GPIO5` | Chip Select khe cắm thẻ MicroSD |
| **AUDIO DAC** | `GPIO26` | Kênh phải (Right Channel) DAC nội 8-bit ESP32 ra loa |
| **BUTTON BOOT**| `GPIO0` | Nút bấm vật lý BOOT (Active LOW) |

---

## 🎬 Hướng Dẫn Chuẩn Bị Video

ESP32 không hỗ trợ giải mã phần cứng chuẩn nén MP4 (H.264), do đó cần chuyển đổi video trước bằng công cụ đi kèm:

### Bước 1: Chuẩn bị file video và FFmpeg
1. Đặt video nguồn của bạn vào thư mục `tools/` với tên `video.mp4`.
2. Đảm bảo đã có `ffmpeg.exe` trong thư mục `tools/` hoặc máy đã cài đặt FFmpeg toàn cục.

### Bước 2: Chạy script chuyển đổi
Mở PowerShell tại thư mục gốc của dự án và chạy:

```powershell
.\tools\video-to-frames.ps1
```

Script sẽ tự động:
- Cắt/scale video về đúng độ phân giải **$480 \times 270$ (16:9)**.
- Đặt tốc độ khung hình chuẩn **20 fps**.
- Trích xuất và mã hóa âm thanh sang **PCM 16-bit Mono @ 22.05kHz**.
- Đóng gói toàn bộ thành file **`tools/out/video.avi`**.

### Bước 3: Copy vào thẻ nhớ MicroSD
1. Định dạng thẻ nhớ MicroSD sang chuẩn **FAT32**.
2. Tạo thư mục `esp32-video` trên thẻ nhớ và copy file vào theo cấu trúc:

```text
[Thẻ MicroSD]:/
  └── esp32-video/
        └── video.avi
```

---

## 🚀 Hướng Dẫn Biên Dịch & Nạp Code (PlatformIO)

### 1. Kiểm tra biên dịch (Build Test)
Biên dịch kiểm tra mã nguồn để đảm bảo không có lỗi cú pháp hoặc thư viện:

```powershell
pio run
```

### 2. Nạp Firmware (Upload)
Kết nối kit ESP32 CYD 3.5" qua cáp USB vào máy tính và chạy lệnh:

```powershell
pio run -t upload
```

### 3. Theo dõi Log Serial Monitor
Mở Serial Monitor ở tốc độ Baud 115200 để theo dõi trạng thái nạp và vận hành:

```powershell
pio device monitor
```

*(Mẹo: Bạn có thể gộp lệnh nạp và monitor trên PowerShell bằng dấu `;`: `pio run -t upload; pio device monitor`)*

---

## 🕹️ Cách Thao Tác & Điều Khiển

1. **Khởi động**: Sau khi nạp code và mount thẻ nhớ thành công, video tự động nạp Frame 0 và dừng ở trạng thái **PAUSED** với nút **▶ Play** lớn ở giữa màn hình.
2. **Phát video (Play)**:
   - Nhấn nút cứng **BOOT (GPIO0)** hoặc chạm vào màn hình.
   - Video bắt đầu phát mượt mà kèm âm thanh từ loa/DAC.
   - Sau **1 giây**, thanh Header, Footer và nút Play tự động biến mất (Cinema Mode).
3. **Hiện điều khiển khi đang xem (Tap to Wake)**:
   - Chạm nhẹ vào màn hình cảm ứng: Thanh Header, Footer lập tức hiển thị trong **1.5 giây** để xem tiến độ thời gian rồi tự ẩn lại nếu không thao tác.
4. **Tạm dừng (Pause)**:
   - Nhấn nút **BOOT (GPIO0)** hoặc chạm màn hình khi thanh điều khiển đang hiện.
   - Video lập tức dừng, nút tròn **▶ Play** xuất hiện trở lại ở tâm màn hình.
5. **Xem hết video**:
   - Khi chạy hết file, thiết bị tự động tua lại từ đầu (Frame 0) và dừng ở trạng thái **PAUSED** sẵn sàng cho lần phát tiếp theo.

---

## 📂 Cấu Trúc Thư Mục Dự Án

```text
esp32-project/
├── include/
│   ├── log.h                     # Hệ thống macro logging chuẩn dự án
│   └── pin_config.h              # Quản lý tập trung toàn bộ chân GPIO phần cứng
├── src/
│   ├── main.cpp                  # Khởi tạo dịch vụ và vòng lặp loop() chính
│   ├── services/
│   │   ├── audio_dac_service.h   # Driver âm thanh DAC nội GPIO26 (I2S DMA 371ms)
│   │   ├── audio_dac_service.cpp
│   │   ├── storage_service.h     # Quản lý thẻ nhớ SD VSPI và Mutex bảo vệ
│   │   ├── storage_service.cpp
│   │   ├── video_player_service.h # Parser AVI, giải mã MJPEG TJpg_Decoder & sync
│   │   └── video_player_service.cpp
│   └── ui/
│       ├── video_ui.h            # Giao diện Cinema Mode (Header, Footer, Center Icon)
│       └── video_ui.cpp
├── tools/
│   ├── video-to-frames.ps1       # Script PowerShell convert MP4 -> AVI All-in-One
│   ├── video.mp4                 # Video nguồn mẫu
│   └── out/
│       └── video.avi             # File AVI thành phẩm nạp vào thẻ nhớ
├── platformio.ini                # Cấu hình PlatformIO, thư viện TFT_eSPI & TJpg_Decoder
└── README.md                     # Tài liệu hướng dẫn sử dụng dự án
```

---

## 📜 Giấy Phép (License)

Dự án được phát hành theo giấy phép mã nguồn mở **MIT License**. Tự do sử dụng, chỉnh sửa và đóng góp cho cộng đồng ESP32 Maker!
