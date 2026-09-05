#include "audio_dac_service.h"
#include "storage_service.h"
#include "log.h"

static const char *TAG = "AudioDacService";

// Kích thước buffer đọc từ SD và chuyển đổi sang DMA
static const size_t kPcmChunkSize = 512;

AudioDacService::AudioDacService()
    : m_state(AudioState::STOPPED),
      m_isFinished(false),
      m_taskHandle(nullptr),
      m_sampleRate(22050),
      m_channels(1),
      m_bitsPerSample(16),
      m_dataOffset(44),
      m_dataSize(0),
      m_bytesPerSec(44100),
      m_totalDurationMs(0),
      m_bytesPlayed(0),
      m_i2sInstalled(false) {
}

AudioDacService::~AudioDacService() {
    stop();
    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
    }
    if (m_i2sInstalled) {
        i2s_driver_uninstall(I2S_NUM_0);
        m_i2sInstalled = false;
    }
    if (m_audioFile) {
        m_audioFile.close();
    }
}

bool AudioDacService::begin() {
    if (m_i2sInstalled) {
        return true;
    }

    // Cấu hình I2S driver với DAC nội ESP32 (GPIO26)
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
        .sample_rate = m_sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_STAND_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 16,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);
    if (err != ESP_OK) {
        LOG_E(TAG, "Lỗi cài đặt driver I2S DAC: %d", err);
        return false;
    }

    // Kích hoạt kênh DAC nội: GPIO26 là DAC Channel 2 (Right channel)
    i2s_set_pin(I2S_NUM_0, NULL);
    i2s_set_dac_mode(I2S_DAC_CHANNEL_BOTH_EN);
    i2s_zero_dma_buffer(I2S_NUM_0);

    m_i2sInstalled = true;
    LOG_I(TAG, "I2S DAC nội (GPIO26) đã khởi tạo thành công");

    // Tạo FreeRTOS Task chạy trên Core 0
    BaseType_t res = xTaskCreatePinnedToCore(
        audioTask,
        "AudioDacTask",
        4096,
        this,
        2,              // Mức ưu tiên vừa phải
        &m_taskHandle,
        0               // Core 0 (để Core 1 cho Video và UI)
    );

    if (res != pdPASS) {
        LOG_E(TAG, "Không thể tạo AudioDacTask!");
        return false;
    }

    return true;
}

bool AudioDacService::parseWavHeader(File& file) {
    uint8_t header[44];
    if (file.read(header, 44) != 44) {
        LOG_E(TAG, "File WAV quá ngắn hoặc lỗi đọc header");
        return false;
    }

    // 1. Kiểm tra Magic Bytes
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        LOG_E(TAG, "Không phải định dạng file WAV chuẩn (thiếu RIFF/WAVE)");
        return false;
    }

    // 2. Phân tích format chunk
    uint16_t audioFormat = header[20] | (header[21] << 8);
    m_channels = header[22] | (header[23] << 8);
    m_sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    m_bytesPerSec = header[28] | (header[29] << 8) | (header[30] << 16) | (header[31] << 24);
    m_bitsPerSample = header[34] | (header[35] << 8);

    if (audioFormat != 1) { // 1 = PCM
        LOG_E(TAG, "Chỉ hỗ trợ file WAV PCM không nén! Format code: %d", audioFormat);
        return false;
    }

    // 3. Tìm chunk "data"
    m_dataOffset = 44;
    m_dataSize = header[40] | (header[41] << 8) | (header[42] << 16) | (header[43] << 24);

    // Nếu chunk ở vị trí 36 không phải là "data", tiến hành quét tìm
    if (memcmp(header + 36, "data", 4) != 0) {
        file.seek(12);
        while (file.available() >= 8) {
            char chunkId[4];
            file.read((uint8_t*)chunkId, 4);
            uint32_t chunkSize = 0;
            file.read((uint8_t*)&chunkSize, 4);
            if (memcmp(chunkId, "data", 4) == 0) {
                m_dataOffset = file.position();
                m_dataSize = chunkSize;
                break;
            }
            file.seek(file.position() + chunkSize);
        }
    }

    if (m_bytesPerSec == 0) {
        m_bytesPerSec = m_sampleRate * m_channels * (m_bitsPerSample / 8);
    }

    if (m_bytesPerSec > 0) {
        m_totalDurationMs = (uint32_t)(((uint64_t)m_dataSize * 1000ULL) / m_bytesPerSec);
    } else {
        m_totalDurationMs = 0;
    }

    LOG_I(TAG, "WAV Info: %u Hz, %d-bit, %s, Duration = %u ms (%u bytes)",
          m_sampleRate, m_bitsPerSample, (m_channels == 1 ? "Mono" : "Stereo"),
          m_totalDurationMs, m_dataSize);

    return true;
}

