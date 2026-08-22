#include "button.h"
#include "log.h"

struct ButtonState {
    uint8_t           gpio;
    volatile bool     is_down;
    volatile bool     long_1s_triggered;
    volatile bool     long_3s_triggered;
    volatile bool     short_flag;
    volatile bool     long_1s_flag;
    volatile bool     long_3s_flag;
    volatile uint32_t down_ts;
    volatile uint32_t release_ts;
};

static ButtonState s_buttons[BTN_COUNT] = {
    { PIN_BTN_PUSH, false, false, false, false, false, false, 0, 0 },
    { PIN_BTN_BACK, false, false, false, false, false, false, 0, 0 },
    { PIN_BTN_PLUS, false, false, false, false, false, false, 0, 0 },
    { PIN_BTN_BOOT, false, false, false, false, false, false, 0, 0 }
};

// Hardware Interrupt Handlers running in IRAM (Immediate Microsecond Response)
static inline void IRAM_ATTR isr_handle_button(BtnId btn) {
    ButtonState &b = s_buttons[btn];
    bool cur_down = (digitalRead(b.gpio) == LOW);
    uint32_t now = millis();

    if (cur_down && !b.is_down) {
        // Press edge detected immediately by hardware interrupt
        if (now - b.release_ts >= BTN_DEBOUNCE_MS) {
            b.is_down = true;
            b.down_ts = now;
            b.long_1s_triggered = false;
            b.long_3s_triggered = false;
        }
    } else if (!cur_down && b.is_down) {
        // Release edge detected immediately by hardware interrupt
        if (now - b.down_ts >= BTN_DEBOUNCE_MS) {
            b.is_down = false;
            b.release_ts = now;

            // Trigger short press only if no long press was activated
            if (!b.long_1s_triggered && !b.long_3s_triggered) {
                b.short_flag = true;
            }
        }
    }
}

static void IRAM_ATTR isr_btn_push() { isr_handle_button(BTN_PUSH); }
static void IRAM_ATTR isr_btn_back() { isr_handle_button(BTN_BACK); }
static void IRAM_ATTR isr_btn_plus() { isr_handle_button(BTN_PLUS); }
static void IRAM_ATTR isr_btn_boot() { isr_handle_button(BTN_BOOT); }

void buttons_init() {
    for (int i = 0; i < BTN_COUNT; i++) {
        pinMode(s_buttons[i].gpio, INPUT_PULLUP);
        s_buttons[i].is_down = (digitalRead(s_buttons[i].gpio) == LOW);
        s_buttons[i].long_1s_triggered = false;
        s_buttons[i].long_3s_triggered = false;
        s_buttons[i].short_flag = false;
        s_buttons[i].long_1s_flag = false;
        s_buttons[i].long_3s_flag = false;
        s_buttons[i].down_ts = 0;
        s_buttons[i].release_ts = 0;
    }

    // Attach Hardware GPIO Interrupts with CHANGE mode (triggers on both press and release)
    attachInterrupt(digitalPinToInterrupt(PIN_BTN_PUSH), isr_btn_push, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_BTN_BACK), isr_btn_back, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_BTN_PLUS), isr_btn_plus, CHANGE);
    attachInterrupt(digitalPinToInterrupt(PIN_BTN_BOOT), isr_btn_boot, CHANGE);

    LOG_I("BTN", "Hardware GPIO Interrupts attached (PUSH:GPIO%d BACK:GPIO%d PLUS/CON:GPIO%d BOOT:GPIO%d)",
                  PIN_BTN_PUSH, PIN_BTN_BACK, PIN_BTN_PLUS, PIN_BTN_BOOT);
}

void buttons_update() {
    uint32_t now = millis();

    for (int i = 0; i < BTN_COUNT; i++) {
        ButtonState &b = s_buttons[i];

        // Backup polling sync in case of glitch
        bool cur_down = (digitalRead(b.gpio) == LOW);
        if (cur_down != b.is_down) {
            isr_handle_button((BtnId)i);
        }

        // Live check while holding (triggers at 600ms and 3000ms thresholds)
        if (b.is_down) {
            uint32_t hold_dur = now - b.down_ts;

            if (!b.long_1s_triggered && hold_dur >= 600) {
                b.long_1s_triggered = true;
                b.long_1s_flag = true;
            }

            if (!b.long_3s_triggered && hold_dur >= 3000) {
                b.long_3s_triggered = true;
                b.long_3s_flag = true;
            }
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
    bool res = s_buttons[btn].long_1s_flag;
    s_buttons[btn].long_1s_flag = false;
    return res;
}

bool button_held_3s(BtnId btn) {
    if (btn >= BTN_COUNT) return false;
    bool res = s_buttons[btn].long_3s_flag;
    s_buttons[btn].long_3s_flag = false;
    return res;
}

bool button_is_down(BtnId btn) {
    if (btn >= BTN_COUNT) return false;
    return s_buttons[btn].is_down;
}

