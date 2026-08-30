# ESP32-S3 AI Camera & Face Detection System (N16R8)

Hệ thống Camera AI nhận diện khuôn mặt thời gian thực kết hợp màn hình TFT ST7789, WiFi Captive Portal cấu hình động và tự động tải ảnh khuôn mặt đã cắt (crop) lên Google Drive.

---

## 🌟 Tính năng nổi bật

- **Camera & Xử lý phần cứng mạnh mẽ**:
  - Hỗ trợ board **ESP32-S3 (N16R8 - 16MB Flash, 8MB Octal PSRAM)** và camera **OV3660 / OV2640**.
  - Pipeline chụp hình hỗ trợ chế độ **VGA (640x480)** (tự động downsample 2:1 mượt mà) hoặc **QVGA (320x240)**.
- **Nhận diện khuôn mặt thời gian thực (AI Face Detection)**:
  - Tích hợp mô hình AI phát hiện khuôn mặt chạy đa luồng trên Core 0 (FreeRTOS Task).
  - Tự động vẽ **Bounding Box** & các điểm mốc (landmarks) khuôn mặt trực tiếp lên màn hình LCD với độ trễ cực thấp.
- **Hiển thị trực quan với ST7789 TFT 2.8" (320x240)**:
  - Tối ưu hóa render bằng thư viện **LovyanGFX** sử dụng DMA / Double-buffering.
  - Hiển thị HUD thông tin hệ thống: FPS Camera/Display, trạng thái WiFi, trạng thái Upload Google Drive, Toast thông báo thời gian thực.
- **Tự động tải ảnh khuôn mặt lên Google Drive**:
  - Tự động crop chỉ vùng khuôn mặt và nén JPEG chất lượng cao trước khi upload.
  - Thuật toán chống upload lặp thông minh: Chỉ upload khi phát hiện khuôn mặt mới xuất hiện hoặc sau khoảng thời gian cooldown tùy chỉnh.
  - Upload bất đồng bộ (Asynchronous Queue) không làm gián đoạn luồng hiển thị video.
  - Hỗ trợ lưu trữ đệm (Cache) trên thẻ nhớ MicroSD khi mất kết nối mạng.
- **WiFi Captive Portal Web UI**:
  - Giao diện web hiện đại (lưu trữ trong LittleFS) tự động mở AP `ESP32S3-CAM-AP` (IP `192.168.4.1`) khi chưa có cấu hình.
  - Cấu hình trực quan qua trình duyệt: SSID/Password WiFi, Google Apps Script Webhook URL, Tần suất upload (Cooldown), Chế độ phân giải camera.
- **Điều khiển đa năng qua nút BOOT (GPIO 0)**:
  - **Nhấn 1 lần**: Bật / Tắt chế độ Upload Google Drive (kèm Toast OSD).
  - **Giữ 1 giây**: Bật / Tắt đèn nền màn hình (Screen Sleep/Wake).
  - **Giữ 3 giây**: Khôi phục cài đặt gốc (Factory Reset NVS & mở lại Captive Portal).
  - **Giữ nút khi cắm nguồn/reset**: Ép buộc vào chế độ Captive Portal Web UI.

---

## 🛠️ Sơ đồ kết nối phần cứng (Pinout)

### 1. Màn hình TFT 2.8" SPI (ST7789 - 320x240)

| Chân TFT | Chân ESP32-S3 | Chức năng |
| :--- | :--- | :--- |
| **SCL / SCK** | `GPIO 19` | SPI Clock |
| **SDA / MOSI** | `GPIO 20` | SPI Data Out |
| **RES / RST** | `GPIO 21` | Reset |
| **DC** | `GPIO 47` | Data / Command |
| **CS** | `GPIO 45` | Chip Select |
| **BLK / BL** | `GPIO 38` | Đèn nền Backlight (PWM / Digital) |
| **VCC** | `5V` hoặc `3.3V` | Nguồn cấp |
| **GND** | `GND` | Nối đất |

### 2. Nút nhấn & Đèn LED chỉ báo RGB (GPIO 48)

