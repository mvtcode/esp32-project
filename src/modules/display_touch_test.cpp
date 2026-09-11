#include "display_touch_test.h"
#include <Wire.h>
#include <esp_heap_caps.h>
#include "log.h"

static const char *TAG = "DisplayTouch";

// Subclass của Arduino_Canvas cấp phát Framebuffer trực tiếp trên 8MB PSRAM (Rule 4)
class CanvasPSRAM : public Arduino_Canvas {
public:
  CanvasPSRAM(int16_t w, int16_t h, Arduino_G *output, int16_t output_x = 0, int16_t output_y = 0, uint8_t r = 0)
    : Arduino_Canvas(w, h, output, output_x, output_y, r) {}

  bool begin(int32_t speed = GFX_NOT_DEFINED) override {
    if ((speed != GFX_SKIP_OUTPUT_BEGIN) && (_output)) {
      if (!_output->begin(speed)) {
        return false;
      }
    }
    if (!_framebuffer) {
      size_t s = (size_t)_width * _height * 2;
      // Cấp phát Framebuffer trên PSRAM với alignment 16-byte
      _framebuffer = (uint16_t *)heap_caps_aligned_alloc(16, s, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
      if (!_framebuffer) {
        _framebuffer = (uint16_t *)ps_malloc(s);
      }
      if (!_framebuffer) {
        _framebuffer = (uint16_t *)aligned_alloc(16, s);
      }
      if (!_framebuffer) {
        LOG_E(TAG, "Khong the cap phat Framebuffer %u bytes tren PSRAM!", s);
        return false;
      }
      LOG_I(TAG, "Da cap phat Framebuffer %u bytes tren PSRAM thanh cong!", s);
    }
    return true;
  }
};

DisplayTouchTest::DisplayTouchTest()
  : _bus(nullptr), _panel(nullptr), _canvas(nullptr), _gfx(nullptr),
    _touchType(TouchDriverType::NONE), _touchI2cAddr(0),
    _initialized(false), _touchInitialized(false) {}

DisplayTouchTest::~DisplayTouchTest() {
  if (_canvas) {
    delete _canvas;
    _canvas = nullptr;
  }
  if (_panel) {
    delete _panel;
    _panel = nullptr;
  }
  if (_bus) {
    delete _bus;
    _bus = nullptr;
  }
}

const char* DisplayTouchTest::getTouchName() const {
  switch (_touchType) {
    case TouchDriverType::AXS15231B: return "AXS15231B (0x3B)";
    case TouchDriverType::GT911:     return "Goodix GT911";
    case TouchDriverType::FT6336:    return "FocalTech FT6336 (0x38)";
    default:                         return "Chua nhan dien";
  }
}

bool DisplayTouchTest::begin() {
  LOG_I(TAG, "Khoi tao man hinh cho %s (%dx%d)...", BOARD_NAME, TFT_WIDTH, TFT_HEIGHT);

  // Bật đèn nền LCD
  pinMode(PIN_TFT_BL, OUTPUT);
  setBacklight(255);

#if defined(BOARD_JC3248W535)
  // Guition JC3248W535: QSPI Bus + AXS15231B Panel
  _bus = new Arduino_ESP32QSPI(
    PIN_TFT_QSPI_CS,
    PIN_TFT_QSPI_SCK,
    PIN_TFT_QSPI_D0,
    PIN_TFT_QSPI_D1,
    PIN_TFT_QSPI_D2,
    PIN_TFT_QSPI_D3
  );
  _panel = new Arduino_AXS15231B(_bus, PIN_TFT_RST, TFT_ROTATION, false /* IPS */, TFT_WIDTH, TFT_HEIGHT);
  _canvas = new CanvasPSRAM(TFT_WIDTH, TFT_HEIGHT, _panel, 0, 0, 0);
  _gfx = _canvas;

#elif defined(BOARD_SUNTON_S3)
  _bus = new Arduino_ESP32SPI(PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_SCK, PIN_TFT_MOSI, GFX_NOT_DEFINED);
  _panel = new Arduino_ST7796(_bus, PIN_TFT_RST, TFT_ROTATION, true /* IPS */, 320, 480);
  _canvas = new CanvasPSRAM(TFT_WIDTH, TFT_HEIGHT, _panel, 0, 0, 0);
  _gfx = _canvas;
#endif

  if (!_gfx->begin()) {
    LOG_E(TAG, "Loi khoi tao man hinh LCD / PSRAM Canvas!");
    return false;
  }

  _gfx->fillScreen(BLACK);
  flush();
  _initialized = true;
  LOG_I(TAG, "Khoi tao man hinh + PSRAM Canvas thanh cong (%dx%d)!", TFT_WIDTH, TFT_HEIGHT);

  // Khoi tao cam ung bang Native Touch Driver
  initTouch();

  return true;
}

void DisplayTouchTest::flush() {
  if (_canvas) {
    _canvas->flush();
  }
}

bool DisplayTouchTest::initTouch() {
  LOG_I(TAG, "Khoi tao Wire cho Touch tai SDA=%d, SCL=%d...", PIN_TOUCH_I2C_SDA, PIN_TOUCH_I2C_SCL);
  Wire.begin(PIN_TOUCH_I2C_SDA, PIN_TOUCH_I2C_SCL, 400000);

  // 1. Kiem tra AXS15231B (0x3B)
  Wire.beginTransmission(0x3B);
  if (Wire.endTransmission() == 0) {
    _touchType = TouchDriverType::AXS15231B;
    _touchI2cAddr = 0x3B;
    _touchInitialized = true;
    LOG_I(TAG, "-> Phat hien chip cam ung: AXS15231B tai 0x3B!");
    return true;
  }

  // 2. Kiem tra GT911 (0x5D hoac 0x14)
  Wire.beginTransmission(0x5D);
  if (Wire.endTransmission() == 0) {
    _touchType = TouchDriverType::GT911;
    _touchI2cAddr = 0x5D;
    _touchInitialized = true;
    LOG_I(TAG, "-> Phat hien chip cam ung: Goodix GT911 tai 0x5D!");
    return true;
  }

  Wire.beginTransmission(0x14);
  if (Wire.endTransmission() == 0) {
    _touchType = TouchDriverType::GT911;
    _touchI2cAddr = 0x14;
    _touchInitialized = true;
    LOG_I(TAG, "-> Phat hien chip cam ung: Goodix GT911 tai 0x14!");
    return true;
  }

  // 3. Kiem tra FT6336 (0x38)
  Wire.beginTransmission(0x38);
  if (Wire.endTransmission() == 0) {
    _touchType = TouchDriverType::FT6336;
    _touchI2cAddr = 0x38;
    _touchInitialized = true;
    LOG_I(TAG, "-> Phat hien chip cam ung: FocalTech FT6336 tai 0x38!");
    return true;
  }

  // Quet toan bo dia chi I2C
  for (uint8_t a = 1; a < 127; a++) {
    Wire.beginTransmission(a);
    if (Wire.endTransmission() == 0) {
      LOG_I(TAG, "-> I2C Device phan hoi tai: 0x%02X", a);
      if (a == 0x3B) {
        _touchType = TouchDriverType::AXS15231B;
        _touchI2cAddr = 0x3B;
        _touchInitialized = true;
        return true;
      }
    }
  }

  LOG_W(TAG, "Chua tim thay chip cam ung tren bus I2C SDA=%d, SCL=%d",
        PIN_TOUCH_I2C_SDA, PIN_TOUCH_I2C_SCL);
  return false;
}

void DisplayTouchTest::setBacklight(uint8_t brightness) {
  analogWrite(PIN_TFT_BL, brightness);
}

void DisplayTouchTest::showColorTest() {
  if (!_initialized || !_canvas) return;

  LOG_I(TAG, "Chay bai test mau (Red, Green, Blue, White, Black)...");
  uint16_t colors[] = {RED, GREEN, BLUE, WHITE, BLACK};
  const char* names[] = {"RED", "GREEN", "BLUE", "WHITE", "BLACK"};

  for (int i = 0; i < 5; i++) {
    _canvas->fillScreen(colors[i]);
    _canvas->setTextColor(colors[i] == WHITE ? BLACK : WHITE);
    _canvas->setTextSize(3);
    _canvas->setCursor(75, 220);
    _canvas->printf("TEST %s", names[i]);
    _canvas->flush();
    vTaskDelay(pdMS_TO_TICKS(400));
  }
}

void DisplayTouchTest::showScreen(const String& sdCapacityStr, const String& sdStatusDetail, bool sdOk,
                                  int32_t touchX, int32_t touchY, int32_t rawX, int32_t rawY, bool touched,
                                  bool bleConnected, const String& bleHost) {
  if (!_initialized || !_canvas) return;

  // 1. Nền màn hình chính (Dark slate navy)
  _canvas->fillScreen(0x0821);

  // 2. Thanh tiêu đề phía trên
  _canvas->fillRect(0, 0, TFT_WIDTH, 36, 0x01E8);
  _canvas->setTextColor(WHITE);
  _canvas->setTextSize(2);
  _canvas->setCursor(40, 10);
  _canvas->print("ESP32-S3 KIT 3.5\"");
  _canvas->setTextSize(1);
  _canvas->setTextColor(0x07FF); // Cyan
  _canvas->setCursor(250, 14);
  _canvas->print("PSRAM 8M");

  // 3. Khung Card 1: DUNG LƯỢNG THẺ NHỚ MICRO SD (Hiển thị nổi bật ở giữa phía trên)
  _canvas->fillRect(10, 46, 300, 110, 0x10A2);
  _canvas->drawRect(10, 46, 300, 110, sdOk ? 0x07E0 : 0xF800);
  _canvas->drawRect(11, 47, 298, 108, sdOk ? 0x0400 : 0x8000); // Viền kép nổi bật

  // Tiêu đề card SD
  _canvas->setTextSize(1);
  _canvas->setTextColor(0x07FF); // Cyan
  _canvas->setCursor(18, 54);
  _canvas->print("[ 1. THONG TIN THE NHO MICRO SD ]");

  if (sdOk) {
    _canvas->setTextColor(0xCE79);
    _canvas->setCursor(18, 70);
    _canvas->print("DUNG LUONG THE SD:");

    // Dòng chữ dung lượng thẻ cực lớn ở giữa
    _canvas->setTextSize(2);
    _canvas->setTextColor(0xFFE0); // Bright Yellow
    _canvas->setCursor(18, 86);
    _canvas->print(sdCapacityStr);

    // Dòng thông tin trạng thái & magic bytes
    _canvas->setTextSize(1);
    _canvas->setTextColor(0x07E0); // Bright Green
    _canvas->setCursor(18, 126);
    _canvas->print(sdStatusDetail);
  } else {
    _canvas->setTextSize(2);
    _canvas->setTextColor(0xF800); // Red
    _canvas->setCursor(18, 78);
    _canvas->print(sdCapacityStr);

    _canvas->setTextSize(1);
    _canvas->setTextColor(WHITE);
    _canvas->setCursor(18, 112);
    _canvas->print(sdStatusDetail);
  }

  // 4. Khung Card 2: VỊ TRÍ CHẠM CẢM ỨNG (Hiển thị nổi bật ở chính giữa màn hình)
  _canvas->fillRect(10, 166, 300, 155, 0x18C3);
  _canvas->drawRect(10, 166, 300, 155, touched ? 0x07E0 : 0x7BEF);
  _canvas->drawRect(11, 167, 298, 153, touched ? 0x07E0 : 0x39E7); // Viền kép

  // Tiêu đề card Touch
  _canvas->setTextSize(1);
  _canvas->setTextColor(0xFD20); // Bright Orange
  _canvas->setCursor(18, 174);
  _canvas->print("[ 2. CAM UNG (TOUCH SCREEN) ]");

  _canvas->setTextColor(0x07FF);
  _canvas->setCursor(220, 174);
  _canvas->print("AXS15231B");

  // Hiển thị tọa độ X, Y cực lớn ở giữa
  char coordBuf[32];
  if (touchX >= 0 && touchY >= 0) {
    snprintf(coordBuf, sizeof(coordBuf), "X:%03d  Y:%03d", (int)touchX, (int)touchY);
  } else {
    snprintf(coordBuf, sizeof(coordBuf), "X:---  Y:---");
  }
  _canvas->setTextSize(3);
  _canvas->setTextColor(touched ? 0x07E0 : (touchX >= 0 ? 0xFFE0 : WHITE));
  _canvas->setCursor(24, 196);
  _canvas->print(coordBuf);

  // Trạng thái chạm
  _canvas->setTextSize(1);
  if (touched) {
    _canvas->setTextColor(0x07E0); // Bright Green
    _canvas->setCursor(18, 238);
    _canvas->print("Trang thai: [ DANG CHAM / TOUCHED ]");
  } else {
    _canvas->setTextColor(0xAD75); // Light Gray
    _canvas->setCursor(18, 238);
    _canvas->print("Trang thai: [ CHO CHAM ] - Cham vao man hinh");
  }

  // Tọa độ thô (Raw) từ sensor và thông tin loa
  char rawBuf[60];
  snprintf(rawBuf, sizeof(rawBuf), "Raw: X=%-3d, Y=%-3d | Loa: Bip khi cham", (int)rawX, (int)rawY);
  _canvas->setTextColor(0x8410);
  _canvas->setCursor(18, 264);
  _canvas->print(rawBuf);

  _canvas->setTextColor(WHITE);
  _canvas->setCursor(18, 288);
  _canvas->print("Cham bat ky dau de kiem tra do nhay");

  // 5. Khung Card 3: BLE HID COMPOSITE & HỆ THỐNG
  _canvas->fillRect(10, 330, 300, 92, 0x10A2);
  _canvas->drawRect(10, 330, 300, 92, bleConnected ? 0x07E0 : 0x07FF);
  _canvas->setTextSize(1);
  _canvas->setTextColor(0x07FF);
  _canvas->setCursor(18, 338);
  _canvas->print("[ 3. BLE HID COMPOSITE & HE THONG ]");

  if (bleConnected) {
    _canvas->setTextColor(0x07E0); // Green
    _canvas->setCursor(18, 356);
    _canvas->print("BLE: [ DA KET NOI / CONNECTED ]");

    _canvas->setTextColor(WHITE);
    _canvas->setCursor(18, 374);
    _canvas->printf("Host: %s", bleHost.length() > 0 ? bleHost.c_str() : "PC / Laptop");

    _canvas->setTextColor(0xFFE0); // Yellow
    _canvas->setCursor(18, 392);
    _canvas->print("Serial CLI: Go 'help' de test lenh HID");
  } else {
    _canvas->setTextColor(0xFD20); // Orange
    _canvas->setCursor(18, 356);
    _canvas->print("BLE: [ DANG CHO KET NOI... ]");

    _canvas->setTextColor(WHITE);
    _canvas->setCursor(18, 374);
    _canvas->print("Ten: ESP32-S3 Smart TouchPad");

    _canvas->setTextColor(0xCE79);
    _canvas->setCursor(18, 392);
    _canvas->print("Vao Bluetooth tren PC de ghep noi");
  }

  // 6. Footer bar dưới cùng
  _canvas->fillRect(10, 432, 300, 38, 0x0842);
  _canvas->drawRect(10, 432, 300, 38, 0x2124);
  _canvas->setTextSize(1);
  _canvas->setTextColor(WHITE);
  _canvas->setCursor(18, 446);
  _canvas->print("Touch IC: AXS15231B (I2C 0x3B) 400kHz");

  // 7. Vẽ Crosshair / Vòng tròn mục tiêu tại điểm chạm nếu đang chạm
  if (touched && touchX >= 0 && touchY >= 0) {
    _canvas->drawCircle(touchX, touchY, 14, RED);
    _canvas->drawCircle(touchX, touchY, 15, RED);
    _canvas->fillCircle(touchX, touchY, 4, YELLOW);
    _canvas->drawFastHLine(touchX - 22, touchY, 44, RED);
    _canvas->drawFastVLine(touchX, touchY - 22, 44, RED);
  }

  // Đẩy toàn bộ Framebuffer lên màn hình qua DMA
  _canvas->flush();
}

bool DisplayTouchTest::readAxs15231Touch(int32_t &x, int32_t &y) {
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
      x = ((buf[2] & 0x0F) << 8) | buf[3];
      y = ((buf[4] & 0x0F) << 8) | buf[5];
      return true;
    }
  }
  return false;
}

