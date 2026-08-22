#include "xiaozhi_mode.h"
#include "xiaozhi_config.h"
#include "nvs_storage.h"
#include "wifi_app.h"
#include "display.h"
#include "log.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "driver/i2s.h"
#include <opus.h>
#include <math.h>

// -----------------------------------------------------------------------
// State variables
// -----------------------------------------------------------------------
static XiaozhiScreen   s_screen       = XZ_SCREEN_ACTIVATION;
static XiaozhiActState s_act_state    = XZ_ACT_IDLE;
static XiaozhiState    s_chat_state   = XZ_IDLE;

static char            s_act_code[16]    = "";
static char            s_act_msg[64]     = "";
static char            s_act_chal[128]   = "";
static int             s_act_timeout_sec = 30;
static uint32_t        s_act_timer_ts    = 0;

static float           s_tts_energy      = 0.0f;
static char            s_emotion[32]     = "neutral";
static uint8_t         s_volume          = 80;
static bool            s_is_auto_mode    = true;

static QueueHandle_t   s_vis_queue       = nullptr;
static TaskHandle_t    s_act_task_handle = nullptr;
static TaskHandle_t    s_ws_task_handle  = nullptr;
static TaskHandle_t    s_mic_task_handle = nullptr;
static volatile bool   s_task_running    = false;
static WebSocketsClient *s_ws_client     = nullptr;
static String            s_session_id     = "";
static bool              s_ws_connected   = false;

// Opus Codec Handles
static OpusEncoder     *s_opus_encoder    = nullptr;
static OpusDecoder     *s_opus_decoder    = nullptr;

// Visualizer Frame Accumulator for TTS audio
struct XZFrameAccumulator {
    int32_t left[FRAME_SIZE];
    int32_t right[FRAME_SIZE];
    size_t count = 0;
};
static XZFrameAccumulator s_xz_fa;

// Forward declarations
static void start_chat_pipeline();
static void stop_chat_pipeline();

// -----------------------------------------------------------------------
// Helper: get device MAC string
// [TEST MODE] Override MAC to use a known-activated device MAC
// -----------------------------------------------------------------------
#define XZ_TEST_MAC "3c:dc:75:6e:87:40"  // Test MAC - remove after debugging
static void get_mac_str(char *out, size_t max_len) {
#ifdef XZ_TEST_MAC
    strncpy(out, XZ_TEST_MAC, max_len - 1);
    out[max_len - 1] = '\0';
    LOG_W("XZ", "[TEST] Using override MAC: %s", out);
#else
    uint8_t mac[6];
    WiFi.macAddress(mac);
    snprintf(out, max_len, "%02x:%02x:%02x:%02x:%02x:%02x",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
#endif
}

// -----------------------------------------------------------------------
// Helper: get or generate UUID client_id
// -----------------------------------------------------------------------
static void get_client_id(char *out, size_t max_len) {
    if (nvs_load_xz_client_id(out, max_len) && strlen(out) > 0) {
        return;
    }
    uint32_t r1 = esp_random();
    uint32_t r2 = esp_random();
    uint32_t r3 = esp_random();
    uint32_t r4 = esp_random();
    snprintf(out, max_len, "%08x-%04x-%04x-%04x-%08x%04x",
             r1, (uint16_t)(r2 >> 16), (uint16_t)(r2 & 0x0FFF) | 0x4000,
             (uint16_t)(r3 & 0x3FFF) | 0x8000, r4, (uint16_t)(r3 >> 16));
    nvs_save_xz_client_id(out);
}

// -----------------------------------------------------------------------
// DAC I2S Init for TTS playback (PCM5102A on I2S_NUM_0)
// -----------------------------------------------------------------------
static bool xz_dac_init() {
    i2s_driver_uninstall(I2S_NUM_0); // Ensure port 0 is completely free
    vTaskDelay(pdMS_TO_TICKS(10));

    static const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = XZ_SAMPLE_RATE_TTS, // 24000 Hz
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 3,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    static const i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = XZ_DAC_BCK_PIN, // GPIO 18
        .ws_io_num = XZ_DAC_LCK_PIN,  // GPIO 19
        .data_out_num = XZ_DAC_DIN_PIN, // GPIO 23
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err == ESP_OK) {
        i2s_set_pin(I2S_NUM_0, &pin_config);
        LOG_I("XZ", "I2S DAC initialized at %d Hz", XZ_SAMPLE_RATE_TTS);
        return true;
    }
    LOG_W("XZ", "I2S DAC init failed: %d", (int)err);
    return false;
}

