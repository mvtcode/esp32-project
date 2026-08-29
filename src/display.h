#ifndef DISPLAY_H
#define DISPLAY_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789      _panel_instance;
    lgfx::Bus_SPI           _bus_instance;
    lgfx::Light_PWM         _light_instance;

public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();

            cfg.spi_host = SPI2_HOST;     // Sử dụng SPI2 (FSPI) của ESP32-S3
            cfg.spi_mode = 0;             // SPI Mode 0
            cfg.freq_write = 80000000;    // Tần số SPI ghi 40MHz ổn định cho ST7789
            cfg.freq_read  = 16000000;
            cfg.spi_3wire  = false;
            cfg.use_lock   = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO; // Sử dụng DMA để truyền hình ảnh tốc độ cao


            cfg.pin_sclk = 19;            // SCL
            cfg.pin_mosi = 20;            // MOSI
            cfg.pin_miso = -1;
            cfg.pin_dc   = 47;            // DC

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        {
            auto cfg = _panel_instance.config();

            cfg.pin_cs           = 45;    // CS
            cfg.pin_rst          = 21;    // RST
            cfg.pin_busy         = -1;

            cfg.panel_width      = 240;
            cfg.panel_height     = 320;
            cfg.offset_x         = 0;
            cfg.offset_y         = 0;
            cfg.offset_rotation  = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable         = false;
            cfg.invert           = false; // Đổi thành true nếu màu sắc bị đảo (âm bản)
            cfg.rgb_order        = false; // Đổi thành true nếu màu đỏ/xanh bị hoán đổi
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;

            _panel_instance.config(cfg);
        }

        {
            auto cfg = _light_instance.config();

            cfg.pin_bl = 38;              // Chân điều khiển đèn nền (BL)
            cfg.invert = false;
            cfg.freq   = 44100;
            cfg.pwm_channel = 7;

            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

#endif // DISPLAY_H
