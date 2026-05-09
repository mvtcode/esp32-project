/**
 * button_manager.cpp — Button Debounce & Event Dispatcher
 * IoT Voice Command System
 */
#include "button_manager.h"
#include "../display/ui_manager.h"
#include "../hardware_config.h"
#include <Arduino.h>

struct ButtonState {
    uint8_t pin;
    UIButtonEvent event;
    bool last_reading;
    bool stable_state;
    uint32_t last_debounce_time;
};

#define DEBOUNCE_DELAY_MS 50

static ButtonState g_buttons[] = {
    { BTN_UP,    BTN_EVENT_UP,    HIGH, HIGH, 0 },
    { BTN_DOWN,  BTN_EVENT_DOWN,  HIGH, HIGH, 0 },
    { BTN_ENTER, BTN_EVENT_ENTER, HIGH, HIGH, 0 },
    { BTN_BACK,  BTN_EVENT_BACK,  HIGH, HIGH, 0 },
    { BTN_WAKE,  BTN_EVENT_BOOT,  HIGH, HIGH, 0 }
};

static const int NUM_BUTTONS = sizeof(g_buttons) / sizeof(g_buttons[0]);

void button_manager_init() {
    for (int i = 0; i < NUM_BUTTONS; i++) {
        pinMode(g_buttons[i].pin, INPUT_PULLUP);
        g_buttons[i].last_reading = HIGH;
        g_buttons[i].stable_state = HIGH;
    }
}

void button_manager_loop() {
    uint32_t current_time = millis();

    for (int i = 0; i < NUM_BUTTONS; i++) {
        bool reading = digitalRead(g_buttons[i].pin);

        if (reading != g_buttons[i].last_reading) {
            g_buttons[i].last_debounce_time = current_time;
        }

        if ((current_time - g_buttons[i].last_debounce_time) > DEBOUNCE_DELAY_MS) {
            if (reading != g_buttons[i].stable_state) {
                g_buttons[i].stable_state = reading;
                
                // Button is pressed (LOW active)
                if (g_buttons[i].stable_state == LOW) {
                    ui_manager_handle_button(g_buttons[i].event);
                }
            }
        }

        g_buttons[i].last_reading = reading;
    }
}
