#pragma once


#include <lvgl.h>

struct PlaylistItem {
    const char* title;
    const char* artist;
    const char* duration;
    bool isPlaying;
};


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

    // Chuyển đổi giữa chế độ hiển thị thông tin text và sóng nhạc spectrum
    void toggleTrackInfoMode();
    void setTrackInfoMode(bool spectrumMode);
    bool isSpectrumMode() const { return showSpectrum; }

    lv_obj_t* getRoot() { return rootContainer; }

private:
    lv_obj_t* rootContainer;

    // Track Details widgets
    lv_obj_t* lblSongTitle;
    lv_obj_t* lblSongArtist;
    lv_obj_t* lblQualityChip;
    lv_obj_t* imgAlbumCover; // simulated cover art (khung thông tin bài hát có thể click)

    // Pure Text Info widgets (Chỉ hiển thị thông số THẬT 100% & hữu ích)
    lv_obj_t* lblAudioCodec;
    lv_obj_t* lblAudioSampleRate;
    lv_obj_t* lblAudioFileSize;

    // Spectrum Visualizer widgets
    lv_obj_t* spectrumCanvas;
    lv_color_t* canvasBuf;
    int barHeights[24];
    bool showSpectrum;


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
    static void track_info_click_cb(lv_event_t* e);

    bool isSeeking;
};