bool DisplayTouchTest::readGt911Touch(int32_t &x, int32_t &y) {
  Wire.beginTransmission(_touchI2cAddr);
  Wire.write(0x81);
  Wire.write(0x4E);
  if (Wire.endTransmission() != 0) return false;

  Wire.requestFrom((uint8_t)_touchI2cAddr, (uint8_t)7);
  if (Wire.available() >= 7) {
    uint8_t status = Wire.read();
    uint8_t points = status & 0x0F;
    if ((status & 0x80) && points > 0) {
      uint8_t xl = Wire.read();
      uint8_t xh = Wire.read();
      uint8_t yl = Wire.read();
      uint8_t yh = Wire.read();
      x = xl | (xh << 8);
      y = yl | (yh << 8);

      Wire.beginTransmission(_touchI2cAddr);
      Wire.write(0x81);
      Wire.write(0x4E);
      Wire.write(0x00);
      Wire.endTransmission();
      return true;
    }
    if (status & 0x80) {
      Wire.beginTransmission(_touchI2cAddr);
      Wire.write(0x81);
      Wire.write(0x4E);
      Wire.write(0x00);
      Wire.endTransmission();
    }
  }
  return false;
}

bool DisplayTouchTest::readFt6336Touch(int32_t &x, int32_t &y) {
  Wire.beginTransmission(_touchI2cAddr);
  Wire.write(0x02);
  if (Wire.endTransmission() != 0) return false;

  Wire.requestFrom((uint8_t)_touchI2cAddr, (uint8_t)5);
  if (Wire.available() >= 5) {
    uint8_t points = Wire.read() & 0x0F;
    if (points > 0) {
      uint8_t xh = Wire.read();
      uint8_t xl = Wire.read();
      uint8_t yh = Wire.read();
      uint8_t yl = Wire.read();
      x = ((xh & 0x0F) << 8) | xl;
      y = ((yh & 0x0F) << 8) | yl;
      return true;
    }
  }
  return false;
}

