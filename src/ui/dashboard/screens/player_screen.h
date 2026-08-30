#ifndef PLAYER_SCREEN_H
#define PLAYER_SCREEN_H

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
    void updatePlaybackMode(bool shuffleActive, bool repeatActive);
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
    lv_obj_t* lblSongAlbum;
    lv_obj_t* lblQualityChip;
    lv_obj_t* imgAlbumCover; // simulated cover art

    // Spectrum bars
    lv_obj_t* spectrumBars[24];
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
    static void playlist_item_click_cb(lv_event_t* e);
};

#endif // PLAYER_SCREEN_H
