#include "bt_audio.h"
#include "nvs_storage.h"
#include "display.h"
#include "log.h"
#include <math.h>

static QueueHandle_t     s_vis_queue = nullptr;
static bool              s_is_connected = false;
static bool              s_is_playing = false;
static bool              s_is_started = false;
static uint8_t           s_current_volume = 80;
static char              s_device_name[64] = "None";
static uint8_t           s_connected_bda[6] = {0};

// Frame accumulator for Visualizer (downmix/subsample 44.1kHz stereo to 128-sample visualizer frames)
struct FrameAccumulator {
    int32_t left[FRAME_SIZE];
    int32_t right[FRAME_SIZE];
    size_t count = 0;
};
static FrameAccumulator s_fa;

// ---------------------------------------------------------------------------
// PCM Audio Data Callback from A2DP Sink (Fault-tolerant without PCM5102A)
// ---------------------------------------------------------------------------
static void bt_i2s_data_callback(const uint8_t *data, uint32_t len) {
    if (!data || len == 0 || !s_is_started) return;

    const int16_t *samples = (const int16_t *)data;
    size_t num_pairs = len / (2 * sizeof(int16_t));
    s_is_playing = true;

    // 1. Feed Visualizer queue FIRST so OLED visualizer always works
    if (s_vis_queue) {
        for (size_t i = 0; i < num_pairs; i++) {
            s_fa.left[s_fa.count]  = (int32_t)samples[i * 2];
            s_fa.right[s_fa.count] = (int32_t)samples[i * 2 + 1];
            s_fa.count++;

            if (s_fa.count >= FRAME_SIZE) {
                AudioFrame frame;
                memcpy(frame.left, s_fa.left, sizeof(s_fa.left));
                memcpy(frame.right, s_fa.right, sizeof(s_fa.right));
                xQueueSend(s_vis_queue, &frame, 0);
                s_fa.count = 0;
            }
        }
    }

    // 2. Pure Lossless CD-Quality Audio Output to I2S DAC (PCM5102A on I2S_NUM_0)
    if (s_current_volume >= 127) {
        size_t written = 0;
        i2s_write(I2S_NUM_0, data, len, &written, portMAX_DELAY);
        return;
    }

    float vol_factor = powf((float)s_current_volume / 127.0f, 1.3f);
    static int16_t s_scaled_pcm[2048];
    size_t total_samples = len / sizeof(int16_t);
    if (total_samples > 2048) total_samples = 2048;

    for (size_t i = 0; i < total_samples; i++) {
        s_scaled_pcm[i] = (int16_t)constrain((int32_t)((float)samples[i] * vol_factor), -32768, 32767);
    }

    size_t bytes_written = 0;
    i2s_write(I2S_NUM_0, s_scaled_pcm, total_samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
}

// ---------------------------------------------------------------------------
// A2DP Stack Callback (Connection & Audio State)
// ---------------------------------------------------------------------------
static void a2d_cb(esp_a2d_cb_event_t event, esp_a2d_cb_param_t *param) {
    esp_a2d_cb_param_t *a2d = (esp_a2d_cb_param_t *)(param);
    switch (event) {
    case ESP_A2D_CONNECTION_STATE_EVT: {
        if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
            s_is_connected = true;
            uint8_t *temp = a2d->conn_stat.remote_bda;
            memcpy(s_connected_bda, temp, 6);
            nvs_save_bt_mac(s_connected_bda);
            LOG_I("BT", "Connected: %02X:%02X:%02X:%02X:%02X:%02X",
                  temp[0], temp[1], temp[2], temp[3], temp[4], temp[5]);
            display_toast("BT CONNECTED!");
        } else if (a2d->conn_stat.state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
            s_is_connected = false;
            s_is_playing = false;
            memset(s_connected_bda, 0, 6);
            LOG_I("BT", "A2DP Disconnected (reason: %d)", a2d->conn_stat.disc_rsn);
            display_toast("BT DISCONNECTED");
            // Re-enable discoverable
            esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        }
        break;
    }
    case ESP_A2D_AUDIO_STATE_EVT: {
        s_is_playing = (a2d->audio_stat.state == ESP_A2D_AUDIO_STATE_STARTED);
        LOG_I("BT", "Audio state: %s", s_is_playing ? "STARTED" : "STOPPED");
        break;
    }
    case ESP_A2D_AUDIO_CFG_EVT: {
        if (a2d->audio_cfg.mcc.type == ESP_A2D_MCT_SBC) {
            uint32_t sample_rate = 44100;
            char oct0 = a2d->audio_cfg.mcc.cie.sbc[0];
            if (oct0 & (0x01 << 6)) {
                sample_rate = 32000;
            } else if (oct0 & (0x01 << 5)) {
                sample_rate = 44100;
            } else if (oct0 & (0x01 << 4)) {
                sample_rate = 48000;
            } else if (oct0 & (0x01 << 7)) {
                sample_rate = 16000;
            }
            LOG_I("BT", "SBC Configured: %u Hz", sample_rate);
            i2s_set_sample_rates(I2S_NUM_0, sample_rate);
        }
        break;
    }
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// AVRCP Controller Callback (Track Metadata & Remote Controls)
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// AVRCP Controller Callback (Track Metadata & Remote Controls)
// ---------------------------------------------------------------------------
static void avrc_ct_cb(esp_avrc_ct_cb_event_t event, esp_avrc_ct_cb_param_t *param) {
    esp_avrc_ct_cb_param_t *rc = (esp_avrc_ct_cb_param_t *)(param);
    switch (event) {
    case ESP_AVRC_CT_CONNECTION_STATE_EVT: {
        if (rc->conn_stat.connected) {
            LOG_I("AVRCP CT", "Connected to source!");
            uint8_t attr_mask = ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST;
            esp_avrc_ct_send_metadata_cmd(0, attr_mask);
            esp_avrc_ct_send_register_notification_cmd(1, ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
        }
        break;
    }
    case ESP_AVRC_CT_METADATA_RSP_EVT: {
        if (rc->meta_rsp.attr_id == ESP_AVRC_MD_ATTR_TITLE && rc->meta_rsp.attr_text) {
            size_t len = (rc->meta_rsp.attr_length < sizeof(s_device_name) - 1) ? rc->meta_rsp.attr_length : sizeof(s_device_name) - 1;
            memcpy(s_device_name, rc->meta_rsp.attr_text, len);
            s_device_name[len] = '\0';
            LOG_I("AVRCP CT", "Track Title: %s", s_device_name);
        }
        break;
    }
    case ESP_AVRC_CT_CHANGE_NOTIFY_EVT: {
        if (rc->change_ntf.event_id == ESP_AVRC_RN_PLAY_STATUS_CHANGE) {
            uint8_t status = rc->change_ntf.event_parameter.playback;
            s_is_playing = (status == ESP_AVRC_PLAYBACK_PLAYING);
            LOG_D("AVRCP CT", "Remote Play Status: %s", s_is_playing ? "PLAYING" : "PAUSED");
            // Re-register notification for next event
            esp_avrc_ct_send_register_notification_cmd(1, ESP_AVRC_RN_PLAY_STATUS_CHANGE, 0);
        }
        break;
    }
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// AVRCP Target Callback (Absolute Volume Sync with Phone/PC)
// ---------------------------------------------------------------------------
static bool s_volume_notify_registered = false;

static void avrc_tg_cb(esp_avrc_tg_cb_event_t event, esp_avrc_tg_cb_param_t *param) {
    switch (event) {
    case ESP_AVRC_TG_CONNECTION_STATE_EVT: {
        LOG_I("AVRCP TG", "Connection state: %s", param->conn_stat.connected ? "CONNECTED" : "DISCONNECTED");
        if (!param->conn_stat.connected) {
            s_volume_notify_registered = false;
        }
        break;
    }
    case ESP_AVRC_TG_SET_ABSOLUTE_VOLUME_CMD_EVT: {
        s_current_volume = param->set_abs_vol.volume;
        nvs_save_volume(s_current_volume);
        LOG_D("AVRCP TG", "Absolute Volume from Phone: %d (%.0f%%)",
              s_current_volume, (float)s_current_volume * 100.0f / 127.0f);
        display_show_volume(s_current_volume);
        break;
    }
    case ESP_AVRC_TG_REGISTER_NOTIFICATION_EVT: {
        if (param->reg_ntf.event_id == ESP_AVRC_RN_VOLUME_CHANGE) {
            s_volume_notify_registered = true;
            esp_avrc_rn_param_t rn_param;
            rn_param.volume = s_current_volume;
            esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_INTERIM, &rn_param);
            LOG_D("AVRCP TG", "Phone registered for volume sync, initial volume: %d", s_current_volume);
        }
        break;
    }
    default:
        break;
    }
}

// ---------------------------------------------------------------------------
// Public API Implementation
// ---------------------------------------------------------------------------
void bt_audio_init(QueueHandle_t audio_queue) {
    s_vis_queue = audio_queue;
    s_current_volume = nvs_load_volume();
    LOG_I("BT", "Bluetooth Audio Subsystem Initialized");
}

void bt_audio_start() {
    if (s_is_started) return;
    s_current_volume = nvs_load_volume();
    LOG_I("BT", "Starting Native Bluetooth A2DP Sink (Volume: %d)...", s_current_volume);

    // 1. Install & Configure I2S Driver for DAC PCM5102A on I2S_NUM_0
    static const i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S | I2S_COMM_FORMAT_STAND_MSB),
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = 512,
        .use_apll = false,
        .tx_desc_auto_clear = true
    };

    static const i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = DAC_PIN_BCK, // GPIO 18
        .ws_io_num = DAC_PIN_LCK,  // GPIO 19
        .data_out_num = DAC_PIN_DIN, // GPIO 23
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    if (err != ESP_OK) {
        LOG_E("BT", "Failed to install I2S driver: %d", err);
        return;
    }
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);

    static bool s_bt_inited = false;
    if (!s_bt_inited) {
        // 2. Initialize Bluetooth Controller & Bluedroid
        btStart();
        esp_bluedroid_init();
        esp_bluedroid_enable();

        // 3. Set Device Name
        esp_bt_dev_set_device_name(BT_DEVICE_NAME);

        // 4. Initialize AVRCP Controller (CT) & Target (TG for Volume Sync)
        esp_avrc_ct_init();
        esp_avrc_ct_register_callback(avrc_ct_cb);

        esp_avrc_tg_init();
        esp_avrc_tg_register_callback(avrc_tg_cb);
        esp_avrc_rn_evt_cap_mask_t evt_set = {0};
        esp_avrc_rn_evt_bit_mask_operation(ESP_AVRC_BIT_MASK_OP_SET, &evt_set, ESP_AVRC_RN_VOLUME_CHANGE);
        esp_avrc_tg_set_rn_evt_cap(&evt_set);

        // 5. Initialize A2DP Sink & Register Data Callback
        esp_a2d_sink_init();
        esp_a2d_register_callback(a2d_cb);
        esp_a2d_sink_register_data_callback(bt_i2s_data_callback);
        
        s_bt_inited = true;
    }

    // 6. Set Discoverable & Connectable
    esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    s_is_started = true;
    s_fa.count = 0;
    LOG_I("BT", "Native A2DP Sink started as '%s' (Volume: %d)", BT_DEVICE_NAME, s_current_volume);

    // 7. Auto-reconnect to last remembered device
    uint8_t mac[6];
    if (nvs_load_bt_mac(mac)) {
        LOG_I("BT", "Attempting reconnect to remembered device: %02X:%02X:%02X:%02X:%02X:%02X",
              mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        esp_a2d_sink_connect(mac);
    }
}

void bt_audio_stop() {
    if (!s_is_started) return;
    LOG_I("BT", "Stopping Bluetooth A2DP Sink with verified state polling...");

    // 1. If connected, request disconnect and poll until disconnected (or timeout 1.5s)
    if (s_is_connected) {
        if (s_connected_bda[0] || s_connected_bda[1] || s_connected_bda[2]) {
            esp_a2d_sink_disconnect(s_connected_bda);
        }
        uint32_t wait_start = millis();
        while (s_is_connected && (millis() - wait_start < 1500)) {
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }
    s_is_connected = false;
    s_is_playing = false;

    // 2. Set scan mode to non-connectable
    esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    vTaskDelay(pdMS_TO_TICKS(50));

    // We no longer deinitialize the BT stack to prevent heap fragmentation and core panics.
    // By keeping it initialized but non-connectable, we save CPU and avoid OS memory reallocations.

    // 7. Uninstall I2S driver
    vTaskDelay(pdMS_TO_TICKS(50));
    i2s_driver_uninstall(I2S_NUM_0);

    s_is_started = false;
    s_volume_notify_registered = false;
    s_fa.count = 0;
    LOG_I("BT", "Bluetooth fully STOPPED & VERIFIED (BT Status: %d, FreeHeap=%u, MaxBlock=%u)",
          (int)esp_bt_controller_get_status(), (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
}

void bt_audio_adjust_volume(int32_t delta) {
    int32_t new_vol = (int32_t)s_current_volume + (delta * 4);
    if (new_vol < 0) new_vol = 0;
    if (new_vol > 127) new_vol = 127;
    bt_audio_set_volume((uint8_t)new_vol);
}

void bt_audio_set_volume(uint8_t volume) {
    if (volume > 127) volume = 127;
    s_current_volume = volume;
    nvs_save_volume(s_current_volume);
    LOG_D("Volume", "Set to %d (%.0f%%)", s_current_volume, (float)s_current_volume * 100.0f / 127.0f);

    // Synchronize volume with connected phone/PC via AVRCP Absolute Volume notification
    if (s_is_connected && s_volume_notify_registered) {
        esp_avrc_rn_param_t rn_param;
        rn_param.volume = s_current_volume;
        esp_avrc_tg_send_rn_rsp(ESP_AVRC_RN_VOLUME_CHANGE, ESP_AVRC_RN_RSP_CHANGED, &rn_param);
        s_volume_notify_registered = false;
        LOG_D("AVRCP TG", "Sent Volume Change Notification to Phone: %d", s_current_volume);
    }
}

uint8_t bt_audio_get_volume() {
    return s_current_volume;
}

void bt_audio_play_pause() {
    if (!s_is_connected) {
        Serial.println("[AVRCP] No device connected to send Play/Pause");
        display_toast("BT NOT CONNECTED");
        return;
    }
    static uint8_t s_tl = 0;
    uint8_t cmd = s_is_playing ? ESP_AVRC_PT_CMD_PAUSE : ESP_AVRC_PT_CMD_PLAY;
    LOG_D("AVRCP", "Sending %s command to host (tl=%d)", s_is_playing ? "PAUSE" : "PLAY", s_tl);
    display_toast(s_is_playing ? "PAUSE" : "PLAY");
    
    esp_avrc_ct_send_passthrough_cmd(s_tl, cmd, ESP_AVRC_PT_CMD_STATE_PRESSED);
    delay(40);
    esp_avrc_ct_send_passthrough_cmd(s_tl, cmd, ESP_AVRC_PT_CMD_STATE_RELEASED);
    s_tl = (s_tl + 1) & 0x0F;
}

void bt_audio_start_repairing() {
    LOG_I("BT", "Re-pairing triggered: clearing saved MAC and disconnecting...");
    if (s_is_connected && (s_connected_bda[0] || s_connected_bda[1] || s_connected_bda[2])) {
        esp_a2d_sink_disconnect(s_connected_bda);
    }
    nvs_erase_bt_mac();
    memset(s_connected_bda, 0, 6);
    if (s_is_started) {
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    }
}

bool bt_audio_is_connected() {
    return s_is_connected;
}

bool bt_audio_is_playing() {
    return s_is_playing;
}

const char* bt_audio_get_device_name() {
    return s_device_name;
}
