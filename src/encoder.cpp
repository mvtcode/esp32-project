#include "encoder.h"

static volatile int32_t s_encoder_delta = 0;
static volatile uint8_t s_prev_state = 0;
static portMUX_TYPE s_encoder_mux = portMUX_INITIALIZER_UNLOCKED;

// State transition lookup table for quadrature decoding
static const int8_t ENC_STATES[] = {
    0, -1,  1,  0,
    1,  0,  0, -1,
   -1,  0,  0,  1,
    0,  1, -1,  0
};

static void IRAM_ATTR encoder_isr() {
    uint8_t a = digitalRead(ENCODER_PIN_CLK);
    uint8_t b = digitalRead(ENCODER_PIN_DT);
    uint8_t curr = (a << 1) | b;

    uint8_t idx = (s_prev_state << 2) | curr;
    int8_t step = ENC_STATES[idx & 0x0F];

    if (step != 0) {
        portENTER_CRITICAL_ISR(&s_encoder_mux);
        s_encoder_delta += step;
        portEXIT_CRITICAL_ISR(&s_encoder_mux);
        s_prev_state = curr;
    }
}

void encoder_init() {
    pinMode(ENCODER_PIN_CLK, INPUT_PULLUP);
    pinMode(ENCODER_PIN_DT, INPUT_PULLUP);

    uint8_t a = digitalRead(ENCODER_PIN_CLK);
    uint8_t b = digitalRead(ENCODER_PIN_DT);
    s_prev_state = (a << 1) | b;

    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_CLK), encoder_isr, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_DT), encoder_isr, CHANGE);
    Serial.printf("[ENC] EC11 Rotary Encoder initialized on CLK: GPIO%d, DT: GPIO%d\n",
                  ENCODER_PIN_CLK, ENCODER_PIN_DT);
}

int32_t encoder_get_delta() {
    portENTER_CRITICAL(&s_encoder_mux);
    int32_t raw = s_encoder_delta;
    // Standard EC11 detent has 2 transitions per notch
    int32_t steps = raw / 2;
    if (steps != 0) {
        s_encoder_delta -= steps * 2;
    }
    portEXIT_CRITICAL(&s_encoder_mux);
    return steps;
}
