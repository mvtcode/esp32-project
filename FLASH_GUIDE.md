# Hướng Dẫn Đóng Gói & Nạp Firmware ESP32 (Single Binary 0x00)

Tài liệu này hướng dẫn chi tiết cách gom toàn bộ các thành phần firmware (Bootloader, Partition Table, OTA Data, Application) thành **một file binary duy nhất** (`merged_firmware_0x00.bin`) để nạp vào board ESP32 tương tự từ địa chỉ **`0x00` (hoặc `0x00000000`)**.

Cách đóng gói này cực kỳ thuận tiện khi sản xuất hàng loạt, chuyển giao firmware cho người khác hoặc nạp cứu hộ thiết bị mà không cần cài đặt PlatformIO / Arduino IDE.

---

## 1. Cấu Trúc Bộ Nhớ ESP32 (Flash Layout)

Khi biên dịch từ PlatformIO với phân vùng `min_spiffs.csv`, bộ nhớ Flash 4MB của ESP32 được cấu trúc như sau:

| Offset (Hex) | Kích thước | Thành phần | File gốc | Chức năng |
| :--- | :---: | :--- | :--- | :--- |
| **`0x0000`** | 4 KB | Padding (`0xFF`) | *(Trống)* | Vùng dự trữ của ESP32 |
| **`0x1000`** | ~17.5 KB | **Bootloader** | `.pio/build/esp32dev/bootloader.bin` | Khởi tạo phần cứng ban đầu |
| **`0x8000`** | 3 KB | **Partition Table** | `.pio/build/esp32dev/partitions.bin` | Định nghĩa sơ đồ phân vùng Flash |
| **`0xe000`** | 8 KB | **OTA Data (boot_app0)** | `tools/boot_app0.bin` | Chỉ định bootloader nạp từ phân vùng `app0` |
| **`0x10000`** | ~1.62 MB | **Application Firmware**| `.pio/build/esp32dev/firmware.bin` | Mã nguồn chương trình chính của thiết bị |

> **Tại sao cần gộp `boot_app0.bin` ở `0xe000`?**  
> Bản phân vùng `min_spiffs.csv` hỗ trợ 2 phân vùng ứng dụng OTA (`app0` và `app1`). Nếu thiếu dữ liệu OTA ở địa chỉ `0xe000`, bootloader có thể không xác định được phân vùng khởi động hợp lệ khi nạp vào một chip ESP32 hoàn toàn mới (chip trắng). Việc chèn sẵn `boot_app0.bin` đảm bảo chip luôn boot ngay vào `app0` (`0x10000`) sau khi bật nguồn.

---

## 2. Cách Tạo File Firmware Gộp (`merged_firmware_0x00.bin`)

### Cách 1: Dùng Script Tự Động (Khuyên dùng)
Dự án đã tích hợp sẵn script Python độc lập tại [`tools/merge_firmware.py`](tools/merge_firmware.py). Script này không phụ thuộc vào `esptool`, chạy bằng Python thuần:

1. Biên dịch dự án PlatformIO:
   ```powershell
   pio run
   ```
2. Chạy script đóng gói:
   ```powershell
   python tools/merge_firmware.py
   ```
3. Kết quả: File [`merged_firmware_0x00.bin`](merged_firmware_0x00.bin) (~1.69 MB) sẽ được tạo ngay tại thư mục gốc của dự án.

---

### Cách 2: Dùng Công Cụ `esptool merge_bin`
Nếu máy tính có cài sẵn `esptool` (hoặc thông qua môi trường PlatformIO):

```powershell
esptool.py --chip esp32 merge_bin `
  --target-offset 0x0 `
  -o merged_firmware_0x00.bin `
  --flash_mode dio `
  --flash_freq 40m `
  --flash_size 4MB `
  0x1000 .pio/build/esp32dev/bootloader.bin `
  0x8000 .pio/build/esp32dev/partitions.bin `
  0xe000 tools/boot_app0.bin `
  0x10000 .pio/build/esp32dev/firmware.bin