- **Nút BOOT**: `GPIO 0` (Có sẵn trên board ESP32-S3 DevKit).
- **LED RGB trên board (`GPIO 48`)**: Cường độ sáng 30%, hiển thị đa trạng thái hệ thống:
  - 🔴 **Đỏ nháy (Blink Red)**: Mất kết nối WiFi hoặc đang kết nối lại.
  - 🟢 **Xanh lá cây (Solid Green)**: Đang tải ảnh khuôn mặt lên Google Drive.
  - 🟡 **Vàng (Yellow - 2s)**: Tải ảnh lên Google Drive thất bại.
  - 🔵 **Xanh dương / Cyan nháy (Blink Blue)**: Đang ở chế độ cấu hình Captive Portal AP.
  - ⚪ **Trắng / Bạc chớp nhẹ (Cyan Flash)**: Vừa nhận diện được khuôn mặt.
  - ⚫ **Tắt (Off)**: Hệ thống hoạt động bình thường, WiFi đã kết nối sẵn sàng (tiết kiệm điện & chống chói).

### 3. Âm thanh I2S Loa MAX98357A & Micro INMP441

| Thiết bị | Chân thiết bị | Chân ESP32-S3 | Chức năng |
| :--- | :--- | :--- | :--- |
| 🔊 **Loa MAX98357A** | **BCLK** | `GPIO 14` | I2S Bit Clock (Output) |
| | **LRC / WS** | `GPIO 3` | I2S Word Select / L-R Clock (Output) |
| | **DIN** | `GPIO 42` | I2S Serial Data Out (Output) |
| | **GAIN** | `GND` / Trống | Độ lợi khuếch đại (Mặc định 9dB/12dB) |
| | **VIN** | `5V` (hoặc `3.3V`) | Nguồn cấp (5V cho âm to & chuẩn nhất) |
| | **GND** | `GND` | Nối đất |
| 🎙️ **Micro INMP441** | **WS** | `GPIO 1` | I2S Word Select (Output) |
| | **SCK** | `GPIO 2` | I2S Clock (Output) |
| | **SD** | `GPIO 46` | I2S Serial Data In (Input Only) |
| | **L/R** | `GND` | Kênh Trái (Left Channel) |

**Các hiệu ứng âm thanh hệ thống (Audio Effects):**
- 🚀 **Khởi động (Startup)**: Arpeggio 4 nốt tươi sáng (C5 -> E5 -> G5 -> C6).
- 👤 **Nhận diện khuôn mặt (Face Detected)**: Âm "Ding-dong / Ting" ấm áp (kèm bộ đệm Cooldown 3s chống lặp khó chịu).
- 🌐 **WiFi Connected**: Giai điệu Chime 2 nốt báo đã có mạng Internet.
- ☁️ **Upload Success / Failed**: Âm Ting nhẹ khi ảnh lên Drive thành công hoặc âm cảnh báo trầm khi lỗi mạng.
- 🔘 **Nút nhấn BOOT**: Tiếng Click 25ms phản hồi thính giác tức thì.
- ⚙️ **Captive Portal**: Âm 3 nốt chào mừng vào chế độ cấu hình AP `ESP32S3-CAM-AP`.

---

## 🧠 Mô hình AI & Cơ chế nhận diện khuôn mặt (Face Detection)

### 1. Thư viện & Mô hình Deep Learning
- **Framework**: **ESP-DL / ESP-WHO** (Khung phần mềm Deep Learning chính thức từ Espressif Systems).
- **Mô hình AI**: **MSR01 (Mobile Screen Recognition v1)**:
  - Mạng nơ-ron tích chập siêu nhẹ (**Lightweight CNN**) được tối ưu hóa riêng cho thị giác máy tính biên (Edge AI).
  - Lượng tử hóa **INT8 / INT16** và tăng tốc phần cứng bằng tập lệnh **Vector Extensions** của vi xử lý Xtensa Dual-Core LX7 trên ESP32-S3.
  - Nhận diện cùng lúc lên tới **5 khuôn mặt** với độ trễ cực thấp.

### 2. Kiến trúc xử lý Bất đồng bộ Đa lõi (Dual-Core Asynchronous Pipeline)

Để đảm bảo luồng video và hiển thị màn hình đạt **FPS cao mượt mà (18 - 25 FPS)** mà không bị đứng hình khi mạng nơ-ron AI tính toán, hệ thống phân chia luồng xử lý độc lập giữa 2 nhân CPU:

```text
[ Core 1: Real-time Pipeline ]                     [ Core 0: AI Deep Learning Worker ]
Camera OV3660 (320x240 RGB565)
        │
        ├───> Copy Frame vào Buffer PSRAM ──(Async)───> [HumanFaceDetectMSR01::infer()]
        │                                                              │
        ├───> Render ST7789 LCD (18-25 FPS) <──(Mutex Sync)────────────┤ (Tính toán 6-10 FPS)
        │     (Vẽ ảnh + Bounding Boxes + 5 Keypoints)                  │
        │                                                              ▼
        └───> Upload Google Drive Task (Crop Face) <─────── [Face Detection Result]
```

