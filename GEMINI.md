# GEMINI.md

## Project Rules & Guidelines

### 1. PlatformIO Command Execution Rules
- **Chỉ để người dùng chạy lệnh upload và monitor**:
  - `pio run -t upload`
  - `pio device monitor`
  - Các script liên quan: `./upload.sh`, `./monitor.sh`
- **Quy tắc cho AI**:
  - **TUYỆT ĐỐI KHÔNG** tự ý chạy các lệnh nạp code (`pio run -t upload`) hay mở Serial Monitor (`pio device monitor`) qua terminal.
  - Khi cần nạp firmware hoặc theo dõi log qua cổng COM, AI chỉ cung cấp/hướng dẫn câu lệnh chính xác để người dùng tự thực thi trên terminal của họ.
  - **Quyền hạn build test**: AI được phép chạy lệnh biên dịch `pio run` để kiểm tra lỗi cú pháp và đảm bảo code build thành công trước khi hoàn thành task.

### 2. Architecture & Code Structure Guidelines
- **Tách biệt tính năng (Modularity)**:
  - Mỗi tính năng/module ưu tiên tách biệt thành class hoặc function riêng trong các cặp file `.h` và `.cpp` độc lập (trong thư mục `src/` hoặc `include/`).
  - Tránh viết dồn toàn bộ logic vào `main.cpp`.
- **Quản lý vòng đời & Bộ nhớ (Tránh Memory Leak)**:
  - Mọi class có cấp phát động bộ nhớ (heap, buffers, task handles, timer handles, thư viện ngoài...) **bắt buộc** phải có **Constructor** để khởi tạo và **Destructor** (`~ClassName()` hoặc phương thức `destroy()`/`cleanup()` tương đương) để thu hồi và giải phóng toàn bộ tài nguyên.
  - Áp dụng nguyên tắc RAII, kiểm tra con trỏ trước và sau khi giải phóng (`nullptr` check & safe delete/free).

### 3. Hardware & Pin Configuration
- **Quản lý chân GPIO tập trung**:
  - Toàn bộ khai báo chân GPIO (I2S, SPI, I2C, TFT, Touch, SD Card, LED, Buttons...) phải gom về **duy nhất 1 file cấu hình tập trung** (ví dụ: `include/pin_config.h` hoặc `include/pins.h`).
  - Tuyệt đối không hardcode chân GPIO rải rác bên trong các file logic/module khác nhằm giúp dễ tra cứu, thay đổi và tránh xung đột chân.

### 4. Concurrency, Timing & Logging
- **Non-blocking Execution**:
  - Tuyệt đối tránh dùng hàm chặn `delay()` trong luồng chính `loop()` và các tác vụ thường trực.
  - Sử dụng non-blocking timer với `millis()` hoặc `vTaskDelay()` của FreeRTOS để tránh kích hoạt Watchdog Timer (WDT) gây reboot ESP32.
- **Quy chuẩn Logging tập trung**:
  - Bắt buộc dùng hệ thống macro trong `include/log.h` (`LOG_I`, `LOG_D`, `LOG_W`, `LOG_E`) đi kèm `TAG` module.
  - Không dùng `Serial.print()` hay `Serial.println()` tùy tiện để đảm bảo khi tắt cờ `-DENABLE_SERIAL_LOG` trong `platformio.ini`, toàn bộ log được lược bỏ sạch ở compile-time nhằm tối ưu Flash/RAM và FPS.
- **An toàn bộ nhớ Stack & Heap**:
  - Không khai báo các mảng/buffer dữ liệu lớn (> 1KB) trên Stack của hàm/task vì stack ESP32 rất nhỏ (dễ gây crash do Stack Overflow).
  - Cấp phát buffer lớn trên Heap hoặc tận dụng PSRAM (`ps_malloc` / `heap_caps_malloc`) nếu có.

### 5. Network & Configuration Management
- **WiFi Captive Portal**:
  - Với các ứng dụng cần WiFi để cấu hình, sử dụng cơ chế **WiFi Captive Portal** khi chưa có thông tin WiFi hoặc mất kết nối.
  - Ưu tiên lưu và nạp giao diện Web UI từ file `/data/index.html` (dùng LittleFS / SPIFFS) thay vì nhúng chuỗi HTML dài vào code C++.
- **Cơ chế Reconnect WiFi định kỳ**:
  - Khi khởi động không kết nối được WiFi (hoặc bị rớt mạng trong quá trình chạy), phải có cơ chế tự động thử kết nối lại (reconnect) định kỳ không chặn (non-blocking) chu kỳ **10 - 15 giây/lần**, không để thiết bị bị treo đơ trong lúc chờ mạng.