```

---

## 3. Hướng Dẫn Nạp Vào Thiết Bị

Bạn có thể chọn 1 trong 3 phương pháp nạp dưới đây:

### Phương pháp 1: Dùng phần mềm ESP Flash Download Tool (Chính hãng Espressif)
*Thích hợp cho nạp hàng loạt trên Windows mà không cần biết dòng lệnh.*

1. **Tải phần mềm:** Tải bản mới nhất của [Espressif Flash Download Tools](https://www.espressif.com/en/support/download/other-tools).
2. **Khởi động:**
   - **Chip Type:** Chọn `ESP32`
   - **WorkMode:** Chọn `Develop`
3. **Cấu hình file nạp (Tab SPIDownload):**
   - Ô đường dẫn `...`: Chọn file `merged_firmware_0x00.bin`
   - Ô địa chỉ bên phải: Nhập `0x0` (hoặc `0x00000000`)
   - **Tích chọn ô vuông màu xanh** ở đầu dòng để kích hoạt file này.
4. **Cấu hình thông số Flash:**
   - **SPI SPEED:** `40MHz`
   - **SPI MODE:** `DIO`
   - **FLASH SIZE:** `32Mbit` (tương đương 4MB)
5. **Chọn cổng kết nối:**
   - **COM:** Chọn cổng COM của ESP32 (xem trong Device Manager).
   - **BAUD:** Chọn `921600` (hoặc `460800` để nạp nhanh).
6. **Bắt đầu nạp:**
   - Bấm **START**.
   - Nếu thanh tiến trình đứng yên ở `SYNC`, hãy **nhấn và giữ nút BOOT** trên board khoảng 2 giây rồi thả ra.
   - Chờ đến khi hiện chữ **FINISH**. Nhấn nút **EN/RST** trên board để khởi động lại thiết bị.

---

### Phương pháp 2: Nạp bằng dòng lệnh `esptool` (CLI)
*Thích hợp cho kỹ thuật viên, tự động hóa script hoặc người dùng Linux/macOS.*

Chạy lệnh sau trong PowerShell hoặc Terminal:
```powershell
esptool.py --chip esp32 --port COMx --baud 921600 write_flash 0x00 merged_firmware_0x00.bin
```
*(Thay `COMx` bằng cổng COM thực tế trên Windows, hoặc `/dev/ttyUSB0` trên Linux, `/dev/cu.usbserial-*` trên macOS)*.

---

### Phương pháp 3: Nạp trực tiếp qua trình duyệt Web (Web Serial)
*Cực kỳ tiện lợi để gửi cho khách hàng/người dùng cuối tự nạp mà không cần cài bất kỳ phần mềm nào.*

1. Mở trình duyệt **Google Chrome** hoặc **Microsoft Edge**.
2. Truy cập: [Espressif Web Flasher](https://espressif.github.io/esptool-js/) hoặc [Adafruit WebSerial ESPTool](https://adafruit.github.io/Adafruit_WebSerial_ESPTool/).
3. Cắm cáp USB nối ESP32 với máy tính.
4. Bấm **Connect** và chọn cổng COM của thiết bị.
5. Tại mục Program/Flash:
   - File: Chọn `merged_firmware_0x00.bin`.
   - Offset/Address: Nhập `0x0`.
6. Bấm **Program** và đợi hoàn tất 100%.

---

## 4. Xử Lý Lỗi Thường Gặp (Troubleshooting)

### 1. Không nhận diện được cổng COM
- Cài đặt driver chip nạp tương ứng của board:
  - Driver **CH340 / CH341**: [Tải driver WCH](http://www.wch-ic.com/downloads/CH341SER_EXE.html)
  - Driver **CP2102 / CP2104**: [Tải driver Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)
- Đảm bảo cáp USB là cáp truyền dữ liệu (Data Cable), không phải cáp chỉ có chức năng sạc.

### 2. Báo lỗi `A fatal error occurred: Failed to connect to ESP32: Timed out waiting for packet header`
- Board chưa vào chế độ Bootloader (Download Mode).
- **Cách xử lý:** Giữ nút **BOOT (IO0)** trên mạch, cắm cáp USB hoặc bấm nhanh nút **EN/RST**, sau đó thả nút BOOT ra rồi bấm Flash lại.

### 3. Thiết bị bị Bootloop hoặc màn hình trắng sau khi nạp
- Có thể bộ nhớ Flash cũ còn lưu các thông số NVS/WiFi cấu hình sai.
- Hãy xóa sạch Flash trước khi nạp lại bằng lệnh:
  ```powershell
  esptool.py --chip esp32 --port COMx erase_flash
  ```
  Sau đó nạp lại file `merged_firmware_0x00.bin` tại `0x00`.
