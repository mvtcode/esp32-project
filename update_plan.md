# Kế Hoạch Cập Nhật Giao Diện & Bố Cục Phím Tắt (UI Update Plan)

## 1. Tab Touchpad (Mouse & Trackpad)
- Giữ nguyên thiết kế và kích thước đã tối ưu (Canvas chiếm 75% + cụm 4 phím: Left Click, Scroll Up, Scroll Down, Right Click).

## 2. Tab Shortcuts (Ma trận 16 Phím - 4 hàng × 4 cột)
- Kích thước mỗi nút: **70px × 98px**, viền phân loại màu trực quan theo từng nhóm.
- Hiển thị 2 tầng: Tầng trên là biểu tượng/chức năng, tầng dưới là phím tắt (tự động đổi `Ctrl` ↔ `Cmd` theo OS Mode).

| Hàng | Nhóm | Phím 1 | Phím 2 | Phím 3 | Phím 4 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Hàng 1** | **Hệ Thống** | 🔒 Lock (`Win+L`) | 📸 Snip (`Win+Shift+S`) | 📋 TaskMgr (`Ctrl+Shift+Esc`) | 🔢 Calc (`Calculator`) |
| **Hàng 2** | **Trình Duyệt** | ◀ Back (`Alt+Left`) | Fwd ▶ (`Alt+Right`) | 🔄 Reload (`F5`) | 📑 +Tab (`Ctrl+T`) |
| **Hàng 3** | **Soạn Thảo** | Copy (`Ctrl+C`) | Paste (`Ctrl+V`) | Cut (`Ctrl+X`) | Undo (`Ctrl+Z`) |
| **Hàng 4** | **Hành Động** | All (`Ctrl+A`) | Save (`Ctrl+S`) | Esc (`Hủy / Esc`) | ↵ ENTER (`Xác nhận`) |

## 3. Tab Numpad (Bàn phím số)
- Giữ nguyên thiết kế chuẩn máy tính 16 phím số & phép tính + 2 phím chức năng lớn `XÓA (BS)` và `ENTER` ở đáy.

## 4. Tab Media Control (Đa Phương Tiện & Điều Hướng)
1. **Âm Lượng Hệ Thống (Volume Master)**:
   - Thanh trượt `lv_slider` (0 - 100%) kéo mượt mà, chống swipe không mong muốn.
   - 3 nút âm lượng tròn phía dưới: `Vol -`, `Mute`, `Vol +`.
2. **Trình Phát Đa Phương Tiện (Playback Controls)**:
   - 4 nút ngang: `⏮ Prev`, `⏯ Play/Pause`, `⏭ Next`, `⏹ Stop`.
3. **Cụm Phím Điều Hướng & Tua Nhanh (D-Pad & Space)**:
   - Cụm chữ thập 4 hướng:
     - `▲ Up`: Tăng âm lượng / Lên dòng.
     - `▼ Down`: Giảm âm lượng / Xuống dòng.
     - `◀ 5s`: Tua lùi 5 giây / Lùi slide (`Arrow Left`).
     - `5s ▶`: Tua tới 5 giây / Tiến slide (`Arrow Right`).
     - Tâm `SPACE`: Tạm dừng / Phát tiếp video (`Space key`).

## 5. Tab Settings (Cài Đặt Hệ Thống)
- Giữ nguyên cấu hình chuyển OS Mode (`Windows / macOS`), âm thanh phản hồi, độ sáng và thông tin phần cứng.
