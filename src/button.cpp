#include "button.h"

static uint8_t  s_gpio;
static bool     s_prev_state = HIGH; // Released = HIGH (active LOW)
static uint32_t s_last_ts    = 0;

void button_init(uint8_t gpio) {
    s_gpio = gpio;
    pinMode(gpio, INPUT_PULLUP);
    s_prev_state = (bool)digitalRead(gpio);
    Serial.printf("[BTN] Init on GPIO%d (BOOT button)\n", gpio);
}

bool button_pressed() {
    bool cur = (bool)digitalRead(s_gpio);
    uint32_t now = millis();

    // Detect falling edge (HIGH→LOW = button pressed) with debounce
    if (cur == LOW && s_prev_state == HIGH && (now - s_last_ts) > BTN_DEBOUNCE_MS) {
        s_prev_state = LOW;
        s_last_ts    = now;
        return true;
    }

    // Reset state when released
    if (cur == HIGH) {
        s_prev_state = HIGH;
    }

    return false;
}