- **Lưu cấu hình vào NVS (Non-Volatile Storage)**:
  - Mọi thông số cấu hình và trạng thái cần lưu trữ (thông tin WiFi, âm lượng, chế độ hoạt động, trạng thái trước khi tắt máy...) phải được lưu vào **NVS** (sử dụng thư viện `Preferences` hoặc ESP-IDF `nvs_flash`).
  - Đảm bảo khi khởi động lại hoặc mất nguồn đột ngột, thiết bị không bị mất trạng thái và khôi phục hoạt động bình thường.

### 6. File System & File Header Validation
- **Kiểm tra tính hợp lệ của File qua Header (Magic Bytes)**:
  - Khi đọc file dữ liệu (âm thanh, ảnh, cấu hình...) từ SD Card hay Flash, **bắt buộc phải có bước kiểm tra Header (Magic Bytes)** thực tế của file, không dựa hoàn toàn vào phần mở rộng đuôi file (extension).
  - Ví dụ: Tránh trường hợp file `.wav` đổi tên thành `.mp3` hoặc ngược lại làm crash thư viện giải mã (Audio Decoder / Image Decoder). Cần đọc một vài byte đầu (ví dụ: `ID3` / `0xFFE-0xFFF` cho MP3, `RIFF....WAVE` cho WAV) để xác thực định dạng trước khi xử lý.

### 7. NVS Flash Endurance & Debounce Rules
- **Chống mòn bộ nhớ Flash (Flash Endurance)**:
  - Flash của ESP32 có giới hạn số lần ghi (khoảng 100.000 lần). **Tuyệt đối không gọi lệnh ghi NVS trong hàm loop() chạy liên tục**.
  - **Dirty Check**: Luôn so sánh giá trị mới với giá trị cũ trong RAM trước khi ghi NVS; chỉ ghi khi có sự thay đổi thực sự (`if (newValue != cachedValue)`).
  - **Debouncing**: Với các giá trị thay đổi liên tục bởi người dùng (thanh trượt âm lượng, độ sáng, seek track, v.v.), phải áp dụng cơ chế debounce (trì hoãn 500ms – 1s sau khi người dùng dừng thao tác mới thực hiện ghi vào NVS).

### 8. FreeRTOS Task Safety & Interrupts (ISR)
- **Chống kích hoạt Task Watchdog Timer (Task Starvation)**:
  - Mọi FreeRTOS task có vòng lặp vô hạn `while(1)` **bắt buộc phải có `vTaskDelay(1)` hoặc `vTaskDelay(pdMS_TO_TICKS(...))`** ở cuối mỗi vòng lặp để nhường thời gian cho IDLE task reset Watchdog Timer.
- **Quy chuẩn an toàn trong hàm ngắt (ISR)**:
  - Hàm phục vụ ngắt phần cứng (nút bấm, encoder, touch pad...) bắt buộc khai báo với thuộc tính `IRAM_ATTR`.
  - Tuyệt đối không gọi các hàm blocking, cấp phát động (`malloc`, `new`), log (`Serial.print`, `LOG_*`) hoặc `delay()` trong ISR. Chỉ cập nhật cờ `volatile` hoặc đẩy sự kiện qua `xQueueSendFromISR` để xử lý ở task chính.

### 9. Network & Peripheral Timeouts
- **Giới hạn thời gian chờ bắt buộc (Timeout)**:
  - Mọi tác vụ mạng (HTTPClient, WiFiClient, DNS, NTP, MQTT...) phải luôn cấu hình explicit timeout (ví dụ `client.setTimeout(3000 - 5000)`). Tuyệt đối không để mặc định không giới hạn khiến luồng bị treo vĩnh viễn khi rớt mạng hoặc máy chủ không phản hồi.
  - Các giao tiếp ngoại vi (I2C, SPI) phải luôn có giá trị timeout và kiểm tra mã trả về (`Wire.endTransmission() == 0`), không giả định ngoại vi luôn phản hồi.

