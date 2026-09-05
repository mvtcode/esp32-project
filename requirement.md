# Yêu Cầu Dự Án: Video Player trên ESP32 CYD 3.5" (ST7796 480x320)

## 1. Mục tiêu & Phạm vi (Scope)

- **Mục tiêu:** Phát video kèm âm thanh đồng bộ mượt mà trên phần cứng ESP32 CYD 3.5".
- **Lộ trình:**
  - **Phase 1:** Hiện thực phát thành công 1 video duy nhất (`video.mjpeg` và `audio.wav`) từ thẻ nhớ MicroSD với đầy đủ đồng bộ A/V và điều khiển cơ bản.
  - **Phase sau:** Mở rộng danh sách phát (playlist), tối ưu hóa nâng cao nếu Phase 1 khả thi và chạy ổn định.

---

## 2. Phần cứng mục tiêu (Hardware Specification)

- **Mạch điều khiển:** ESP32-3248S035 (Kit CYD 3.5" màn hình cảm ứng).
- **Màn hình LCD:** ST7796 độ phân giải 480x320, giao tiếp HSPI (TFT_MOSI=13, TFT_MISO=12, TFT_SCLK=14, TFT_CS=15, TFT_DC=2, TFT_BL=27).
- **Cảm ứng:** Cảm ứng điện trở XPT2046 (TOUCH_CS=33, cùng bus SPI hoặc giao tiếp touch).
- **Thẻ nhớ MicroSD:** Giao tiếp VSPI độc lập (SD_CS=5, SD_MOSI=23, SD_MISO=19, SD_SCLK=18), định dạng FAT32.
- **Nút bấm vật lý:** Nút BOOT (GPIO0) có sẵn trên board.
- **Âm thanh:** DAC nội 8-bit của ESP32 xuất tín hiệu ra chân **GPIO26**.

---

## 3. Định dạng dữ liệu & Công cụ chuyển đổi (Conversion Tool)

ESP32 không hỗ trợ giải mã phần cứng chuẩn nén MP4 (H.264/H.265), do đó video sẽ được xử lý trước (offline pre-processing):

### Script chuyển đổi: `tools/video-to-frames.ps1`

- **File đầu vào:** `tools/video.mp4` (video mẫu tỉ lệ 16:9, 1080p).
- **Công cụ thực thi:** Sử dụng `tools/ffmpeg.exe`.
- **Thư mục đầu ra:** `tools/out/` bao gồm **1 file duy nhất**:
  - **`video.avi`**: Đóng gói **cả Hình ảnh (Motion JPEG) và Âm thanh (PCM WAV)** vào 1 file container AVI duy nhất (Audio Video Interleave).
    - **Video:** Motion JPEG, độ phân giải **$480 \times 270$** (chuẩn tỉ lệ 16:9), tốc độ **20 fps**, mức nén `q:v = 4` sắc nét.
    - **Audio:** PCM 16-bit Mono @ **22.05 kHz** (không nén, tối ưu cho DAC nội GPIO26).
    - **Tính tương thích:** Vừa mở xem và nghe trực tiếp mượt mà trên VLC/PC, vừa phát trực tiếp trên ESP32 với hiệu năng cao nhất (chỉ mở 1 file duy nhất, đọc tuần tự không bị trễ seek).

### Cấu trúc lưu trên thẻ nhớ SD (FAT32)

Sau khi convert, người dùng chỉ cần copy 1 file duy nhất vào thư mục `/esp32-video` trên thẻ MicroSD:

```text
/esp32-video/
  └── video.avi
```

---

## 4. Giao diện & Trải nghiệm người dùng (UI/UX - Cinema Mode)

- **Bố cục phân bổ không gian màn hình ($480 \times 320$):**
  - **Header (Thanh tiêu đề trên cùng, $y: 0 \rightarrow 19$, cao $20\text{ px}$):**
    - Hiển thị tên file video căn lề trái.
    - Nằm hoàn toàn phía trên video, **không đè lên video**.
  - **Vùng phát Video (Chính giữa màn hình, $y: 20 \rightarrow 289$, kích thước $480 \times 270$):**
    - Phát trọn vẹn $100\%$ khung hình gốc 16:9, hình ảnh sắc nét, không bị che khuất.
    - Ở giữa màn hình có nút tròn lớn Play/Pause (hiệu ứng kính mờ).
  - **Footer (Thanh điều khiển đáy màn hình, $y: 290 \rightarrow 319$, cao $30\text{ px}$):**
    - Nằm hoàn toàn phía dưới video, **không đè lên video**.
    - Hiển thị trên cùng 1 hàng: `Current Time (trái) | Seek Bar tiến độ (giữa) | Total Duration (phải)`.
- **Cơ chế Ẩn/Hiện thông minh (Cinema Mode):**
  - Khi **PAUSED (Tạm dừng hoặc mới khởi động):** Header, Footer và Center Button (▶ Play) hiển thị đầy đủ, rõ ràng.
  - Khi **PLAYING (Đang phát):** Sau **1 giây**, nút giữa màn hình, Header và Footer tự động ẩn (màn hình chuyển sang chế độ rạp chiếu phim tập trung $100\%$ vào thước phim).
  - Khi **CHẠM CẢM ỨNG lúc đang xem:** Toàn bộ thanh Header, Footer và nút điều khiển lập tức sáng lên trong **1.5 giây** để theo dõi tiến độ rồi tự động ẩn nếu không thao tác tiếp.
- **Tương tác Điều khiển (Play / Pause):**
  - Nhấn nút cứng **BOOT (GPIO0)**: Chuyển đổi trạng thái giữa Play và Pause.
  - Chạm cảm ứng vào màn hình: Chuyển đổi trạng thái giữa Play và Pause.

---

## 5. Cơ chế vận hành & Đồng bộ Âm thanh - Hình ảnh (A/V Sync)

- **Audio Master Clock:** Âm thanh phát qua DAC DMA liên tục và được chọn làm trục thời gian chuẩn (Master Clock).
- **Đồng bộ khung hình:**
  - ESP32 tính toán frame video tương ứng với thời gian đã phát của âm thanh.
  - **Chiến lược Drop frame (Bỏ khung hình):** Nếu việc đọc thẻ nhớ hoặc giải mã màn hình bị trễ khiến hình ảnh chậm hơn tiếng (> 40ms), hệ thống sẽ chủ động bỏ qua (skip) frame bị trễ và nhảy thẳng đến frame khớp với âm thanh. Đảm bảo hình ảnh và tiếng không bao giờ bị lệch nhau.
- **Tối ưu đa nhiệm (FreeRTOS Dual-Core):**
  - Phân tách luồng đọc dữ liệu từ thẻ nhớ SD (VSPI), giải mã hình ảnh TJpg_Decoder (HSPI) và xuất DAC audio để tận dụng tối đa 2 nhân ESP32.

Kết luận phase 1:

- Video khá giật, chỉ đạt ~2 FPS do kích thước khung hình 480x270 quá lớn đối với CPU ESP32 gốc không có tập lệnh SIMD và băng thông SPI.

---

## Phase 2: Chuyển sang ESP32-S3 2.8" (320x240 Touch)

- **Mạch điều khiển:** ESP32-S3 (ESP32-S3-WROOM-1 N16R8, 16MB Flash, 8MB Octal PSRAM, Xtensa LX7 @ 240MHz hỗ trợ tăng tốc SIMD PIE).
- **Màn hình:** 2.8 inch TFT LCD độ phân giải **$320 \times 240$** (ILI9341V / ST7789).
  - Tần số SPI: 40MHz.
  - Pinout: `TFT_MOSI=11`, `TFT_MISO=13`, `TFT_SCLK=12`, `TFT_CS=10`, `TFT_DC=46`, `TFT_BL=45`.
- **Cảm ứng:** Hỗ trợ cảm ứng điện dung FT6336G (I2C: SDA=16, SCL=15) hoặc cảm ứng điện trở XPT2046.
- **Thẻ nhớ MicroSD:** Giao tiếp SPI (SD_CS=4, SD_MOSI=11, SD_MISO=13, SD_SCLK=12), định dạng FAT32.
- **Âm thanh:** Chuẩn **I2S DMA** (BCLK=5, LRC=7, DOUT=8) kết hợp chip codec **ES8311** (I2C 0x18) và bộ khuếch đại onboard (PA_ENABLE=48) hoặc I2S DAC rời (MAX98357A).

### Định dạng & Script chuyển đổi Phase 2
- **File đầu ra:** `tools/out/video.avi` (All-in-One: Video MJPEG + Audio PCM WAV).
  - **Video:** Motion JPEG, độ phân giải **$320 \times 180$** (chuẩn 16:9 Cinema Mode), tốc độ **20 FPS**, chất lượng `q:v = 7`.
  - **Audio:** PCM 16-bit Mono @ **22.05 kHz** (I2S DMA).
  - **Bố cục Cinema Mode ($320 \times 240$):**
    - Header: $y: 0 \rightarrow 29$ (Cao $30\text{ px}$, không che video).
    - Video: $y: 30 \rightarrow 209$ (Kích thước $320 \times 180$, hiển thị $100\%$ tỉ lệ 16:9).
    - Footer: $y: 210 \rightarrow 239$ (Cao $30\text{ px}$, chứa Current Time, Progress Bar, Total Duration).
    - Center Icon: Tâm $(160, 120)$.
- **Link phần cứng tham khảo:** https://shopee.vn/product/997003090/48300510827 (Board ES3N28P / ES3C28P).

