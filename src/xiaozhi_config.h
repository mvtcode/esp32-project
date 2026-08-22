#pragma once

// OTA/Activation HTTP endpoint
#define XZ_OTA_DEFAULT_URL    "https://api.tenclass.net/xiaozhi/ota/"

// WebSocket default endpoint
#define XZ_WS_DEFAULT_URL     "wss://api.tenclass.net/xiaozhi/v1/"

// Application version reported to OTA server
#define XZ_APP_VERSION        "2.2.1"

// Audio parameters
#define XZ_SAMPLE_RATE_MIC    16000
#define XZ_SAMPLE_RATE_TTS    24000
#define XZ_FRAME_DURATION_MS  60
#define XZ_OPUS_COMPLEXITY    5

// DAC I2S (PCM5102A) pins
#define XZ_DAC_BCK_PIN        18
#define XZ_DAC_LCK_PIN        19
#define XZ_DAC_DIN_PIN        23

// Timeouts & Retry
#define XZ_ACT_TIMEOUT_MS     30000
#define XZ_ACT_MAX_RETRY      10
#define XZ_CONNECT_TIMEOUT_MS 10000
#define XZ_ERROR_RECOVERY_MS  3000
