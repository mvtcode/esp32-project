Project hiện tại sử dụng ESP32s3 super mini và 2 mic INMP441, hiển thị lên màn hình OLED 1.3 inch, 128x64, dùng thư viện u8g2,

Plan của tôi:

- Sử dụng ESP32-wroom thay thế cho ESP32s3 super mini
- Source audio sẽ lấy từ 2 nguồn:
  - Mic INMP441 2 kênh trái phải, giống hiện tại
  - Sử dụng chế độ bluetooth audio receiver
- Khi ở chế độ bluetooth audio receiver thì bỏ qua 2 mic INMP441 và có thể phát ra nguồn loa qua module PCM5102A DAC GY-PCM5102 I2S, lúc này thiết bị giống như adapter để vừa nhận nhạc từ bluetooth và vừa out được ra âm thanh.

Cơ chế chịu lỗi:

- Có thể chưa tích hợp ngay module PCM5102A DAC GY-PCM5102 I2S nhưng không được lỗi, hoặc module này lỗi thì không ảnh hưởng hiển thị lên màn hình oled.
- Nếu không có 2 micophone INMP441 thì không ảnh hưởng tới chế độ bluetooth audio receiver.
- Nếu không có mạch giải mã xoay encoder EC11 thì không ảnh hưởng tới chế độ mic.

Điều khiển:

- Sử dụng button ngoài và mạch giải mã xoay encoder EC11 (Hình ảnh màn oled ở file oled1.3.webp)

Trong đó:

- Giải mã xoay để điều khiển âm lượng output của tín hiệu âm thanh bluetooth
- Button push (ở phía trong module xoay) làm chức năng switch chế độ (mic hoặc bluetooth).
- Button confirm (phía dưới) để chọn chế độ hiển thị của màn hình oled (giống button GPIO 0 hiện tại)
- Button back (phía trên) có chức năng play/pause khi kết nối bluetooth

Yêu cầu:
Kết nối bluetooth thì phải có tính năng giống như loa bluetooth có thể điều chỉnh đồng bộ volume được cả âm lượng của máy tính/điện thoại.
Chế độ play/pause cũng đồng bộ được với thiết bị phát.
Khi chuyển đổi chế độ phát thì phải giải phóng tài nguyên của chế độ trước mượt mà và không bị crash

Tên bluetooth: MVT VU METER V2
