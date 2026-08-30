#include "system_telemetry.h"
#include <esp_system.h>
#include <esp_heap_caps.h>

unsigned long SystemTelemetry::frameCount = 0;
int64_t SystemTelemetry::lastCalcTimeUs = 0;
int64_t SystemTelemetry::totalActiveTimeUs = 0;
float SystemTelemetry::currentFps = 0.0f;
uint8_t SystemTelemetry::cpuUsage = 15;

void SystemTelemetry::init() {
    frameCount = 0;
    lastCalcTimeUs = esp_timer_get_time();
    totalActiveTimeUs = 0;
    currentFps = 30.0f;
    cpuUsage = 15;
}

void SystemTelemetry::recordFrame() {
    frameCount++;
}

void SystemTelemetry::recordActiveTime(int64_t durationUs) {
    if (durationUs > 0) {
        totalActiveTimeUs += durationUs;
    }
}

void SystemTelemetry::update() {
    int64_t nowUs = esp_timer_get_time();
    int64_t deltaUs = nowUs - lastCalcTimeUs;

    if (deltaUs >= 1000000) { // Mỗi 1 giây tính toán lại FPS và CPU usage thực tế
        currentFps = (float)(frameCount * 1000000.0f) / (float)deltaUs;
        frameCount = 0;

        // Tính CPU thực tế: Tỷ lệ thời gian CPU bận rộn trên tổng thời gian trôi qua
        int64_t busyUs = totalActiveTimeUs;
        totalActiveTimeUs = 0;
        lastCalcTimeUs = nowUs;

        float usage = (float)(busyUs * 100.0f) / (float)deltaUs;
        if (usage < 2.0f) usage = 2.0f;
        if (usage > 99.0f) usage = 99.0f;
        cpuUsage = (uint8_t)round(usage);
    }
}

float SystemTelemetry::getFPS() {
    return currentFps;
}

uint32_t SystemTelemetry::getFreeHeap() {
    return esp_get_free_heap_size();
}

uint32_t SystemTelemetry::getTotalHeap() {
    return heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
}

float SystemTelemetry::getHeapUsagePercent() {
    uint32_t total = getTotalHeap();
    if (total == 0) return 0.0f;
    uint32_t free = getFreeHeap();
    return ((float)(total - free) * 100.0f) / (float)total;
}

uint32_t SystemTelemetry::getMinFreeHeap() {
    return esp_get_minimum_free_heap_size();
}

uint8_t SystemTelemetry::getCpuUsage() {
    return cpuUsage;
}

String SystemTelemetry::getUptimeFormatted() {
    unsigned long sec = millis() / 1000;
    unsigned long days = sec / 86400;
    sec %= 86400;
    unsigned long hours = sec / 3600;
    sec %= 3600;
    unsigned long mins = sec / 60;
    unsigned long secs = sec % 60;

    char buf[32];
    if (days > 0) {
        snprintf(buf, sizeof(buf), "%lud %02lu:%02lu:%02lu", days, hours, mins, secs);
    } else {
        snprintf(buf, sizeof(buf), "%02lu:%02lu:%02lu", hours, mins, secs);
    }
    return String(buf);
}