TouchPoint DisplayTouchTest::getTouch() {
  TouchPoint pt = {0, 0, 0, 0, false};
  if (!_touchInitialized) return pt;

  bool ok = false;
  int32_t tx = 0, ty = 0;

  switch (_touchType) {
    case TouchDriverType::AXS15231B:
      ok = readAxs15231Touch(tx, ty);
      break;
    case TouchDriverType::GT911:
      ok = readGt911Touch(tx, ty);
      break;
    case TouchDriverType::FT6336:
      ok = readFt6336Touch(tx, ty);
      break;
    default:
      break;
  }

  if (ok) {
    pt.rawX = tx;
    pt.rawY = ty;
    pt.touched = true;

    // Trong Portrait 320x480:
    // Trục ngắn của sensor tx (0..320) tương ứng trục X (0..319)
    // Trục dài của sensor ty (0..480) tương ứng trục Y (0..479)
    int32_t sx = tx;
    int32_t sy = ty;

    if (sx < 0) sx = 0;
    if (sx >= TFT_WIDTH) sx = TFT_WIDTH - 1;
    if (sy < 0) sy = 0;
    if (sy >= TFT_HEIGHT) sy = TFT_HEIGHT - 1;

    pt.x = sx;
    pt.y = sy;
  }
  return pt;
}