static void xz_dac_deinit() {
    i2s_driver_uninstall(I2S_NUM_0);
}

// -----------------------------------------------------------------------
// Opus Codec Init & Deinit
// -----------------------------------------------------------------------
static bool opus_init() {
    int error = 0;
    
    // Mic Opus Encoder (16kHz, mono, VOIP)
    int enc_size = opus_encoder_get_size(1);
    if (enc_size > 0) {
        s_opus_encoder = (OpusEncoder *)malloc(enc_size);
        if (s_opus_encoder) {
            error = opus_encoder_init(s_opus_encoder, 16000, 1, OPUS_APPLICATION_VOIP);
            if (error != OPUS_OK) {
                error = opus_encoder_init(s_opus_encoder, 16000, 1, OPUS_APPLICATION_RESTRICTED_LOWDELAY);
            }
        }
    }
    if (s_opus_encoder && error == OPUS_OK) {
        opus_encoder_ctl(s_opus_encoder, OPUS_SET_COMPLEXITY(0));
        opus_encoder_ctl(s_opus_encoder, OPUS_SET_BITRATE(24000));
        opus_encoder_ctl(s_opus_encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_VOICE));
    } else {
        LOG_W("XZ", "Opus encoder init failed: %d, trying fallback create...", error);
        s_opus_encoder = opus_encoder_create(16000, 1, OPUS_APPLICATION_VOIP, &error);
    }

    // TTS Opus Decoder (16kHz / 24kHz mono)
    int dec_size = opus_decoder_get_size(1);
    if (dec_size > 0) {
        s_opus_decoder = (OpusDecoder *)malloc(dec_size);
        if (s_opus_decoder) {
            error = opus_decoder_init(s_opus_decoder, 16000, 1);
            if (error != OPUS_OK) {
                error = opus_decoder_init(s_opus_decoder, 24000, 1);
            }
        }
    }
    if (!s_opus_decoder || error != OPUS_OK) {
        LOG_W("XZ", "Opus decoder init error: %d, fallback to create 16kHz", error);
        s_opus_decoder = opus_decoder_create(16000, 1, &error);
    }

    LOG_I("XZ", "Opus Codecs initialized (Encoder: %p, Decoder: %p)", s_opus_encoder, s_opus_decoder);
    return true;
}

static void opus_deinit() {
    if (s_opus_encoder) {
        opus_encoder_destroy(s_opus_encoder);
        s_opus_encoder = nullptr;
    }
    if (s_opus_decoder) {
        opus_decoder_destroy(s_opus_decoder);
        s_opus_decoder = nullptr;
    }
}

// -----------------------------------------------------------------------
// WebSocket JSON Dispatcher
// -----------------------------------------------------------------------
static void send_ws_json(const JsonDocument &doc) {
    if (!s_ws_connected || !s_ws_client) return;
    String out;
    serializeJson(doc, out);
    s_ws_client->sendTXT(out);
}

