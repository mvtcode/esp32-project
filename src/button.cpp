#include "button.h"

static uint8_t  s_gpio;
static bool     s_prev_state = HIGH; // Released = HIGH (active LOW)
static uint32_t s_last_ts    = 0;

static bool s_short_flag = false;
static bool s_long_flag = false;
static bool s_is_down = false;
static bool s_long_triggered = false;
static uint32_t s_down_ts = 0;

void button_init(uint8_t gpio) {
    s_gpio = gpio;
    pinMode(gpio, INPUT_PULLUP);
    s_is_down = (digitalRead(gpio) == LOW);
    Serial.printf("[BTN] Init on GPIO%d (BOOT button)\n", gpio);
}

void button_update() {
    bool cur_down = (digitalRead(s_gpio) == LOW);
    uint32_t now = millis();

    if (cur_down && !s_is_down) {
        if (now - s_down_ts > BTN_DEBOUNCE_MS) { // Debounce press
            s_is_down = true;
            s_down_ts = now;
            s_long_triggered = false;
        }
    } else if (!cur_down && s_is_down) {
        if (now - s_down_ts > 50) { // Debounce release
            s_is_down = false;
            if (!s_long_triggered) {
                s_short_flag = true; // Was released before long press triggered
            }
        }
    }

    if (s_is_down && !s_long_triggered && (now - s_down_ts >= 1000)) {
        s_long_triggered = true;
        s_long_flag = true;
    }
}

bool button_pressed() {
    bool res = s_short_flag;
    s_short_flag = false;
    return res;
}

bool button_long_pressed() {
    bool res = s_long_flag;
    s_long_flag = false;
    return res;
}
