#ifndef CAMERA_SERVICE_H
#define CAMERA_SERVICE_H

#include <Arduino.h>
#include "esp_camera.h"
#include "camera_pins.h"

// Cấu trúc đại diện cho một frame hình ảnh từ Camera
struct CameraFrame {
    camera_fb_t *raw_fb = nullptr;
    uint8_t *buffer = nullptr;
    size_t len = 0;
    int width = 0;
    int height = 0;
    pixformat_t format = PIXFORMAT_RGB565;

    bool isValid() const { return buffer != nullptr && len > 0; }
};

class CameraService {
public:
    // Cấu hình Camera luôn chạy cố định ở độ phân giải VGA (640x480)
    static bool init(framesize_t frame_size = FRAMESIZE_VGA, pixformat_t pixel_format = PIXFORMAT_RGB565, int fb_count = 2) {
        camera_config_t config;
        config.ledc_channel = LEDC_CHANNEL_0;
        config.ledc_timer   = LEDC_TIMER_0;
        config.pin_d0       = Y2_GPIO_NUM;
        config.pin_d1       = Y3_GPIO_NUM;
        config.pin_d2       = Y4_GPIO_NUM;
        config.pin_d3       = Y5_GPIO_NUM;
        config.pin_d4       = Y6_GPIO_NUM;
        config.pin_d5       = Y7_GPIO_NUM;
        config.pin_d6       = Y8_GPIO_NUM;
        config.pin_d7       = Y9_GPIO_NUM;
        config.pin_xclk     = XCLK_GPIO_NUM;
        config.pin_pclk     = PCLK_GPIO_NUM;
        config.pin_vsync    = VSYNC_GPIO_NUM;
        config.pin_href     = HREF_GPIO_NUM;
        config.pin_sccb_sda = SIOD_GPIO_NUM;
        config.pin_sccb_scl = SIOC_GPIO_NUM;
        config.pin_pwdn     = PWDN_GPIO_NUM;
        config.pin_reset    = RESET_GPIO_NUM;
        config.xclk_freq_hz = 20000000;
        config.pixel_format = pixel_format;
        config.frame_size   = frame_size;
        config.jpeg_quality = 12;
        config.fb_count     = fb_count;
        config.grab_mode    = CAMERA_GRAB_LATEST;

        esp_err_t err = esp_camera_init(&config);
        if (err != ESP_OK) {
            Serial.printf("[CameraService] Init failed: 0x%x\n", err);
            return false;
        }

        sensor_t *s = esp_camera_sensor_get();
        if (s != nullptr) {
            if (s->id.PID == OV3660_PID) {
                s->set_vflip(s, 1);           // Lật ảnh đúng chiều camera
                s->set_brightness(s, 0);      // Độ sáng tự nhiên
                s->set_contrast(s, 1);        // Tăng tương phản nhẹ để ảnh trong và rõ
                s->set_saturation(s, 0);      // Màu sắc tự nhiên
                s->set_sharpness(s, 1);       // Tăng nét chi tiết
                s->set_whitebal(s, 1);        // Cân bằng trắng tự động
                s->set_awb_gain(s, 1);
                s->set_exposure_ctrl(s, 1);   // Phơi sáng tự động
                s->set_gain_ctrl(s, 1);       // Tự động kiểm soát Gain
            }
        }

        Serial.println("[CameraService] Camera OV3660 initialized at VGA (640x480)");
        return true;
    }

    // Lấy một khung hình mới từ Camera (Stream Output của khối Camera: 640x480)
    static CameraFrame getFrame() {
        CameraFrame frame;
        frame.raw_fb = esp_camera_fb_get();
        if (frame.raw_fb) {
            frame.buffer = frame.raw_fb->buf;
            frame.len    = frame.raw_fb->len;
            frame.width  = frame.raw_fb->width;
            frame.height = frame.raw_fb->height;
            frame.format = frame.raw_fb->format;
        }
        return frame;
    }

    // Giải phóng bộ đệm frame
    static void releaseFrame(CameraFrame &frame) {
        if (frame.raw_fb) {
            esp_camera_fb_return(frame.raw_fb);
            frame.raw_fb = nullptr;
            frame.buffer = nullptr;
        }
    }
};

#endif // CAMERA_SERVICE_H
