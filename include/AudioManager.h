#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "Constants.h"
#include "UIManager.h" // For PlayerStatus
#include <Arduino.h>
#include <vector>

// Audio libraries
#include "btAudio.h"
#include "driver/i2s.h"
#include <AudioFileSourceBuffer.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceID3.h>
#include <AudioFileSourceSD.h>
#include <AudioGeneratorMP3.h>
#include <AudioOutputI2S.h>

class AudioManager {
public:
  AudioManager();
  void begin(int initialMode, int initialVolume);
  void update(); // Call this in the Audio Task loop

  // Control methods (Thread-safe)
  void setMode(int mode);
  void setVolume(int volume);
  void togglePlayPause();
  void nextTrack();
  void prevTrack();

  PlayerStatus getStatus();

  // Helpers
  bool ensureSD();

private:
  void setupAudio();
  void stopAudio();
  String getNextMP3(String current, bool next);
  int countTracks();

  // Data
  int currentMode;
  int volume;
  bool isPlaying;
  String currentTitle;
  String currentArtist;
  int currentTrackIndex;
  int totalTracks;

  // Stats for UI
  unsigned long trackStartTime;
  unsigned long trackPausedTime;
  unsigned long lastPauseStart;
  volatile uint32_t mp3Pos;
  volatile uint32_t mp3Size;

  // Audio Objects
  btAudio bt;
  AudioGeneratorMP3 *mp3;
  AudioFileSourceSD *sourceSD;
  AudioFileSourceID3 *sourceID3;
  AudioFileSourceICYStream *sourceStream;
  AudioFileSourceBuffer *buff;
  AudioOutputI2S *out;

  // Thread safety
  SemaphoreHandle_t mutex;
};

#endif // AUDIO_MANAGER_H
