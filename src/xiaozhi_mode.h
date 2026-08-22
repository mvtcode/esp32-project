#pragma once
#include <Arduino.h>

// Activation sub-states (Screen A)
enum XiaozhiActState {
    XZ_ACT_IDLE      = 0,  // Chưa bắt đầu
    XZ_ACT_WIFI_WAIT = 1,  // Đang đợi WiFi hoặc đang ở AP Portal
    XZ_ACT_FETCH     = 2,  // Đang gọi HTTP /check-version
    XZ_ACT_WAIT      = 3,  // Đang hiển thị code 6 số, chờ user xác nhận
    XZ_ACT_DONE      = 4,  // Activation thành công
    XZ_ACT_ERROR     = 5,  // Lỗi kết nối
};

// Chat sub-states (Screen B)
enum XiaozhiState {
    XZ_IDLE       = 0,
    XZ_CONNECTING = 1,
    XZ_LISTENING  = 2,
    XZ_SPEAKING   = 3,
    XZ_ERROR      = 4,
};

// Screen selector
enum XiaozhiScreen {
    XZ_SCREEN_ACTIVATION = 0,  // Screen A: 6-digit code / activation
    XZ_SCREEN_CHAT       = 1,  // Screen B: eye animation / chat
};

// Lifecycle
void         xiaozhi_init();
void         xiaozhi_start(QueueHandle_t audio_queue = nullptr);
void         xiaozhi_stop();
void         xiaozhi_loop();

// Chat controls
void         xiaozhi_start_listen();
void         xiaozhi_stop_listen();
void         xiaozhi_abort_speaking();
void         xiaozhi_next_face_theme();
bool         xiaozhi_is_auto_mode();
void         xiaozhi_set_auto_mode(bool is_auto);
void         xiaozhi_adjust_volume(int32_t delta);
uint8_t      xiaozhi_get_volume();

// Query for display
XiaozhiScreen   xiaozhi_get_screen();
XiaozhiActState xiaozhi_get_act_state();
const char*     xiaozhi_get_act_code();        // e.g. "123456"
const char*     xiaozhi_get_act_message();     // Server message string
int             xiaozhi_get_act_timeout_sec(); // Countdown remaining seconds
XiaozhiState    xiaozhi_get_state();
float           xiaozhi_get_tts_energy();      // 0.0-1.0
const char*     xiaozhi_get_emotion();
