# Hướng dẫn sử dụng PlatformIO với ESP32

## Cài đặt PlatformIO

### Cách 1: Cài đặt PlatformIO CLI (Command Line Interface)

```bash
# Cài đặt PlatformIO Core
pip install -U platformio

# Hoặc sử dụng Homebrew trên macOS
brew install platformio
```

### Cách 2: Sử dụng VS Code Extension

1. Mở VS Code
2. Vào Extensions (⌘+Shift+X)
3. Tìm "PlatformIO IDE"
4. Click Install

## Cấu trúc dự án

```
P5_Matrix/
├── platformio.ini      # File cấu hình dự án
├── src/                # Thư mục chứa source code
│   └── main.cpp        # File code chính (thay thế .ino)
├── lib/                # Thư mục chứa thư viện tự tạo
├── include/            # Thư mục chứa header files
└── .pio/               # Thư mục build (tự động tạo)
```

## Các lệnh cơ bản

### Build dự án

```bash
pio run
```

### Upload code lên ESP32

```bash
pio run --target upload
```

### Mở Serial Monitor

```bash
pio device monitor
```

### Build + Upload + Monitor (một lệnh)

```bash
pio run --target upload && pio device monitor
```

### Clean build files

```bash
pio run --target clean
```

## Cấu hình trong platformio.ini

File `platformio.ini` đã được cấu hình với:

- **Platform**: espressif32 (ESP32)
- **Board**: esp32dev (ESP32 DevKit)
- **Framework**: Arduino
- **Monitor speed**: 115200 baud
- **Upload speed**: 921600 baud

### Thêm thư viện

Để thêm thư viện, uncomment và chỉnh sửa phần `lib_deps` trong `platformio.ini`:

```ini
lib_deps =
    adafruit/Adafruit NeoPixel@^1.10.0
    fastled/FastLED@^3.5.0
```

Hoặc sử dụng lệnh:

```bash
pio pkg install --library "adafruit/Adafruit NeoPixel@^1.10.0"
```

## Chọn board ESP32 khác

Nếu bạn sử dụng board ESP32 khác, thay đổi giá trị `board` trong `platformio.ini`:

```ini
[env:esp32dev]
board = esp32dev          # ESP32 DevKit V1
# board = esp32-s3-devkitc-1  # ESP32-S3
# board = esp32-c3-devkitm-1  # ESP32-C3
# board = nodemcu-32s     # NodeMCU-32S
```

Xem danh sách đầy đủ: https://docs.platformio.org/en/latest/boards/index.html#espressif-32

## Chọn cổng Serial

PlatformIO tự động phát hiện cổng, nhưng bạn có thể chỉ định cụ thể:

```ini
upload_port = /dev/cu.usbserial-0001
monitor_port = /dev/cu.usbserial-0001
```

Xem danh sách cổng:

```bash
pio device list
```

## So sánh với Arduino IDE

| Arduino IDE                                    | PlatformIO                                                 |
| ---------------------------------------------- | ---------------------------------------------------------- |
| `.ino` file                                    | `.cpp` file trong thư mục `src/`                           |
| Thư viện trong `~/Documents/Arduino/libraries` | Thư viện trong `lib/` hoặc khai báo trong `platformio.ini` |
| Chọn board qua GUI                             | Cấu hình trong `platformio.ini`                            |
| Serial Monitor trong IDE                       | `pio device monitor`                                       |

## Chọn cổng Serial

### Tự động phát hiện cổng (khuyến nghị)

PlatformIO tự động phát hiện cổng ESP32 khi upload:

```bash
pio run --target upload
```

### Xem danh sách cổng

```bash
pio device list
```

### Chỉ định cổng cụ thể

Nếu PlatformIO phát hiện sai cổng, chỉ định cổng thủ công:

```bash
# Upload với cổng cụ thể
pio run --target upload --upload-port /dev/cu.usbserial-0001

# Monitor với cổng cụ thể
pio device monitor --port /dev/cu.usbserial-0001
```

### Script tiện lợi (khuyến nghị)

Dự án đã có sẵn 2 script tự động tìm cổng ESP32:

**Upload code:**

```bash
./upload.sh
```

**Mở Serial Monitor:**

```bash
./monitor.sh
```

Các script này sẽ tự động tìm cổng ESP32 (usbserial, SLAB_USBtoUART) và thực hiện upload/monitor.

## Troubleshooting

### Lỗi không tìm thấy cổng

```bash
# Kiểm tra quyền truy cập (Linux/macOS)
sudo usermod -a -G dialout $USER
# hoặc
sudo chmod 666 /dev/ttyUSB0
```

### Lỗi upload

- Nhấn giữ nút BOOT trên ESP32 khi upload
- Thử giảm upload_speed xuống 115200

### Lỗi thiếu driver

- Cài đặt driver CH340/CP2102 cho chip USB-to-Serial của board
