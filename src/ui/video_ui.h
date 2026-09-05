#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>

class VideoUI {
public:
    VideoUI(TFT_eSPI& tft);
    ~VideoUI() = default;

    void init();
    void setVideoTitle(const char* title);
    
    // Đánh thức lớp điều khiển (Overlay) hiện lên trong durationMs (mặc định 1500ms)
    void triggerOverlay(uint32_t durationMs = 1500);
    void forceShowOverlay();
    void update(bool isPlaying);

    // Vẽ giao diện (Header, Center, Footer)
    void drawHeader();
    void drawFooter(uint32_t currentMs, uint32_t totalMs);
    void drawCenterPlayIcon();
    void drawCenterPauseIcon();
    void clearOverlayWhenPlaying();

    bool isOverlayVisible() const { return m_overlayVisible; }
    bool needClearCenter() {
        bool val = m_needClearCenter;
        m_needClearCenter = false;
        return val;
    }

private:
    void formatTime(uint32_t ms, char* buf, size_t len);

    TFT_eSPI& m_tft;
    char m_title[64];
    bool m_overlayVisible;
    bool m_needClearCenter;
    uint32_t m_overlayShowTime;
    uint32_t m_overlayDuration;
    uint32_t m_lastDrawnSec;
    int m_lastProgressWidth;
};