static void handle_ws_json_message(const char *payload) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (err) {
        LOG_W("XZ", "Malformed WS JSON: %s", payload);
        return;
    }

    const char *type = doc["type"] | "";

    if (strcmp(type, "hello") == 0) {
        s_session_id = doc["session_id"].as<String>();
        LOG_I("XZ", "Server Hello received! Session: %s", s_session_id.c_str());
        s_chat_state = XZ_IDLE;
        if (s_is_auto_mode) {
            xiaozhi_start_listen();
        }
    }
    else if (strcmp(type, "stt") == 0) {
        const char *text = doc["text"] | "";
        LOG_I("XZ", "STT: %s", text);
        display_toast(text, 2500);
    }
    else if (strcmp(type, "llm") == 0) {
        const char *emo = doc["emotion"] | "neutral";
        strncpy(s_emotion, emo, sizeof(s_emotion) - 1);
        LOG_I("XZ", "LLM Emotion: %s", s_emotion);
    }
    else if (strcmp(type, "tts") == 0) {
        const char *state = doc["state"] | "";
        if (strcmp(state, "start") == 0) {
            LOG_I("XZ", "TTS Started -> SPEAKING");
            s_chat_state = XZ_SPEAKING;
        } else if (strcmp(state, "stop") == 0) {
            LOG_I("XZ", "TTS Stopped");
            s_tts_energy = 0.0f;
            if (s_is_auto_mode) {
                xiaozhi_start_listen();
            } else {
                s_chat_state = XZ_IDLE;
            }
        }
    }
    else if (strcmp(type, "system") == 0) {
        const char *cmd = doc["command"] | "";
        if (strcmp(cmd, "reboot") == 0) {
            LOG_I("XZ", "System reboot requested by server");
            ESP.restart();
        }
    }
}

// -----------------------------------------------------------------------
// Playback Binary Opus Frame (TTS)
// -----------------------------------------------------------------------
static void handle_ws_binary_audio(const uint8_t *data, size_t len) {
    if (!s_opus_decoder || !data || len == 0) return;

    // Decode Opus frame (up to 60ms @ 24kHz = 1440 samples)
    int16_t pcm_out[1440];
    int samples_decoded = opus_decode(s_opus_decoder, data, len, pcm_out, 1440, 0);

    if (samples_decoded <= 0) {
        LOG_W("XZ", "Opus decode error: %d", samples_decoded);
        return;
    }

    // 1. Calculate audio energy for mouth animation & feed Visualizer
    float sum_sq = 0.0f;
    for (int i = 0; i < samples_decoded; i++) {
        float sample = (float)pcm_out[i] / 32768.0f;
        sum_sq += sample * sample;

        // Feed to visualizer queue
        if (s_vis_queue) {
            s_xz_fa.left[s_xz_fa.count]  = (int32_t)pcm_out[i];
            s_xz_fa.right[s_xz_fa.count] = (int32_t)pcm_out[i];
            s_xz_fa.count++;

            if (s_xz_fa.count >= FRAME_SIZE) {
                AudioFrame frame;
                memcpy(frame.left, s_xz_fa.left, sizeof(s_xz_fa.left));
                memcpy(frame.right, s_xz_fa.right, sizeof(s_xz_fa.right));
                xQueueSend(s_vis_queue, &frame, 0);
                s_xz_fa.count = 0;
            }
        }
    }

    float rms = sqrtf(sum_sq / (float)samples_decoded);
    s_tts_energy = constrain(rms * 2.8f, 0.0f, 1.0f);

    // 2. Scale volume & output to I2S DAC (stereo interleaved)
    float vol_factor = powf((float)s_volume / 127.0f, 1.3f);
    int16_t stereo_buf[256 * 2];
    size_t written = 0;

    for (int i = 0; i < samples_decoded; i += 256) {
        int chunk = (samples_decoded - i > 256) ? 256 : (samples_decoded - i);
        for (int j = 0; j < chunk; j++) {
            int16_t sample = (int16_t)constrain((int32_t)((float)pcm_out[i + j] * vol_factor), -32768, 32767);
            stereo_buf[j * 2]     = sample;
            stereo_buf[j * 2 + 1] = sample;
        }
        i2s_write(I2S_NUM_0, stereo_buf, chunk * 4, &written, pdMS_TO_TICKS(50));
    }
}

