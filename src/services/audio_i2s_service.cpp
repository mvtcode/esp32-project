#include "audio_i2s_service.h"
#include "storage_service.h"
#include "log.h"
#include <Wire.h>

static const char *TAG = "AudioI2sService";

// Địa chỉ I2C của codec ES8311
static const uint8_t kEs8311I2cAddr = 0x18;

// Kích thước buffer đọc từ SD và chuyển đổi sang DMA
static const size_t kPcmChunkSize = 512;

AudioI2sService::AudioI2sService()
    : m_state(AudioState::STOPPED),
      m_isFinished(false),
      m_taskHandle(nullptr),
      m_ringBuffer(nullptr),
      m_sampleRate(22050),
      m_channels(1),
      m_bitsPerSample(16),
      m_dataOffset(44),
      m_dataSize(0),
      m_bytesPerSec(44100),
      m_totalDurationMs(0),
      m_bytesPlayed(0),
      m_i2sInstalled(false),
      m_volume(100),
      m_codecDetected(false) {
}

AudioI2sService::~AudioI2sService() {
    stop();
    if (m_taskHandle) {
        vTaskDelete(m_taskHandle);
        m_taskHandle = nullptr;
    }
    if (m_ringBuffer) {
        vRingbufferDelete(m_ringBuffer);
        m_ringBuffer = nullptr;
    }
    if (m_i2sInstalled) {
        i2s_driver_uninstall(I2S_NUM_0);
        m_i2sInstalled = false;
    }
    if (m_audioFile) {
        m_audioFile.close();
    }
}

void AudioI2sService::writeCodecRegister(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(kEs8311I2cAddr);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
}

uint8_t AudioI2sService::readCodecRegister(uint8_t reg) {
    Wire.beginTransmission(kEs8311I2cAddr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return 0xFF;
    Wire.requestFrom((uint8_t)kEs8311I2cAddr, (uint8_t)1);
    if (Wire.available()) {
        return Wire.read();
    }
    return 0xFF;
}

bool AudioI2sService::initEs8311Codec() {
#if defined(PIN_I2C_SDA) && defined(PIN_I2C_SCL)
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL, 100000);
    Wire.beginTransmission(kEs8311I2cAddr);
    if (Wire.endTransmission() != 0) {
        LOG_W(TAG, "Không tìm thấy codec ES8311 tại I2C 0x%02X (dùng I2S chuẩn)", kEs8311I2cAddr);
        return false;
    }

    uint8_t idHi = readCodecRegister(0xFD);
    uint8_t idLo = readCodecRegister(0xFE);
    LOG_I(TAG, "Phát hiện Audio Codec ES8311 tại I2C 0x%02X (Chip ID: 0x%02X%02X)", kEs8311I2cAddr, idHi, idLo);
    m_codecDetected = true;

    // 1. Soft Reset CSM & Digital Logic
    writeCodecRegister(0x00, 0x1F); // Chip power down / reset
    delay(20);

    // 2. Thiết lập bộ chia xung đồng hồ nội (Internal Clock Management)
    // 22050 Hz / 24000 Hz: pre_div=1, mult=1, adc_osr=16, dac_osr=16, bclk_div=4, lrck=256
    writeCodecRegister(0x01, 0x30); // Clock manager: MCLK pin enabled
    writeCodecRegister(0x02, 0x00); // Pre-divider = 1, Pre-multiplier = 1
    writeCodecRegister(0x03, 0x10); // fs_mode = 0 (single speed), adc_osr = 16
    writeCodecRegister(0x04, 0x10); // dac_osr = 16
    writeCodecRegister(0x05, 0x00); // adc_div = 1, dac_div = 1
    writeCodecRegister(0x06, 0x03); // bclk_div = 4 (bclk = 64*fs, mclk = 256*fs)
    writeCodecRegister(0x07, 0x00); // lrck_h = 0
    writeCodecRegister(0x08, 0xFF); // lrck_l = 255
    writeCodecRegister(0x16, 0x24); // ADC power config

    // 3. Cấu hình Bias nguồn và Analog Circuit
    writeCodecRegister(0x0B, 0x00);
    writeCodecRegister(0x0C, 0x00);
    writeCodecRegister(0x10, 0x1F);
    writeCodecRegister(0x11, 0x7F);

    // 4. Kích hoạt CSM ở chế độ Slave (ESP32 cấp MCLK & BCLK)
    writeCodecRegister(0x00, 0x80); // Bit 7=1 (CSM ON), Bit 6=0 (Slave mode)
    writeCodecRegister(0x01, 0x3F); // Bật tất cả internal clocks (MCLK, BCLK, DCLK, Anaclck)

    // 5. System analog & routing
    writeCodecRegister(0x13, 0x10);
    writeCodecRegister(0x1B, 0x0A);
    writeCodecRegister(0x1C, 0x6A);
    writeCodecRegister(0x44, 0x08);

    // 6. Cấu hình Serial Data Port (SDP): Chuẩn I2S 16-bit
    writeCodecRegister(0x09, 0x0C); // SDP IN (DAC): 16-bit I2S format, power up (Bit 6 = 0)
    writeCodecRegister(0x0A, 0x0C); // SDP OUT (ADC): 16-bit I2S format, power up (Bit 6 = 0)

    // 7. Kích hoạt mạch Analog DAC và ngõ ra loa
    writeCodecRegister(0x0D, 0x01); // Power up analog DAC
    writeCodecRegister(0x0E, 0x02); // Enable DAC output to Line/Headphone (FM8002E)
    writeCodecRegister(0x12, 0x00); // DAC unmute & power up
    writeCodecRegister(0x14, 0x1A); // DAC PGA Gain (+3dB)
    writeCodecRegister(0x15, 0x40);

    // 8. Bỏ Mute DAC & cấu hình âm lượng (Mặc định chip mở lên Reg 0x31 là MUTE)
    writeCodecRegister(0x31, 0x00); // DAC UNMUTE! (Bit 5 & 6 = 0)
    writeCodecRegister(0x37, 0x48); // DAC ramp rate & power stage
    writeCodecRegister(0x45, 0x00);

    setVolume(m_volume);

    uint8_t reg00 = readCodecRegister(0x00);
    uint8_t reg31 = readCodecRegister(0x31);
    uint8_t reg32 = readCodecRegister(0x32);
    LOG_I(TAG, "Codec ES8311 sẵn sàng (Reg00=0x%02X, Reg31=0x%02X, Reg32=0x%02X, Vol=%u%%)",
          reg00, reg31, reg32, m_volume);
    return true;
#else
    LOG_I(TAG, "Không có chân I2C SDA/SCL cho Codec (dùng I2S chuẩn)");
    return false;
#endif
}