bool AudioDacService::openFile(const char* wavPath) {
    if (!StorageService::lock()) return false;

    if (m_audioFile) {
        m_audioFile.close();
    }

    m_audioFile = SD.open(wavPath, FILE_READ);
    if (!m_audioFile) {
        LOG_E(TAG, "Không thể mở file audio: %s", wavPath);
        StorageService::unlock();
        return false;
    }

    bool ok = parseWavHeader(m_audioFile);
    if (!ok) {
        m_audioFile.close();
        StorageService::unlock();
        return false;
    }

    // Cập nhật lại sample rate cho driver I2S nếu khác
    if (m_i2sInstalled) {
        i2s_set_sample_rates(I2S_NUM_0, m_sampleRate);
    }

    reset();
    StorageService::unlock();
    return true;
}

void AudioDacService::play() {
    m_state = AudioState::PLAYING;
    LOG_I(TAG, "Audio: PLAY");
}

void AudioDacService::pause() {
    m_state = AudioState::PAUSED;
    if (m_i2sInstalled) {
        i2s_zero_dma_buffer(I2S_NUM_0);
    }
    LOG_I(TAG, "Audio: PAUSE");
}

void AudioDacService::stop() {
    m_state = AudioState::STOPPED;
    if (m_i2sInstalled) {
        i2s_zero_dma_buffer(I2S_NUM_0);
    }
    LOG_I(TAG, "Audio: STOP");
}

void AudioDacService::reset() {
    m_bytesPlayed = 0;
    m_isFinished = false;
    if (m_audioFile) {
        m_audioFile.seek(m_dataOffset);
    }
    if (m_i2sInstalled) {
        i2s_zero_dma_buffer(I2S_NUM_0);
    }
}

uint32_t AudioDacService::getCurrentTimeMs() const {
    if (m_bytesPerSec == 0) return 0;
    uint32_t ms = (uint32_t)(((uint64_t)m_bytesPlayed * 1000ULL) / m_bytesPerSec);
    if (ms > m_totalDurationMs && m_totalDurationMs > 0) {
        ms = m_totalDurationMs;
    }
    return ms;
}

