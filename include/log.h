#pragma once
#include <Arduino.h>

/**
 * log.h - Quản lý log tập trung cho dự án ESP32
 *
 * Điều khiển bật/tắt log qua build flag trong platformio.ini:
 *   -DENABLE_SERIAL_LOG   -> Bật toàn bộ log (Development)
 *   (comment cờ này)      -> Tắt log (Production / Release, tiết kiệm bộ nhớ Flash/RAM và tối ưu FPS)
 *
 * Cách sử dụng:
 *   LOG_D("TAG", "Format string %d", val);  -> Debug log (cho chu kỳ lặp hoặc thông tin chi tiết)
 *   LOG_I("TAG", "Format string %s", str);  -> Info log (khởi tạo, chuyển trạng thái)
 *   LOG_W("TAG", "Format string %s", str);  -> Warning log (cảnh báo)
 *   LOG_E("TAG", "Format string %s", str);  -> Error log (luôn in qua Serial)
 */

#ifdef ENABLE_SERIAL_LOG
  #define LOG_D(tag, fmt, ...) do { Serial.printf("[%s][DEBUG] " fmt "\n", tag, ##__VA_ARGS__); } while(0)
  #define LOG_I(tag, fmt, ...) do { Serial.printf("[%s] " fmt "\n", tag, ##__VA_ARGS__); } while(0)
  #define LOG_W(tag, fmt, ...) do { Serial.printf("[%s][WARN] " fmt "\n", tag, ##__VA_ARGS__); } while(0)
#else
  #define LOG_D(tag, fmt, ...) do {} while(0)
  #define LOG_I(tag, fmt, ...) do {} while(0)
  #define LOG_W(tag, fmt, ...) do {} while(0)
#endif

// Error logs luôn được giữ lại để chẩn đoán sự cố nghiêm trọng
#define LOG_E(tag, fmt, ...) do { Serial.printf("[%s][ERROR] " fmt "\n", tag, ##__VA_ARGS__); } while(0)