void AudioI2sService::setVolume(uint8_t volume) {
    m_volume = (volume > 100) ? 100 : volume;
    if (m_codecDetected) {
        // ES8311 volume register 0x32 (0x00 = mute, 0xBF = 0dB)
        uint8_t regVal = (uint8_t)(((uint32_t)m_volume * 0xBF) / 100);
        writeCodecRegister(0x32, regVal);
    }
}

bool AudioI2sService::begin() {
    if (m_i2sInstalled) {
        return true;
    }

    // 1. Cấu hình I2S DMA driver trước để ESP32 bắt đầu xuất xung MCLK, BCLK, WS cho Codec
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = m_sampleRate,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Stereo để tương thích mọi DAC/Amp
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);
    if (err != ESP_OK) {
        LOG_E(TAG, "Lỗi cài đặt driver I2S: %d", err);
        return false;
    }

    // Gán chân GPIO cho I2S
    i2s_pin_config_t pinConfig = {
#if defined(PIN_I2S_MCLK)
        .mck_io_num = PIN_I2S_MCLK,
#else
        .mck_io_num = I2S_PIN_NO_CHANGE,
#endif
        .bck_io_num = PIN_I2S_BCLK,
        .ws_io_num = PIN_I2S_LRC,
        .data_out_num = PIN_I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    err = i2s_set_pin(I2S_NUM_0, &pinConfig);
    if (err != ESP_OK) {
        LOG_E(TAG, "Lỗi gán chân I2S: %d", err);
        return false;
    }

    i2s_zero_dma_buffer(I2S_NUM_0);
    m_i2sInstalled = true;
    LOG_I(TAG, "I2S DMA đã khởi tạo thành công (MCLK=%d, BCLK=%d, LRC=%d, DOUT=%d, Rate=%uHz)",
          PIN_I2S_MCLK, PIN_I2S_BCLK, PIN_I2S_LRC, PIN_I2S_DOUT, m_sampleRate);

    // 2. Khởi tạo Codec ES8311 qua I2C sau khi I2S clock đã xuất ổn định
    initEs8311Codec();

    // 3. Kích hoạt Power Amplifier FM8002E (Active LOW trên ES3C28P)
#if defined(PIN_PA_ENABLE)
    pinMode(PIN_PA_ENABLE, OUTPUT);
    digitalWrite(PIN_PA_ENABLE, LOW); // Bật bộ khuếch đại công suất FM8002E (Active LOW)
    LOG_I(TAG, "Đã kích hoạt PIN_PA_ENABLE (GPIO %d, Active LOW - Amp ON)", PIN_PA_ENABLE);
#endif

    // 4. Tạo RingBuffer 16KB cho luồng audio từ file AVI (Non-blocking)
    if (!m_ringBuffer) {
        m_ringBuffer = xRingbufferCreate(16384, RINGBUF_TYPE_BYTEBUF);
        if (m_ringBuffer) {
            LOG_I(TAG, "Đã khởi tạo Audio RingBuffer 16KB");
        }
    }

    // 5. Tạo FreeRTOS Task chạy trên Core 0
    BaseType_t res = xTaskCreatePinnedToCore(
        audioTask,
        "AudioI2sTask",
        4096,
        this,
        2,              // Mức ưu tiên vừa phải
        &m_taskHandle,
        0               // Core 0 (nhường Core 1 cho Video và UI)
    );

    if (res != pdPASS) {
        LOG_E(TAG, "Không thể tạo AudioI2sTask!");
        return false;
    }

    return true;
}

