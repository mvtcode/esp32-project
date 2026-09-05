#pragma once

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <driver/i2s.h>
#include "pin_config.h"

enum class AudioState {
    STOPPED,
    PLAYING,
    PAUSED
};

class AudioDacService {
public:
    AudioDacService();
    ~AudioDacService();

    bool begin();
    bool openFile(const char* wavPath);
    void play();
    void pause();
    void stop();
    void reset();

    AudioState getState() const { return m_state; }
    bool isPlaying() const { return m_state == AudioState::PLAYING; }
    bool isFinished() const { return m_isFinished; }

    uint32_t getCurrentTimeMs() const;
    uint32_t getTotalDurationMs() const { return m_totalDurationMs; }
    uint32_t getSampleRate() const { return m_sampleRate; }

    // Hỗ trợ luồng audio trực tiếp từ file AVI (All-in-One)
    void writePcmChunk(const uint8_t* pcmData, size_t len);
    void setTotalDurationMs(uint32_t totalMs) { m_totalDurationMs = totalMs; }
    void setBytesPerSec(uint32_t rate) { m_bytesPerSec = rate; }
    bool hasAudioFile() const { return (bool)m_audioFile; }

private:
    static void audioTask(void* parameter);
    void processAudio();
    bool parseWavHeader(File& file);

    File m_audioFile;
    AudioState m_state;
    bool m_isFinished;
    TaskHandle_t m_taskHandle;

    uint32_t m_sampleRate;
    uint16_t m_channels;
    uint16_t m_bitsPerSample;
    uint32_t m_dataOffset;
    uint32_t m_dataSize;
    uint32_t m_bytesPerSec;
    uint32_t m_totalDurationMs;

    volatile uint32_t m_bytesPlayed;
    bool m_i2sInstalled;
};