void AudioDacService::audioTask(void* parameter) {
    AudioDacService* self = static_cast<AudioDacService*>(parameter);
    while (true) {
        self->processAudio();
        // Nhường 1 tick cho IDLE task của Core 0 để reset WDT
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

void AudioDacService::writePcmChunk(const uint8_t* pcmData, size_t len) {
    if (!m_i2sInstalled || !pcmData || len == 0) return;
    if (m_state != AudioState::PLAYING) return;

    static uint16_t dacDmaBuf[512 * 2];
    size_t bytesPerSample = (m_bitsPerSample / 8);
    if (bytesPerSample == 0) return;

    size_t totalSamples = len / bytesPerSample;
    size_t sampleOffset = 0;

    while (sampleOffset < totalSamples) {
        size_t batch = totalSamples - sampleOffset;
        if (batch > 512) batch = 512;

        if (m_bitsPerSample == 16) {
            const int16_t* samples = reinterpret_cast<const int16_t*>(pcmData + sampleOffset * 2);
            for (size_t i = 0; i < batch; ++i) {
                uint16_t dacVal = (uint16_t)(((int32_t)samples[i] + 32768) & 0xFFFF);
                dacDmaBuf[i * 2 + 0] = dacVal; // Left
                dacDmaBuf[i * 2 + 1] = dacVal; // Right (GPIO26)
            }
        } else {
            const uint8_t* samples = pcmData + sampleOffset;
            for (size_t i = 0; i < batch; ++i) {
                uint16_t dacVal = (uint16_t)samples[i] << 8;
                dacDmaBuf[i * 2 + 0] = dacVal;
                dacDmaBuf[i * 2 + 1] = dacVal;
            }
        }

        size_t bytesWritten = 0;
        size_t dmaBytesToWrite = batch * 2 * sizeof(uint16_t);
        i2s_write(I2S_NUM_0, dacDmaBuf, dmaBytesToWrite, &bytesWritten, pdMS_TO_TICKS(50));
        sampleOffset += batch;
    }

    m_bytesPlayed += len;
}

void AudioDacService::processAudio() {
    if (m_state != AudioState::PLAYING) {
        vTaskDelay(pdMS_TO_TICKS(10));
        return;
    }

    // Nếu không mở file wav riêng (đang dùng AVI mode), task này chỉ nhường CPU
    if (!m_audioFile) {
        vTaskDelay(pdMS_TO_TICKS(50));
        return;
    }

    if (m_bytesPlayed >= m_dataSize) {
        m_isFinished = true;
        m_state = AudioState::PAUSED;
        return;
    }

    // Đọc một khối dữ liệu từ thẻ nhớ SD với mutex bảo vệ
    uint8_t rawPcm[kPcmChunkSize];
    size_t bytesToRead = kPcmChunkSize;
    if (m_bytesPlayed + bytesToRead > m_dataSize) {
        bytesToRead = m_dataSize - m_bytesPlayed;
    }

    size_t bytesRead = 0;
    if (StorageService::lock(pdMS_TO_TICKS(50))) {
        bytesRead = m_audioFile.read(rawPcm, bytesToRead);
        StorageService::unlock();
    } else {
        // Nếu không lấy được mutex ngay, nhường thời gian
        vTaskDelay(pdMS_TO_TICKS(2));
        return;
    }

    if (bytesRead == 0) {
        m_isFinished = true;
        m_state = AudioState::PAUSED;
        return;
    }

    // Chuyển đổi dữ liệu PCM sang định dạng DAC nội (16-bit stereo pair cho I2S DAC)
    // Cấp phát buffer tĩnh để tránh cấp phát động liên tục
    static uint16_t dacDmaBuf[kPcmChunkSize * 2];
    size_t sampleCount = bytesRead / (m_bitsPerSample / 8);

    if (m_bitsPerSample == 16) {
        const int16_t* samples = reinterpret_cast<const int16_t*>(rawPcm);
        for (size_t i = 0; i < sampleCount; ++i) {
            // Chuyển signed 16-bit (-32768..32767) sang unsigned cho DAC
            uint16_t dacVal = (uint16_t)(((int32_t)samples[i] + 32768) & 0xFFFF);
            dacDmaBuf[i * 2 + 0] = dacVal; // Left
            dacDmaBuf[i * 2 + 1] = dacVal; // Right (GPIO26)
        }
    } else if (m_bitsPerSample == 8) {
        for (size_t i = 0; i < sampleCount; ++i) {
            uint16_t dacVal = (uint16_t)rawPcm[i] << 8;
            dacDmaBuf[i * 2 + 0] = dacVal;
            dacDmaBuf[i * 2 + 1] = dacVal;
        }
    }

    size_t bytesWritten = 0;
    size_t dmaBytesToWrite = sampleCount * 2 * sizeof(uint16_t);
    i2s_write(I2S_NUM_0, dacDmaBuf, dmaBytesToWrite, &bytesWritten, portMAX_DELAY);

    m_bytesPlayed += bytesRead;
}