bool AudioI2sService::parseWavHeader(File& file) {
    uint8_t header[44];
    if (file.read(header, 44) != 44) {
        LOG_E(TAG, "File WAV quá ngắn hoặc lỗi đọc header");
        return false;
    }

    // 1. Kiểm tra Magic Bytes
    if (memcmp(header, "RIFF", 4) != 0 || memcmp(header + 8, "WAVE", 4) != 0) {
        LOG_E(TAG, "Header không hợp lệ: Không tìm thấy Magic Bytes RIFF/WAVE");
        return false;
    }

    // 2. Định dạng âm thanh PCM
    uint16_t audioFormat = header[20] | (header[21] << 8);
    if (audioFormat != 1) { // 1 = PCM không nén
        LOG_E(TAG, "Định dạng audio không phải PCM không nén (Format ID: %d)", audioFormat);
        return false;
    }

    m_channels = header[22] | (header[23] << 8);
    m_sampleRate = header[24] | (header[25] << 8) | (header[26] << 16) | (header[27] << 24);
    m_bitsPerSample = header[34] | (header[35] << 8);
    m_dataSize = header[40] | (header[41] << 8) | (header[42] << 16) | (header[43] << 24);
    m_dataOffset = 44;

    m_bytesPerSec = m_sampleRate * m_channels * (m_bitsPerSample / 8);
    if (m_bytesPerSec > 0) {
        m_totalDurationMs = (uint32_t)(((uint64_t)m_dataSize * 1000) / m_bytesPerSec);
    }

    LOG_I(TAG, "File WAV: %u Hz, %d channels, %d bits, Size: %u bytes (%u ms)",
          m_sampleRate, m_channels, m_bitsPerSample, m_dataSize, m_totalDurationMs);

    return true;
}

bool AudioI2sService::openFile(const char* wavPath) {
    stop();

    if (!StorageService::lock()) {
        return false;
    }

    m_audioFile = StorageService::openFile(wavPath, FILE_READ);
    StorageService::unlock();

    if (!m_audioFile) {
        LOG_E(TAG, "Không thể mở file audio: %s", wavPath);
        return false;
    }

    if (!parseWavHeader(m_audioFile)) {
        m_audioFile.close();
        return false;
    }

    // Cập nhật sample rate cho driver I2S nếu khác
    if (m_i2sInstalled) {
        i2s_set_sample_rates(I2S_NUM_0, m_sampleRate);
    }

    reset();
    return true;
}

void AudioI2sService::play() {
    m_state = AudioState::PLAYING;
    LOG_I(TAG, "Audio: PLAY");
}

void AudioI2sService::pause() {
    m_state = AudioState::PAUSED;
    if (m_i2sInstalled) {
        i2s_zero_dma_buffer(I2S_NUM_0);
    }
    LOG_I(TAG, "Audio: PAUSE");
}

void AudioI2sService::stop() {
    m_state = AudioState::STOPPED;
    if (m_i2sInstalled) {
        i2s_zero_dma_buffer(I2S_NUM_0);
    }
    LOG_I(TAG, "Audio: STOP");
}

