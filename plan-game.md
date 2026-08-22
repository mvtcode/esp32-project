# Game Console Implementation Plan (ESP32-WROOM OLED 128x64)

## 1. Overview
Tích hợp chế độ **Game Console** (`AUDIO_MODE_GAME`) trên ESP32 với màn hình OLED 1.3" SH1106, núm xoay EC11 và các nút Confirm / Back.

## 2. Giao diện Danh sách Game (Games Menu Screen)
Tương tự giao diện danh sách bài hát (Playlist) của chế độ MP3 Player:
- **Header Banner:** `GAMES MENU [x/y]`
- **Danh sách Game (4 dòng hiển thị đồng thời):**
  1. `BRICK BREAKER` (Arkanoid / Phá gạch)
  2. `RETRO SNAKE` (Rắn săn mồi cổ điển)
  3. `SPACE INVADERS` (Chiến cơ bắn ruồi)
  4. `FLAPPY BIRD` (Chim vượt chướng ngại vật)
- **Thanh highlight chọn (Inverted Rounded Box):** Làm nổi bật game đang trỏ tới.
- **Thanh cuộn dọc (Scrollbar):** Hiển thị vị trí con trỏ tương đối.
- **Điều khiển:**
  - Xoay **EC11** (GPIO 32, 33): Cuộn con trỏ lên / xuống mượt mà.
  - Nút **Confirm (BTN_PLUS - GPIO 14)**: Chọn và vào chơi game.

## 3. Danh sách 4 Game & Điều khiển trong Game
1. **🧱 Brick Breaker:**
   - Xoay EC11: Di chuyển thanh đỡ bóng (Paddle) sang Trái / Phải mượt mà.
   - Confirm: Phóng bóng / Bắt đầu lại.
   - Back (BTN_BACK - GPIO 13): Thoát ra Menu Game.
2. **🐍 Retro Snake:**
   - Xoay EC11: Chuyển hướng rẽ của rắn (Clockwise / Counter-Clockwise theo góc 90°).
   - Confirm: Bắt đầu lại khi Game Over / Tăng tốc.
   - Back: Thoát ra Menu Game.
3. **🚀 Space Invaders:**
   - Xoay EC11: Di chuyển tàu chiến sang Trái / Phải.
   - Confirm: Bắn tia laser tiêu diệt hạm đội quái vật ngoài hành tinh.
   - Back: Thoát ra Menu Game.
4. **🐦 Flappy Bird:**
   - Confirm (hoặc xoay EC11): Vỗ cánh nhảy lên vượt qua các đường ống.
   - Back: Thoát ra Menu Game.

## 4. Chuyển đổi Chế độ Hệ thống
- Nút **BTN_PUSH (GPIO 4)**: Nhấn một chạm để chuyển qua lại 5 chế độ:
  `MIC` ➔ `BLUETOOTH` ➔ `CLOCK & WEATHER` ➔ `MP3 PLAYER` ➔ `GAME CONSOLE` ➔ `MIC`.
