#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <Arduino_GFX_Library.h>
#include "pin_config.h"

class LvglHal {
public:
  LvglHal();
  ~LvglHal();

  // Khởi tạo màn hình, cảm ứng và LVGL framework ở chế độ Portrait (320x480)
  bool begin(uint8_t rotation = 0);

  // Vòng lặp cập nhật LVGL (gọi trong loop hoặc FreeRTOS task)
  void update();

  // Điều chỉnh độ sáng màn hình (0 - 255)
  void setBacklight(uint8_t brightness);

  // Lấy kích thước màn hình
  uint16_t getWidth() const { return 320; }
  uint16_t getHeight() const { return 480; }

  // Singleton instance
  static LvglHal *getInstance();

private:
  static LvglHal *s_instance;

  Arduino_DataBus *_bus;
  Arduino_GFX *_panel;
  uint16_t *_framebuffer;

  lv_disp_draw_buf_t _dispBuf;
  lv_color_t *_buf1;
  lv_color_t *_buf2;
  lv_disp_drv_t _dispDrv;
  lv_indev_drv_t _indevDrv;
  lv_indev_t *_indevTouch;

  uint8_t _rotation;
  bool _touchFound;

  bool initTouch();
  bool readTouch(int32_t &x, int32_t &y);

  static void dispFlush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
  static void touchRead(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
};
