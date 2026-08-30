#include "backlight_manager.h"
#include "config_manager.h"

uint8_t BacklightManager::targetBrightness = 80;
uint8_t BacklightManager::currentBrightness = 80;
unsigned long BacklightManager::lastActivityTime = 0;
bool BacklightManager::isSleeping = false;

void BacklightManager::init() {
    ledcSetup(LEDC_BL_CHANNEL, LEDC_BL_FREQ, LEDC_BL_RES);
    ledcAttachPin(TFT_BL_PIN, LEDC_BL_CHANNEL);

    targetBrightness = ConfigManager::getBrightness();
    currentBrightness = targetBrightness;
    lastActivityTime = millis();
    isSleeping = false;

    applyDuty((uint8_t)((currentBrightness * 255) / 100));
}

void BacklightManager::applyDuty(uint8_t duty) {
    ledcWrite(LEDC_BL_CHANNEL, duty);
}

void BacklightManager::setBrightness(uint8_t percent) {
    if (percent > 100) percent = 100;
    if (percent < 10) percent = 10; // Đảm bảo không bị tắt hẳn khi chỉnh tay

    targetBrightness = percent;
    ConfigManager::setBrightness(percent);

    if (!isSleeping) {
        currentBrightness = targetBrightness;
        applyDuty((uint8_t)((currentBrightness * 255) / 100));
    }
}

uint8_t BacklightManager::getBrightness() {
    return targetBrightness;
}

void BacklightManager::feedActivity() {
    lastActivityTime = millis();
    if (isSleeping) {
        wakeUp();
    }
}

void BacklightManager::wakeUp() {
    isSleeping = false;
    currentBrightness = targetBrightness;
    applyDuty((uint8_t)((currentBrightness * 255) / 100));
    lastActivityTime = millis();
}

bool BacklightManager::isScreenSleeping() {
    return isSleeping;
}

void BacklightManager::update() {
    int timeoutSec = ConfigManager::getSleepTimeoutSeconds();
    if (timeoutSec <= 0) return; // 0 = Không bao giờ tắt màn hình

    if (!isSleeping && (millis() - lastActivityTime >= (unsigned long)timeoutSec * 1000)) {
        // Tắt đèn nền chuyển sang chế độ ngủ
        isSleeping = true;
        applyDuty(0);
    }
}
