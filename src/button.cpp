#include "button.h"
#include "log.h"

struct ButtonState {
    uint8_t  gpio;
    uint32_t long_ms;
    bool     is_down;
    bool     long_triggered;
    bool     short_flag;
    bool     long_flag;
    uint32_t down_ts;
    uint32_t release_ts;
};

static ButtonState s_buttons[BTN_COUNT] = {
    { PIN_BTN_PUSH, 1000, false, false, false, false, 0, 0 },
    { PIN_BTN_BACK, 1000, false, false, false, false, 0, 0 },
    { PIN_BTN_PLUS, 1000, false, false, false, false, 0, 0 },
    { PIN_BTN_BOOT, 1000, false, false, false, false, 0, 0 }  // BOOT long press is 1s
};

void buttons_init() {
    for (int i = 0; i < BTN_COUNT; i++) {
        pinMode(s_buttons[i].gpio, INPUT_PULLUP);
        s_buttons[i].is_down = (digitalRead(s_buttons[i].gpio) == LOW);
        s_buttons[i].long_triggered = false;
        s_buttons[i].short_flag = false;
        s_buttons[i].long_flag = false;
        s_buttons[i].down_ts = 0;
        s_buttons[i].release_ts = 0;
    }
    LOG_I("BTN", "Multi-buttons initialized (PUSH:GPIO%d BACK:GPIO%d PLUS:GPIO%d BOOT:GPIO%d)",
                  PIN_BTN_PUSH, PIN_BTN_BACK, PIN_BTN_PLUS, PIN_BTN_BOOT);
}

void buttons_update() {
    uint32_t now = millis();

    for (int i = 0; i < BTN_COUNT; i++) {
        ButtonState &b = s_buttons[i];
        bool cur_down = (digitalRead(b.gpio) == LOW);

        if (cur_down && !b.is_down) {
            // Check debounce after last release
            if (now - b.release_ts >= BTN_DEBOUNCE_MS) {
                b.is_down = true;
                b.down_ts = now;
                b.long_triggered = false;
            }
        } else if (!cur_down && b.is_down) {
            // Check debounce after press
            if (now - b.down_ts >= BTN_DEBOUNCE_MS) {
                b.is_down = false;
                b.release_ts = now;
                if (!b.long_triggered) {
                    b.short_flag = true;
                }
            }
        }

        // Long press check while holding down
        if (b.is_down && !b.long_triggered && (now - b.down_ts >= b.long_ms)) {
            b.long_triggered = true;
            b.long_flag = true;
        }
    }
}

bool button_pressed(BtnId btn) {
    if (btn >= BTN_COUNT) return false;
    bool res = s_buttons[btn].short_flag;
    s_buttons[btn].short_flag = false;
    return res;
}

bool button_long_pressed(BtnId btn) {
    if (btn >= BTN_COUNT) return false;
    bool res = s_buttons[btn].long_flag;
    s_buttons[btn].long_flag = false;
    return res;
}

bool button_is_down(BtnId btn) {
    if (btn >= BTN_COUNT) return false;
    return s_buttons[btn].is_down;
}

