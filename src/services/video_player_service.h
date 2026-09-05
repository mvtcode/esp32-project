#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "audio_i2s_service.h"
#include "ui/video_ui.h"

enum class VideoState {
    STOPPED,
    PLAYING,
    PAUSED
};

class VideoPlayerService {
public:
    VideoPlayerService(TFT_eSPI& tft, AudioI2sService& audioService, VideoUI& ui);
    ~VideoPlayerService();

    bool begin();
    bool openVideo(const char* videoPath, const char* wavPath = nullptr, int fps = 20);
    
    void play();
    void pause();
    void togglePlayPause();
    void reset();

    void update();

    VideoState getState() const { return m_state; }
    bool isPlaying() const { return m_state == VideoState::PLAYING; }
    uint32_t getCurrentFrame() const { return m_currentFrame; }

private:
    bool readNextFrame(size_t& frameSize, bool skipVideo = false);
    bool readNextFrameAvi(size_t& frameSize, bool skipVideo = false);
    bool readNextFrameMjpeg(size_t& frameSize);

    TFT_eSPI& m_tft;
    AudioI2sService& m_audioService;
    VideoUI& m_ui;

    File m_videoFile;
    VideoState m_state;
    int m_fps;
    uint32_t m_currentFrame;
    uint32_t m_totalDurationMs;

    bool m_isAvi;
    uint32_t m_moviOffset;

    uint8_t* m_frameBuffer;
    size_t m_frameBufferCapacity;

    uint8_t* m_readChunkBuffer;
    size_t m_readChunkSize;
    size_t m_readChunkPos;
    size_t m_readChunkLen;

    bool m_isFirstFrameRendered;
    uint32_t m_playbackStartTime;
    uint32_t m_playbackElapsedMs;
};