### 3. Dữ liệu đầu ra của khối AI
- **Hộp bao khuôn mặt (Bounding Box)**: Tọa độ pixel `[x1, y1, x2, y2]` tương ứng tỷ lệ màn hình 320x240.
- **Điểm số tin cậy (Confidence Score)**: Mức độ chắc chắn (ngưỡng phát hiện mặc định `≥ 0.30`).
- **5 Điểm mốc khuôn mặt (5 Keypoints)**: Tọa độ 2 mắt, đỉnh mũi và 2 bên khóe miệng (phục vụ căn chỉnh góc nhìn).
- **Tốc độ AI FPS**: Được đo đạc thời gian thực và đồng bộ lên màn hình OSD.

---

## ☁️ Upload Pipeline & Cơ chế chịu lỗi (Fault Tolerance)

### 1. Flow tổng quát

```text
processFaceTrigger() [Core 1 – Main Loop]
│
├─ Điều kiện kích hoạt (cooldown ĐỦ thời gian  VÀ  KHÔNG đang UPLOAD_IN_PROGRESS)
│   └─ Tốc độ upload thực tế = max(cooldown, upload_duration)
│      → Không bao giờ upload 2 file cùng lúc, dù upload chậm hơn cooldown
│
└─ should_trigger = true
    ├─ Crop vùng khuôn mặt từ frame VGA gốc (độ phân giải cao)
    ├─ Convert sang JPEG (chất lượng 90)
    │
    ├─ [SD mounted?] ──YES──→ saveToSDAtomic()          [Lớp 1: Persistent Queue]
    │       │                   ├─ Kiểm tra dung lượng còn trống (> 10 MB)
    │       │                   ├─ Ghi vào file *.tmp trước
    │       │                   ├─ flush() + close() – đảm bảo dữ liệu xuống vật lý
    │       │                   └─ Rename *.tmp → *.jpg  (Atomic – không bị corrupt)
    │       │
    │       └─ FAIL ──────────→ RAM Queue (fallback)    [Lớp 2: In-memory Fallback]
    │
    └─ [SD not mounted] ──────→ RAM Queue               [Lớp 2: In-memory Fallback]

uploadTaskWorker() [Core 0 – Background FreeRTOS Task]
│
├─ Boot: cleanupOrphanTmpFiles()                        [Lớp 3: Boot Recovery]
│         └─ Xóa *.tmp còn sót từ lần chạy trước (mất điện / reset giữa chừng)
│
└─ Loop vô tận:
    ├─ [WiFi mất] → WiFi.reconnect() → retry NTP sync
    │
    ├─ [Quét SD /faces/*.jpg] → upload tuần tự 1 file / vòng lặp
    │   ├─ current_status = UPLOAD_IN_PROGRESS   ← chặn trigger mới ở Core 1
    │   ├─ isValidJPEG() – kiểm tra header SOI/EOI  [Lớp 4: Data Integrity]
    │   ├─ Upload thành công → xóa file khỏi SD    [Lớp 5: Safe Delete after ACK]
    │   └─ Upload thất bại  → giữ nguyên file      [Lớp 6: Auto Retry on next loop]
    │
    └─ [RAM Queue] → nhận 1 job / vòng lặp, upload ngay nếu WiFi OK
```

### 2. Bảng 7 lớp bảo vệ

| Lớp | Cơ chế | Bảo vệ khỏi |
| :--: | :--- | :--- |
| **1** | **Atomic Write** (`.tmp` → rename `.jpg`) | File bị corrupt khi mất điện giữa chừng ghi SD |
| **2** | **RAM Queue fallback** (3 slots) | Không có thẻ SD hoặc SD ghi thất bại |
| **3** | **Boot cleanup** – xóa `*.tmp` còn sót | Orphan files từ phiên trước bị reset |
| **4** | **JPEG Validation** – kiểm tra SOI/EOI | Upload ảnh bị hỏng lên Google Drive |
| **5** | **Safe Delete after ACK** | Xóa file khi chưa upload xong |
| **6** | **Auto Retry** – giữ file trên SD nếu thất bại | Mất kết nối mạng tạm thời |
| **7** | **Space Management** – xóa file cũ nhất khi < 10 MB | SD card đầy, không ghi được |

