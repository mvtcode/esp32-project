#include "video_player_service.h"
#include "storage_service.h"
#include "log.h"
#include <esp_heap_caps.h>

static const char *TAG = "VideoPlayerService";
static TFT_eSPI* s_activeTft = nullptr;

// Callback cho TJpg_Decoder render từng block ảnh lên màn hình ST7796
static bool tftOutputCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (s_activeTft) {
        s_activeTft->pushImage(x, y, w, h, bitmap);
    }
    return 1;
}

VideoPlayerService::VideoPlayerService(TFT_eSPI& tft, AudioDacService& audioService, VideoUI& ui)
    : m_tft(tft),
      m_audioService(audioService),
      m_ui(ui),
      m_state(VideoState::PAUSED),
      m_fps(20),
      m_currentFrame(0),
      m_totalDurationMs(0),
      m_isAvi(false),
      m_moviOffset(0),
      m_frameBuffer(nullptr),
      m_frameBufferCapacity(65536), // 64 KB đủ cho frame JPEG 480x270 nén
      m_readChunkBuffer(nullptr),
      m_readChunkSize(8192),        // Chunk 8 KB đọc thẻ nhớ SD tối ưu
      m_readChunkPos(0),
      m_readChunkLen(0),
      m_isFirstFrameRendered(false),
      m_playbackStartTime(0),
      m_playbackElapsedMs(0) {
}

VideoPlayerService::~VideoPlayerService() {
    if (m_videoFile) {
        m_videoFile.close();
    }
    if (m_frameBuffer) {
        free(m_frameBuffer);
        m_frameBuffer = nullptr;
    }
    if (m_readChunkBuffer) {
        free(m_readChunkBuffer);
        m_readChunkBuffer = nullptr;
    }
}

bool VideoPlayerService::begin() {
    s_activeTft = &m_tft;

    // Cấu hình giải mã TJpg_Decoder
    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(true); // Đổi byte màu RGB565 cho màn hình ST7796
    TJpgDec.setCallback(tftOutputCallback);

    // Cấp phát bộ nhớ: ưu tiên PSRAM nếu có, nếu không thì dùng Internal Heap
    if (psramFound()) {
        m_frameBuffer = (uint8_t*)ps_malloc(m_frameBufferCapacity);
        m_readChunkBuffer = (uint8_t*)ps_malloc(m_readChunkSize);
        LOG_I(TAG, "Đã cấp phát bộ đệm Video trên PSRAM");
    } else {
        m_frameBuffer = (uint8_t*)malloc(m_frameBufferCapacity);
        m_readChunkBuffer = (uint8_t*)malloc(m_readChunkSize);
        LOG_I(TAG, "Đã cấp phát bộ đệm Video trên Internal Heap");
    }

    if (!m_frameBuffer || !m_readChunkBuffer) {
        LOG_E(TAG, "Không đủ bộ nhớ heap để cấp phát bộ đệm Video!");
        return false;
    }

    return true;
}

