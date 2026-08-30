#ifndef PLAYER_SCREEN_H
#define PLAYER_SCREEN_H

#include <lvgl.h>

struct PlaylistItem {
    const char* title;
    const char* artist;
    const char* duration;
    bool isPlaying;
};

// ==============================================================================
// CẤU HÌNH HIỆU ỨNG SÓNG NHẠC (SPECTRUM VISUALIZER)
// - Đặt 0 (MẶC ĐỊNH): Tắt sóng nhạc Canvas, dùng Badge Hi-Fi Audio (FPS 60 mượt mà, siêu ổn định, tiết kiệm 10KB RAM)
// - Đặt 1: Bật lại sóng nhạc Canvas 24 cột động
// ==============================================================================
#define ENABLE_SPECTRUM_CANVAS 0

class PlayerScreen {
public:
    PlayerScreen(lv_obj_t* parent);
    ~PlayerScreen();


    // Setters for dynamic updates
    void updateTrackInfo(const char* title, const char* artist, const char* album, const char* qualityStr);
    void updatePlaybackProgress(int currentTimeSecs, int totalTimeSecs);
    void setPlayState(bool isPlaying);
    void updatePlaybackMode(bool shuffleActive, int repeatMode);
    void updateVolume(int volume);
    void updateEQ(const char* eqMode);
    
    void clearPlaylist();
    void addPlaylistItem(const PlaylistItem& item, int trackIndex = -1);
    void syncCurrentTrackUI();

    // Micro-animation for spectrum wave
    void tickSpectrumAnimation();

    lv_obj_t* getRoot() { return rootContainer; }

private:
    lv_obj_t* rootContainer;

    // Track Details widgets
    lv_obj_t* lblSongTitle;
    lv_obj_t* lblSongArtist;
    lv_obj_t* lblQualityChip;
    lv_obj_t* imgAlbumCover; // simulated cover art

    // Pure Text Info widgets (Chỉ hiển thị thông số THẬT 100% & hữu ích)
    lv_obj_t* lblAudioCodec;
    lv_obj_t* lblAudioSampleRate;
    lv_obj_t* lblAudioFileSize;

    // Spectrum Visualizer
    lv_obj_t* spectrumCanvas;
    lv_color_t* canvasBuf;
    int barHeights[24];


    // Playback slider & times
    lv_obj_t* seekSlider;
    lv_obj_t* lblCurrentTime;
    lv_obj_t* lblTotalTime;

    // Playback Buttons
    lv_obj_t* btnShuffle;
    lv_obj_t* btnPrev;
    lv_obj_t* btnPlayPause;
    lv_obj_t* lblPlayPauseSymbol;
    lv_obj_t* btnNext;
    lv_obj_t* btnRepeat;
    lv_obj_t* lblRepeatSymbol;


    // Bottom parameters
    lv_obj_t* volSlider;
    lv_obj_t* lblVolVal;
    lv_obj_t* lblEQVal;
    lv_obj_t* objEQVisualizer;

    // Playlist Scroll Pane
    lv_obj_t* playlistScrollContainer;
    lv_obj_t* lblPlaylistCount;

    // Helper functions
    void createPlayerControlsPane(lv_obj_t* parent);
    void createPlaylistPane(lv_obj_t* parent);

    // UI Event Callbacks
    static void play_pause_click_cb(lv_event_t* e);
    static void prev_click_cb(lv_event_t* e);
    static void next_click_cb(lv_event_t* e);
    static void shuffle_click_cb(lv_event_t* e);
    static void repeat_click_cb(lv_event_t* e);
    static void volume_slider_cb(lv_event_t* e);
    static void seek_slider_cb(lv_event_t* e);
    static void playlist_item_click_cb(lv_event_t* e);

    bool isSeeking;
};

#endif // PLAYER_SCREEN_H
