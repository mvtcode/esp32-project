Phần cứng của tôi là esp32-s3-n16r8
Tôi có 6 tấm led matrix p5 64x32 được kết nối thành 1 màn hình 128x96

sơ đồ đấu:

```
[p1] -> [p2]
          ↓
[p4] <- [p3]
↓
[p5] -> [p6]
```

Project hiện tại là chạy cho esp32-wroom-32 và 1 tấm led matrix p5 (64x32)

Có các thông tin:

- Thời gian (lấy từ NTP server)
- Thứ ngày tháng dương lịch (lấy từ NTP serverr và cả RTC DS1302)
- Từ dương lịch đổi sang ngày âm lịch
- Nhiệt độ, độ ẩm trong nhà (lấy từ DHT22)
- Nhiệt độ, độ ẩm ngoài trời (lấy từ internet weather API)

Khi hiển thị ở led p5 trật nên phải hiển thị 2 row và hiển thị lần lượt.
Với 6 tấm led thì hiển thị hết thông tin cho tôi:

Hàng 1: Thời gian hiện tại, hiển thị full 128x32.
Hàng 2: hiển thị ngày dương lịch và ngày âm lịch
hàng 3: hiển thị nhiệt độ trong nhà và ngoài trời
hàng 4: (128x32) là hàng chạy chữ do user nhập vào, có thể chạy dạng marquee lặp lại 1 hoặc nhiều dòng (có thể sửa ở phần config bằng web ui)

Bạn sẽ refactor lại project theo yêu cầu mới này của tôi.
Tất cả thông tin đã có, chỉ là sắp xếp lại để hiển thị theo yêu cầu.

Hãy cài font hỗ trợ Tiếng Việt và hiển thị tốt trên led p5 - đề xuất sử dụng font Verdana
