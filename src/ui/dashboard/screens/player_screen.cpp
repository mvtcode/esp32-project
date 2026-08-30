#include "player_screen.h"
#include "../cyd_theme.h"
#include "../../../services/audio_player_service.h"
#include <stdio.h>
#include <stdlib.h>

PlayerScreen::PlayerScreen(lv_obj_t* parent)
    : rootContainer(nullptr),
      lblSongTitle(nullptr),
      lblSongArtist(nullptr),
      lblSongAlbum(nullptr),
      lblQualityChip(nullptr),
      imgAlbumCover(nullptr),
      seekSlider(nullptr),
      lblCurrentTime(nullptr),
      lblTotalTime(nullptr),
      btnShuffle(nullptr),
      btnPrev(nullptr),
      btnPlayPause(nullptr),
      lblPlayPauseSymbol(nullptr),
      btnNext(nullptr),
      btnRepeat(nullptr),
      volSlider(nullptr),
      lblVolVal(nullptr),
      lblEQVal(nullptr),
      objEQVisualizer(nullptr),
      playlistScrollContainer(nullptr),
      lblPlaylistCount(nullptr) {
    // 1. Create root screen container
    rootContainer = lv_obj_create(parent);
    lv_obj_set_size(rootContainer, 480, 282);
    lv_obj_set_style_bg_opa(rootContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rootContainer, 0, 0);
    lv_obj_set_style_pad_all(rootContainer, 0, 0);
    lv_obj_clear_flag(rootContainer, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Initialize spectrum data
    for (int i = 0; i < 24; i++) {
        barHeights[i] = 2;
        spectrumBars[i] = nullptr;
    }

    // 3. Build left and right layout panes
    createPlayerControlsPane(rootContainer);
    createPlaylistPane(rootContainer);
}

PlayerScreen::~PlayerScreen() {
    // Xóa tất cả LVGL animations trên spectrum bars trước khi delete object.
    // Nếu không, lv_anim callback có thể chạy sau khi C++ object đã freed.
    for (int i = 0; i < 24; i++) {
        if (spectrumBars[i]) {
            lv_anim_del(spectrumBars[i], nullptr); // Hủy mọi animation gắn với bar
        }
    }
    if (rootContainer) {
        lv_obj_del(rootContainer);
        rootContainer = nullptr;
    }
}

void PlayerScreen::createPlayerControlsPane(lv_obj_t* parent) {
    lv_obj_t* leftCard = lv_obj_create(parent);
    lv_obj_set_size(leftCard, 230, 270);
    lv_obj_align(leftCard, LV_ALIGN_TOP_LEFT, 6, 6);
    CydTheme::applyCardStyle(leftCard);

    // 1. Simulated Album Cover Container (Glowing Card Frame) - 136px height
    imgAlbumCover = lv_obj_create(leftCard);
    lv_obj_set_size(imgAlbumCover, 210, 136);
    lv_obj_align(imgAlbumCover, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(imgAlbumCover, lv_color_make(18, 12, 34), 0); // cover art background purple tint
    lv_obj_set_style_bg_opa(imgAlbumCover, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(imgAlbumCover, CydTheme::getAccentColor(), 0);
    lv_obj_set_style_border_width(imgAlbumCover, 1, 0);
    lv_obj_set_style_radius(imgAlbumCover, 8, 0);
    lv_obj_set_style_pad_all(imgAlbumCover, 4, 0);
    lv_obj_clear_flag(imgAlbumCover, LV_OBJ_FLAG_SCROLLABLE);

    // Dynamic track labels inside the cover art
    lblSongTitle = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblSongTitle, "Chưa chọn bài");
    CydTheme::applyTextFont(lblSongTitle, CydTheme::getFont14(), CydTheme::getWhiteColor());
    lv_obj_align(lblSongTitle, LV_ALIGN_TOP_LEFT, 6, 4);
    lv_label_set_long_mode(lblSongTitle, LV_LABEL_LONG_DOT);
    lv_obj_set_width(lblSongTitle, 196);

    lblSongArtist = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblSongArtist, "SD Card");
    CydTheme::applyTextFont(lblSongArtist, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblSongArtist, LV_ALIGN_TOP_LEFT, 6, 22);

    lblSongAlbum = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblSongAlbum, "Album: SD Music");
    CydTheme::applyTextFont(lblSongAlbum, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblSongAlbum, LV_ALIGN_TOP_LEFT, 6, 38);

    // 2. 24-Bar Spectrum Visualizer (76px Tall Spectrum Wave Container)
    lv_obj_t* spectrumBox = lv_obj_create(imgAlbumCover);
    lv_obj_set_size(spectrumBox, 200, 76);
    lv_obj_align(spectrumBox, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(spectrumBox, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spectrumBox, 0, 0);
    lv_obj_set_style_pad_all(spectrumBox, 0, 0);
    lv_obj_clear_flag(spectrumBox, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 24; i++) {
        // We create tall vertical bars in a row
        spectrumBars[i] = lv_obj_create(spectrumBox);
        lv_obj_set_size(spectrumBars[i], 5, barHeights[i]);
        lv_obj_set_pos(spectrumBars[i], i * 8 + 4, 76 - barHeights[i]);
        lv_obj_set_style_bg_color(spectrumBars[i], CydTheme::getAccentGlowColor(), 0);
        lv_obj_set_style_bg_opa(spectrumBars[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(spectrumBars[i], 0, 0);
        lv_obj_set_style_radius(spectrumBars[i], 2, 0);
        lv_obj_clear_flag(spectrumBars[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    // 3. Playback progress seek bar (Shifted up with comfortable margin)
    seekSlider = lv_slider_create(leftCard);
    lv_obj_set_size(seekSlider, 206, 8);
    lv_obj_align(seekSlider, LV_ALIGN_BOTTOM_MID, 0, -90);
    CydTheme::applySliderStyle(seekSlider, CydTheme::getAccentGlowColor());
    lv_slider_set_range(seekSlider, 0, 100);
    lv_slider_set_value(seekSlider, 0, LV_ANIM_OFF);

    lblCurrentTime = lv_label_create(leftCard);
    lv_label_set_text(lblCurrentTime, "00:00");
    CydTheme::applyTextFont(lblCurrentTime, CydTheme::getFont12(), CydTheme::getWhiteColor());
    lv_obj_align(lblCurrentTime, LV_ALIGN_BOTTOM_LEFT, 2, -72);

    lblTotalTime = lv_label_create(leftCard);
    lv_label_set_text(lblTotalTime, "00:00");
    CydTheme::applyTextFont(lblTotalTime, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblTotalTime, LV_ALIGN_BOTTOM_RIGHT, -2, -72);

    // 4. Playback Controls Buttons (Shuffle, Prev, Play, Next, Repeat)
    lv_obj_t* btnRow = lv_obj_create(leftCard);
    lv_obj_set_size(btnRow, 210, 32);
    lv_obj_align(btnRow, LV_ALIGN_BOTTOM_MID, 0, -28);
    lv_obj_set_style_bg_opa(btnRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btnRow, 0, 0);
    lv_obj_set_style_pad_all(btnRow, 0, 0);
    lv_obj_set_layout(btnRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btnRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btnRow, LV_OBJ_FLAG_SCROLLABLE);

    btnShuffle = lv_btn_create(btnRow);
    lv_obj_set_size(btnShuffle, 28, 28);
    CydTheme::applyButtonStyle(btnShuffle, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_set_style_pad_all(btnShuffle, 0, 0);
    lv_obj_set_ext_click_area(btnShuffle, 6);
    lv_obj_t* lblShuf = lv_label_create(btnShuffle);
    lv_label_set_text(lblShuf, LV_SYMBOL_LOOP); // Shuffle representation
    lv_obj_clear_flag(lblShuf, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblShuf);
    lv_obj_add_event_cb(btnShuffle, shuffle_click_cb, LV_EVENT_CLICKED, this);

    btnPrev = lv_btn_create(btnRow);
    lv_obj_set_size(btnPrev, 30, 30);
    CydTheme::applyButtonStyle(btnPrev, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_set_style_pad_all(btnPrev, 0, 0);
    lv_obj_set_ext_click_area(btnPrev, 6);
    lv_obj_t* lblPr = lv_label_create(btnPrev);
    lv_label_set_text(lblPr, LV_SYMBOL_PREV);
    lv_obj_clear_flag(lblPr, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblPr);
    lv_obj_add_event_cb(btnPrev, prev_click_cb, LV_EVENT_CLICKED, this);

    btnPlayPause = lv_btn_create(btnRow);
    lv_obj_set_size(btnPlayPause, 34, 34);
    CydTheme::applyButtonStyle(btnPlayPause, CydTheme::getAccentColor(), CydTheme::getWhiteColor());
    lv_obj_set_style_pad_all(btnPlayPause, 0, 0);
    lv_obj_set_ext_click_area(btnPlayPause, 6);
    lblPlayPauseSymbol = lv_label_create(btnPlayPause);
    lv_label_set_text(lblPlayPauseSymbol, LV_SYMBOL_PLAY);
    lv_obj_clear_flag(lblPlayPauseSymbol, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblPlayPauseSymbol);
    lv_obj_add_event_cb(btnPlayPause, play_pause_click_cb, LV_EVENT_CLICKED, this);

    btnNext = lv_btn_create(btnRow);
    lv_obj_set_size(btnNext, 30, 30);
    CydTheme::applyButtonStyle(btnNext, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_set_style_pad_all(btnNext, 0, 0);
    lv_obj_set_ext_click_area(btnNext, 6);
    lv_obj_t* lblNx = lv_label_create(btnNext);
    lv_label_set_text(lblNx, LV_SYMBOL_NEXT);
    lv_obj_clear_flag(lblNx, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblNx);
    lv_obj_add_event_cb(btnNext, next_click_cb, LV_EVENT_CLICKED, this);

    btnRepeat = lv_btn_create(btnRow);
    lv_obj_set_size(btnRepeat, 30, 30);
    CydTheme::applyButtonStyle(btnRepeat, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_set_style_pad_all(btnRepeat, 0, 0);
    lv_obj_set_ext_click_area(btnRepeat, 6);
    lv_obj_t* lblRep = lv_label_create(btnRepeat);
    lv_label_set_text(lblRep, LV_SYMBOL_REFRESH); // Repeat representation
    lv_obj_clear_flag(lblRep, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblRep);
    lv_obj_add_event_cb(btnRepeat, repeat_click_cb, LV_EVENT_CLICKED, this);

    // 5. Volume Adjust Bar (Footer)
    lv_obj_t* volFooter = lv_obj_create(leftCard);
    lv_obj_set_size(volFooter, 210, 28);
    lv_obj_align(volFooter, LV_ALIGN_BOTTOM_MID, 0, 4);
    lv_obj_set_style_bg_opa(volFooter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(volFooter, 0, 0);
    lv_obj_set_style_pad_all(volFooter, 0, 0);
    lv_obj_clear_flag(volFooter, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lblVolIcon = lv_label_create(volFooter);
    lv_label_set_text(lblVolIcon, LV_SYMBOL_VOLUME_MAX);
    CydTheme::applyTextFont(lblVolIcon, CydTheme::getFont12(), CydTheme::getTextPrimary());
    lv_obj_clear_flag(lblVolIcon, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(lblVolIcon, LV_ALIGN_LEFT_MID, 0, 0);

    volSlider = lv_slider_create(volFooter);
    lv_obj_set_size(volSlider, 134, 8);
    lv_obj_align(volSlider, LV_ALIGN_LEFT_MID, 22, 0);
    CydTheme::applySliderStyle(volSlider, CydTheme::getAccentGlowColor());
    lv_slider_set_range(volSlider, 0, 100);
    lv_slider_set_value(volSlider, 50, LV_ANIM_OFF);
    lv_obj_add_event_cb(volSlider, volume_slider_cb, LV_EVENT_VALUE_CHANGED, this);

    lblVolVal = lv_label_create(volFooter);
    lv_label_set_text(lblVolVal, "50%");
    CydTheme::applyTextFont(lblVolVal, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_clear_flag(lblVolVal, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(lblVolVal, LV_ALIGN_RIGHT_MID, 0, 0);
}

void PlayerScreen::createPlaylistPane(lv_obj_t* parent) {
    lv_obj_t* rightCard = lv_obj_create(parent);
    lv_obj_set_size(rightCard, 230, 270);
    lv_obj_align(rightCard, LV_ALIGN_TOP_RIGHT, -6, 6);
    CydTheme::applyCardStyle(rightCard);

    // 1. Header: Title "Danh Sách Phát" and capsule count badge
    lv_obj_t* lblHeaderTitle = lv_label_create(rightCard);
    lv_label_set_text(lblHeaderTitle, "Danh Sách Phát");
    CydTheme::applyTextFont(lblHeaderTitle, CydTheme::getFont14(), CydTheme::getTextPrimary());
    lv_obj_align(lblHeaderTitle, LV_ALIGN_TOP_LEFT, 0, 0);

    lblPlaylistCount = lv_label_create(rightCard);
    lv_label_set_text(lblPlaylistCount, "0 bài");
    CydTheme::applyTextFont(lblPlaylistCount, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblPlaylistCount, LV_ALIGN_TOP_RIGHT, 0, 2);

    // 2. Playlist scrolling rows pane
    playlistScrollContainer = lv_obj_create(rightCard);
    lv_obj_set_size(playlistScrollContainer, 210, 224);
    lv_obj_align(playlistScrollContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(playlistScrollContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(playlistScrollContainer, 0, 0);
    lv_obj_set_style_pad_all(playlistScrollContainer, 0, 0);
    lv_obj_set_layout(playlistScrollContainer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(playlistScrollContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(playlistScrollContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(playlistScrollContainer, 4, 0);
    lv_obj_set_scroll_dir(playlistScrollContainer, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(playlistScrollContainer, LV_SCROLLBAR_MODE_AUTO);
    lv_obj_add_flag(playlistScrollContainer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(playlistScrollContainer, LV_OBJ_FLAG_SCROLL_ELASTIC);
}

// --- Dynamic Setter Updates ---

void PlayerScreen::updateTrackInfo(const char* title, const char* artist, const char* album, const char* qualityStr) {
    if (lblSongTitle && title) {
        char shortTitle[36];
        if (strlen(title) > 22) {
            strncpy(shortTitle, title, 19);
            shortTitle[19] = '\0';
            strcat(shortTitle, "...");
            lv_label_set_text(lblSongTitle, shortTitle);
        } else {
            lv_label_set_text(lblSongTitle, title);
        }
    }
    if (lblSongArtist && artist) {
        lv_label_set_text(lblSongArtist, (strlen(artist) > 0 && strcmp(artist, "SD Card") != 0) ? artist : "SD Card");
    }
    if (lblSongAlbum && album) {
        char buf[48];
        snprintf(buf, sizeof(buf), "Album: %s", album);
        lv_label_set_text(lblSongAlbum, buf);
    }
    
    if (lblQualityChip) lv_label_set_text(lblQualityChip, qualityStr);
}

void PlayerScreen::updatePlaybackProgress(int currentTimeSecs, int totalTimeSecs) {
    char buf[16];
    
    // Update seek slider value percentage
    if (totalTimeSecs > 0) {
        int pct = (currentTimeSecs * 100) / totalTimeSecs;
        lv_slider_set_value(seekSlider, pct, LV_ANIM_OFF);
    }

    // Format current elapsed time (HH:mm:ss if >= 3600s, else mm:ss)
    if (currentTimeSecs >= 3600 || totalTimeSecs >= 3600) {
        int h = currentTimeSecs / 3600;
        int m = (currentTimeSecs % 3600) / 60;
        int s = currentTimeSecs % 60;
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
    } else {
        snprintf(buf, sizeof(buf), "%02d:%02d", currentTimeSecs / 60, currentTimeSecs % 60);
    }
    lv_label_set_text(lblCurrentTime, buf);

    // Format total song duration (HH:mm:ss if >= 3600s, else mm:ss)
    if (totalTimeSecs >= 3600) {
        int h = totalTimeSecs / 3600;
        int m = (totalTimeSecs % 3600) / 60;
        int s = totalTimeSecs % 60;
        snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
    } else {
        snprintf(buf, sizeof(buf), "%02d:%02d", totalTimeSecs / 60, totalTimeSecs % 60);
    }
    lv_label_set_text(lblTotalTime, buf);
}

void PlayerScreen::setPlayState(bool isPlaying) {
    if (lblPlayPauseSymbol) {
        lv_label_set_text(lblPlayPauseSymbol, isPlaying ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    }
}

void PlayerScreen::updatePlaybackMode(bool shuffleActive, bool repeatActive) {
    if (shuffleActive) {
        lv_obj_set_style_bg_color(btnShuffle, CydTheme::getAccentColor(), 0);
        lv_obj_set_style_text_color(btnShuffle, CydTheme::getWhiteColor(), 0);
    } else {
        lv_obj_set_style_bg_color(btnShuffle, CydTheme::getCardBorderColor(), 0);
        lv_obj_set_style_text_color(btnShuffle, CydTheme::getTextSecondary(), 0);
    }

    if (repeatActive) {
        lv_obj_set_style_bg_color(btnRepeat, CydTheme::getAccentColor(), 0);
        lv_obj_set_style_text_color(btnRepeat, CydTheme::getWhiteColor(), 0);
    } else {
        lv_obj_set_style_bg_color(btnRepeat, CydTheme::getCardBorderColor(), 0);
        lv_obj_set_style_text_color(btnRepeat, CydTheme::getTextSecondary(), 0);
    }
}

void PlayerScreen::updateVolume(int volume) {
    if (volSlider) lv_slider_set_value(volSlider, volume, LV_ANIM_OFF);
    
    char buf[8];
    sprintf(buf, "%d%%", volume);
    if (lblVolVal) lv_label_set_text(lblVolVal, buf);
}

void PlayerScreen::updateEQ(const char* eqMode) {
    if (lblEQVal && eqMode) lv_label_set_text(lblEQVal, eqMode);
}

void PlayerScreen::clearPlaylist() {
    lv_obj_clean(playlistScrollContainer);
    if (lblPlaylistCount) lv_label_set_text(lblPlaylistCount, "0 bài");
}

void PlayerScreen::addPlaylistItem(const PlaylistItem& item, int trackIndex) {
    if (!playlistScrollContainer) return;

    // 1. Create list card row
    lv_obj_t* row = lv_obj_create(playlistScrollContainer);
    if (!row) return;

    lv_obj_set_size(row, 206, 28);
    lv_obj_set_style_bg_color(row, item.isPlaying ? CydTheme::getCardBorderColor() : CydTheme::getCardColor(), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, CydTheme::getAccentColor(), 0);
    lv_obj_set_style_border_width(row, item.isPlaying ? 1 : 0, 0);
    lv_obj_set_style_radius(row, 4, 0);
    lv_obj_set_style_pad_all(row, 2, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(row, LV_OBJ_FLAG_SCROLL_CHAIN_VER);
    lv_obj_add_flag(row, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_user_data(row, (void*)(intptr_t)trackIndex);
    lv_obj_add_event_cb(row, playlist_item_click_cb, LV_EVENT_SHORT_CLICKED, this);

    // 2. Track information text (Single lightweight label, strictly 1 line)
    lv_obj_t* lblInfo = lv_label_create(row);
    if (lblInfo) {
        char shortTitle[32];
        const char* title = (item.title && strlen(item.title) > 0) ? item.title : "Track";
        if (strlen(title) > 23) {
            strncpy(shortTitle, title, 20);
            shortTitle[20] = '\0';
            strcat(shortTitle, "...");
        } else {
            strncpy(shortTitle, title, sizeof(shortTitle) - 1);
            shortTitle[sizeof(shortTitle) - 1] = '\0';
        }

        char infoBuf[64];
        const char* icon = item.isPlaying ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_PLAY;
        snprintf(infoBuf, sizeof(infoBuf), "%s  %s", icon, shortTitle);
        
        lv_label_set_long_mode(lblInfo, LV_LABEL_LONG_CLIP);
        lv_obj_set_size(lblInfo, 192, 16);
        lv_label_set_text(lblInfo, infoBuf);
        CydTheme::applyTextFont(lblInfo, CydTheme::getFont12(), item.isPlaying ? CydTheme::getAccentGlowColor() : CydTheme::getTextSecondary());
        lv_obj_clear_flag(lblInfo, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_align(lblInfo, LV_ALIGN_LEFT_MID, 4, 0);
    }

    // Update counter total size
    int childCount = lv_obj_get_child_cnt(playlistScrollContainer);
    char countBuf[16];
    snprintf(countBuf, sizeof(countBuf), "%d bài", childCount);
    if (lblPlaylistCount) lv_label_set_text(lblPlaylistCount, countBuf);
}

void PlayerScreen::tickSpectrumAnimation() {
    // Highly engaging dynamic sound waves simulation ticks (scaled for 80px box)
    for (int i = 0; i < 24; i++) {
        // Adjust the height smoothly using a delta
        int delta = (rand() % 20) - 10;
        barHeights[i] += delta;
        
        // Boundaries checks
        if (barHeights[i] < 4) barHeights[i] = 4;
        if (barHeights[i] > 72) barHeights[i] = 72;

        // Apply visual size transformations
        if (spectrumBars[i]) {
            lv_obj_set_height(spectrumBars[i], barHeights[i]);
            lv_obj_set_y(spectrumBars[i], 80 - barHeights[i]);
        }
    }
}

void PlayerScreen::syncCurrentTrackUI() {
    AudioTrack cur = AudioPlayerService::getCurrentTrack();
    int currentIdx = AudioPlayerService::getCurrentTrackIndex();
    bool isPlaying = AudioPlayerService::isPlaying();

    updateTrackInfo(cur.title, cur.artist, "SD Music", cur.format);
    updatePlaybackProgress(AudioPlayerService::getCurrentElapsedSec(), cur.durationSec);
    setPlayState(isPlaying);

    // Update active highlight in playlist
    if (playlistScrollContainer) {
        uint32_t count = lv_obj_get_child_cnt(playlistScrollContainer);
        for (uint32_t i = 0; i < count; i++) {
            lv_obj_t* row = lv_obj_get_child(playlistScrollContainer, i);
            if (!row) continue;
            int rowIdx = (int)(intptr_t)lv_obj_get_user_data(row);
            bool isActive = (rowIdx == currentIdx);

            lv_obj_set_style_bg_color(row, isActive ? CydTheme::getCardBorderColor() : CydTheme::getCardColor(), 0);
            lv_obj_set_style_border_color(row, CydTheme::getAccentColor(), 0);
            lv_obj_set_style_border_width(row, isActive ? 1 : 0, 0);

            // Update label inside row
            lv_obj_t* lbl = lv_obj_get_child(row, 0);
            if (lbl) {
                const AudioTrack* t = AudioPlayerService::getTrack(rowIdx);
                if (t) {
                    char shortTitle[32];
                    const char* title = (strlen(t->title) > 0) ? t->title : "Track";
                    if (strlen(title) > 23) {
                        strncpy(shortTitle, title, 20);
                        shortTitle[20] = '\0';
                        strcat(shortTitle, "...");
                    } else {
                        strncpy(shortTitle, title, sizeof(shortTitle) - 1);
                        shortTitle[sizeof(shortTitle) - 1] = '\0';
                    }

                    char infoBuf[64];
                    const char* icon = isActive ? (isPlaying ? LV_SYMBOL_VOLUME_MAX : LV_SYMBOL_PAUSE) : LV_SYMBOL_PLAY;
                    snprintf(infoBuf, sizeof(infoBuf), "%s  %s", icon, shortTitle);
                    lv_label_set_text(lbl, infoBuf);
                    CydTheme::applyTextFont(lbl, CydTheme::getFont12(), isActive ? CydTheme::getAccentGlowColor() : CydTheme::getTextSecondary());
                }
            }
        }
    }
}

// -------------------------------------------------------------
// EVENT CALLBACKS
// -------------------------------------------------------------
void PlayerScreen::play_pause_click_cb(lv_event_t* e) {
    Serial.println("[PlayerUI] Play/Pause clicked");
    AudioPlayerService::togglePlay();
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->syncCurrentTrackUI();
    }
}

void PlayerScreen::prev_click_cb(lv_event_t* e) {
    Serial.println("[PlayerUI] Prev clicked");
    AudioPlayerService::prev();
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->syncCurrentTrackUI();
    }
}

void PlayerScreen::next_click_cb(lv_event_t* e) {
    Serial.println("[PlayerUI] Next clicked");
    AudioPlayerService::next();
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->syncCurrentTrackUI();
    }
}

void PlayerScreen::shuffle_click_cb(lv_event_t* e) {
    bool newShuf = !AudioPlayerService::isShuffle();
    AudioPlayerService::setShuffle(newShuf);
    Serial.printf("[PlayerUI] Shuffle toggled: %d\n", newShuf);
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->updatePlaybackMode(newShuf, AudioPlayerService::getRepeatMode() != REPEAT_MODE_OFF);
    }
}

void PlayerScreen::repeat_click_cb(lv_event_t* e) {
    AudioRepeatMode cur = AudioPlayerService::getRepeatMode();
    AudioRepeatMode nextMode = (cur == REPEAT_MODE_OFF) ? REPEAT_MODE_ALL : (cur == REPEAT_MODE_ALL ? REPEAT_MODE_ONE : REPEAT_MODE_OFF);
    AudioPlayerService::setRepeatMode(nextMode);
    Serial.printf("[PlayerUI] Repeat mode: %d\n", nextMode);
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->updatePlaybackMode(AudioPlayerService::isShuffle(), nextMode != REPEAT_MODE_OFF);
    }
}

void PlayerScreen::volume_slider_cb(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_current_target(e);
    int val = lv_slider_get_value(slider);
    AudioPlayerService::setVolume((uint8_t)val);
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->updateVolume(val);
    }
}

void PlayerScreen::playlist_item_click_cb(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_current_target(e);
    int trackIdx = (int)(intptr_t)lv_obj_get_user_data(target);
    Serial.printf("[PlayerUI] Playlist item clicked: track index %d\n", trackIdx);
    if (trackIdx >= 0) {
        AudioPlayerService::playTrack(trackIdx);
        PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
        if (screen) {
            screen->syncCurrentTrackUI();
        }
    }
}