### 10. Fault Tolerance & Graceful Degradation (Cơ chế chịu lỗi)
- **Cô lập lỗi và hoạt động suy thoái mềm (Graceful Degradation)**:
  - Khi một ngoại vi gặp sự cố hoặc ngắt kết nối đột ngột (rút thẻ nhớ SD khi đang đọc/phát nhạc, cảm biến I2C hỏng hoặc lỏng dây, DAC I2S mất nguồn, rớt mạng WiFi...), hệ thống **tuyệt đối không được crash hoặc treo cứng** (`Guru Meditation Error`, `LoadProhibited`, `nullptr dereference`).
  - Lỗi của một module độc lập không được làm dừng các module khác (ví dụ: mất thẻ nhớ SD thì dừng phát nhạc và cập nhật trạng thái lỗi, nhưng màn hình UI, đồng hồ, và WiFi vẫn phải hoạt động bình thường).
- **Kiểm tra an toàn con trỏ & trạng thái trước khi thao tác**:
  - Luôn kiểm tra con trỏ khác `nullptr` và kiểm tra cờ sẵn sàng (như `SD.exists()`, trạng thái file handle, kết nối WiFi) trước khi đọc/ghi hoặc gọi hàm thư viện.
- **Tự phục hồi không chặn (Non-blocking Self-Healing)**:
  - Khi một ngoại vi bị lỗi hoặc mất kết nối, hệ thống chuyển module đó sang trạng thái chờ và thử khởi tạo lại (re-init) định kỳ (ví dụ mỗi 5–10 giây một lần với `millis()`), tự động khôi phục hoạt động khi phần cứng được cắm lại mà không cần khởi động lại toàn bộ thiết bị.
- **Bảo vệ toàn vẹn file system khi mất nguồn đột ngột**:
  - Khi ghi dữ liệu vào Flash (LittleFS/SPIFFS) hoặc thẻ nhớ SD, luôn thực hiện `file.flush()` và `file.close()` ngay sau khi hoàn thành khối dữ liệu để tránh làm hỏng cấu trúc FATFS khi ngắt nguồn đột ngột.

### 11. Naming & Coding Conventions (Quy chuẩn đặt tên)
- **Tên File & Thư mục**:
  - Dùng `snake_case` (chữ thường nối dấu gạch dưới) cho tất cả file `.h` và `.cpp` (ví dụ: `pin_config.h`, `audio_player.cpp`, `wifi_service.h`).
  - File header luôn có `#pragma once` ở đầu file.
- **Tên Class, Struct & Enum**:
  - Dùng `PascalCase` cho tên Class, Struct, Enum (ví dụ: `AudioPlayer`, `WifiManager`, `TrackInfo`).
  - Giá trị của Enum dùng `UPPER_SNAKE_CASE` (ví dụ: `enum class PlayerState { STOPPED, PLAYING, PAUSED };`).
- **Chân GPIO & Macro**:
  - Chân GPIO trong file `pin_config.h` bắt buộc có tiền tố **`PIN_`** + `UPPER_SNAKE_CASE` (ví dụ: `PIN_LED_BUILTIN`, `PIN_I2S_BCLK`, `PIN_TFT_CS`, `PIN_SD_CS`).
  - Macro cấu hình hệ thống dùng `UPPER_SNAKE_CASE` (ví dụ: `ENABLE_SERIAL_LOG`, `MAX_BUFFER_SIZE`).
- **Tên Biến (Variables)**:
  - Biến cục bộ (Local): Dùng `camelCase` (ví dụ: `bytesRead`, `lastTickTime`).
  - Biến thành viên Class (Private/Protected): Tiền tố `_` hoặc `m_` + `camelCase` (ví dụ: `_volume`, `_isPlaying`, `_taskHandle`).
  - Biến Static / Singleton: Tiền tố `s_` + `camelCase` (ví dụ: `s_instance`).
  - Biến toàn cục (hạn chế): Tiền tố `g_` + `camelCase` (ví dụ: `g_systemConfig`).
  - Hằng số (`const`/`constexpr`): Dùng `kPascalCase` hoặc `UPPER_SNAKE_CASE` (ví dụ: `kDefaultBaudRate = 115200`).
- **Tên Hàm & Phương thức (Functions & Methods)**:
  - Hàm & method thông thường: Dùng `camelCase`, bắt đầu bằng động từ (ví dụ: `begin()`, `update()`, `playTrack()`, `setVolume()`, `isConnected()`).
  - Hàm ngắt phần cứng: Khai báo `IRAM_ATTR void onXxxISR()` (kết thúc bằng `ISR`).
  - FreeRTOS Task function: Dùng `camelCase` kết thúc bằng `Task` (ví dụ: `audioPlaybackTask`, `networkMonitorTask`).
- **Logging Tag**:
  - Mỗi file `.cpp` khai báo `static const char *TAG = "ModuleName";` (ngắn gọn, trực quan, trùng tên module).