void AudioI2sService::reset() {
    m_bytesPlayed = 0;
    m_isFinished = false;

    if (m_ringBuffer) {
        size_t itemSize = 0;
        uint8_t* item = nullptr;
        while ((item = (uint8_t*)xRingbufferReceiveUpTo(m_ringBuffer, &itemSize, 0, 4096)) != nullptr) {
            vRingbufferReturnItem(m_ringBuffer, item);
        }
    }

    if (m_audioFile) {
        if (StorageService::lock()) {
            m_audioFile.seek(m_dataOffset);
            StorageService::unlock();
        }
    }
    if (m_i2sInstalled) {
        i2s_zero_dma_buffer(I2S_NUM_0);
    }
}

uint32_t AudioI2sService::getCurrentTimeMs() const {
    if (m_bytesPerSec == 0) return 0;
    return (uint32_t)(((uint64_t)m_bytesPlayed * 1000) / m_bytesPerSec);
}

void AudioI2sService::writePcmChunk(const uint8_t* pcmData, size_t len) {
    if (!m_i2sInstalled || pcmData == nullptr || len == 0) return;

    if (m_ringBuffer) {
        // Đẩy thẳng vào RingBuffer không chặn (timeout = 0), Core 0 sẽ đọc và phát
        xRingbufferSend(m_ringBuffer, pcmData, len, 0);
    } else {
        playPcmDirect(pcmData, len);
    }
    m_bytesPlayed += len;
}

void AudioI2sService::playPcmDirect(const uint8_t* pcmData, size_t len) {
    if (!m_i2sInstalled || pcmData == nullptr || len == 0) return;

    // PCM 16-bit Mono: Duplicating Mono sample sang Stereo L+R để cả 2 kênh đều có tín hiệu
    size_t samples = len / 2;
    static int16_t stereoBuffer[512]; // 256 samples stereo (L+R)
    
    const int16_t* inSamples = (const int16_t*)pcmData;
    size_t samplesProcessed = 0;

    while (samplesProcessed < samples) {
        size_t batch = (samples - samplesProcessed > 256) ? 256 : (samples - samplesProcessed);
        for (size_t i = 0; i < batch; ++i) {
            int16_t s = inSamples[samplesProcessed + i];
            // Áp dụng volume scaling đơn giản nếu không có codec I2C
            if (!m_codecDetected && m_volume < 100) {
                s = (int16_t)(((int32_t)s * m_volume) / 100);
            }
            stereoBuffer[i * 2] = s;     // Left
            stereoBuffer[i * 2 + 1] = s; // Right
        }

        size_t bytesWritten = 0;
        i2s_write(I2S_NUM_0, stereoBuffer, batch * 4, &bytesWritten, pdMS_TO_TICKS(50));
        samplesProcessed += batch;
    }
}

void AudioI2sService::processAudio() {
    if (m_state != AudioState::PLAYING || !m_audioFile) {
        return;
    }

    uint8_t buffer[kPcmChunkSize];
    size_t bytesRead = 0;

    if (StorageService::lock(pdMS_TO_TICKS(50))) {
        if (m_audioFile.available()) {
            bytesRead = m_audioFile.read(buffer, kPcmChunkSize);
        } else {
            m_isFinished = true;
            m_state = AudioState::PAUSED;
        }
        StorageService::unlock();
    }

    if (bytesRead > 0) {
        playPcmDirect(buffer, bytesRead);
        m_bytesPlayed += bytesRead;
    }
}

void AudioI2sService::audioTask(void* parameter) {
    AudioI2sService* service = static_cast<AudioI2sService*>(parameter);
    LOG_I(TAG, "AudioI2sTask bắt đầu chạy trên Core %d", xPortGetCoreID());

    while (true) {
        if (service->m_state == AudioState::PLAYING) {
            bool hasData = false;

            // 1. Ưu tiên đọc từ RingBuffer (cho luồng AVI stream từ Core 1)
            if (service->m_ringBuffer) {
                size_t itemSize = 0;
                uint8_t* item = (uint8_t*)xRingbufferReceiveUpTo(service->m_ringBuffer, &itemSize, pdMS_TO_TICKS(2), 512);
                if (item && itemSize > 0) {
                    service->playPcmDirect(item, itemSize);
                    vRingbufferReturnItem(service->m_ringBuffer, item);
                    hasData = true;
                }
            }

            // 2. Nếu không có dữ liệu ringbuffer và có file WAV riêng
            if (!hasData && service->m_audioFile) {
                service->processAudio();
                hasData = true;
            }

            if (!hasData) {
                vTaskDelay(pdMS_TO_TICKS(2));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}
