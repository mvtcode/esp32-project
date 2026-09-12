# ESP32-S3 Realtime Volume Sync Companion App

Ứng dụng chạy ngầm trên máy tính (Windows) giúp **đồng bộ âm lượng hai chiều tuyệt đối** với ESP32-S3 Smart TouchPad qua sóng Bluetooth Low Energy (BLE).

---

## 📌 Tính Năng Nổi Bật

1. **Đồng bộ thời gian thực 2 chiều**:
   - Khi kéo thanh trượt Slider trên màn hình ESP32: Âm lượng hệ thống Windows thay đổi tương ứng ngay lập tức.
   - Khi chỉnh âm lượng trên Windows (bấm phím Fn volume trên bàn phím laptop, lăn chuột trên thanh taskbar): Thanh trượt trên màn hình ESP32 tự động trượt theo chuẩn xác.
2. **Tự động kết nối lại (Auto-Reconnect)**:
   - Tự động dò tìm và kết nối lại ngay khi ESP32 bật nguồn hoặc quay lại phạm vi phát sóng Bluetooth.
3. **Chống lặp phản hồi (Anti-Feedback Loop)**:
   - Cơ chế cờ thông minh ngăn chặn hiện tượng vòng lặp echo vô hạn khi hai bên cùng đồng bộ giá trị.

---

## 🚀 Hướng Dẫn Sử Dụng

### Cách 1: Chạy nhanh bằng 1 click chuột (Khuyên dùng)
* Nhấp đúp chuột vào file `run_sync.bat`. Script sẽ tự động cài đặt thư viện cần thiết (`bleak`, `pycaw`, `comtypes`) và khởi động ứng dụng.

### Cách 2: Chạy từ Terminal / Command Prompt
```powershell
cd companion_app
pip install -r requirements.txt
python volume_sync.py
```

---

## ⚙️ Tự Động Khởi Động Cùng Windows (Tùy chọn)

Nếu bạn muốn ứng dụng tự động chạy ngầm mỗi khi bật máy tính:
1. Nhấn tổ hợp phím `Win + R`, nhập `shell:startup` và nhấn Enter (thư mục Startup sẽ mở ra).
2. Tạo một **Shortcut** trỏ tới file `run_sync.bat` và dán vào thư mục Startup này.
3. Từ lần khởi động máy kế tiếp, âm lượng giữa PC và ESP32 sẽ luôn được đồng bộ tự động.
