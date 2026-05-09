/**
 * main.cpp — Application entry point & orchestration
 * IoT Voice Command System — ESP32-S3-N16R8
 */
#include <Arduino.h>
#include "hardware_config.h"
#include "display/display_driver.h"
#include "display/ui_manager.h"
#include "control/button_manager.h"
#include "audio/i2s_mic.h"
#include "voice/voice_engine.h"
#include "control/relay_controller.h"

// ─── Shared state ────────────────────────────────────────────────────────────

// ─── Voice command callback ──────────────────────────────────────────────────

static void on_voice_command(int cmd_id, const char* cmd_name) {
    if (cmd_id == VOICE_CMD_WAKE) {
        ui_manager_handle_button(BTN_EVENT_BOOT);
        Serial.println("[CMD] System Waked by Voice");
        return;
    }

    int relay_idx = -1;
    bool is_on = false;

    if (cmd_id >= VOICE_CMD_ON_BASE && cmd_id < VOICE_CMD_ON_BASE + VOICE_CMD_COUNT) {
        relay_idx = cmd_id - VOICE_CMD_ON_BASE;
        is_on = true;
    } else if (cmd_id >= VOICE_CMD_OFF_BASE && cmd_id < VOICE_CMD_OFF_BASE + VOICE_CMD_COUNT) {
        relay_idx = cmd_id - VOICE_CMD_OFF_BASE;
        is_on = false;
    }

    if (relay_idx != -1) {
        relay_set(relay_idx, is_on);
        
        // Construct display name: "ALIAS ON/OFF"
        char buf[64];
        snprintf(buf, sizeof(buf), "%s %s", relay_get_alias(relay_idx), is_on ? "ON" : "OFF");
        ui_manager_set_command(buf);
        Serial.printf("[CMD] id=%d  display=%s\n", cmd_id, buf);
    } else if (cmd_id == -1) {
        ui_manager_set_command("NO MATCH");
    } else {
        ui_manager_set_command(cmd_name);
    }
}

// ─── FreeRTOS Tasks ───────────────────────────────────────────────────────────

static void ui_update_task(void* arg) {
    while (true) {
        button_manager_loop();
        if (display_is_ok()) {
            ui_manager_loop();
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ─── Setup ───────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(115200);
    delay(1000); // Đợi hardware UART ổn định
    
    Serial.println("\n======================================");
    Serial.println("[SYS] ESP32-S3 Voice Project Booting");
    Serial.println("======================================");
    Serial.flush();

    // 1. Status LED
    pinMode(LED_STATUS, OUTPUT);
    digitalWrite(LED_STATUS, HIGH);
    Serial.println("[SYS] LED initialized");
    Serial.flush();

    // 2. Buttons
    button_manager_init();
    Serial.println("[SYS] Buttons initialized");
    Serial.flush();

    // 3. Display
    Serial.println("[SYS] Initializing display...");
    Serial.flush();
    display_init();
    
    if (display_is_ok()) {
        ui_manager_init();
    } else {
        Serial.println("[SYS] WARNING: Display init failed or not found");
    }

    // 4. Relay controller
    Serial.println("[SYS] Initializing relays...");
    Serial.flush();
    relay_init();

    // 5. Voice engine
    Serial.println("[SYS] Initializing voice engine...");
    Serial.flush();
    voice_engine_set_callback(on_voice_command);
    if (!voice_engine_init()) {
        Serial.println("[SYS] ERROR: voice_engine_init failed");
    }

    Serial.println("[SYS] Hardware initialization done");
    Serial.flush();

    // 6. Tạo FreeRTOS tasks
    Serial.println("[SYS] Starting tasks...");
    Serial.flush();

    xTaskCreatePinnedToCore(
        voice_engine_task,
        "voice_task",
        VOICE_TASK_STACK,
        nullptr,
        VOICE_TASK_PRIO,
        nullptr,
        VOICE_TASK_CORE
    );

    xTaskCreatePinnedToCore(
        ui_update_task,
        "ui_task",
        UI_TASK_STACK,
        nullptr,
        UI_TASK_PRIO,
        nullptr,
        UI_TASK_CORE
    );

    Serial.println("[SYS] Setup complete");
    Serial.flush();
}

void loop() {
    // Everything is handled in FreeRTOS tasks.
    vTaskDelay(pdMS_TO_TICKS(1000));
}