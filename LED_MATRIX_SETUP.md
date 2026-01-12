# Hướng Dẫn Kết Nối LED Matrix P5 với ESP32

## 📋 Giới Thiệu

LED Matrix P5 là loại màn hình LED RGB full color với khoảng cách giữa các pixel là 5mm. Module thường có kích thước 64x32 pixels và sử dụng giao thức HUB75 để điều khiển.

## 🔌 Sơ Đồ Kết Nối

### Pin Mapping HUB75 ↔ ESP32

| HUB75 Pin | ESP32 GPIO | Chức Năng     | Mô Tả                             |
| --------- | ---------- | ------------- | --------------------------------- |
| **R1**    | GPIO25     | Red Data 1    | Dữ liệu màu đỏ (nửa trên)         |
| **G1**    | GPIO26     | Green Data 1  | Dữ liệu màu xanh lá (nửa trên)    |
| **B1**    | GPIO27     | Blue Data 1   | Dữ liệu màu xanh dương (nửa trên) |
| **R2**    | GPIO14     | Red Data 2    | Dữ liệu màu đỏ (nửa dưới)         |
| **G2**    | GPIO12     | Green Data 2  | Dữ liệu màu xanh lá (nửa dưới)    |
| **B2**    | GPIO13     | Blue Data 2   | Dữ liệu màu xanh dương (nửa dưới) |
| **A**     | GPIO23     | Address A     | Chọn hàng bit 0                   |
| **B**     | GPIO19     | Address B     | Chọn hàng bit 1                   |
| **C**     | GPIO5      | Address C     | Chọn hàng bit 2                   |
| **D**     | GPIO17     | Address D     | Chọn hàng bit 3                   |
| **E**     | GPIO18     | Address E     | Chọn hàng bit 4 (panel >32 rows)  |
| **CLK**   | GPIO16     | Clock         | Tín hiệu xung clock               |
| **LAT**   | GPIO4      | Latch         | Tín hiệu chốt dữ liệu             |
| **OE**    | GPIO15     | Output Enable | Kích hoạt đầu ra (active LOW)     |
| **GND**   | GND        | Ground        | Đất                               |

### Connector HUB75

```
┌─────────────────────────────────┐
│  R1  G1  B1  GND                │
│  R2  G2  B2  GND                │
│   A   B   C   D   CLK  LAT  OE  │
│                         GND     │
└─────────────────────────────────┘
```

## ⚡ Cấp Nguồn

### Yêu Cầu Nguồn

- **Điện áp**: 5V DC
- **Dòng điện**:
  - 1 panel 64x32: ~2-4A (tùy độ sáng)
  - Nhiều panel: tính ~60mA/pixel ở độ sáng tối đa
- **Khuyến nghị**: Sử dụng nguồn 5V/5A trở lên cho 1 panel

### Lưu Ý Quan Trọng

> [!CAUTION] > **KHÔNG** cấp nguồn cho LED matrix từ ESP32! ESP32 chỉ cung cấp tín hiệu điều khiển.

> [!IMPORTANT]
>
> - LED matrix cần nguồn 5V riêng biệt
> - ESP32 cần nguồn 5V hoặc 3.3V riêng (có thể dùng chung nguồn 5V với LED matrix qua USB)
> - Nối chung GND giữa ESP32 và LED matrix

### Sơ Đồ Cấp Nguồn

```
┌──────────────┐
│  Nguồn 5V    │
│   (5A+)      │
└──────┬───────┘
       │
       ├─────────► LED Matrix (5V, GND)
       │
       └─────────► ESP32 (VIN/5V, GND)
                   hoặc qua USB
```

## 🛠️ Cài Đặt Thư Viện

Thư viện đã được cấu hình trong `platformio.ini`:

```ini
lib_deps =
    https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA.git
    adafruit/Adafruit GFX Library@^1.11.9
    adafruit/Adafruit BusIO@^1.16.1
```

## 💻 Code Mẫu

### Khởi Tạo Cơ Bản

```cpp
#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#define PANEL_RES_X 64
#define PANEL_RES_Y 32
#define PANEL_CHAIN 1

MatrixPanel_I2S_DMA *dma_display = nullptr;

void setup() {
  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN);

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  dma_display->begin();
  dma_display->setBrightness8(90); // 0-255
  dma_display->clearScreen();
}
```

### Vẽ Pixel

```cpp
// Vẽ 1 pixel màu đỏ tại (10, 10)
uint16_t red = dma_display->color565(255, 0, 0);
dma_display->drawPixel(10, 10, red);
```

