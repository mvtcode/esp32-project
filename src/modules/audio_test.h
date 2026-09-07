#pragma once
#include <Arduino.h>
#include <driver/i2s.h>
#include "pin_config.h"

class AudioTest {
public:
  AudioTest();
  ~AudioTest();

  bool begin();
  void playTone(float frequency, int durationMs, float volume = 0.5f);
  void playChime();

private:
  bool _initialized;
  i2s_port_t _i2sPort;
  static const int SAMPLE_RATE = 44100;
};
