# Kế Hoạch Tích Hợp Module SD Card & Chế Độ MP3 Player

Tích hợp module thẻ nhớ MicroSD qua giao tiếp SPI để bổ sung chế độ phát nhạc **MP3 Player** trực tiếp từ thẻ nhớ, đồng thời đồng bộ toàn bộ 65 hiệu ứng Sound Visualizer trên màn hình OLED 1.3".

---

## 1. Sơ Đồ Đấu Nối Phần Cứng (Wiring Pinout)

ESP32 kết nối với Module Thẻ Nhớ MicroSD qua bus SPI (tạo instance `SPIClass` riêng để tránh trùng với DAC PCM5102A):

| Chân Module Thẻ Nhớ     |        ESP32 Pin        | Chức năng               | Ghi chú kỹ thuật                                         |
| :---------------------- | :---------------------: | :---------------------- | :------------------------------------------------------- |
| **CS (Chip Select)**    |       **GPIO 5**        | SPI Chip Select         | Hardware Default VSPI CS                                 |
| **SCK (Clock)**         |       **GPIO 16**       | SPI Clock               | Chân Digital I/O sạch, an toàn                           |
| **MOSI (Data In Thẻ)**  |       **GPIO 17**       | SPI Master Out Slave In | Chân Digital I/O sạch, an toàn                           |
| **MISO (Data Out Thẻ)** | **GPIO 34** _(hoặc 35)_ | SPI Master In Slave Out | Chân Input-only (Tiết kiệm pin Output)\*                 |
| **VCC**                 |  **5V (VBUS) / 3.3V**   | Nguồn nuôi              | Khuyên dùng 5V nếu module có LDO AMS1117 để ổn định dòng |
| **GND**                 |         **GND**         | Nối đất                 | Nối Mass chung với ESP32                                 |

> ⚠️ **Lưu ý kỹ thuật về SPI Bus:** Do GPIO 18, 19, 23 đang được dùng cho DAC PCM5102A, module SD Card sẽ được khởi tạo qua instance SPI độc lập:
> `SPIClass spiSD(HSPI); spiSD.begin(16, 34, 17, 5); SD.begin(5, spiSD, 20000000);` (Tần số 20MHz đảm bảo tín hiệu truyền ổn định).
> ⚠️ **Lưu ý về MISO (GPIO 34):** Hầu hết các module MicroSD trên thị trường đã có sẵn trở pull-up 10k nên cắm thẳng GPIO 34 hoạt động bình thường. Nếu dùng adapter tự hàn không có trở pull-up, có thể chuyển sang **GPIO 2** hoặc **GPIO 15** (hỗ trợ `INPUT_PULLUP` nội).

---

## 2. Giao Diện & Cơ Chế Điều Khiển (UI / Controls)

### A. Chuyển Đổi Chế Độ Hệ Thống

- Nhấn nút **PUSH** (hoặc chuyển qua Web Interface): Vòng lặp các chế độ `MIC` $\to$ `BLUETOOTH` $\to$ `CLOCK` $\to$ `MP3 PLAYER` $\to$ `MIC`.

---

### B. Cơ Chế Nút Bấm Trong Chế Độ MP3 Player

```
                  ┌──────────────────────────────────────────────┐
                  │                MP3 PLAYER                    │
                  └──────────────────────────────────────────────┘
                         │                                │
            (Bấm PUSH)   │                                │  (Bấm PUSH)
                         ▼                                ▼
┌──────────────────────────────────────────┐    ┌──────────────────────────────────────────┐
│     1. MÀN HÌNH BÀI HÁT / VISUALIZER     │    │      2. MÀN HÌNH DANH SÁCH (MENU)        │
├──────────────────────────────────────────┤    ├──────────────────────────────────────────┤
│ • Xoay EC11 : Tăng / Giảm Volume         │    │ • Xoay EC11 : Cuộn danh sách bài hát     │
│ • Bấm BACK  : Play / Pause (▶️ / ⏸️)    │    │ • Bấm PLUS  : Chọn & Phát bài đang chọn  │
│ • Giữ BACK  : Prev bài hát (⏮️)          │    │ • Bấm BACK  : Trở về màn hình Player    │
│ • Bấm PLUS  : Chuyển hiệu ứng Visualizer │    │                                          │
│ • Giữ PLUS  : Next bài hát (⏭️)          │    │                                          │
└──────────────────────────────────────────┘    └──────────────────────────────────────────┘
```