bool VideoPlayerService::openVideo(const char* videoPath, const char* wavPath, int fps) {
    m_fps = fps;
    m_state = VideoState::PAUSED;
    m_currentFrame = 0;
    m_readChunkPos = 0;
    m_readChunkLen = 0;
    m_isAvi = false;
    m_moviOffset = 0;

    // Kiểm tra định dạng file (hỗ trợ cả .avi All-in-One và .mjpeg)
    String pathStr = String(videoPath);
    pathStr.toLowerCase();
    if (pathStr.endsWith(".avi")) {
        m_isAvi = true;
    }

    // 1. Mở file video
    if (!StorageService::lock()) return false;
    if (m_videoFile) {
        m_videoFile.close();
    }

    m_videoFile = SD.open(videoPath, FILE_READ);
    StorageService::unlock();

    if (!m_videoFile) {
        LOG_E(TAG, "Không thể mở file video: %s", videoPath);
        return false;
    }

    // Cập nhật tiêu đề video trên UI
    const char* baseName = strrchr(videoPath, '/');
    if (!baseName) baseName = strrchr(videoPath, '\\');
    m_ui.setVideoTitle(baseName ? baseName + 1 : videoPath);

    if (m_isAvi) {
        // Chế độ AVI: Parse header RIFF/AVI để tìm movi offset và totalFrames từ avih
        // Hoàn toàn không dùng buffer lớn trên Stack (tránh Stack Overflow)
        uint8_t riffHdr[12];
        if (m_videoFile.read(riffHdr, 12) != 12 ||
            memcmp(riffHdr, "RIFF", 4) != 0 ||
            memcmp(riffHdr + 8, "AVI ", 4) != 0) {
            LOG_E(TAG, "File không đúng định dạng RIFF/AVI!");
            return false;
        }

        m_moviOffset = 0;
        uint32_t totalFrames = 0;

        while (m_videoFile.available() >= 8) {
            uint8_t hdr[8];
            if (m_videoFile.read(hdr, 8) != 8) break;
            uint32_t chunkSize = hdr[4] | (hdr[5] << 8) | (hdr[6] << 16) | (hdr[7] << 24);
            uint32_t pad = (chunkSize & 1);

            if (memcmp(hdr, "LIST", 4) == 0) {
                uint8_t listType[4];
                if (m_videoFile.read(listType, 4) != 4) break;
                if (memcmp(listType, "movi", 4) == 0) {
                    m_moviOffset = m_videoFile.position();
                    LOG_I(TAG, "Đã tìm thấy movi chunk tại offset: %u", m_moviOffset);
                    break;
                } else if (memcmp(listType, "hdrl", 4) == 0) {
                    uint32_t hdrlEnd = m_videoFile.position() + (chunkSize - 4);
                    while (m_videoFile.position() + 8 <= hdrlEnd) {
                        uint8_t subHdr[8];
                        if (m_videoFile.read(subHdr, 8) != 8) break;
                        uint32_t subSize = subHdr[4] | (subHdr[5] << 8) | (subHdr[6] << 16) | (subHdr[7] << 24);
                        uint32_t subPad = (subSize & 1);

                        if (memcmp(subHdr, "avih", 4) == 0 && subSize >= 32) {
                            uint8_t avihBuf[32];
                            if (m_videoFile.read(avihBuf, 32) == 32) {
                                uint32_t usPerFrame = avihBuf[0] | (avihBuf[1] << 8) | (avihBuf[2] << 16) | (avihBuf[3] << 24);
                                if (usPerFrame > 0) {
                                    m_fps = 1000000 / usPerFrame;
                                    if (m_fps == 0) m_fps = 15;
                                }
                                totalFrames = avihBuf[16] | (avihBuf[17] << 8) | (avihBuf[18] << 16) | (avihBuf[19] << 24);
                            }
                            m_videoFile.seek(m_videoFile.position() + (subSize - 32) + subPad);
                        } else {
                            m_videoFile.seek(m_videoFile.position() + subSize + subPad);
                        }
                    }
                    m_videoFile.seek(hdrlEnd + pad);
                } else {
                    m_videoFile.seek(m_videoFile.position() + (chunkSize - 4) + pad);
                }
            } else {
                m_videoFile.seek(m_videoFile.position() + chunkSize + pad);
            }
        }

        if (m_moviOffset == 0) {
            LOG_W(TAG, "Không tìm thấy LIST movi, quay về đầu file");
            m_videoFile.seek(0);
        } else {
            m_videoFile.seek(m_moviOffset);
        }

        if (totalFrames > 0) {
            m_totalDurationMs = (uint32_t)(((uint64_t)totalFrames * 1000ULL) / m_fps);
        } else {
            m_totalDurationMs = 57600; // Dự phòng ~58 giây
        }

        m_audioService.setTotalDurationMs(m_totalDurationMs);
        m_audioService.setBytesPerSec(44100); // 22050 Hz 16-bit Mono = 44100 bytes/s
        LOG_I(TAG, "AVI Mode (All-in-One): Frames = %u, Duration = %u ms", totalFrames, m_totalDurationMs);
    } else {
        // Chế độ MJPEG + WAV rời rạc
        if (wavPath && strlen(wavPath) > 0) {
            if (!m_audioService.openFile(wavPath)) {
                LOG_E(TAG, "Không thể mở file audio: %s", wavPath);
                return false;
            }
            m_totalDurationMs = m_audioService.getTotalDurationMs();
        }
    }

    LOG_I(TAG, "Đã mở video: %s (Kích thước: %u KB, Thời lượng: %u ms)",
          videoPath, (uint32_t)(m_videoFile.size() / 1024), m_totalDurationMs);

    // Đọc và vẽ ngay frame đầu tiên (Frame 0) tại y=20 (tâm màn hình), sau đó dừng ở trạng thái PAUSED
    size_t frameSize = 0;
    if (readNextFrame(frameSize)) {
        TJpgDec.drawJpg(0, 20, m_frameBuffer, frameSize);
        m_isFirstFrameRendered = true;
    }

    // Vẽ giao diện Header 20px, Footer 30px và nút Play ở giữa
    m_ui.forceShowOverlay();
    m_ui.drawHeader();
    m_ui.drawFooter(0, m_totalDurationMs);
    m_ui.drawCenterPlayIcon();

    return true;
}

bool VideoPlayerService::readNextFrame(size_t& frameSize, bool skipVideo) {
    if (m_isAvi) {
        return readNextFrameAvi(frameSize, skipVideo);
    } else {
        return readNextFrameMjpeg(frameSize);
    }
}

