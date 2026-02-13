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
#include <AudioGenerator.h>
#include <AudioGeneratorAAC.h>
#include <AudioGeneratorFLAC.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>
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
  void setCrashRecovery(String badFile);

  // Helpers
  bool ensureSD();

private:
  void setupAudio();
  void stopAudio();
  String getNextTrack(String current, bool next);
  int countTracks();
  String sanitizeFilename(String filename);
  bool isSupportedFile(String fileName);

  // Data
  int currentMode;
  int volume;
  bool isPlaying;
  String currentTitle; // Tên file gốc (để mở file)
  String displayTitle; // Tên file hiển thị (đã sanitize)
  String currentArtist;
  int currentTrackIndex;
  int totalTracks;

  // Stats for UI
  unsigned long trackStartTime;
  unsigned long trackPausedTime;
  unsigned long lastPauseStart;
  unsigned long lastTrackStartTime; // Để phát hiện file lỗi
  volatile uint32_t audioPos;
  volatile uint32_t audioSize;

  // Audio Objects
  btAudio bt;
  AudioGenerator *gen; // Con trỏ đa năng
  AudioFileSourceSD *sourceSD;
  AudioFileSourceID3 *sourceID3;
  AudioFileSourceICYStream *sourceStream;
  AudioFileSourceBuffer *buff;
  AudioOutputI2S *out;

  // Thread safety
  SemaphoreHandle_t mutex;
};

#endif // AUDIO_MANAGER_H
