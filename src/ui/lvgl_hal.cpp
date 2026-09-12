#include "lvgl_hal.h"
#include "log.h"
#include <Wire.h>

static const char *TAG = "LvglHal";

LvglHal *LvglHal::s_instance = nullptr;

LvglHal::LvglHal()
  : _bus(nullptr)
  , _panel(nullptr)
  , _framebuffer(nullptr)
  , _buf1(nullptr)
  , _buf2(nullptr)
  , _indevTouch(nullptr)
  , _rotation(1)
  , _touchFound(false)
{
  s_instance = this;
}

LvglHal::~LvglHal() {
  if (_framebuffer != nullptr) {
    free(_framebuffer);
    _framebuffer = nullptr;
  }
  if (_buf1 != nullptr) {
    free(_buf1);
    _buf1 = nullptr;
  }
  if (_buf2 != nullptr) {
    free(_buf2);
    _buf2 = nullptr;
  }
  if (_panel != nullptr) {
    delete _panel;
    _panel = nullptr;
  }
  if (_bus != nullptr) {
    delete _bus;
    _bus = nullptr;
  }
  if (s_instance == this) {
    s_instance = nullptr;
  }
}

LvglHal *LvglHal::getInstance() {
  return s_instance;
}

bool LvglHal::begin(uint8_t rotation) {
  _rotation = rotation;
  LOG_I(TAG, "Khoi tao LVGL HAL o che do Portrait (320x480, Rotation=%d)...", _rotation);

  // 1. Khoi tao den nen LCD (PWM)
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
  setBacklight(255);

  // 2. Khoi tao Bus QSPI va Display Panel o Native Portrait (320x480)
#if defined(BOARD_JC3248W535)
  _bus = new Arduino_ESP32QSPI(
    PIN_TFT_QSPI_CS,
    PIN_TFT_QSPI_SCK,
    PIN_TFT_QSPI_D0,
    PIN_TFT_QSPI_D1,
    PIN_TFT_QSPI_D2,
    PIN_TFT_QSPI_D3
  );
  // AXS15231B khoi tao o do phan giai goc (320, 480) voi rotation 0 de dam bao tuong thich QSPI
  _panel = new Arduino_AXS15231B(_bus, PIN_TFT_RST, 0 /* Native Portrait */, false /* IPS */, TFT_WIDTH, TFT_HEIGHT);
#elif defined(BOARD_SUNTON_S3)
  _bus = new Arduino_ESP32SPI(PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_SCK, PIN_TFT_MOSI, GFX_NOT_DEFINED);
  _panel = new Arduino_ST7796(_bus, PIN_TFT_RST, 0, true, 320, 480);
#endif

  if (_panel == nullptr || !_panel->begin()) {
    LOG_E(TAG, "Khong the khoi tao panel man hinh!");
    return false;
  }

  // 3. Cap phat Framebuffer toan man hinh (320x480 = 307.2 KB) tren 8MB PSRAM (Rule 4)
  const size_t fbBytes = (size_t)TFT_WIDTH * TFT_HEIGHT * sizeof(uint16_t);
  _framebuffer = (uint16_t *)heap_caps_aligned_alloc(16, fbBytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (_framebuffer == nullptr) {
    _framebuffer = (uint16_t *)ps_malloc(fbBytes);
  }
  if (_framebuffer == nullptr) {
    LOG_E(TAG, "Khong the cap phat Framebuffer %u bytes tren PSRAM!", fbBytes);
    return false;
  }
  LOG_I(TAG, "Da cap phat Framebuffer PSRAM %u bytes (16-byte aligned) thanh cong!", fbBytes);

  // Xoa man hinh ve mau den va day toan bo frame len man hinh qua DMA QSPI
  memset(_framebuffer, 0, fbBytes);
  _panel->draw16bitRGBBitmap(0, 0, _framebuffer, TFT_WIDTH, TFT_HEIGHT);
  LOG_I(TAG, "Man hinh khoi tao va da flush frame dau tien!");

  // 4. Khoi tao I2C va Cam ung
  initTouch();

  // 5. Khoi tao LVGL core
  lv_init();

  // 6. Cap phat Double Buffer cho LVGL (Uu tien PSRAM de tiet kiem Internal SRAM cho BLE)
  const size_t bufferSize = 320 * 40; // 40 dong quet Portrait
  const size_t bufferBytes = bufferSize * sizeof(lv_color_t);

  _buf1 = (lv_color_t *)ps_malloc(bufferBytes);
  if (_buf1 == nullptr) {
    LOG_W(TAG, "ps_malloc cho buf1 that bai, thu malloc noi bo...");
    _buf1 = (lv_color_t *)malloc(bufferBytes);
  }

  _buf2 = (lv_color_t *)ps_malloc(bufferBytes);
  if (_buf2 == nullptr) {
    LOG_W(TAG, "ps_malloc cho buf2 that bai, thu malloc noi bo...");
    _buf2 = (lv_color_t *)malloc(bufferBytes);
  }

  if (_buf1 == nullptr) {
    LOG_E(TAG, "Khong the cap phat LVGL display buffer!");
    return false;
  }

  lv_disp_draw_buf_init(&_dispBuf, _buf1, _buf2, bufferSize);
  LOG_I(TAG, "Da cap phat Double Buffer LVGL (2 x %u bytes) thanh cong!", bufferBytes);

  // 7. Dang ky Display Driver cho LVGL (Portrait 320x480)
  lv_disp_drv_init(&_dispDrv);
  _dispDrv.hor_res = 320;
  _dispDrv.ver_res = 480;
  _dispDrv.flush_cb = dispFlush;
  _dispDrv.draw_buf = &_dispBuf;
  lv_disp_drv_register(&_dispDrv);

  // 8. Dang ky Input Device (Touch) cho LVGL
  lv_indev_drv_init(&_indevDrv);
  _indevDrv.type = LV_INDEV_TYPE_POINTER;
  _indevDrv.read_cb = touchRead;
  _indevTouch = lv_indev_drv_register(&_indevDrv);

  LOG_I(TAG, "LVGL HAL khoi tao hoan tat o che do Portrait (320x480)!");
  return true;
}

void LvglHal::setBacklight(uint8_t brightness) {
  analogWrite(PIN_TFT_BL, brightness);
}

void LvglHal::update() {
  lv_timer_handler();
}

bool LvglHal::initTouch() {
  Wire.begin(PIN_TOUCH_I2C_SDA, PIN_TOUCH_I2C_SCL, 400000);
  Wire.beginTransmission(0x3B);
  if (Wire.endTransmission() == 0) {
    _touchFound = true;
    LOG_I(TAG, "-> Phat hien chip cam ung AXS15231B tai dia chi I2C 0x3B!");
    return true;
  }

  // Thu kiem tra cac dia chi khac phong truong hop khac board
  Wire.beginTransmission(0x14);
  if (Wire.endTransmission() == 0) {
    _touchFound = true;
    LOG_I(TAG, "-> Phat hien chip cam ung GT911 tai 0x14!");
    return true;
  }

  LOG_W(TAG, "Chua phan hoi chip cam ung tren bus I2C!");
  return false;
}

bool LvglHal::readTouch(int32_t &x, int32_t &y) {
  if (!_touchFound) return false;

  uint8_t readCmd[8] = {0xb5, 0xab, 0xa5, 0x5a, 0, 0, 0, 0x08};
  Wire.beginTransmission(0x3B);
  Wire.write(readCmd, 8);
  if (Wire.endTransmission() != 0) {
    return false;
  }

  uint8_t buf[14] = {0};
  uint8_t count = Wire.requestFrom((uint8_t)0x3B, (uint8_t)14);
  if (count >= 6) {
    Wire.readBytes(buf, count);
    uint8_t points = buf[1];
    if (points > 0 && points <= 2 && buf[0] == 0) {
      int32_t rawX = ((buf[2] & 0x0F) << 8) | buf[3];
      int32_t rawY = ((buf[4] & 0x0F) << 8) | buf[5];

      // Hệ tọa độ cảm ứng ở chế độ Native Portrait (320x480)
      if (rawX < 0) rawX = 0;
      if (rawX > 319) rawX = 319;
      if (rawY < 0) rawY = 0;
      if (rawY > 479) rawY = 479;

      x = rawX;
      y = rawY;
      return true;
    }
  }
  return false;
}

void LvglHal::dispFlush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
  if (s_instance != nullptr && s_instance->_panel != nullptr && s_instance->_framebuffer != nullptr) {
    const uint16_t *src = (const uint16_t *)color_p;
    uint16_t *fb = s_instance->_framebuffer;
    int16_t w = area->x2 - area->x1 + 1;

    // Trong Native Portrait (320x480), bộ đệm LVGL và Framebuffer PSRAM cùng chiều!
    // Sao chép trực tiếp từng dòng bằng memcpy tối ưu phần cứng cực nhanh:
    for (int16_t y = area->y1; y <= area->y2; y++) {
      memcpy(&fb[y * TFT_WIDTH + area->x1], src, w * sizeof(uint16_t));
      src += w;
    }

    // Khi LVGL đã vẽ xong toàn bộ các vùng bẩn (dirty areas) của frame này:
    // Đẩy toàn bộ Framebuffer (320x480) qua DMA QSPI lên chip AXS15231B
    if (lv_disp_flush_is_last(disp_drv)) {
      s_instance->_panel->draw16bitRGBBitmap(0, 0, fb, TFT_WIDTH, TFT_HEIGHT);
    }
  }
  lv_disp_flush_ready(disp_drv);
}

void LvglHal::touchRead(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
  if (s_instance == nullptr) {
    data->state = LV_INDEV_STATE_REL;
    return;
  }

  int32_t tx = 0, ty = 0;
  if (s_instance->readTouch(tx, ty)) {
    data->point.x = (lv_coord_t)tx;
    data->point.y = (lv_coord_t)ty;
    data->state = LV_INDEV_STATE_PR;
  } else {
    data->state = LV_INDEV_STATE_REL;
  }
}