#### Chi tiết tương tác:

1. **Màn hình Bài Hát & Visualizer (Now Playing):**
   - **Hiển thị:** Tên bài hát (cắt ngắn vừa khung hoặc cuộn Marquee), thanh % tiến độ bài hát, thời lượng (phút:giây), mức Volume, và hiệu ứng Visualizer sóng nhạc.
   - **Xoay núm EC11:** Tăng / Giảm Volume (hiển thị popup volume tức thì, đồng bộ với NVS).
   - **Bấm nút BACK (GPIO 13):** Play / Pause bài hát.
   - **Nhấn giữ nút BACK (> 1s):** Chuyển về bài hát trước (**Prev ⏮️**).
   - **Bấm nút PLUS (GPIO 14):** Đổi sang hiệu ứng Visualizer tiếp theo (trong 65 hiệu ứng).
   - **Nhấn giữ nút PLUS (> 1s):** Chuyển sang bài hát kế tiếp (**Next ⏭️**).
   - **Bấm nút PUSH (GPIO 4):** Mở màn hình danh sách bài hát (Playlist Menu).

2. **Màn hình Danh Sách Bài Hát (Playlist Menu):**
   - **Hiển thị:** Danh sách tên các file nhạc trong thư mục thẻ nhớ với thanh cuộn và con trỏ highlight (tên bài dài tự động cắt bớt và thêm `...`).
   - **Xoay núm EC11:** Di chuyển con trỏ lên / xuống qua từng bài hát.
   - **Bấm nút PLUS** (hoặc bấm PUSH): Xác nhận chọn và phát bài hát đang được highlight.
   - **Bấm nút BACK:** Hủy bỏ chọn, quay trở lại màn hình Player đang phát hiện tại.

---

## 3. Quy Tắc Quét Thẻ Nhớ, Định Dạng Hỗ Trợ & Quản Lý Danh Sách Lớn

### A. Phạm Vi Quét Thẻ Nhớ (Whole SD Card Recursive Scan)

- Quét **toàn bộ thẻ nhớ**: Quét đệ quy từ thư mục gốc `/` và tất cả các thư mục con bên trong thẻ (ví dụ `/music`, `/album1`, `/downloads`, v.v.).
- **Cơ chế Caching trong RAM (Smart Session Cache):** Chỉ quét trực tiếp 1 lần lúc vừa chuyển sang MP3 Mode (hiện thông báo `Scanning SD...` ~0.5s), sau đó lưu mảng trong RAM để thao tác mở menu tức thì (0ms), tuyệt đối không ghi file rác lên thẻ để chống hỏng phân vùng FAT32 khi rút thẻ.

### B. Quy Tắc Lọc Định Dạng File (File Extension Filter)

| Phân loại                  |    Định dạng / Phần mở rộng     | Hành vi xử lý của hệ thống                                                                                               |
| :------------------------- | :-----------------------------: | :----------------------------------------------------------------------------------------------------------------------- |
| **Được hỗ trợ chính thức** |       **`.mp3`, `.MP3`**        | ✅ Hỗ trợ đầy đủ (32–320 kbps CBR/VBR, 8–48 kHz) qua **Helix MP3 Decoder**.                                              |
| **Được hỗ trợ chính thức** |       **`.wav`, `.WAV`**        | ✅ Hỗ trợ đầy đủ (16-bit PCM Linear Stereo/Mono) đọc trực tiếp ra I²S DAC.                                               |
| **Bỏ qua (Ignored)**       |      **`.flac`, `.FLAC`**       | ❌ **Bỏ qua hoàn toàn:** Bộ giải mã FLAC cần bộ đệm RAM quá lớn (40–60KB), không phù hợp với ESP32-WROOM không có PSRAM. |
| **Bỏ qua (Ignored)**       |       **`.ogg`, `.OGG`**        | ❌ **Bỏ qua hoàn toàn:** Decoder Tremor Vorbis tiêu tốn nhiều SRAM, dễ gây tràn bộ nhớ.                                  |
| **Bỏ qua (Ignored)**       | **File ẩn / File rác hệ thống** | ❌ **Bỏ qua:** Bất kỳ file nào bắt đầu bằng dấu chấm `.` (ví dụ `._*`, `.DS_Store`, `.Trashes`, `.Spotlight`).           |
| **Bỏ qua (Ignored)**       |  **File không phải âm thanh**   | ❌ **Bỏ qua:** `.txt`, `.lrc`, `.jpg`, `.png`, `.mp4`, `.pdf`, v.v.                                                      |

