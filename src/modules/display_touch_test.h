#pragma once
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include "pin_config.h"

enum class TouchDriverType {
  NONE,
  AXS15231B,
  GT911,
  FT6336
};

struct TouchPoint {
  int32_t x;
  int32_t y;
  int32_t rawX;
  int32_t rawY;
  bool touched;
};

class DisplayTouchTest {
public:
  DisplayTouchTest();
  ~DisplayTouchTest();

  bool begin();
  void showColorTest();
  void showScreen(const String& sdCapacityStr, const String& sdStatusDetail, bool sdOk,
                  int32_t touchX, int32_t touchY, int32_t rawX, int32_t rawY, bool touched);
  void flush();
  TouchPoint getTouch();
  void setBacklight(uint8_t brightness);

  TouchDriverType getTouchType() const { return _touchType; }
  const char* getTouchName() const;

  Arduino_GFX* getGfx() { return _gfx; }

private:
  bool initTouch();
  bool readAxs15231Touch(int32_t &x, int32_t &y);
  bool readGt911Touch(int32_t &x, int32_t &y);
  bool readFt6336Touch(int32_t &x, int32_t &y);

  Arduino_DataBus* _bus;
  Arduino_GFX* _panel;
  Arduino_Canvas* _canvas;
  Arduino_GFX* _gfx;
  TouchDriverType _touchType;
  uint8_t _touchI2cAddr;
  bool _initialized;
  bool _touchInitialized;
};
