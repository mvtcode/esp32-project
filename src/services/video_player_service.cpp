#include "video_player_service.h"
#include "storage_service.h"
#include "log.h"
#include <esp_heap_caps.h>

static const char *TAG = "VideoPlayerService";
static TFT_eSPI* s_activeTft = nullptr;
static uint16_t* s_renderBuffer = nullptr;  // PSRAM framebuffer cho single-shot frame push

// Kích thước vùng video trên màn hình (320x240 có Header 30px + Footer 30px)
static const int16_t kVideoW = 320;
static const int16_t kVideoH = 180;

// Callback fallback: render từng tile trực tiếp lên màn hình (dùng khi không có PSRAM buffer)
static bool tftOutputCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (s_activeTft) {
        s_activeTft->pushImage(x, y + 30, w, h, bitmap);
    }
    return 1;
}

// Callback chính: giải mã JPEG vào PSRAM framebuffer (320x180), push 1 lần
static bool tftFrameCallback(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    if (!s_renderBuffer || x < 0 || y < 0) return 1;
    if ((x + w) > kVideoW || (y + h) > kVideoH) return 1;
    uint16_t* dst = s_renderBuffer + (int32_t)y * kVideoW + x;
    for (int16_t row = 0; row < h; row++) {
        memcpy(dst, bitmap + (int32_t)row * w, (size_t)w * sizeof(uint16_t));
        dst += kVideoW;
    }
    return 1;
}

VideoPlayerService::VideoPlayerService(TFT_eSPI& tft, AudioI2sService& audioService, VideoUI& ui)
    : m_tft(tft),
      m_audioService(audioService),
      m_ui(ui),
      m_state(VideoState::PAUSED),
      m_fps(20),
      m_currentFrame(0),
      m_totalDurationMs(0),
      m_isAvi(false),
      m_isRawRgb(false),
      m_moviOffset(0),
      m_frameBuffer(nullptr),
#if defined(BOARD_HAS_PSRAM) || defined(CONFIG_SPIRAM_SUPPORT)
      m_frameBufferCapacity(98304),    // 96 KB: đủ cho MJPEG 320x180 quality cao
      m_readChunkBuffer(nullptr),
      m_readChunkSize(32768),           // 32 KB (PSRAM): giảm latency FATFS read
#else
      m_frameBufferCapacity(40960),    // 40 KB: an toàn cho Internal SRAM của esp32dev
      m_readChunkBuffer(nullptr),
      m_readChunkSize(4096),            // 4 KB: đọc SD theo block nhỏ để tiết kiệm RAM
#endif
      m_readChunkPos(0),
      m_readChunkLen(0),
      m_audioReadBuf(nullptr),
      m_audioReadBufSize(4096),         // 4 KB: đủ đọc toàn bộ audio chunk ~2205 bytes/@20fps trong 1 transaction
      m_renderBuffer(nullptr),
      m_isFirstFrameRendered(false),
      m_playbackStartTime(0),
      m_playbackElapsedMs(0) {
    memset(m_videoPath, 0, sizeof(m_videoPath));
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
    if (m_audioReadBuf) {
        free(m_audioReadBuf);
        m_audioReadBuf = nullptr;
    }
    if (m_renderBuffer) {
        free(m_renderBuffer);
        m_renderBuffer = nullptr;
        s_renderBuffer = nullptr;
    }
}

