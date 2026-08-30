#ifndef AUDIO_PLAYER_SERVICE_H
#define AUDIO_PLAYER_SERVICE_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

#define MAX_AUDIO_TRACKS 32

enum AudioPlaybackState {
    STATE_STOPPED = 0,
    STATE_PLAYING,
    STATE_PAUSED
};

enum AudioRepeatMode {
    REPEAT_MODE_OFF = 0,
    REPEAT_MODE_ALL = 1,
    REPEAT_MODE_ONE = 2
};

struct AudioTrack {
    char path[96];
    char title[64];
    char artist[32];
    char format[12];
    uint32_t fileSize;
    int durationSec;
};

class AudioPlayerService {
public:
    static void init();
    static void scanMusicFiles();
    
    // Playback controls
    static bool playTrack(int index);
    static bool play();
    static bool pause();
    static bool resume();
    static bool togglePlay();
    static void stop();
    static void next();
    static void prev();
    
    // Settings & Modes
    static void setVolume(uint8_t volumePercent); // 0 - 100
    static uint8_t getVolume();
    static void setShuffle(bool enabled);
    static bool isShuffle();
    static void setRepeatMode(AudioRepeatMode mode);
    static AudioRepeatMode getRepeatMode();
    
    // Status Getters
    static AudioPlaybackState getState();
    static bool isPlaying();
    static int getCurrentTrackIndex();
    static AudioTrack getCurrentTrack();
    static const AudioTrack* getTrack(int index);
    static int getCurrentElapsedSec();
    static int getCurrentTotalSec();
    static int getTrackCount();
    static bool isSdReady();
    static bool isInitialized();

    // Call from main loop to process auto-advance if needed
    static void update();

private:
    static void audioTask(void* parameter);
    static void scanDirectory(const char* dirPath, int maxDepth);
    static void cleanupCurrentPlayback();
    static int getNextIndex();
    static int getPrevIndex();

    static bool initialized;
    static TaskHandle_t audioTaskHandle;
    static SemaphoreHandle_t audioMutex;
    
    static AudioTrack playlist[MAX_AUDIO_TRACKS];
    static int playlistCount;
    static int currentTrackIdx;
    static AudioPlaybackState state;
    static uint8_t volume;
    static bool shuffle;
    static AudioRepeatMode repeatMode;
    
    static uint32_t playStartMillis;
    static uint32_t pausedElapsedMillis;
    static uint32_t pauseStartMillis;
    static bool trackJustFinished;
};

#endif // AUDIO_PLAYER_SERVICE_H