// -----------------------------------------------------------------------
static void ws_event_cb(WStype_t type, uint8_t *payload, size_t length) {
    switch (type) {
    case WStype_ERROR:
        LOG_E("XZ", "WebSocket Error: %s", payload ? (const char*)payload : "unknown");
        break;

    case WStype_DISCONNECTED:
        s_ws_connected = false;
        s_chat_state = XZ_ERROR;
        LOG_W("XZ", "WebSocket Disconnected (len=%d): %s", (int)length, payload ? (const char*)payload : "");
        break;

    case WStype_CONNECTED: {
        s_ws_connected = true;
        s_chat_state = XZ_CONNECTING;
        LOG_I("XZ", "WebSocket Connected! Sending hello packet...");

        // Send Hello Message matching exact XiaoZhi cJSON format
        const char *hello_str = "{\"type\":\"hello\",\"version\":1,\"features\":{\"mcp\":true},\"transport\":\"websocket\",\"audio_params\":{\"format\":\"opus\",\"sample_rate\":16000,\"channels\":1,\"frame_duration\":60}}";
        s_ws_client->sendTXT(hello_str);
        LOG_I("XZ", "Sent Hello: %s", hello_str);
        break;
    }

    case WStype_TEXT:
        LOG_D("XZ", "WS RECV TXT: %s", (const char *)payload);
        handle_ws_json_message((const char *)payload);
        break;

    case WStype_BIN:
        handle_ws_binary_audio(payload, length);
        break;

    case WStype_PING:
        LOG_D("XZ", "WS PING");
        break;

    case WStype_PONG:
        LOG_D("XZ", "WS PONG");
        break;

    default:
        break;
    }
}