### Vẽ Text

```cpp
dma_display->setTextSize(1);
dma_display->setCursor(2, 2);
dma_display->setTextColor(dma_display->color565(255, 255, 0));
dma_display->println("Hello");
```

### Vẽ Hình

```cpp
// Hình chữ nhật
dma_display->drawRect(x, y, width, height, color);
dma_display->fillRect(x, y, width, height, color);

// Hình tròn
dma_display->drawCircle(x, y, radius, color);
dma_display->fillCircle(x, y, radius, color);

// Đường thẳng
dma_display->drawLine(x0, y0, x1, y1, color);
```

## 🎨 Làm Việc Với Màu Sắc

### Định Dạng Màu RGB565

LED matrix sử dụng định dạng RGB565 (16-bit):

- 5 bit đỏ (0-31)
- 6 bit xanh lá (0-63)
- 5 bit xanh dương (0-31)

```cpp
// Chuyển đổi RGB888 (0-255) sang RGB565
uint16_t color = dma_display->color565(r, g, b);

// Ví dụ
uint16_t red    = dma_display->color565(255, 0, 0);
uint16_t green  = dma_display->color565(0, 255, 0);
uint16_t blue   = dma_display->color565(0, 0, 255);
uint16_t white  = dma_display->color565(255, 255, 255);
uint16_t yellow = dma_display->color565(255, 255, 0);
```

## 🔧 Cấu Hình Nâng Cao

### Nhiều Panel Nối Tiếp

```cpp
#define PANEL_CHAIN 2  // 2 panel nối tiếp
// Tổng độ phân giải: 128x32 (2 panel 64x32)
```

### Panel Lớn Hơn 32 Hàng

```cpp
HUB75_I2S_CFG mxconfig(64, 64, 1);
mxconfig.gpio.e = 18;  // Kích hoạt pin E
```

### Thay Đổi Pin Mapping

```cpp
HUB75_I2S_CFG::i2s_pins _pins = {
  R1_PIN, G1_PIN, B1_PIN, R2_PIN, G2_PIN, B2_PIN,
  A_PIN, B_PIN, C_PIN, D_PIN, E_PIN,
  LAT_PIN, OE_PIN, CLK_PIN
};
HUB75_I2S_CFG mxconfig(64, 32, 1, _pins);
```

### Driver IC Đặc Biệt

Một số panel sử dụng driver IC FM6126A:

```cpp
mxconfig.driver = HUB75_I2S_CFG::FM6126A;
```

## 📊 Hiệu Năng

### DMA (Direct Memory Access)

Thư viện sử dụng DMA để cập nhật màn hình mà không làm gián đoạn CPU:

- Refresh rate: ~60Hz
- Không cần `delay()` trong loop
- CPU tự do xử lý logic khác

### Tối Ưu Bộ Nhớ

```cpp
// Giảm bộ đệm nếu thiếu RAM
mxconfig.double_buff = false;  // Tắt double buffering
```

## 🐛 Xử Lý Sự Cố

### Màn Hình Không Sáng

1. Kiểm tra nguồn 5V cho LED matrix
2. Kiểm tra kết nối GND chung
3. Kiểm tra cáp HUB75 cắm đúng hướng

### Màu Sắc Sai

1. Kiểm tra kết nối R1, G1, B1, R2, G2, B2
2. Thử đổi driver IC: `mxconfig.driver = HUB75_I2S_CFG::FM6126A;`

### Hình Ảnh Bị Lệch

1. Kiểm tra pin A, B, C, D
2. Với panel >32 rows, kích hoạt pin E

### Nhấp Nháy

1. Tăng độ sáng: `setBrightness8(255)`
2. Kiểm tra nguồn đủ dòng

## 📚 Tài Liệu Tham Khảo

- [ESP32-HUB75-MatrixPanel-DMA GitHub](https://github.com/mrfaptastic/ESP32-HUB75-MatrixPanel-DMA)
- [Adafruit GFX Graphics Library](https://learn.adafruit.com/adafruit-gfx-graphics-library)
- [HUB75 Protocol Specification](https://www.sparkfun.com/sparkle/hub75.pdf)

## ⚙️ Build & Upload

```bash
# Build project
pio run

# Upload to ESP32
pio run --target upload

# Monitor serial output
pio device monitor
```

Hoặc sử dụng script có sẵn:

```bash
./upload.sh    # Upload code
./monitor.sh   # Xem serial monitor
```