### C. Quản Lý Danh Sách Bài Hát Khi Thẻ Có Quá Nhiều File (Large Playlist & RAM Optimization)

- **Vấn đề:** Nếu thẻ nhớ có 500 – 1.000 bài hát, việc lưu toàn bộ chuỗi đường dẫn dài vào RAM sẽ gây cạn kiệt bộ nhớ SRAM trên ESP32.
- **Giải pháp thiết kế:**
  1. **Cấu trúc Compact Struct:** Mỗi bài hát chỉ chiếm ~36 bytes bộ nhớ:
     ```cpp
     struct PlaylistItem {
         char path[32];   // Đường dẫn tương đối hoặc rút gọn
     };
     ```
  2. **Giới hạn an toàn (Safe Playlist Cap):** Lưu tối đa **250 bài hát** trong RAM (~9KB RAM). Nếu thẻ có nhiều hơn 250 bài, hệ thống nạp 250 bài đầu tiên và hiển thị thông báo `Loaded 250 tracks`.
  3. **Phân trang Menu OLED (Viewport Pagination):** Màn hình OLED chỉ vẽ 4 dòng trong tầm nhìn hiển thị (Viewport), tính toán vị trí con trỏ và cuộn trượt mượt mà thay vì render toàn bộ danh sách cùng lúc.

---

## 4. Kiến Trúc Xử Lý Âm Thanh & Visualizer

### A. Phân Bổ Tài Nguyên Dual-Core (FreeRTOS)

- **Core 0 (Audio Core):** Chạy `mp3_player_task` độc quyền:
  - Đọc khối dữ liệu từ thẻ nhớ SD qua SPI.
  - Giải mã dữ liệu MP3 sang PCM 16-bit stereo (sử dụng **Helix MP3 Decoder** tối ưu cho ESP32 SRAM).
  - Điều chỉnh âm lượng kỹ thuật số (Perceptual Volume curve đồng bộ với Bluetooth).
  - Đẩy luồng âm thanh liên tục ra I²S DAC (PCM5102A trên GPIO 18, 19, 23).
- **Core 1 (App Core):** Chạy `main loop`:
  - Quét sự kiện nút bấm & núm xoay EC11.
  - Tính toán FFT và render 65 hiệu ứng Visualizer ra màn hình OLED 1.3" với tốc độ ~60 FPS.

### B. Đồng Bộ Âm Nhạc Với 65 Hiệu Ứng Visualizer

- Tương tự cơ chế của Bluetooth, khi decoder giải mã ra các gói PCM, hệ thống sẽ trích xuất đồng thời các frame **128 mẫu** và đẩy vào `s_audio_queue`.
- Bộ phân tích FFT và Beat Detector sẽ phân tích tín hiệu trực tiếp từ file MP3 $\to$ Toàn bộ 65 hiệu ứng nháy theo bass/treble hoàn toàn đồng bộ với âm thanh phát ra loa.

### C. Chống Méo Tiếng, Giật Lag & Pop Noise

- **Pre-Buffer / Ring Buffer (8KB – 16KB):** Bù đắp độ trễ khi thẻ SD đọc sector mới, ngăn chặn hiện tượng hụt buffer (underrun) gây giật tiếng.
- **Soft Volume Ramp (Fade-in/Fade-out 30ms):** Khi Play, Pause, hoặc Chuyển bài, âm lượng được tăng/giảm tuyến tính mềm mại kết hợp xóa buffer DMA I²S (`i2s_zero_dma_buffer`), triệt tiêu hoàn toàn tiếng "bụp" (pop noise) ở loa.

