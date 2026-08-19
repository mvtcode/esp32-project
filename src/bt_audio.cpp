#include "bt_audio.h"
#include "nvs_storage.h"
#include "display.h"
#include "driver/i2s.h"
#include "esp_gap_bt_api.h"

static BluetoothA2DPSink s_a2dp_sink;
static QueueHandle_t     s_vis_queue = nullptr;
static bool              s_is_connected = false;
static bool              s_is_playing = false;
static bool              s_is_started = false;
static uint8_t           s_current_volume = 80;
static char              s_device_name[64] = "None";

// Frame accumulator for Visualizer (downmix/subsample 44.1kHz stereo to 128-sample visualizer frames)
struct FrameAccumulator {
    int32_t left[FRAME_SIZE];
    int32_t right[FRAME_SIZE];
    size_t count = 0;
};
static FrameAccumulator s_fa;

static void bt_stream_reader(const uint8_t *data, uint32_t len) {
    if (!s_vis_queue || len == 0) return;

    // Data format: signed 16-bit PCM stereo [L0, R0, L1, R1, ...]
    const int16_t *samples = (const int16_t *)data;
    size_t num_pairs = len / (2 * sizeof(int16_t));

    for (size_t i = 0; i < num_pairs; i++) {
        s_fa.left[s_fa.count]  = (int32_t)samples[i * 2];
        s_fa.right[s_fa.count] = (int32_t)samples[i * 2 + 1];
        s_fa.count++;

        if (s_fa.count >= FRAME_SIZE) {
            AudioFrame frame;
            memcpy(frame.left, s_fa.left, sizeof(s_fa.left));
            memcpy(frame.right, s_fa.right, sizeof(s_fa.right));
            // Send to queue without blocking audio playback
            xQueueSend(s_vis_queue, &frame, 0);
            s_fa.count = 0;
        }
    }
}

static void avrc_volume_change_cb(int volume) {
    s_current_volume = (uint8_t)volume;
    nvs_save_volume(s_current_volume);
    Serial.printf("[AVRCP] Volume synced from device: %d\n", volume);
}

static void avrc_playstatus_cb(esp_avrc_playback_stat_t playback) {
    s_is_playing = (playback == ESP_AVRC_PLAYBACK_PLAYING);
    Serial.printf("[AVRCP] Playback status: %s\n", s_is_playing ? "PLAYING" : "PAUSED/STOPPED");
}

static void on_connection_state_changed(esp_a2d_connection_state_t state, void * /*obj*/) {
    if (state == ESP_A2D_CONNECTION_STATE_CONNECTED) {
        s_is_connected = true;
        Serial.println("[BT] A2DP Connected!");
        display_toast("BT CONNECTED!");
        esp_bd_addr_t *bda = s_a2dp_sink.get_current_peer_address();
        if (bda) {
            nvs_save_bt_mac(*bda);
        }
    } else if (state == ESP_A2D_CONNECTION_STATE_DISCONNECTED) {
        s_is_connected = false;
        s_is_playing = false;
        Serial.println("[BT] A2DP Disconnected");
        display_toast("BT DISCONNECTED");
        // Re-enable discoverability so devices can reconnect immediately
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
    }
}

void bt_audio_init(QueueHandle_t audio_queue) {
    s_vis_queue = audio_queue;
    s_current_volume = nvs_load_volume();

    // Configure DAC PCM5102A I2S pins (fault-tolerant)
    i2s_pin_config_t pin_config = {
        .mck_io_num = I2S_PIN_NO_CHANGE,
        .bck_io_num = DAC_PIN_BCK,
        .ws_io_num = DAC_PIN_LCK,
        .data_out_num = DAC_PIN_DIN,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    s_a2dp_sink.set_pin_config(pin_config);
    s_a2dp_sink.set_i2s_port(I2S_NUM_0); // Output to I2S_NUM_0 when in BT mode
    s_a2dp_sink.set_stream_reader(bt_stream_reader, true);
    s_a2dp_sink.set_avrc_rn_volumechange(avrc_volume_change_cb);
    s_a2dp_sink.set_avrc_rn_playstatus_callback(avrc_playstatus_cb);
    s_a2dp_sink.set_on_connection_state_changed(on_connection_state_changed);
    s_a2dp_sink.set_auto_reconnect(false); // Don't lock into reconnect loop; allow any device to discover & pair

    Serial.println("[BT] A2DP Sink configured (Name: " BT_DEVICE_NAME ")");
}

void bt_audio_start() {
    if (s_is_started) return;
    Serial.println("[BT] Starting A2DP Sink...");
    s_a2dp_sink.start(BT_DEVICE_NAME);
    s_a2dp_sink.set_volume(s_current_volume);

    delay(100); // Allow Bluedroid stack event loop to settle

    // Explicitly set device name on GAP layer
    esp_bt_gap_set_device_name(BT_DEVICE_NAME);

    // Set Class of Device (CoD) to Audio/Video Loudspeaker
    esp_bt_cod_t cod = {0};
    cod.major = 0x04;    // Audio/Video device class
    cod.minor = 0x04;    // Wearable Headset / Audio device (universally supported by iOS & Windows)
    cod.service = 0x20;  // Audio rendering service
    esp_err_t err_cod = esp_bt_gap_set_cod(cod, ESP_BT_SET_COD_ALL);

    // Ensure general discoverability and connectability
    esp_err_t err_scan = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);

    s_is_started = true;
    Serial.printf("[BT] GAP Status -> COD: %s | ScanMode: %s\n", esp_err_to_name(err_cod), esp_err_to_name(err_scan));
    Serial.printf("[BT] A2DP Sink active & broadcasting as '%s' (Volume: %d)\n", BT_DEVICE_NAME, s_current_volume);
}

void bt_audio_stop() {
    if (!s_is_started) return;
    Serial.println("[BT] Stopping A2DP Sink...");
    s_a2dp_sink.end(false); // Don't release memory completely to allow re-start
    s_is_started = false;
    s_is_connected = false;
    s_is_playing = false;
    Serial.println("[BT] A2DP Sink stopped");
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
    s_a2dp_sink.set_volume(s_current_volume);
    nvs_save_volume(s_current_volume);
    Serial.printf("[AVRCP] Volume set to %d (%.0f%%)\n", s_current_volume, (float)s_current_volume * 100.0f / 127.0f);
}

uint8_t bt_audio_get_volume() {
    return s_current_volume;
}

void bt_audio_play_pause() {
    if (!s_is_connected) {
        Serial.println("[AVRCP] Cannot toggle Play/Pause - no device connected");
        return;
    }
    if (s_is_playing) {
        Serial.println("[AVRCP] Sending PAUSE command");
        s_a2dp_sink.pause();
    } else {
        Serial.println("[AVRCP] Sending PLAY command");
        s_a2dp_sink.play();
    }
}

void bt_audio_start_repairing() {
    Serial.println("[BT] Starting Re-pairing mode: clearing MAC and disconnecting...");
    nvs_erase_bt_mac();
    if (s_is_started) {
        s_a2dp_sink.clean_last_connection();
        s_a2dp_sink.disconnect();
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
