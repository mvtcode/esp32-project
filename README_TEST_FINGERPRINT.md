# Hướng dẫn Test Module Vân tay TZM1026

## 🚀 Cách sử dụng

### 1. Chạy script

```bash
python test_fingerprint.py
```

### 2. Các lệnh thường dùng

#### Kiểm tra kết nối và số người dùng
```
Nhập: 0
Kết quả: Hiển thị số lượng vân tay đã lưu trong module
```

#### Đăng ký vân tay mới (ID tự động)
```
Nhập: 16
Hướng dẫn: 
  - Chạm ngón tay lần 1 khi thấy "Đang chờ chạm ngón tay... (Bước 1/3)"
  - Nhấc tay ra
  - Chạm lại lần 2 khi thấy "Đang chờ chạm ngón tay... (Bước 2/3)"
  - Nhấc tay ra
  - Chạm lại lần 3 khi thấy "Đang chờ chạm ngón tay... (Bước 3/3)"
  - Thấy "ĐĂNG KÝ THÀNH CÔNG!" là hoàn tất
```

#### Xác thực vân tay (tìm trong database)
```
Nhập: 18
Hướng dẫn:
  - Chạm ngón tay đã đăng ký
  - Nếu khớp: "XÁC THỰC THÀNH CÔNG! User ID: X"
  - Nếu không khớp: "KHÔNG KHỚP! Vân tay không tìm thấy."
```

#### Xóa toàn bộ dữ liệu
```
Nhập: 5
Cảnh báo: Lệnh này sẽ xóa TẤT CẢ vân tay đã lưu!
```

#### Hủy thao tác hiện tại
```
Nhập: 20
Dùng khi: Đang trong quá trình đăng ký hoặc xác thực và muốn hủy
```

### 3. Quy trình test đầy đủ

```
Bước 1: Kiểm tra kết nối
  → Nhập: 0 (Lấy số người dùng)
  → Kỳ vọng: Nhận được phản hồi với số lượng = 0 (nếu module mới)

Bước 2: Đăng ký vân tay đầu tiên
  → Nhập: 16 (Đăng ký chế độ NCNR)
  → Làm theo hướng dẫn chạm 3 lần
  → Kỳ vọng: "ĐĂNG KÝ THÀNH CÔNG!"

Bước 3: Kiểm tra lại số người dùng
  → Nhập: 0
  → Kỳ vọng: Số lượng = 1

Bước 4: Test xác thực
  → Nhập: 18 (Xác thực 1:N)
  → Chạm ngón tay đã đăng ký
  → Kỳ vọng: "XÁC THỰC THÀNH CÔNG! User ID: 1"

Bước 5: Test vân tay không khớp
  → Nhập: 18
  → Chạm ngón tay KHÁC chưa đăng ký
  → Kỳ vọng: "KHÔNG KHỚP! Vân tay không tìm thấy."
```

## 📋 Danh sách lệnh đầy đủ

| Số | Lệnh | Mô tả | Khi nào dùng |
|:--:|:-----|:------|:-------------|
| **0** | Lấy số người dùng | Kiểm tra số vân tay đã lưu | Kiểm tra trạng thái database |
| **1** | Lấy ID duy nhất | Lấy mã định danh module | Nhận diện module |
| **5** | Xóa toàn bộ | Xóa hết vân tay đã lưu | Reset module về trạng thái ban đầu |
| **16** | Đăng ký (NCNR) | Đăng ký vân tay mới | Thêm người dùng mới |
| **17** | Xác thực 1:1 | So khớp với ID cụ thể | Verify một người cụ thể |
| **18** | Xác thực 1:N | Tìm trong toàn bộ database | Nhận diện người dùng |
| **20** | Hủy thao tác | Ngắt lệnh đang thực hiện | Hủy khi đang đăng ký/xác thực |

## 🔧 Khắc phục sự cố

### Lỗi: "Lỗi kết nối serial"
**Nguyên nhân:**
- Cổng COM8 không tồn tại hoặc đang được sử dụng
- Module chưa được kết nối
- Driver USB-Serial chưa cài đặt

**Giải pháp:**
1. Kiểm tra Device Manager → Ports (COM & LPT)
2. Xác nhận cổng COM đúng (có thể là COM3, COM4, v.v.)
3. Sửa biến `PORT` trong file `test_fingerprint.py` nếu cần
4. Đảm bảo không có chương trình nào khác đang mở cổng COM

### Lỗi: "Không nhận được phản hồi từ module"
**Nguyên nhân:**
- Kết nối dây bị lỏng
- Sai tốc độ baud
- Module chưa được cấp nguồn đúng cách

**Giải pháp:**
1. Kiểm tra lại kết nối TX/RX (có thể bị đảo ngược)
2. Đảm bảo module được cấp nguồn 3.3V (KHÔNG dùng 5V!)
3. Kiểm tra mạch chuyển đổi mức điện áp nếu dùng Arduino 5V

### Lỗi: "Không đọc được vân tay"
**Nguyên nhân:**
- Ngón tay quá khô hoặc quá ướt
- Chạm không đủ mạnh
- Cảm biến bị bẩn

**Giải pháp:**
1. Lau sạch cảm biến bằng vải mềm
2. Đảm bảo ngón tay sạch và khô ráo vừa phải
3. Chạm đủ mạnh và giữ trong 1-2 giây

## 💡 Tips

1. **Đăng ký vân tay tốt:**
   - Dùng cùng một ngón tay cho cả 3 lần chạm
   - Chạm ở vị trí tương tự nhau
   - Đảm bảo ngón tay phủ kín cảm biến

2. **Tăng độ chính xác:**
   - Đăng ký cùng một ngón tay nhiều lần với các ID khác nhau
   - Đăng ký ở nhiều góc độ khác nhau

3. **Bảo mật:**
   - Không để module ở chế độ cho phép đăng ký tự do
   - Sử dụng lệnh 6 để khóa/mở khóa đăng ký

## 📝 Ghi chú

- Module hỗ trợ lưu tối đa **200 vân tay** (tùy phiên bản)
- Mỗi vân tay được gán một **User ID** duy nhất (1-200)
- Thời gian timeout mặc định: **10 giây**
- Tốc độ xác thực: **< 1 giây**

## 🔗 Tài liệu tham khảo

- File tài liệu đầy đủ: `TZM1026_Complete_Guide.md`
- Datasheet: Biosec TA0702
- Giao thức: F5 Protocol (8 bytes)
