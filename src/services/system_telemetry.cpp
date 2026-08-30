#include "system_telemetry.h"
#include <esp_system.h>
#include <esp_heap_caps.h>

unsigned long SystemTelemetry::frameCount = 0;
unsigned long SystemTelemetry::lastFpsCalcTime = 0;
float SystemTelemetry::currentFps = 0.0f;
unsigned long SystemTelemetry::lastIdleTime = 0;
uint8_t SystemTelemetry::cpuUsage = 15;

void SystemTelemetry::init() {
    frameCount = 0;
    lastFpsCalcTime = millis();
    currentFps = 30.0f;
    lastIdleTime = millis();
    cpuUsage = 20;
}

void SystemTelemetry::recordFrame() {
    frameCount++;
}

void SystemTelemetry::update() {
    unsigned long now = millis();
    unsigned long delta = now - lastFpsCalcTime;

    if (delta >= 1000) { // Mỗi 1s tính toán lại FPS
        currentFps = (float)(frameCount * 1000.0f) / (float)delta;
        frameCount = 0;
        lastFpsCalcTime = now;

        // Tính toán ước lượng CPU usage dựa trên heap delta và render rate
        // (Hoặc dựa trên esp_timer)
        uint32_t freeH = esp_get_free_heap_size();
        uint32_t totalH = getTotalHeap();
        float memUsedPct = 100.0f - ((float)freeH * 100.0f / (float)totalH);
        
        // CPU load ước lượng theo cường độ render & hoạt động
        int load = (int)(currentFps * 1.8f + (memUsedPct * 0.2f));
        if (load < 5) load = 5;
        if (load > 95) load = 95;
        cpuUsage = (uint8_t)load;
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
