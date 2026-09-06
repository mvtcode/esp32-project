#pragma once

#include <Arduino.h>
#include "pin_config.h"

#define TFT_BL_PIN       PIN_TFT_BL
#define LEDC_BL_CHANNEL  0
#define LEDC_BL_FREQ     5000
#define LEDC_BL_RES      8


class BacklightManager {
public:
    static void init();
    static void setBrightness(uint8_t percent); // 0 - 100%
    static uint8_t getBrightness();
    
    // Gọi khi có tương tác người dùng (chạm màn hình)
    static void feedActivity();
    
    // Cập nhật chu kỳ định kỳ (kiểm tra sleep timeout)
    static void update();
    
    static bool isScreenSleeping();
    static void wakeUp();

private:
    static uint8_t targetBrightness;
    static uint8_t currentBrightness;
    static unsigned long lastActivityTime;
    static bool isSleeping;
    static void applyDuty(uint8_t duty);
};