// -----------------------------------------------------------------------
// Phase B Tasks: WebSocket Client & Mic Streaming
// -----------------------------------------------------------------------
static void xz_ws_task(void *pv) {
    LOG_I("XZ", "WS task started");
    char ws_url[160];
    char token[128];
    char mac[24];
    char cid[64];

    get_mac_str(mac, sizeof(mac));
    get_client_id(cid, sizeof(cid));
    if (!nvs_load_xz_ws_url(ws_url, sizeof(ws_url))) {
        strncpy(ws_url, XZ_WS_DEFAULT_URL, sizeof(ws_url) - 1);
    }
    nvs_load_xz_token(token, sizeof(token));

    // Parse URL (e.g. wss://api.tenclass.net/xiaozhi/v1/)
    String url = String(ws_url);
    bool is_ssl = url.startsWith("wss://");
    url.replace("wss://", "");
    int slash_idx = url.indexOf('/');
    String host = (slash_idx > 0) ? url.substring(0, slash_idx) : url;
    String path = (slash_idx > 0) ? url.substring(slash_idx) : "/";
    uint16_t port = is_ssl ? 443 : 80;

    // Headers exact match with py-xiaozhi & xiaozhi-esp32:
    // Authorization: Bearer <token>
    // Protocol-Version: 1
    // Device-Id: <mac>
    // Client-Id: <cid>
    String headers = "Authorization: Bearer " + String(token) + "\r\n"
                   + "Protocol-Version: 1\r\n"
                   + "Device-Id: " + String(mac) + "\r\n"
                   + "Client-Id: " + String(cid) + "\r\n";

    LOG_I("XZ", "Connecting WebSocket to %s:%d%s (SSL: %d)", host.c_str(), port, path.c_str(), is_ssl ? 1 : 0);
    LOG_I("XZ", "Headers:\n%s", headers.c_str());

    s_ws_client = new WebSocketsClient();
    s_ws_client->setExtraHeaders(headers.c_str());
    if (is_ssl) {
        s_ws_client->beginSSL(host.c_str(), port, path.c_str(), "", "");
    } else {
        s_ws_client->begin(host.c_str(), port, path.c_str(), "");
    }
    s_ws_client->onEvent(ws_event_cb);
    s_ws_client->setReconnectInterval(3000);
    // Enable heartbeat ping
    s_ws_client->enableHeartbeat(30000, 10000, 2);

    while (s_task_running) {
        if (s_ws_client) s_ws_client->loop();
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    if (s_ws_client) {
        s_ws_client->disconnect();
        delete s_ws_client;
        s_ws_client = nullptr;
    }
    s_ws_task_handle = nullptr;
    vTaskDelete(NULL);
}

static void xz_mic_task(void *pv) {
    LOG_I("XZ", "Mic streaming task started");
    const int frame_samples = XZ_SAMPLE_RATE_MIC * XZ_FRAME_DURATION_MS / 1000; // 960 samples @ 16kHz
    static int16_t pcm_in[960];
    static uint8_t opus_out[256];

    while (s_task_running) {
        if (s_ws_connected && s_chat_state == XZ_LISTENING && s_opus_encoder) {
            // Read samples from I2S mic (L/R downmixed to mono)
            int32_t left[128], right[128];
            int collected = 0;

            while (collected < frame_samples && s_task_running && s_chat_state == XZ_LISTENING) {
                if (i2s_mic_read(left, right, 128)) {
                    for (int i = 0; i < 128 && (collected + i) < frame_samples; i++) {
                        int32_t mono = (left[i] + right[i]) / 2;
                        // Convert 24-bit to 16-bit
                        pcm_in[collected + i] = (int16_t)constrain(mono >> 8, -32768, 32767);
                    }
                    collected += 128;
                } else {
                    vTaskDelay(pdMS_TO_TICKS(5));
                }
            }

            if (collected >= frame_samples && s_chat_state == XZ_LISTENING) {
                int encoded_bytes = opus_encode(s_opus_encoder, pcm_in, frame_samples, opus_out, sizeof(opus_out));
                if (encoded_bytes > 0 && s_ws_client) {
                    s_ws_client->sendBIN(opus_out, encoded_bytes);
                }
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }

    s_mic_task_handle = nullptr;
    vTaskDelete(NULL);
}

// -----------------------------------------------------------------------
// Chat Pipeline Starter
// -----------------------------------------------------------------------
static void start_chat_pipeline() {
    LOG_I("XZ", "Starting chat pipeline (free heap: %d)...", (int)ESP.getFreeHeap());
    xz_dac_init();
    opus_init();
    s_screen = XZ_SCREEN_CHAT;
    s_chat_state = XZ_CONNECTING;

    if (s_ws_task_handle == nullptr) {
        BaseType_t res = xTaskCreatePinnedToCore(xz_ws_task, "xz_ws_task", 10240, nullptr, 3, &s_ws_task_handle, 1);
        LOG_I("XZ", "xz_ws_task create: %s", (res == pdPASS) ? "OK" : "FAIL (OOM)");
    }
    if (s_mic_task_handle == nullptr) {
        BaseType_t res = xTaskCreatePinnedToCore(xz_mic_task, "xz_mic_task", 8192, nullptr, 2, &s_mic_task_handle, 0);
        LOG_I("XZ", "xz_mic_task create: %s", (res == pdPASS) ? "OK" : "FAIL (OOM)");
    }
}

static void stop_chat_pipeline() {
    if (s_ws_task_handle != nullptr) {
        vTaskDelete(s_ws_task_handle);
        s_ws_task_handle = nullptr;
    }
    if (s_mic_task_handle != nullptr) {
        vTaskDelete(s_mic_task_handle);
        s_mic_task_handle = nullptr;
    }
    if (s_ws_client != nullptr) {
        s_ws_client->disconnect();
        delete s_ws_client;
        s_ws_client = nullptr;
    }
    opus_deinit();
    xz_dac_deinit();
    s_ws_connected = false;
    s_chat_state = XZ_IDLE;
}

// -----------------------------------------------------------------------
// Phase A: Activation FreeRTOS Task
// -----------------------------------------------------------------------
static void xz_act_task(void *pv) {
    LOG_I("XZ", "Activation task started");
    char mac[24];
    char cid[64];
    char ota_url[128];

    get_mac_str(mac, sizeof(mac));
    get_client_id(cid, sizeof(cid));
    if (!nvs_load_xz_ota_url(ota_url, sizeof(ota_url))) {
        strncpy(ota_url, XZ_OTA_DEFAULT_URL, sizeof(ota_url) - 1);
    }

#ifdef XZ_TEST_MAC
    // [TEST MODE] Clear cached token and WS URL so OTA fetches fresh credentials for test MAC
    nvs_save_xz_token("");
    nvs_save_xz_ws_url("");
    nvs_save_xz_activated(false);
    LOG_W("XZ", "[TEST] Cleared cached token/URL for test MAC: %s", mac);
#endif

    while (s_task_running) {
        // Step 1: Wait for WiFi connection
        if (!wifi_app_is_connected()) {
            s_act_state = XZ_ACT_WIFI_WAIT;
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        // Step 2: Call HTTP /check-version
        s_act_state = XZ_ACT_FETCH;
        snprintf(s_act_msg, sizeof(s_act_msg), "Dang ket noi Server...");

        WiFiClientSecure client;
        client.setInsecure();
        HTTPClient http;

        String check_url = String(ota_url);
        http.begin(client, check_url);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Device-Id", mac);
        http.addHeader("Client-Id", cid);
        http.addHeader("Activation-Version", "1");
        http.addHeader("User-Agent", "Xiaozhi/1.0.0 (ESP32)");
        http.addHeader("Accept-Language", "vi-VN,vi;q=0.9,en;q=0.8");
        http.setTimeout(8000);

        JsonDocument req_doc;
        req_doc["version"] = 2;
        req_doc["language"] = "vi-VN";
        req_doc["flash_size"] = 4194304;
        req_doc["psram_size"] = 0;
        req_doc["minimum_free_heap_size"] = ESP.getMinFreeHeap();
        req_doc["mac_address"] = mac;
        req_doc["uuid"] = cid;
        req_doc["chip_model_name"] = "esp32";

        JsonObject app = req_doc["application"].to<JsonObject>();
        app["name"] = "bread-compact-esp32";
        app["version"] = XZ_APP_VERSION;
        app["elf_sha256"] = "0000000000000000000000000000000000000000000000000000000000000000";

        JsonObject chip = req_doc["chip_info"].to<JsonObject>();
        chip["model"] = 1;
        chip["cores"] = 2;
        chip["revision"] = 3;
        chip["features"] = 0;

        JsonObject board = req_doc["board"].to<JsonObject>();
        board["type"] = "bread-compact-esp32";
        board["name"] = "bread-compact-esp32";
        board["manufacturer"] = "78";
        board["mac"] = mac;

        String req_body;
        serializeJson(req_doc, req_body);

        LOG_I("XZ", "POST OTA Check: %s", req_body.c_str());
        int http_code = http.POST(req_body);
        if (http_code == 200) {
            String payload = http.getString();
            http.end();

            JsonDocument res_doc;
            DeserializationError err = deserializeJson(res_doc, payload);
            if (!err) {
                if (res_doc["activation"].is<JsonObject>()) {
                    JsonObject act = res_doc["activation"];
                    const char *code = act["code"] | "";
                    const char *msg  = act["message"] | "Nhap code vao App";
                    const char *chal = act["challenge"] | "";
                    int timeout_ms   = act["timeout_ms"] | 30000;

                    strncpy(s_act_code, code, sizeof(s_act_code) - 1);
                    strncpy(s_act_msg, msg, sizeof(s_act_msg) - 1);
                    strncpy(s_act_chal, chal, sizeof(s_act_chal) - 1);
                    s_act_timeout_sec = timeout_ms / 1000;
                    s_act_timer_ts = millis();
                    s_act_state = XZ_ACT_WAIT;

                    LOG_I("XZ", "Activation Code: %s, Chal: %s", s_act_code, s_act_chal);

                    int retry_count = 0;
                    bool activated = false;
                    while (s_task_running && retry_count < XZ_ACT_MAX_RETRY && !activated) {
                        vTaskDelay(pdMS_TO_TICKS(3000));
                        retry_count++;

                        int elapsed = (millis() - s_act_timer_ts) / 1000;
                        int remain = (timeout_ms / 1000) - elapsed;
                        s_act_timeout_sec = remain > 0 ? remain : 0;

                        if (s_act_timeout_sec <= 0) {
                            LOG_W("XZ", "Activation code expired, refreshing...");
                            break;
                        }

                        String act_url = String(ota_url);
                        if (!act_url.endsWith("/")) act_url += "/";
                        act_url += "activate";

                        WiFiClientSecure act_client;
                        act_client.setInsecure();
                        HTTPClient act_http;
                        act_http.begin(act_client, act_url);
                        act_http.addHeader("Content-Type", "application/json");
                        act_http.addHeader("Device-Id", mac);
                        act_http.addHeader("Client-Id", cid);
                        act_http.addHeader("Activation-Version", "1");
                        act_http.addHeader("User-Agent", "Xiaozhi/1.0.0 (ESP32)");
                        act_http.setTimeout(5000);

                        JsonDocument act_req;
                        act_req["challenge"] = s_act_chal;
                        act_req["code"] = s_act_code;
                        String act_body;
                        serializeJson(act_req, act_body);

                        int act_code_res = act_http.POST(act_body);
                        if (act_code_res == 202) {
                            LOG_D("XZ", "Waiting for user confirmation on app/web (HTTP 202)...");
                        } else {
                            LOG_I("XZ", "POST /activate try %d -> HTTP %d", retry_count, act_code_res);
                        }

                        if (act_code_res == 200) {
                            String act_payload = act_http.getString();
                            JsonDocument act_res;
                            deserializeJson(act_res, act_payload);
                            
                            if (act_res["websocket"].is<JsonObject>()) {
                                const char *ws_url = act_res["websocket"]["url"] | "";
                                const char *token  = act_res["websocket"]["token"] | "";
                                if (strlen(ws_url) > 0) nvs_save_xz_ws_url(ws_url);
                                if (strlen(token) > 0)  nvs_save_xz_token(token);
                            }

                            activated = true;
                            s_act_state = XZ_ACT_DONE;
                            nvs_save_xz_activated(true);
                            act_http.end();
                            break;
                        }
                        act_http.end();
                    }
                } else if (res_doc["websocket"].is<JsonObject>()) {
                    LOG_I("XZ", "Device already active on server. Starting Chat...");
                    LOG_I("XZ", "Server response: %s", payload.c_str());
                    const char *ws_url = res_doc["websocket"]["url"] | "";
                    const char *token  = res_doc["websocket"]["token"] | "";
                    if (strlen(ws_url) > 0) {
                        LOG_I("XZ", "WS URL from server: %s", ws_url);
                        nvs_save_xz_ws_url(ws_url);
                    }
                    if (strlen(token) > 0) {
                        LOG_I("XZ", "Token from server: %s", token);
                        nvs_save_xz_token(token);
                    }
                    nvs_save_xz_activated(true);
                    s_act_state = XZ_ACT_DONE;
                    break;
                }
            } else {
                s_act_state = XZ_ACT_ERROR;
                snprintf(s_act_msg, sizeof(s_act_msg), "Parse JSON loi");
                vTaskDelay(pdMS_TO_TICKS(3000));
            }
        } else {
            String err_resp = http.getString();
            LOG_E("XZ", "HTTP failed (%d): %s", http_code, err_resp.c_str());
            http.end();
            s_act_state = XZ_ACT_ERROR;
            snprintf(s_act_msg, sizeof(s_act_msg), "HTTP Loi: %d", http_code);
            vTaskDelay(pdMS_TO_TICKS(3000));
        }
    }

    s_act_task_handle = nullptr;
    vTaskDelay(pdMS_TO_TICKS(500));
    start_chat_pipeline();
    vTaskDelete(NULL);
}

// -----------------------------------------------------------------------
// Public Control & Lifecycle APIs
// -----------------------------------------------------------------------
void xiaozhi_init() {
    s_volume = nvs_load_xz_volume();
}

void xiaozhi_start(QueueHandle_t audio_queue) {
    LOG_I("XZ", "Starting XiaoZhi AI Mode...");
    s_vis_queue = audio_queue;
    s_task_running = true;
    s_volume = nvs_load_xz_volume();

    // Always run activation/check task on boot to obtain fresh token and endpoint
    s_screen = XZ_SCREEN_ACTIVATION;
    s_act_state = XZ_ACT_IDLE;
    if (s_act_task_handle == nullptr) {
        xTaskCreatePinnedToCore(xz_act_task, "xz_act_task", 6144, nullptr, 3, &s_act_task_handle, 1);
    }
}

void xiaozhi_stop() {
    LOG_I("XZ", "Stopping XiaoZhi AI Mode...");
    s_task_running = false;
    if (s_act_task_handle != nullptr) {
        vTaskDelete(s_act_task_handle);
        s_act_task_handle = nullptr;
    }
    stop_chat_pipeline();
    s_act_state = XZ_ACT_IDLE;
    s_chat_state = XZ_IDLE;
}

void xiaozhi_loop() {
    if (s_screen == XZ_SCREEN_ACTIVATION && s_act_state == XZ_ACT_WAIT) {
        int elapsed = (millis() - s_act_timer_ts) / 1000;
        int remain = 30 - elapsed;
        s_act_timeout_sec = remain > 0 ? remain : 0;
    }
}

void xiaozhi_start_listen() {
    if (!s_ws_connected) return;
    s_chat_state = XZ_LISTENING;
    JsonDocument doc;
    doc["session_id"] = s_session_id;
    doc["type"] = "listen";
    doc["state"] = "start";
    doc["mode"] = s_is_auto_mode ? "auto" : "manual";
    send_ws_json(doc);
    LOG_I("XZ", "Start listening (mode: %s)", s_is_auto_mode ? "auto" : "manual");
}

void xiaozhi_stop_listen() {
    if (!s_ws_connected) return;
    s_chat_state = XZ_IDLE;
    JsonDocument doc;
    doc["session_id"] = s_session_id;
    doc["type"] = "listen";
    doc["state"] = "stop";
    send_ws_json(doc);
    LOG_I("XZ", "Stop listening");
}

void xiaozhi_abort_speaking() {
    if (!s_ws_connected) return;
    s_chat_state = XZ_IDLE;
    s_tts_energy = 0.0f;
    JsonDocument doc;
    doc["session_id"] = s_session_id;
    doc["type"] = "abort";
    doc["reason"] = "user_interrupted";
    send_ws_json(doc);
    LOG_I("XZ", "Abort speaking (interrupted)");
}

void xiaozhi_next_face_theme() {
    display_next_mode();
}

bool xiaozhi_is_auto_mode() {
    return s_is_auto_mode;
}

void xiaozhi_set_auto_mode(bool is_auto) {
    s_is_auto_mode = is_auto;
}

void xiaozhi_adjust_volume(int32_t delta) {
    int v = (int)s_volume + delta * 4;
    if (v < 0) v = 0;
    if (v > 127) v = 127;
    s_volume = (uint8_t)v;
    nvs_save_xz_volume(s_volume);
}

uint8_t xiaozhi_get_volume() {
    return s_volume;
}

XiaozhiScreen xiaozhi_get_screen() {
    return s_screen;
}

XiaozhiActState xiaozhi_get_act_state() {
    return s_act_state;
}

const char* xiaozhi_get_act_code() {
    return s_act_code;
}

const char* xiaozhi_get_act_message() {
    return s_act_msg;
}

int xiaozhi_get_act_timeout_sec() {
    return s_act_timeout_sec;
}

XiaozhiState xiaozhi_get_state() {
    return s_chat_state;
}

float xiaozhi_get_tts_energy() {
    return s_tts_energy;
}

const char* xiaozhi_get_emotion() {
    return s_emotion;
}