bool VideoPlayerService::begin() {
    s_activeTft = &m_tft;

    // Cấu hình giải mã TJpg_Decoder
    TJpgDec.setJpgScale(1);
    TJpgDec.setSwapBytes(true); // Đổi byte màu RGB565 cho màn hình
    // Callback sẽ được set sau khi biết có render buffer hay không

    // Frame Buffer: ưu tiên Internal Fast SRAM (240MHz 0-wait) cho tốc độ giải mã JPEG tối đa
    m_frameBuffer = (uint8_t*)heap_caps_malloc(m_frameBufferCapacity, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!m_frameBuffer) {
        // Fallback: dùng PSRAM (chậm hơn ~15% nhưng đủ dùng)
        m_frameBuffer = (uint8_t*)heap_caps_malloc(m_frameBufferCapacity, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
        if (m_frameBuffer) {
            LOG_W(TAG, "Frame Buffer 96KB đặt trên PSRAM (Internal SRAM không đủ)");
        } else {
            m_frameBuffer = (uint8_t*)malloc(m_frameBufferCapacity);
            LOG_W(TAG, "Frame Buffer dùng mặc định malloc");
        }
    } else {
        LOG_I(TAG, "Frame Buffer 96KB trên Fast Internal SRAM!");
    }

    // Read Chunk Buffer: Dùng PSRAM để tiết kiệm Internal SRAM cho tốc độ decode JPEG
    m_readChunkBuffer = (uint8_t*)heap_caps_malloc(m_readChunkSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!m_readChunkBuffer) {
        // Fallback: Internal heap
        m_readChunkBuffer = (uint8_t*)heap_caps_malloc(m_readChunkSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
        if (!m_readChunkBuffer) {
            m_readChunkBuffer = (uint8_t*)malloc(m_readChunkSize);
        }
        LOG_W(TAG, "Read Chunk Buffer 32KB dùng Internal/default heap");
    } else {
        LOG_I(TAG, "Read Chunk Buffer 32KB trên PSRAM");
    }

    // Audio Read Buffer: Dùng PSRAM, chỉ là staging buffer cho SD read
    m_audioReadBuf = (uint8_t*)heap_caps_malloc(m_audioReadBufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!m_audioReadBuf) {
        m_audioReadBuf = (uint8_t*)malloc(m_audioReadBufSize);
    }

    // Render Buffer: PSRAM framebuffer 320x180x2=112KB
    // Thay vì 240 tile pushImage riêng lẻ (àoverhead SPI) → 1 pushImage duy nhất sau khi decode xong
    m_renderBuffer = (uint16_t*)heap_caps_malloc(
        (size_t)kVideoW * kVideoH * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (m_renderBuffer) {
        s_renderBuffer = m_renderBuffer;
        // Khởi tạo toàn bộ buffer với màu đen → viền đen quanh video luôn được điền sẵn, không cần fillRect
        memset(m_renderBuffer, 0, (size_t)kVideoW * kVideoH * sizeof(uint16_t));
        TJpgDec.setCallback(tftFrameCallback);  // Decode vào buffer, push 1 lần
        LOG_I(TAG, "Render Buffer %u KB trên PSRAM: single-shot push OK! (%dx%d)",
              (uint32_t)((size_t)kVideoW * kVideoH * 2 / 1024), kVideoW, kVideoH);
    } else {
        TJpgDec.setCallback(tftOutputCallback); // Fallback: tile-by-tile
        LOG_W(TAG, "Không cấp phát được Render Buffer PSRAM, dùng tile-by-tile fallback");
    }

    if (!m_frameBuffer || !m_readChunkBuffer || !m_audioReadBuf) {
        LOG_E(TAG, "Không đủ bộ nhớ heap để cấp phát bộ đệm Video!");
        return false;
    }

    LOG_I(TAG, "VideoPlayerService bắt đầu: FrameBuf=%u KB, ReadChunk=%u KB, AudioBuf=%u B, RenderBuf=%s",
          (uint32_t)(m_frameBufferCapacity / 1024),
          (uint32_t)(m_readChunkSize / 1024),
          (uint32_t)(m_audioReadBufSize),
          m_renderBuffer ? "PSRAM 112KB" : "N/A (fallback)");
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

    m_videoFile = StorageService::openFile(videoPath, FILE_READ);
    StorageService::unlock();

    if (!m_videoFile) {
        LOG_E(TAG, "Không thể mở file video: %s", videoPath);
        return false;
    }

    strncpy(m_videoPath, videoPath, sizeof(m_videoPath) - 1);
    m_videoPath[sizeof(m_videoPath) - 1] = '\0';

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

    // Đọc và vẽ ngay frame đầu tiên (Frame 0) tại y=30, sau đó dừng ở trạng thái PAUSED
    size_t frameSize = 0;
    if (readNextFrame(frameSize)) {
        if (m_isRawRgb) {
            if (s_renderBuffer) {
                // Raw RGB: Dữ liệu đã nạp trực tiếp vào s_renderBuffer -> Đẩy thẳng
                m_tft.pushImage(0, 30, kVideoW, kVideoH, s_renderBuffer);
            }
        } else if (s_renderBuffer) {
            TJpgDec.drawJpg(0, 0, m_frameBuffer, frameSize);
            m_tft.pushImage(0, 30, kVideoW, kVideoH, s_renderBuffer);
        } else {
            TJpgDec.drawJpg(0, 30, m_frameBuffer, frameSize);
        }
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

        // 1. Chunk Video: 00dc hoặc 00db (MJPEG frame hoặc Raw RGB565 frame)
        if (chunkHdr[2] == 'd' && (chunkHdr[3] == 'c' || chunkHdr[3] == 'b')) {
            const size_t rawFrameBytes = (size_t)kVideoW * kVideoH * sizeof(uint16_t); // 115200 bytes
            if (chunkSize == rawFrameBytes) {
                m_isRawRgb = true;
                if (m_renderBuffer) {
                    // CÓ PSRAM (ESP32-S3): Đọc 1 lần vào m_renderBuffer, update() sẽ push lên màn hình
                    if (!StorageService::lock(pdMS_TO_TICKS(100))) return false;
                    size_t bytesRead = m_videoFile.read(reinterpret_cast<uint8_t*>(m_renderBuffer), chunkSize);
                    if (pad > 0) m_videoFile.read(); // Bỏ qua padding byte
                    StorageService::unlock();

                    if (bytesRead != chunkSize) return false;
                    if (skipVideo) {
                        frameSize = 0; // Đã đọc xong nhưng bỏ qua vẽ màn hình
                    } else {
                        frameSize = chunkSize;
                    }
                    return true;
                } else {
                    // KHÔNG CÓ PSRAM (esp32dev): Stream từng block từ SD trực tiếp lên TFT SPI!
                    if (!StorageService::lock(pdMS_TO_TICKS(100))) return false;
                    if (skipVideo) {
                        m_videoFile.seek(m_videoFile.position() + chunkSize + pad);
                        StorageService::unlock();
                        frameSize = 0;
                        return true;
                    }

                    m_tft.startWrite();
                    m_tft.setAddrWindow(0, 30, kVideoW, kVideoH);
                    uint32_t remaining = chunkSize;
                    size_t chunkBufSize = (m_frameBufferCapacity > 0) ? m_frameBufferCapacity : 4096;
                    while (remaining > 0) {
                        size_t toRead = (remaining > chunkBufSize) ? chunkBufSize : remaining;
                        size_t r = m_videoFile.read(m_frameBuffer, toRead);
                        if (r == 0) break;
                        m_tft.pushPixels(reinterpret_cast<uint16_t*>(m_frameBuffer), (uint32_t)(r / 2));
                        remaining -= r;
                    }
                    if (pad > 0) m_videoFile.read(); // Bỏ qua padding byte
                    m_tft.endWrite();
                    StorageService::unlock();

                    if (remaining > 0) return false;
                    frameSize = chunkSize;
                    return true;
                }
            } else {
                // Motion JPEG: Frame nhỏ (~4-10KB)
                if (skipVideo) {
                    if (StorageService::lock(pdMS_TO_TICKS(50))) {
                        m_videoFile.seek(m_videoFile.position() + chunkSize + pad);
                        StorageService::unlock();
                    }
                    frameSize = 0;
                    return true;
                }

                m_isRawRgb = false;
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
            }
        }
        // 2. Chunk Audio: 01wb (PCM WAV audio) — Đọc toàn bộ trong 1 SD transaction
        else if (chunkHdr[2] == 'w' && chunkHdr[3] == 'b') {
            uint32_t remaining = chunkSize;
            while (remaining > 0) {
                size_t toRead = (remaining > m_audioReadBufSize) ? m_audioReadBufSize : remaining;
                // Một lần lock duy nhất cho toàn bộ khối dữ liệu cần đọc
                if (!StorageService::lock(pdMS_TO_TICKS(50))) break;
                size_t bytesRead = m_videoFile.read(m_audioReadBuf, toRead);
                StorageService::unlock();
                if (bytesRead == 0) break;
                if (m_state == VideoState::PLAYING) {
                    m_audioService.writePcmChunk(m_audioReadBuf, bytesRead);
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
    // Đồng bộ tuyệt đối m_playbackStartTime khớp chính xác với m_currentFrame hiện tại!
    if (m_fps > 0) {
        m_playbackStartTime = millis() - ((uint64_t)m_currentFrame * 1000ULL / m_fps);
    } else {
        m_playbackStartTime = millis() - m_playbackElapsedMs;
    }
    m_audioService.play();
    m_ui.triggerOverlay(1000); // Hiện overlay 1s rồi tự ẩn (Cinema Mode)
    LOG_I(TAG, "VideoPlayer: PLAY (Frame: %u)", m_currentFrame);
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
            m_videoFile.close();
            // Đóng và mở lại từ đầu: reset hoàn toàn FATFS cluster chain
            if (m_videoPath[0] != '\0') {
                m_videoFile = StorageService::openFile(m_videoPath, FILE_READ);
                if (m_videoFile && m_isAvi && m_moviOffset > 0) {
                    // Đọc tuần tự tới m_moviOffset thay vì gọi seek() để giữ trọn vẹn multi-block streaming
                    uint32_t toSkip = m_moviOffset;
                    uint8_t dummy[512];
                    while (toSkip > 0 && m_videoFile.available()) {
                        size_t n = (toSkip > sizeof(dummy)) ? sizeof(dummy) : toSkip;
                        size_t r = m_videoFile.read(dummy, n);
                        if (r == 0) break;
                        toSkip -= r;
                    }
                }
            }
        }
        StorageService::unlock();
    }

    m_audioService.reset();

    // Vẽ lại frame 0 tại y=30 và cập nhật UI về 00:00
    size_t frameSize = 0;
    if (readNextFrame(frameSize)) {
        if (m_isRawRgb) {
            if (s_renderBuffer) {
                m_tft.pushImage(0, 30, kVideoW, kVideoH, s_renderBuffer);
            }
        } else if (s_renderBuffer) {
            TJpgDec.drawJpg(0, 0, m_frameBuffer, frameSize);
            m_tft.pushImage(0, 30, kVideoW, kVideoH, s_renderBuffer);
        } else {
            TJpgDec.drawJpg(0, 30, m_frameBuffer, frameSize);
        }
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

    // QUAN TRỌNG: FPS Pacing - Nếu chưa đến thời điểm hiển thị frame tiếp theo, nhường CPU
    // Giúp duy trì nhịp khung hình đều đặn 20 FPS (50ms/frame), tránh hiện tượng giật cục do chạy quá nhanh
    if (m_currentFrame > targetFrame) {
        return;
    }

    // Nếu bị trễ hơn 2 frame (do thẻ SD chậm đột ngột), skip video frame này để bắt kịp âm thanh
    bool shouldSkip = (targetFrame > m_currentFrame + 2);

    size_t frameSize = 0;

    uint32_t t0 = micros();
    bool ok = readNextFrame(frameSize, shouldSkip);
    uint32_t t1 = micros();

    if (ok) {
        if (!shouldSkip && frameSize > 0) {
            uint32_t t2 = 0, t3 = 0;
            if (m_isRawRgb) {
                t2 = micros();
                if (s_renderBuffer) {
                    m_tft.startWrite();
                    m_tft.setAddrWindow(0, 30, kVideoW, kVideoH);
                    m_tft.pushPixels(s_renderBuffer, (uint32_t)kVideoW * kVideoH);
                    m_tft.endWrite();
                }
                t3 = micros();
            } else if (s_renderBuffer) {
                TJpgDec.drawJpg(0, 0, m_frameBuffer, frameSize);
                t2 = micros();
                // startWrite/endWrite giữ SPI CS LOW liên tục trong suốt pushImage → không bị overhead CS toggle
                m_tft.startWrite();
                m_tft.setAddrWindow(0, 30, kVideoW, kVideoH);
                m_tft.pushPixels(s_renderBuffer, (uint32_t)kVideoW * kVideoH);
                m_tft.endWrite();
                t3 = micros();
            } else {
                // Fallback tile-by-tile với startWrite/endWrite wrapper
                m_tft.startWrite();
                TJpgDec.drawJpg(0, 30, m_frameBuffer, frameSize);
                m_tft.endWrite();
                t2 = t3 = micros();
            }

            // FPS + Profiling: đo thực tế từng giai đoạn (chỉ đếm frame được render)
            static uint32_t s_fpsFrameCount = 0;
            static uint32_t s_fpsStartMs    = 0;
            static uint32_t s_sumSdUs       = 0;
            static uint32_t s_sumDecodeUs   = 0;
            static uint32_t s_sumPushUs     = 0;

            if (++s_fpsFrameCount == 1) {
                s_fpsStartMs  = millis();
                s_sumSdUs = s_sumDecodeUs = s_sumPushUs = 0;
            }
            s_sumSdUs     += (t1 - t0);
            s_sumDecodeUs += (t2 - t1);
            s_sumPushUs   += (t3 - t2);

            if (s_fpsFrameCount >= 61) {
                uint32_t elapsed = millis() - s_fpsStartMs;
                if (elapsed > 0) {
                    LOG_I(TAG, "[FPS] 60 rendered / %u ms = %.1f fps | skip=%s",
                          elapsed, 60000.0f / (float)elapsed,
                          (targetFrame > m_currentFrame) ? "YES" : "no");
                    LOG_I(TAG, "[Profile/frame] SD=%ums  Decode=%ums  Push=%ums",
                          s_sumSdUs / 60000,
                          s_sumDecodeUs / 60000,
                          s_sumPushUs / 60000);
                }
                s_fpsFrameCount = 0;
            }
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

    // Nếu đang trong thời gian hiển thị overlay: chỉ vẽ Header và Footer trong dải viền đen
    if (m_ui.isOverlayVisible()) {
        m_ui.drawHeader();
        m_ui.drawFooter(elapsedMs, m_totalDurationMs);
    }
}