bool VideoPlayerService::readNextFrameAvi(size_t& frameSize, bool skipVideo) {
    frameSize = 0;
    if (!m_videoFile) return false;

    while (m_videoFile.available() >= 8) {
        uint8_t chunkHdr[8];
        if (!StorageService::lock(pdMS_TO_TICKS(50))) return false;
        size_t n = m_videoFile.read(chunkHdr, 8);
        StorageService::unlock();
        if (n != 8) return false;

        uint32_t chunkSize = chunkHdr[4] | (chunkHdr[5] << 8) | (chunkHdr[6] << 16) | (chunkHdr[7] << 24);
        uint32_t pad = (chunkSize & 1);

        // Sanity check tránh lỗi tràn dữ liệu hoặc corrupted chunk
        if (chunkSize > 1000000) {
            LOG_W(TAG, "Chunk bất thường: size=%u, dừng", chunkSize);
            return false;
        }

        // 1. Chunk Video: 00dc (MJPEG frame)
        if (chunkHdr[2] == 'd' && chunkHdr[3] == 'c') {
            if (skipVideo) {
                // Bỏ qua giải mã video: Chỉ cần seek qua chunk trong 0.05ms (không đọc 51KB từ SD)
                if (StorageService::lock(pdMS_TO_TICKS(50))) {
                    m_videoFile.seek(m_videoFile.position() + chunkSize + pad);
                    StorageService::unlock();
                }
                frameSize = 0;
                return true; // Đã xử lý xong 1 frame (bỏ qua hình ảnh)
            }

            if (chunkSize > m_frameBufferCapacity) {
                LOG_W(TAG, "Frame size %u vượt quá buffer %u", chunkSize, m_frameBufferCapacity);
                if (StorageService::lock(pdMS_TO_TICKS(50))) {
                    m_videoFile.seek(m_videoFile.position() + chunkSize + pad);
                    StorageService::unlock();
                }
                continue;
            }

            if (!StorageService::lock(pdMS_TO_TICKS(100))) return false;
            size_t bytesRead = m_videoFile.read(m_frameBuffer, chunkSize);
            if (pad > 0) m_videoFile.read(); // Bỏ qua padding byte
            StorageService::unlock();

            if (bytesRead != chunkSize) return false;
            frameSize = chunkSize;
            return true;
        }
        // 2. Chunk Audio: 01wb (PCM WAV audio)
        else if (chunkHdr[2] == 'w' && chunkHdr[3] == 'b') {
            static uint8_t audioChunkBuf[1024];
            uint32_t remaining = chunkSize;
            while (remaining > 0) {
                size_t toRead = (remaining > sizeof(audioChunkBuf)) ? sizeof(audioChunkBuf) : remaining;
                if (!StorageService::lock(pdMS_TO_TICKS(50))) break;
                size_t bytesRead = m_videoFile.read(audioChunkBuf, toRead);
                StorageService::unlock();
                if (bytesRead == 0) break;
                if (m_state == VideoState::PLAYING) {
                    m_audioService.writePcmChunk(audioChunkBuf, bytesRead);
                }
                remaining -= bytesRead;
            }
            if (pad > 0) {
                if (StorageService::lock(pdMS_TO_TICKS(50))) {
                    m_videoFile.read();
                    StorageService::unlock();
                }
            }
        }
        // 3. Các chunk phụ (JUNK, idx1, LIST...) -> Bỏ qua nhanh
        else {
            if (StorageService::lock(pdMS_TO_TICKS(50))) {
                m_videoFile.seek(m_videoFile.position() + chunkSize + pad);
                StorageService::unlock();
            }
        }
    }

    return false; // Hết file video
}

bool VideoPlayerService::readNextFrameMjpeg(size_t& frameSize) {
    frameSize = 0;
    if (!m_videoFile) return false;

    // 1. Tìm byte mở đầu JPEG: 0xFF, 0xD8 (SOI) bằng sliding window
    uint8_t prevByte = 0;
    bool foundSoi = false;

    while (!foundSoi) {
        if (m_readChunkPos >= m_readChunkLen) {
            if (!StorageService::lock(pdMS_TO_TICKS(100))) return false;
            m_readChunkLen = m_videoFile.read(m_readChunkBuffer, m_readChunkSize);
            m_readChunkPos = 0;
            StorageService::unlock();
            if (m_readChunkLen == 0) return false; // Hết file
        }

        uint8_t b = m_readChunkBuffer[m_readChunkPos++];
        if (prevByte == 0xFF && b == 0xD8) {
            m_frameBuffer[0] = 0xFF;
            m_frameBuffer[1] = 0xD8;
            frameSize = 2;
            foundSoi = true;
            break;
        }
        prevByte = b;
    }

    // 2. Đọc tuần tự cho đến khi gặp byte kết thúc JPEG: 0xFF, 0xD9 (EOI)
    while (foundSoi) {
        if (m_readChunkPos >= m_readChunkLen) {
            if (!StorageService::lock(pdMS_TO_TICKS(100))) return false;
            m_readChunkLen = m_videoFile.read(m_readChunkBuffer, m_readChunkSize);
            m_readChunkPos = 0;
            StorageService::unlock();
            if (m_readChunkLen == 0) return false; // File bị cắt ngắn
        }

        uint8_t b = m_readChunkBuffer[m_readChunkPos++];
        if (frameSize < m_frameBufferCapacity) {
            m_frameBuffer[frameSize++] = b;
        } else {
            LOG_W(TAG, "Frame vượt quá dung lượng buffer!");
            return false;
        }

        if (b == 0xD9 && frameSize >= 2 && m_frameBuffer[frameSize - 2] == 0xFF) {
            // Tìm thấy EOI hoàn chỉnh của 1 frame JPEG
            return true;
        }
    }

    return false;
}

