#include "audio_test.h"
#include "log.h"
#include <math.h>

static const char *TAG = "AudioTest";

AudioTest::AudioTest()
  : _initialized(false), _i2sPort(I2S_NUM_0) {}

AudioTest::~AudioTest() {
  if (_initialized) {
    i2s_driver_uninstall(_i2sPort);
    _initialized = false;
  }
}

bool AudioTest::begin() {
  if (_initialized) return true;

  LOG_I(TAG, "Khoi tao I2S Audio cho NS4168 tai BCLK=%d, LRCK=%d, DOUT=%d",
        PIN_I2S_BCLK, PIN_I2S_LRCK, PIN_I2S_DOUT);

  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 8,
    .dma_buf_len = 128,
    .use_apll = false,
    .tx_desc_auto_clear = true,
    .fixed_mclk = 0
  };

  i2s_pin_config_t pin_config = {
    .bck_io_num = PIN_I2S_BCLK,
    .ws_io_num = PIN_I2S_LRCK,
    .data_out_num = PIN_I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  esp_err_t err = i2s_driver_install(_i2sPort, &i2s_config, 0, NULL);
  if (err != ESP_OK) {
    LOG_E(TAG, "Loi i2s_driver_install: 0x%X", err);
    return false;
  }

  err = i2s_set_pin(_i2sPort, &pin_config);
  if (err != ESP_OK) {
    LOG_E(TAG, "Loi i2s_set_pin: 0x%X", err);
    i2s_driver_uninstall(_i2sPort);
    return false;
  }

  i2s_zero_dma_buffer(_i2sPort);
  _initialized = true;
  LOG_I(TAG, "Khoi tao I2S thanh cong (44.1kHz Stereo/Mono NS4168)!");
  return true;
}

void AudioTest::playTone(float frequency, int durationMs, float volume) {
  if (!_initialized) {
    if (!begin()) return;
  }

  if (volume < 0.0f) volume = 0.0f;
  if (volume > 1.0f) volume = 1.0f;

  const int bufferSamples = 128;
  int16_t buffer[bufferSamples * 2]; 

  int totalSamples = (SAMPLE_RATE * durationMs) / 1000;
  float phaseStep = (2.0f * M_PI * frequency) / (float)SAMPLE_RATE;
  float phase = 0.0f;
  size_t bytesWritten = 0;

  int samplesRemaining = totalSamples;
  while (samplesRemaining > 0) {
    int samplesChunk = (samplesRemaining > bufferSamples) ? bufferSamples : samplesRemaining;
    for (int i = 0; i < samplesChunk; i++) {
      int16_t sample = (int16_t)(sinf(phase) * 32767.0f * volume);
      buffer[i * 2]     = sample; // Left
      buffer[i * 2 + 1] = sample; // Right
      phase += phaseStep;
      if (phase >= 2.0f * M_PI) phase -= 2.0f * M_PI;
    }

    i2s_write(_i2sPort, buffer, samplesChunk * 2 * sizeof(int16_t), &bytesWritten, portMAX_DELAY);
    samplesRemaining -= samplesChunk;
  }

  // Xoa bo dem DMA voi silence de tranh xung pop
  memset(buffer, 0, sizeof(buffer));
  for (int i = 0; i < 4; i++) {
    i2s_write(_i2sPort, buffer, sizeof(buffer), &bytesWritten, portMAX_DELAY);
  }
}

void AudioTest::playChime() {
  LOG_I(TAG, "Phat chuoi am thanh Test Chime am luong lon (880Hz -> 1108Hz -> 1318Hz -> 1760Hz)...");
  playTone(880.0f, 120, 0.85f);   // A5
  vTaskDelay(pdMS_TO_TICKS(15));
  playTone(1108.7f, 120, 0.85f);  // C#6
  vTaskDelay(pdMS_TO_TICKS(15));
  playTone(1318.5f, 150, 0.85f);  // E6
  vTaskDelay(pdMS_TO_TICKS(15));
  playTone(1760.0f, 250, 0.90f);  // A6
  LOG_I(TAG, "Hoan tat phat am thanh!");
}