### 3. Hành vi khi upload chậm hơn cooldown

```text
Ví dụ: Cooldown = 5s, thời gian upload thực tế = 8s

❌ Hành vi CŨ (trước fix):
  t = 0s  → Trigger, lưu file A vào SD
  t = 0s  → Worker bắt đầu upload file A
  t = 5s  → Cooldown hết → Trigger lại, lưu file B vào SD (upload A vẫn chưa xong!)
  t = 8s  → Worker xong file A → bắt đầu file B ngay lập tức
  ⚠️ Kết quả: 2 file trong queue, nhịp upload phụ thuộc vào cooldown chứ không phải network

✅ Hành vi MỚI (sau fix):
  t = 0s  → Trigger, lưu file A vào SD
  t = 0s  → Worker bắt đầu upload file A [UPLOAD_IN_PROGRESS]
  t = 5s  → Cooldown hết NHƯNG đang IN_PROGRESS → SKIP, không trigger
  t = 8s  → Worker xong file A [UPLOAD_SUCCESS]
  t = 10s → Cooldown hết VÀ không IN_PROGRESS → Trigger, lưu file B
  ✅ Kết quả: Tối đa 1 file pending tại mọi thời điểm, nhịp upload thực tế = max(cooldown, upload_duration)
```

---

## 📁 Cấu trúc thư mục dự án


```text
esp32-project/
├── data/
│   └── index.html               # Giao diện Web Captive Portal (Lưu trong LittleFS)
├── include/                     # Header files cấu hình mở rộng
├── src/
│   ├── camera_pins.h            # Định nghĩa chân kết nối Camera OV3660/OV2640
│   ├── camera_service.h         # Khởi tạo và quản lý Driver Camera
│   ├── display.h                # Cấu hình Driver LovyanGFX cho màn hình ST7789
│   ├── display_service.h/.cpp   # Quản lý hiển thị HUD, OSD, FPS, Bounding Boxes
│   ├── face_detector.h/.cpp     # Luồng nhận diện khuôn mặt AI (Core 0)
│   ├── google_drive_service.h/.cpp # Quản lý kết nối WiFi, crop face & upload Google Drive
│   ├── led_service.h/.cpp       # Quản lý đèn LED RGB GPIO 48 hiển thị trạng thái hệ thống
│   ├── portal_service.h/.cpp    # Quản lý Captive Portal, DNS & Lưu cấu hình NVS
│   ├── upload_types.h           # Enum UploadStatus dùng chung (tránh circular dependency)
│   └── main.cpp                 # Điểm khởi chạy chính & điều khiển nút bấm
├── platformio.ini               # Cấu hình PlatformIO (Board, Flash, PSRAM, Thư viện)
└── README.md
```

---

## 🚀 Hướng dẫn cài đặt & Nạp code

### 1. Yêu cầu môi trường

- Cài đặt **VS Code** với Extension **PlatformIO IDE** (hoặc sử dụng PlatformIO Core CLI).
- Cài đặt Driver USB tương ứng cho ESP32-S3 (CH343 / CP2102 / USB-CDC).

### 2. Cấu hình `platformio.ini`

Dự án đã được cấu hình tối ưu cho dòng ESP32-S3 N16R8:
```ini
[env:esp32s3]
platform = espressif32 @ 6.5.0
board = esp32-s3-devkitc-1
framework = arduino

monitor_speed = 115200
upload_speed = 921600

build_flags = 
    -DCORE_DEBUG_LEVEL=0
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue
    -DARDUINO_USB_CDC_ON_BOOT=0

board_build.arduino.memory_type = qio_opi
board_build.flash_mode = qio
board_build.psram_type = opi
board_build.flash_size = 16MB
board_build.filesystem = littlefs

lib_deps = 
    esp32-camera
    lovyan03/LovyanGFX @ ^1.1.16
```

### 3. Nạp Filesystem (Thư mục tĩnh Public `data/`)

> [!IMPORTANT]
> Toàn bộ các file bạn đặt trong thư mục `data/` (như `index.html`, `style.css`, `app.js`, hình ảnh `.png`, `.jpg`, `.ico`, v.v.) sẽ hoạt động như một **Static Public Web Server**. Khi truy cập URL tương ứng (ví dụ `http://192.168.4.1/style.css`), thiết bị sẽ tự động phục vụ file từ LittleFS với đúng MIME type.