---

## 5. Quản Lý Bộ Nhớ & NVS Storage

- **Giải phóng RAM:** Khi chuyển sang chế độ MP3, tắt hoàn toàn stack Bluetooth (`bt_audio_stop()`) và tắt WiFi để thu hồi **~90KB – 100KB heap**, đảm bảo SRAM luôn dư thừa và an toàn.
- **Đồng bộ Volume:** Sử dụng chung biến Volume trong NVS. Mức âm lượng được đồng bộ xuyên suốt giữa chế độ Bluetooth và chế độ MP3 Player.
- **Tự động chuyển bài (Repeat All Loop):** Khi phát hết bài hát (EOF), player tự động chuyển sang bài tiếp theo. Khi hết danh sách, tự động quay lại bài đầu tiên.
- **Bảo vệ Flash khi Lưu Vị Trí Phát (NVS Wear-Leveling):** Chỉ ghi vị trí bài hát và offset vào NVS khi: **Bấm Pause, Đổi bài, Chuyển Mode**, hoặc định kỳ **mỗi 30 giây** khi đang phát (tránh ghi flash từng giây gây mòn chip nhớ).

---

## 6. Cơ Chế Chịu Lỗi Toàn Diện (Fault-Tolerance)

| Tình huống lỗi                                | Cơ chế xử lý an toàn                                                                                                                                                                             |
| :-------------------------------------------- | :----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Chưa cắm thẻ nhớ / Thẻ không nhận**         | Hiển thị thông báo `"No SD Card!"` trên OLED $\to$ Tự động chuyển về chế độ MIC sau 2 giây.                                                                                                      |
| **Thẻ nhớ không có file nhạc**                | Hiển thị thông báo `"No MP3 Files Found!"` trên OLED.                                                                                                                                            |
| **Rút thẻ nhớ đột ngột khi đang phát**        | Bắt lỗi I/O đọc thẻ $\to$ Mute I²S ngay lập tức $\to$ Hủy tác vụ decoder an toàn $\to$ Hiển thị Toast `"SD Card Removed!"` trên màn hình OLED $\to$ Chuyển về chế độ MIC.                        |
| **File MP3 thiếu Metadata / ID3 tag**         | **Vẫn phát bình thường:** Dùng tên file (đã lọc dấu) làm tên hiển thị; Tính % tiến độ dựa trên `Bytes đã đọc / Tổng File Size`; Hiển thị thời gian dạng `01:23 / --:--`.                         |
| **File MP3 bị hỏng nặng / Không decode được** | Helix Decoder tự động bỏ qua frame lỗi. Nếu sau **1.5 giây** không decode được frame nào $\to$ Hiển thị Toast `"File Error!"` và tự động nhảy sang bài tiếp theo (Tuyệt đối không Crash/Reboot). |
| **Đổi thẻ nhớ khác**                          | Tự động quét lại cây thư mục, cập nhật danh sách bài hát mới và phát từ bài đầu tiên.                                                                                                            |
| **Chuyển bài hát liên tục**                   | Hủy file stream cũ an toàn, giải phóng bộ nhớ đệm trước khi mở stream mới $\to$ Tránh rò rỉ bộ nhớ (memory leak) và đơ hệ thống.                                                                 |

---

## 7. Xử Lý Tên Bài Hát Tiếng Việt & Ký Tự Đặc Biệt

Để chống lỗi font vỡ chữ trên OLED 1.3" (U8g2) và đảm bảo `SD.open()` luôn mở file chính xác:

### A. Tách biệt Đường dẫn Thật & Tên Hiển Thị

- **Đường dẫn mở file (`raw_path`):** Giữ nguyên chuỗi UTF-8 gốc (ví dụ: `/music/01. Em Gái Mưa.mp3`) để hệ thống FAT32 tìm đúng file vật lý trên thẻ.
- **Tên hiển thị (`display_title`):** Được lọc qua bộ chuyển đổi chuỗi chuẩn hóa trước khi vẽ ra buffer màn hình OLED.

### B. Bộ Lọc Chuyển Tiếng Việt Có Dấu $\to$ Không Dấu (Vietnamese Diacritics Sanitizer)

