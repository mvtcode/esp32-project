#include "video_ui.h"
#include "log.h"

static const char *TAG = "VideoUI";

// Định nghĩa màu sắc Cinema Mode
#define COLOR_CINEMA_BG     0x0000  // Đen tuyền cho rạp phim
#define COLOR_PROGRESS_BG   0x2945  // Xám tối cho thanh nền
#define COLOR_PROGRESS_FILL 0x07FF  // Cyan rực rỡ cho tiến độ
#define COLOR_BUTTON_BG     0x18E3  // Nền mờ cho nút Play/Pause
#define COLOR_TEXT_MUTED    0xAD55  // Xám nhạt cho thời lượng tổng

VideoUI::VideoUI(TFT_eSPI& tft)
    : m_tft(tft),
      m_overlayVisible(true),
      m_needClearCenter(false),
      m_overlayShowTime(0),
      m_overlayDuration(1000),
      m_lastDrawnSec(0xFFFFFFFF),
      m_lastProgressWidth(-1) {
    strncpy(m_title, "video.avi", sizeof(m_title) - 1);
}

void VideoUI::init() {
    // 1. Vẽ nền đen cho Header (y: 0..19)
    m_tft.fillRect(0, 0, 480, 20, COLOR_CINEMA_BG);

    // 2. Vẽ nền đen cho Footer (y: 290..319)
    m_tft.fillRect(0, 290, 480, 30, COLOR_CINEMA_BG);

    forceShowOverlay();
    LOG_I(TAG, "VideoUI đã khởi tạo (Header 20px, Video 270px, Footer 30px)");
}

void VideoUI::setVideoTitle(const char* title) {
    if (title && strlen(title) > 0) {
        strncpy(m_title, title, sizeof(m_title) - 1);
    }
}

void VideoUI::triggerOverlay(uint32_t durationMs) {
    m_overlayVisible = true;
    m_overlayDuration = durationMs;
    m_overlayShowTime = millis();
    m_lastDrawnSec = 0xFFFFFFFF; // Bắt buộc vẽ lại
    m_lastProgressWidth = -1;
}

void VideoUI::forceShowOverlay() {
    m_overlayVisible = true;
    m_overlayDuration = 0xFFFFFFFF; // Giữ vô hạn khi Pause
    m_lastDrawnSec = 0xFFFFFFFF;
    m_lastProgressWidth = -1;
}

void VideoUI::clearOverlayWhenPlaying() {
    // Xóa sạch vùng Header và Footer về màu đen tuyền khi vào chế độ phát
    m_tft.fillRect(0, 0, 480, 20, COLOR_CINEMA_BG);
    m_tft.fillRect(0, 290, 480, 30, COLOR_CINEMA_BG);
    m_needClearCenter = true; // Báo hiệu frame video tiếp theo ghi đè nút giữa
}

void VideoUI::update(bool isPlaying) {
    if (isPlaying) {
        // Đang phát: Kiểm tra hết thời gian hiển thị overlay
        if (m_overlayVisible && (millis() - m_overlayShowTime >= m_overlayDuration)) {
            m_overlayVisible = false;
            clearOverlayWhenPlaying();
        }
    } else {
        // Khi Pause: Luôn luôn giữ overlay hiển thị
        m_overlayVisible = true;
    }
}

void VideoUI::formatTime(uint32_t ms, char* buf, size_t len) {
    uint32_t totalSec = ms / 1000;
    uint32_t min = totalSec / 60;
    uint32_t sec = totalSec % 60;
    snprintf(buf, len, "%02u:%02u", min, sec);
}

void VideoUI::drawHeader() {
    if (!m_overlayVisible) return;

    // Header nằm ở y: 0 -> 19 (Cao 20px, hoàn toàn không đè lên video y=20..289)
    m_tft.fillRect(0, 0, 480, 20, COLOR_CINEMA_BG);
    m_tft.setTextColor(TFT_WHITE, COLOR_CINEMA_BG);
    m_tft.drawString(m_title, 12, 2, 2);
}

void VideoUI::drawFooter(uint32_t currentMs, uint32_t totalMs) {
    if (!m_overlayVisible) return;

    uint32_t currentSec = currentMs / 1000;

    // Footer nằm ở y: 290 -> 319 (Cao 30px, hoàn toàn không đè lên video y=20..289)
    if (currentSec != m_lastDrawnSec) {
        m_lastDrawnSec = currentSec;

        // Current time căn trái ở x=10, y=297
        char timeBuf[16];
        formatTime(currentMs, timeBuf, sizeof(timeBuf));
        m_tft.setTextColor(TFT_WHITE, COLOR_CINEMA_BG);
        m_tft.drawString(timeBuf, 10, 297, 2);

        // Total duration căn phải ở x=420, y=297
        char totalBuf[16];
        formatTime(totalMs, totalBuf, sizeof(totalBuf));
        m_tft.setTextColor(COLOR_TEXT_MUTED, COLOR_CINEMA_BG);
        m_tft.drawString(totalBuf, 422, 297, 2);
    }

    // Tính toán chiều dài thanh progress bar (x: 70 -> 410, rộng 340px, cao 4px tại y=303)
    int progressWidth = 0;
    if (totalMs > 0) {
        progressWidth = (int)(((uint64_t)currentMs * 340ULL) / totalMs);
        if (progressWidth > 340) progressWidth = 340;
    }

    if (progressWidth != m_lastProgressWidth) {
        if (progressWidth > m_lastProgressWidth && m_lastProgressWidth >= 0) {
            int deltaX = m_lastProgressWidth;
            int deltaW = progressWidth - deltaX;
            if (deltaW > 0) {
                m_tft.fillRoundRect(70 + deltaX, 303, deltaW, 4, 1, COLOR_PROGRESS_FILL);
            }
        } else {
            m_tft.fillRoundRect(70, 303, 340, 4, 2, COLOR_PROGRESS_BG);
            if (progressWidth > 0) {
                m_tft.fillRoundRect(70, 303, progressWidth, 4, 1, COLOR_PROGRESS_FILL);
            }
        }
        m_lastProgressWidth = progressWidth;
    }
}

void VideoUI::drawCenterPlayIcon() {
    if (!m_overlayVisible) return;

    // Nút tròn nằm chính giữa video: x=240, y=155 (vì video từ y=20..289, tâm là 20+135=155)
    m_tft.fillCircle(240, 155, 30, COLOR_BUTTON_BG);
    m_tft.drawCircle(240, 155, 30, TFT_WHITE);

    // Tam giác Play ▶ căn giữa
    m_tft.fillTriangle(234, 142, 234, 168, 254, 155, TFT_WHITE);
}

void VideoUI::drawCenterPauseIcon() {
    if (!m_overlayVisible) return;

    // Nút tròn nằm chính giữa video
    m_tft.fillCircle(240, 155, 30, COLOR_BUTTON_BG);
    m_tft.drawCircle(240, 155, 30, TFT_WHITE);

    // Hai vạch Pause ❚❚
    m_tft.fillRect(232, 143, 6, 24, TFT_WHITE);
    m_tft.fillRect(242, 143, 6, 24, TFT_WHITE);
}
