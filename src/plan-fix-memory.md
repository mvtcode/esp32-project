# Kế Hoạch Xử Lý Lỗi Phân Mảnh & Rò Rỉ Bộ Nhớ (Heap Fragmentation & Memory Leak)

Dựa trên quá trình kiểm tra mã nguồn (`main.cpp`, `bt_audio.cpp`, `mp3_player.cpp`), nguyên nhân chính gây ra phân mảnh bộ nhớ (Memory Fragmentation) và rò rỉ bộ nhớ (Memory Leak) khi chuyển chế độ (Mode Switching) xuất phát từ việc khởi tạo và giải phóng các tài nguyên lớn (FreeRTOS Task, Bluetooth Stack, I2S DMA Buffers) liên tục.

Dưới đây là kế hoạch chi tiết để khắc phục:

## 1. Tối ưu hoá việc quản lý Bluetooth Stack (Tránh phân mảnh heap)
**Vấn đề:** 
Hàm `bt_audio_stop()` trong `bt_audio.cpp` hiện tại gọi `esp_bluedroid_deinit()` và `btStop()` (thực hiện `esp_bt_controller_deinit()`). Việc `init`/`deinit` toàn bộ Bluetooth stack liên tục sẽ làm hệ điều hành liên tục cấp phát và giải phóng các khối nhớ lớn, gây ra phân mảnh Heap nghiêm trọng sau vài lần chuyển chế độ, dẫn đến cạn kiệt RAM (OOM) cho I2S DMA.

**Giải pháp:**
- **Chỉ Enable/Disable:** Khởi tạo controller và bluedroid một lần, sau đó khi chuyển chế độ chỉ gọi `esp_bluedroid_disable()` và `esp_bt_controller_disable()`. Bỏ qua việc gọi `deinit()`.
- Sửa hàm `bt_audio_stop()`: Xoá `esp_bluedroid_deinit()`, xoá `btStop()` (hoặc thay bằng `esp_bt_controller_disable()`).
- Sửa hàm `bt_audio_start()`: Kiểm tra trạng thái đã được khởi tạo hay chưa để gọi `enable()` thay vì `init()` lại từ đầu.

## 2. Giữ nguyên Task xử lý MP3 (Tránh Memory Leak và Task Fragmentation)
**Vấn đề:**
Hàm `mp3_player_stop()` gọi `vTaskDelete(s_mp3_task_handle)` để tiêu diệt Task giải mã âm thanh. Việc tạo và xoá FreeRTOS Task liên tục (mỗi lần cần stack 4KB) cực kỳ dễ gây phân mảnh bộ nhớ. Hơn nữa, nếu Task bị xoá khi đang thực hiện các tác vụ dang dở, bộ nhớ động bên trong có thể bị rò rỉ (leak).

**Giải pháp:**
- **Không xoá Task:** Trong `mp3_player_start()`, chỉ tạo `mp3_player_task` **một lần duy nhất** (nếu chưa có). 
- **Idle Loop:** Trong `mp3_player_stop()`, chỉ gửi lệnh `CMD_STOP` để đóng các stream (file, decoder). Task MP3 sẽ tự động rơi vào trạng thái ngủ/chờ (idle delay) với mức tiêu thụ CPU gần như bằng 0 khi không có file chạy.
- Xoá đoạn mã gọi `esp_task_wdt_delete(s_mp3_task_handle)` và `vTaskDelete()` trong `mp3_player_stop()`.

## 3. Quản lý thống nhất DMA Buffer của I2S Driver (Tuỳ chọn)
**Vấn đề:**
`bt_audio.cpp` và `mp3_player.cpp` đều cài đặt (`i2s_driver_install`) và gỡ bỏ (`i2s_driver_uninstall`) I2S_NUM_0 liên tục để xuất âm thanh ra DAC PCM5102A. Quá trình này đòi hỏi khối bộ nhớ liên tục (contiguous block) cho DMA Buffer. Nếu Heap bị phân mảnh, `i2s_driver_install` sẽ thất bại.

**Giải pháp:**
- Vì cả MP3 và BT đều dùng chung một thông số (44100Hz, 16-bit, 2 channels, cùng Pinout), ta có thể cân nhắc chuyển việc `i2s_driver_install` sang cấu hình chung và không bao giờ `uninstall`, hoặc đảm bảo các giải pháp 1 và 2 ở trên đã đủ để Heap không bị phân mảnh, giúp cấp phát DMA buffer luôn thành công. 

## Các bước thực thi (Action Items)
1. Cập nhật `bt_audio.cpp` (`bt_audio_start` và `bt_audio_stop`) theo nguyên tắc chỉ Disable, không Deinit BT controller.
2. Cập nhật `mp3_player.cpp` (`mp3_player_start` và `mp3_player_stop`) để không bao giờ xoá `mp3_player_task`.
3. Kiểm tra lại chu trình `switch_audio_mode` trong `main.cpp` để xác minh số free heap sau khi áp dụng thay đổi. 

Vui lòng xem qua kế hoạch này. Nếu bạn đồng ý, tôi sẽ tiến hành sửa đổi các file `bt_audio.cpp` và `mp3_player.cpp` theo hướng trên.