- Tự động map các byte UTF-8 Tiếng Việt nhiều byte thành ký tự ASCII 1 byte:
  - `á, à, ả, ã, ạ, ă, ắ, ằ, ẵ, ặ, â, ấ, ầ, ẩ, ẫ, ậ` $\to$ `a` (và `A`)
  - `đ, Đ` $\to$ `d, D`
  - `é, è, ẻ, ẽ, ẹ, ê, ế, ề, ể, ễ, ệ` $\to$ `e` (và `E`)
  - `í, ì, ỉ, ĩ, ị` $\to$ `i` (và `I`)
  - `ó, ò, ỏ, õ, ọ, ô, ố, ồ, ổ, ỗ, ộ, ơ, ớ, ờ, ở, ỡ, ợ` $\to$ `o` (và `O`)
  - `ú, ù, ủ, ũ, ụ, ư, ứ, ừ, ử, ữ, ự` $\to$ `u` (và `U`)
  - `ý, ỳ, ỷ, ỹ, ỵ` $\to$ `y` (và `Y`)
- _Ví dụ:_ `"01. Em Gái Mưa - Hương Tràm.mp3"` $\to$ Hiển thị thành: `"01. Em Gai Mua - Huong Tram"`

### C. Lọc Ký Tự Lạ, Ký Tự Điều Khiển & Emoji

- **Ký tự hợp lệ:** `[a-z]`, `[A-Z]`, `[0-9]`, khoảng trắng và các dấu chuẩn: `-`, `_`, `(`, `)`, `[`, `]`, `.`, `&`, `+`, `,`.
- **Ký tự điều khiển (`\r`, `\n`, `\t`, `0x00 - 0x1F`):** Tự động bỏ qua hoặc thay bằng khoảng trắng.
- **Ký tự lạ / Emoji / Font đa ngôn ngữ:** Tự động thay thế bằng khoảng trắng hoặc dấu `_` thay vì để lọt ký tự rác ra OLED.
- **Ẩn phần mở rộng:** Tự động cắt bỏ `.mp3`, `.wav` ở cuối để giao diện trực quan và sạch sẽ.

### D. Cắt Ngắn Tên Bài Hát Quá Dài (String Truncation & Ellipsis)

- **Quy tắc cắt ngắn (Truncation with `...`):**
  - **Trên Menu Danh Sách:** Tối đa **18–20 ký tự**. Nếu tên bài dài hơn, cắt bớt và thêm `...` ở cuối (ví dụ: `"01. Em Gai Mua - Hu..."`).
  - **Trên Màn Hình Now Playing:** Tối đa **22–24 ký tự** (hoặc cuộn chữ Marquee), đảm bảo không bao giờ vẽ đè lên icon âm lượng, % pin hay khung visualizer.
- **An toàn bộ đệm:** Cố định buffer `char display_title[32]` và luôn đảm bảo kết thúc bằng byte `\0`.

---

## 8. Danh Mục Triển Khai Mã Nguồn (Implementation Roadmap)

| File                         | Hành động | Mục đích                                                                                                                     |
| :--------------------------- | :-------: | :--------------------------------------------------------------------------------------------------------------------------- |
| `platformio.ini`             | [MODIFY]  | Thêm thư viện giải mã MP3 tối ưu RAM (`earlephilhower/ESP8266Audio`).                                                        |
| `src/sd_card.h` & `.cpp`     |   [NEW]   | Khởi tạo SPI riêng (GPIO 5, 16, 17, 34), quét đệ quy toàn thẻ nhớ, lọc đuôi file, sanitize tên tiếng Việt và cắt ngắn `...`. |
| `src/mp3_player.h` & `.cpp`  |   [NEW]   | Task giải mã MP3/WAV trên Core 0, đẩy I²S DAC, bơm dữ liệu visualizer `s_audio_queue`, chống pop noise và auto-next.         |
| `src/display.h` & `.cpp`     | [MODIFY]  | Thêm giao diện Menu Playlist phân trang, thanh trạng thái Now Playing (% bài hát, thời lượng, volume).                       |
| `src/main.cpp`               | [MODIFY]  | Thêm `AUDIO_MODE_SD_MP3`, cơ chế chuyển chế độ mượt mà, phân luồng nút bấm & núm xoay EC11 cho MP3.                          |
| `src/nvs_storage.h` & `.cpp` | [MODIFY]  | Lưu/đọc chỉ số bài hát và vị trí phát gần nhất có bảo vệ ghi Flash.                                                          |

