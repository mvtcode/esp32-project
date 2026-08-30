#ifndef SYSTEM_TELEMETRY_H
#define SYSTEM_TELEMETRY_H

#include <Arduino.h>

class SystemTelemetry {
public:
    static void init();
    static void recordFrame(); // Gọi sau mỗi lần lv_timer_handler() hoặc flush
    static void recordActiveTime(int64_t durationUs); // Ghi nhận thời gian CPU xử lý thực tế
    static void update();      // Cập nhật tính toán mỗi 500ms - 1s

    static float getFPS();
    static uint32_t getFreeHeap();
    static uint32_t getTotalHeap();
    static float getHeapUsagePercent();
    static uint32_t getMinFreeHeap();
    static uint8_t getCpuUsage(); // 0 - 100%
    static String getUptimeFormatted();

private:
    static unsigned long frameCount;
    static int64_t lastCalcTimeUs;
    static int64_t totalActiveTimeUs;
    static float currentFps;
    static uint8_t cpuUsage;
};

#endif // SYSTEM_TELEMETRY_H