void VideoPlayerService::play() {
    m_state = VideoState::PLAYING;
    m_playbackStartTime = millis() - m_playbackElapsedMs;
    m_audioService.play();
    m_ui.triggerOverlay(1000); // Hiện overlay 1s rồi tự ẩn (Cinema Mode)
    LOG_I(TAG, "VideoPlayer: PLAY (Elapsed: %u ms)", m_playbackElapsedMs);
}

void VideoPlayerService::pause() {
    m_state = VideoState::PAUSED;
    m_playbackElapsedMs = millis() - m_playbackStartTime;
    m_audioService.pause();
    m_ui.forceShowOverlay();
    m_ui.drawHeader();
    m_ui.drawFooter(m_playbackElapsedMs, m_totalDurationMs);
    m_ui.drawCenterPlayIcon();
    LOG_I(TAG, "VideoPlayer: PAUSE (Elapsed: %u ms)", m_playbackElapsedMs);
}

void VideoPlayerService::togglePlayPause() {
    if (m_state == VideoState::PLAYING) {
        pause();
    } else {
        play();
    }
}

void VideoPlayerService::reset() {
    m_currentFrame = 0;
    m_playbackElapsedMs = 0;
    m_playbackStartTime = millis();
    m_readChunkPos = 0;
    m_readChunkLen = 0;

    if (StorageService::lock()) {
        if (m_videoFile) {
            if (m_isAvi && m_moviOffset > 0) {
                m_videoFile.seek(m_moviOffset);
            } else {
                m_videoFile.seek(0);
            }
        }
        StorageService::unlock();
    }

    m_audioService.reset();

    // Vẽ lại frame 0 tại y=20 và cập nhật UI về 00:00
    size_t frameSize = 0;
    if (readNextFrame(frameSize)) {
        TJpgDec.drawJpg(0, 20, m_frameBuffer, frameSize);
    }
    m_ui.forceShowOverlay();
    m_ui.drawHeader();
    m_ui.drawFooter(0, m_totalDurationMs);
    m_ui.drawCenterPlayIcon();
}

void VideoPlayerService::update() {
    if (m_state != VideoState::PLAYING) {
        return;
    }

    uint32_t now = millis();
    uint32_t elapsedMs = now - m_playbackStartTime;
    if (m_totalDurationMs > 0 && elapsedMs >= m_totalDurationMs) {
        reset();
        pause();
        return;
    }

    // Tính toán frame mục tiêu theo trục thời gian thực (Master Clock)
    uint32_t targetFrame = (m_fps > 0) ? ((elapsedMs * m_fps) / 1000) : 0;

    // Nếu bị trễ hơn 1 frame (do frame trước decode lâu),
    // skip video của frame này (seek 0.05ms) nhưng VẪN ĐỌC VÀ NẠP ĐẦY ĐỦ AUDIO!
    bool shouldSkip = (targetFrame > m_currentFrame + 1);

    size_t frameSize = 0;
    if (readNextFrame(frameSize, shouldSkip)) {
        if (!shouldSkip && frameSize > 0) {
            TJpgDec.drawJpg(0, 20, m_frameBuffer, frameSize);
        }
        m_currentFrame++;
    } else {
        // Hết video: Tự động tua về đầu và tạm dừng
        reset();
        pause();
        return;
    }

    // Cập nhật timer ẩn/hiện của giao diện Cinema Mode
    m_ui.update(true);

    // Nếu đang trong thời gian hiển thị overlay: vẽ Header, Footer và Center Icon
    if (m_ui.isOverlayVisible()) {
        m_ui.drawHeader();
        m_ui.drawFooter(elapsedMs, m_totalDurationMs);
        m_ui.drawCenterPauseIcon();
    }
}