---

## 9. Đánh Giá Mức Độ Sẵn Sàng & Checklist Triển Khai 🚀

Toàn bộ các khía cạnh kỹ thuật đã được làm rõ và chốt phương án rõ ràng 100%:

- [x] **Phần cứng & Chân cắm:** SPI độc lập trên `GPIO 5 (CS), 16 (SCK), 17 (MOSI), 34 (MISO)` — không xung đột với DAC PCM5102A hay OLED.
- [x] **Xử lý âm thanh & Visualizer:** Dual-Core FreeRTOS (Core 0 giải mã MP3 Helix, Core 1 chạy 65 hiệu ứng Visualizer 60 FPS từ luồng PCM trích xuất).
- [x] **Bộ điều khiển (UI/UX):** Núm EC11 cuộn menu / chỉnh volume; nút Back Play/Pause/Prev; nút Plus Đổi effect / Next bài; nút Push mở Menu.
- [x] **Quản lý thẻ nhớ & Định dạng:** Quét toàn thẻ nhớ (Smart Session Cache), chỉ nạp `.mp3` & `.wav` (bỏ qua `.flac, .ogg, ._*`), giới hạn an toàn 250 bài hát trong RAM.
- [x] **Chống lỗi Tiếng Việt & File hỏng:** Lọc dấu Tiếng Việt sang ASCII, bắt lỗi timeout 1.5s tự động next bài mà không crash ESP32.
- [x] **Bảo vệ Flash & Chống giật tiếng:** NVS wear-leveling 30s, Pre-buffer 16KB và Soft Volume Ramp chống tiếng "bụp" (Pop noise).

---

## 10 Fix plan

Các màn hình ở chế độ player:

- s1: Màn hình player normal (default)
- s2: Màn hình sound visualizer
- s3: Màn hình player list bài hát

ở chế độ màn hình s1:

- Chỉ hiển thị bài hát đang mở (có Marquee nếu quá dài)
- Thứ tự bài hát/tổng bài hát
- Volume value
- Buttons:
  - Button push vẫn giữ vài trò switch mode(luôn luôn không đổi ở các mode)
  - Button back: Play/Pause, long press: Prev
  - Button confirm: nhấn 1 lần chuyển sang s3, nhấn giữ 1s: Next bài hát, giữ 3s: sang màn hình s2
  - Nut EC11: Tăng/Giảm volume

ở chế độ màn hình s2:

- Visualizer
- Không cần hiển thị bài hát, volume, thứ tự bài hát, tổng bài hát (hiển thị sạch bóng như chế độ mic/bluetooth)
- Buttons:
  - Button push vẫn giữ vài trò switch mode(luôn luôn không đổi ở các mode)
  - Button back: Play/Pause, long press: Prev
  - Button confirm: nhấn 1 lần chuyển sang s3, nhấn giữ 1s: Next bài hát, giữ 3s: sang màn hình s1
  - Nut EC11: Tăng/Giảm volume (có toast lên màn hình nếu thay đổi)

ở chế độ màn hình s3:

- Hiển thị danh sách các bài hát bằng icon play ở đầu hoặc ở bên phải, focus vào bài hiện tại (kéo scroll nếu danh sách dài)
- Không hiển thị Volume
- Buttons:
  - Button push vẫn giữ vài trò switch mode(luôn luôn không đổi ở các mode)
  - Button back: Trở về màn hình trước.
  - Button confirm: chọn bài hát hiện tại đang focus (chuyển sang s1, phát bài này)
  - Nut EC11: lên/xuống (focus) trong danh sách

Chịu lỗi:
Hiện tại đang có 1 bài hát lỗi, tôi bị kẹt cứng không thể thoát được ra khỏi bài hát này hoặc sang chế độ khác, ứng dụng bị reboot liên tục.
