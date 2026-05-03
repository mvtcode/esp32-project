#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <lvgl.h>
#include "ui/ui.h" // Thêm khi tích hợp SquareLine Studio

// --- 1. Khởi tạo phần cứng ---
TFT_eSPI tft = TFT_eSPI();

// --- 2. Cấu hình LVGL ---
static const uint32_t screenWidth  = 480;
static const uint32_t screenHeight = 320;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10]; 

/* Display flushing */
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp);
}

/* Reading input device (Touchpad) */
void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    uint16_t touchX, touchY;
    bool touched = tft.getTouch(&touchX, &touchY);
    if (!touched) {
        data->state = LV_INDEV_STATE_REL;
    } else {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touchX;
        data->point.y = touchY;
    }
}

// --- 3. Main ---
void setup() {
    Serial.begin(115200);

    // Bật đèn nền (Backlight)
    pinMode(27, OUTPUT);
    digitalWrite(27, HIGH);

    // TFT & Touch
    tft.init();
    tft.setRotation(1);
    // Thay calData bằng kết quả hiệu chuẩn của bạn đã lưu
    uint16_t calData[5] = { 334, 3478, 384, 3435, 3 }; 
    tft.setTouch(calData);

    // LVGL Init
    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    /*Initialize the input device driver*/
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    // Khởi tạo UI từ SquareLine Studio
    ui_init(); 
}

void loop() {
    lv_timer_handler(); // Xử lý các tác vụ đồ họa
    delay(5);
}