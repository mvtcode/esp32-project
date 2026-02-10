# ESP32 Audio Player – Bluetooth Speaker / MP3 / Radio

## 1. Mục tiêu sản phẩm

Xây dựng thiết bị dựa trên **ESP32-WROOM-32** có các chức năng:

- Loa Bluetooth (A2DP Sink)
- Máy nghe nhạc MP3 từ thẻ nhớ SD
- Nghe Radio Internet qua WiFi
- Điều khiển bằng 4 phím vật lý
- Hiển thị thông tin trên màn hình OLED
- Lưu cấu hình (mode, volume) vào NVS
- Có thể tắt/mở thiết bị bằng phím (Deep Sleep)

---

## 2. Phần cứng sử dụng

### 2.1 Vi điều khiển

- ESP32-WROOM-32 (Bluetooth Classic + WiFi)

### 2.2 Audio

- DAC I2S: **PCM5102**
- Amp + Loa ngoài

### 2.3 Hiển thị

- OLED 1.3 inch (SSD1306, I2C)

### 2.4 Lưu trữ

- Module thẻ nhớ SD (SPI mode)

### 2.5 Điều khiển

- 4 phím vật lý:
  - MODE / POWER
  - PLAY / PAUSE
  - PREV / VOLUME-
  - NEXT / VOLUME+

---

## 3. Mapping chân (gợi ý)

### I2S – PCM5102

| Signal | ESP32 GPIO |
| ------ | ---------- |
| BCK    | GPIO 26    |
| WS     | GPIO 27    |
| DATA   | GPIO 25    |

### OLED (I2C)

| Signal | ESP32 GPIO |
| ------ | ---------- |
| SDA    | GPIO 21    |
| SCL    | GPIO 22    |

### SD Card (SPI)

| Signal | ESP32 GPIO |
| ------ | ---------- |
| CS     | GPIO 5     |
| MOSI   | GPIO 23    |
| MISO   | GPIO 19    |
| SCK    | GPIO 18    |

### Buttons

| Button      | GPIO    |
| ----------- | ------- |
| MODE        | GPIO 32 |
| PLAY        | GPIO 33 |
| PREV / VOL- | GPIO 34 |
| NEXT / VOL+ | GPIO 35 |

---

## 4. Các chế độ hoạt động

```text
MODE_BT     : Bluetooth Speaker (A2DP Sink)
MODE_MP3    : MP3 Player từ thẻ SD
MODE_RADIO  : Internet Radio (MP3/AAC stream)
```

## 5. Quy ước phím điều khiển

| Phím        | Nhấn ngắn    | Giữ                      |
| ----------- | ------------ | ------------------------ |
| MODE        | Chuyển mode  | Tắt máy (Deep Sleep ~3s) |
| PLAY        | Play / Pause | —                        |
| PREV / VOL- | Giảm volume  | Previous track           |
| NEXT / VOL+ | Tăng volume  | Next track               |

- Long press: ~600 ms
- Volume repeat khi giữ

## 6. Tắt / Mở thiết bị

- ESP32 sử dụng Deep Sleep để giả lập Power Off
- Giữ phím MODE ~3 giây → Deep Sleep
- Nhấn MODE → Wake up
- GPIO MODE phải là chân hỗ trợ wake-up (GPIO 32–39)

## 7. Lưu cấu hình (NVS)

Lưu bằng Preferences:

- mode (BT / MP3 / RADIO)
- volume (0–100)
- last_mp3_index
- last_radio_index

❌ Không lưu:

- trạng thái playing
- stream runtime

## 8. Hiển thị OLED

Thông tin hiển thị:

- Mode hiện tại
- Trạng thái play / pause
- Tên bài hát (MP3 / Radio)
- Volume

Ví dụ:

```text
MP3 PLAYER
▶ Song Name.mp3
VOL: 72
```

## 9. Bluetooth Audio

- Chỉ dùng A2DP Sink
- Điện thoại phát nhạc (ZingMP3, Spotify, YouTube, v.v.)
- ESP32 nhận audio → I2S → PCM5102

❌ Không hỗ trợ:

- A2DP Source (ESP32 phát ra loa Bluetooth khác)

## 10. MP3 Player

- Thư viện: ESP8266Audio
- Decode MP3 từ SD Card
- Phát qua I2S DAC
- Hỗ trợ:
  - Play / Pause
  - Next / Prev
  - Volume
  - Đọc ID3 (tuỳ chọn)

## 11. Internet Radio

- Phát stream MP3 / AAC qua HTTP
- Không DRM
- Ví dụ nguồn:
  - Icecast
  - Shoutcast
  - Radio địa phương (VOV, BBC, …)

❌ Không hỗ trợ:

- ZingMP3 trực tiếp
- Spotify / Apple Music (DRM, private API)

## 12. Kiến trúc audio chuẩn

```texxt
Bluetooth Mode : Phone → ESP32 → I2S → PCM5102 → Amp → Speaker
MP3 Mode       : SD → ESP32 → I2S → PCM5102 → Amp → Speaker
Radio Mode     : WiFi → ESP32 → I2S → PCM5102 → Amp → Speaker
```

## 13. Best Practices

- Chỉ chạy 1 mode audio tại 1 thời điểm
- Stop audio hoàn toàn trước khi switch mode
- Không save NVS liên tục trong loop
- Tách task:
  - Audio Task (core 0)
  - UI + Button Task (core 1)

## 14. Giới hạn kỹ thuật

- ESP32-WROOM-32:
  - RAM hạn chế (~320 KB usable)
  - Không PSRAM
- Không phù hợp:
  - Bluetooth Source
  - Streaming có DRM
  - WiFi + BT Source song song

## 15. Hướng nâng cấp (tuỳ chọn)

- Scan thư mục MP3 tự động
- Playlist Radio bằng JSON
- Progress bar / time counter
- Rotary encoder thay phím
- ESP32-S3 + PSRAM cho UI phức tạp hơn
