#ifndef UI_MANAGER_H
#define UI_MANAGER_H

#include "Constants.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Arduino.h>
#include <Wire.h>

struct PlayerStatus {
  AudioMode mode;
  bool isPlaying;
  int volume;
  String title;
  String artist;
  int currentTrack;
  int totalTracks;
  uint32_t mp3Size;
  uint32_t mp3Pos;
  unsigned long trackStartTime;
  unsigned long trackPausedTime;
  unsigned long lastPauseStart;
  int bufferLevel;
};

class UIManager {
public:
  UIManager();
  void begin();
  void update(const PlayerStatus &status);
  void showOff();
  void showMessage(String msg);

private:
  Adafruit_SH1106G display;
  String formatTime(unsigned long ms);
};

#endif // UI_MANAGER_H