```bash
# Nạp toàn bộ dữ liệu thư mục data/ lên Flash LittleFS của ESP32-S3
pio run --target uploadfs
```

### 4. Build và Nạp Firmware

```bash
# Build mã nguồn
pio run

# Nạp Firmware lên ESP32-S3
pio run --target upload

# Mở Serial Monitor theo dõi log
pio device monitor
```

---

## ⚙️ Hướng dẫn cấu hình hệ thống (Captive Portal)

1. Khi bật nguồn lần đầu (hoặc sau khi giữ nút BOOT khi cắm nguồn), ESP32-S3 sẽ phát mạng WiFi Access Point:
   - **SSID**: `ESP32S3-CAM-AP`
   - **Mật khẩu**: Không có (Open)
2. Kết nối điện thoại hoặc máy tính vào WiFi trên, trình duyệt sẽ tự động mở trang cấu hình hoặc truy cập:
   - **Địa chỉ**: `http://192.168.4.1`
3. Điền và tùy chỉnh các thông số:
   - **WiFi SSID & Mật khẩu**: Mạng WiFi kết nối Internet (hỗ trợ quét WiFi trực tiếp).
   - **Google Apps Script URL**: Đường dẫn Webhook nhận ảnh khuôn mặt.
   - **Tần suất Upload (giây)**: Khoảng thời gian tối thiểu giữa các lần upload (5s, 10s, 15s, 30s, 60s).
   - **Độ phân giải Camera**: Chế độ VGA (640x480) hoặc QVGA (320x240).
   - **Đèn LED chỉ báo trạng thái**: Switch button Bật / Tắt đèn LED GPIO 48 (Mặc định: BẬT).
4. Nhấn **Lưu cấu hình** -> ESP32-S3 sẽ tự động lưu vào bộ nhớ NVS và khởi động vào luồng Camera AI.

---

## ☁️ Hướng dẫn tạo Google Apps Script Webhook

1. Truy cập [Google Apps Script](https://script.google.com/) và tạo dự án mới.
2. Dán đoạn mã sau vào `Code.gs`:

```javascript
function doPost(e) {
  try {
    var data = JSON.parse(e.postData.contents);
    var imageBytes = Utilities.base64Decode(data.image);
    var blob = Utilities.newBlob(imageBytes, "image/jpeg", "face_" + new Date().getTime() + ".jpg");
    
    // Lưu vào thư mục Google Drive mong muốn (hoặc thư mục gốc)
    var folder = DriveApp.getRootFolder();
    var file = folder.createFile(blob);
    
    return ContentService.createTextOutput(JSON.stringify({
      status: "success",
      url: file.getUrl()
    })).setMimeType(ContentService.MimeType.JSON);
  } catch (error) {
    return ContentService.createTextOutput(JSON.stringify({
      status: "error",
      message: error.toString()
    })).setMimeType(ContentService.MimeType.JSON);
  }
}
```

3. Nhấn **Deploy** > **New deployment** > Chọn loại **Web app**:
   - **Execute as**: *Me*
   - **Who has access**: *Anyone* (Bất kỳ ai)
4. Sao chép URL Web App thu được và dán vào mục **Google Script URL** trong Captive Portal.

---

## 🎮 Thao tác điều khiển với nút BOOT

| Thao tác | Chức năng | Phản hồi trên màn hình |
| :--- | :--- | :--- |
| **Nhấn 1 lần (Click)** | Bật / Tắt chế độ tải ảnh lên Google Drive | Toast `Upload: ON` (Xanh) / `Upload: OFF` (Đỏ) |
| **Nhấn giữ 1 giây** | Tắt / Bật đèn nền màn hình ST7789 | Tắt màn hình hoặc Toast `Screen: ON` |
| **Nhấn giữ 3 giây** | Khôi phục cài đặt gốc (Xóa NVS) & Khởi động lại | Toast `Factory Reset...` (Đỏ) |
| **Giữ nút khi cắm nguồn** | Ép buộc vào chế độ Captive Portal | Hiển thị thông tin kết nối AP |

---

## 📊 Giấy phép & Đóng góp

Dự án phát triển mã nguồn mở phục vụ nghiên cứu và ứng dụng Edge AI trên dòng vi điều khiển ESP32-S3. Mọi đóng góp và báo lỗi (Issue / Pull Request) luôn được chào đón!
