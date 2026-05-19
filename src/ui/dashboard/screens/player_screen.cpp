#include "player_screen.h"
#include "../cyd_theme.h"
#include <stdio.h>
#include <stdlib.h>

PlayerScreen::PlayerScreen(lv_obj_t* parent) {
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
    }

    // 3. Build left and right layout panes
    createPlayerControlsPane(rootContainer);
    createPlaylistPane(rootContainer);
}

void PlayerScreen::createPlayerControlsPane(lv_obj_t* parent) {
    lv_obj_t* leftCard = lv_obj_create(parent);
    lv_obj_set_size(leftCard, 230, 270);
    lv_obj_align(leftCard, LV_ALIGN_TOP_LEFT, 6, 6);
    CydTheme::applyCardStyle(leftCard);

    // 1. Simulated Album Cover Container (Glowing Card Frame)
    imgAlbumCover = lv_obj_create(leftCard);
    lv_obj_set_size(imgAlbumCover, 210, 102);
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
    lv_label_set_text(lblSongTitle, "Chúng Ta Của Tương Lai");
    CydTheme::applyTextFont(lblSongTitle, CydTheme::getFont14(), CydTheme::getWhiteColor());
    lv_obj_align(lblSongTitle, LV_ALIGN_TOP_LEFT, 6, 4);
    lv_label_set_long_mode(lblSongTitle, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lblSongTitle, 140);

    lblSongArtist = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblSongArtist, "Sơn Tùng M-TP");
    CydTheme::applyTextFont(lblSongArtist, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblSongArtist, LV_ALIGN_TOP_LEFT, 6, 22);

    lblSongAlbum = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblSongAlbum, "Album: Singles");
    CydTheme::applyTextFont(lblSongAlbum, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblSongAlbum, LV_ALIGN_TOP_LEFT, 6, 38);

    lblQualityChip = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblQualityChip, "FLAC 24bit");
    CydTheme::applyTextFont(lblQualityChip, CydTheme::getFont12(), CydTheme::getSuccessColor());
    lv_obj_align(lblQualityChip, LV_ALIGN_TOP_RIGHT, -6, 4);

    // 2. 24-Bar Spectrum Visualizer (Bottom inside Cover art container)
    lv_obj_t* spectrumBox = lv_obj_create(imgAlbumCover);
    lv_obj_set_size(spectrumBox, 200, 42);
    lv_obj_align(spectrumBox, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(spectrumBox, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(spectrumBox, 0, 0);
    lv_obj_set_style_pad_all(spectrumBox, 0, 0);
    lv_obj_clear_flag(spectrumBox, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 24; i++) {
        // We create tiny vertical bars in a row
        spectrumBars[i] = lv_obj_create(spectrumBox);
        lv_obj_set_size(spectrumBars[i], 4, barHeights[i]);
        lv_obj_set_pos(spectrumBars[i], i * 8 + 4, 42 - barHeights[i]);
        lv_obj_set_style_bg_color(spectrumBars[i], CydTheme::getAccentGlowColor(), 0);
        lv_obj_set_style_bg_opa(spectrumBars[i], LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(spectrumBars[i], 0, 0);
        lv_obj_set_style_radius(spectrumBars[i], 2, 0);
        lv_obj_clear_flag(spectrumBars[i], LV_OBJ_FLAG_SCROLLABLE);
    }

    // 3. Playback progress seek bar
    seekSlider = lv_slider_create(leftCard);
    lv_obj_set_size(seekSlider, 206, 6);
    lv_obj_align(seekSlider, LV_ALIGN_TOP_MID, 0, 114);
    CydTheme::applySliderStyle(seekSlider, CydTheme::getAccentGlowColor());
    lv_slider_set_range(seekSlider, 0, 100);
    lv_slider_set_value(seekSlider, 35, LV_ANIM_OFF);

    lblCurrentTime = lv_label_create(leftCard);
    lv_label_set_text(lblCurrentTime, "01:24");
    CydTheme::applyTextFont(lblCurrentTime, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblCurrentTime, LV_ALIGN_TOP_LEFT, 4, 124);

    lblTotalTime = lv_label_create(leftCard);
    lv_label_set_text(lblTotalTime, "04:35");
    CydTheme::applyTextFont(lblTotalTime, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblTotalTime, LV_ALIGN_TOP_RIGHT, -4, 124);

    // 4. Playback Controls Buttons (Shuffle, Prev, Play, Next, Repeat)
    lv_obj_t* btnRow = lv_obj_create(leftCard);
    lv_obj_set_size(btnRow, 210, 36);
    lv_obj_align(btnRow, LV_ALIGN_TOP_MID, 0, 150);
    lv_obj_set_style_bg_opa(btnRow, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(btnRow, 0, 0);
    lv_obj_set_style_pad_all(btnRow, 0, 0);
    lv_obj_set_layout(btnRow, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(btnRow, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btnRow, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(btnRow, LV_OBJ_FLAG_SCROLLABLE);

    btnShuffle = lv_btn_create(btnRow);
    lv_obj_set_size(btnShuffle, 30, 30);
    CydTheme::applyButtonStyle(btnShuffle, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_t* lblShuf = lv_label_create(btnShuffle);
    lv_label_set_text(lblShuf, LV_SYMBOL_LOOP); // Shuffle representation
    lv_obj_center(lblShuf);

    btnPrev = lv_btn_create(btnRow);
    lv_obj_set_size(btnPrev, 30, 30);
    CydTheme::applyButtonStyle(btnPrev, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_t* lblPr = lv_label_create(btnPrev);
    lv_label_set_text(lblPr, LV_SYMBOL_PREV);
    lv_obj_center(lblPr);

    btnPlayPause = lv_btn_create(btnRow);
    lv_obj_set_size(btnPlayPause, 36, 36);
    CydTheme::applyButtonStyle(btnPlayPause, CydTheme::getAccentColor(), CydTheme::getWhiteColor());
    lblPlayPauseSymbol = lv_label_create(btnPlayPause);
    lv_label_set_text(lblPlayPauseSymbol, LV_SYMBOL_PLAY);
    lv_obj_center(lblPlayPauseSymbol);

    btnNext = lv_btn_create(btnRow);
    lv_obj_set_size(btnNext, 30, 30);
    CydTheme::applyButtonStyle(btnNext, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_t* lblNx = lv_label_create(btnNext);
    lv_label_set_text(lblNx, LV_SYMBOL_NEXT);
    lv_obj_center(lblNx);

    btnRepeat = lv_btn_create(btnRow);
    lv_obj_set_size(btnRepeat, 30, 30);
    CydTheme::applyButtonStyle(btnRepeat, CydTheme::getCardBorderColor(), CydTheme::getTextSecondary());
    lv_obj_t* lblRep = lv_label_create(btnRepeat);
    lv_label_set_text(lblRep, LV_SYMBOL_REFRESH); // Repeat representation
    lv_obj_center(lblRep);

    // 5. Volume Adjust Bar & EQ Tickers (Footer)
    lv_obj_t* volFooter = lv_obj_create(leftCard);
    lv_obj_set_size(volFooter, 210, 38);
    lv_obj_align(volFooter, LV_ALIGN_BOTTOM_MID, 0, 4);
    lv_obj_set_style_bg_opa(volFooter, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(volFooter, 0, 0);
    lv_obj_set_style_pad_all(volFooter, 0, 0);
    lv_obj_clear_flag(volFooter, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t* lblVolIcon = lv_label_create(volFooter);
    lv_label_set_text(lblVolIcon, LV_SYMBOL_VOLUME_MAX);
    CydTheme::applyTextFont(lblVolIcon, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblVolIcon, LV_ALIGN_LEFT_MID, 2, 0);

    volSlider = lv_slider_create(volFooter);
    lv_obj_set_size(volSlider, 90, 6);
    lv_obj_align(volSlider, LV_ALIGN_LEFT_MID, 24, 0);
    CydTheme::applySliderStyle(volSlider, CydTheme::getAccentGlowColor());
    lv_slider_set_range(volSlider, 0, 100);
    lv_slider_set_value(volSlider, 20, LV_ANIM_OFF);

    lblVolVal = lv_label_create(volFooter);
    lv_label_set_text(lblVolVal, "20%");
    CydTheme::applyTextFont(lblVolVal, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblVolVal, LV_ALIGN_LEFT_MID, 120, 0);

    // Active EQ label
    lblEQVal = lv_label_create(volFooter);
    lv_label_set_text(lblEQVal, "EQ: POP");
    CydTheme::applyTextFont(lblEQVal, CydTheme::getFont12(), CydTheme::getGoldColor());
    lv_obj_align(lblEQVal, LV_ALIGN_RIGHT_MID, -2, 0);
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
    lv_obj_set_size(playlistScrollContainer, 210, 216);
    lv_obj_align(playlistScrollContainer, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_opa(playlistScrollContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(playlistScrollContainer, 0, 0);
    lv_obj_set_style_pad_all(playlistScrollContainer, 0, 0);
    lv_obj_set_layout(playlistScrollContainer, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(playlistScrollContainer, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(playlistScrollContainer, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(playlistScrollContainer, 4, 0);
    lv_obj_clear_flag(playlistScrollContainer, LV_OBJ_FLAG_SCROLL_ELASTIC);
}

// --- Dynamic Setter Updates ---

void PlayerScreen::updateTrackInfo(const char* title, const char* artist, const char* album, const char* qualityStr) {
    if (lblSongTitle) lv_label_set_text(lblSongTitle, title);
    if (lblSongArtist) lv_label_set_text(lblSongArtist, artist);
    
    char buf[48];
    sprintf(buf, "Album: %s", album);
    if (lblSongAlbum) lv_label_set_text(lblSongAlbum, buf);
    
    if (lblQualityChip) lv_label_set_text(lblQualityChip, qualityStr);
}

void PlayerScreen::updatePlaybackProgress(int currentTimeSecs, int totalTimeSecs) {
    char buf[16];
    
    // Update seek slider value percentage
    if (totalTimeSecs > 0) {
        int pct = (currentTimeSecs * 100) / totalTimeSecs;
        lv_slider_set_value(seekSlider, pct, LV_ANIM_OFF);
    }

    // Format current elapsed time
    sprintf(buf, "%02d:%02d", currentTimeSecs / 60, currentTimeSecs % 60);
    lv_label_set_text(lblCurrentTime, buf);

    // Format total song duration
    sprintf(buf, "%02d:%02d", totalTimeSecs / 60, totalTimeSecs % 60);
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
    char buf[16];
    sprintf(buf, "EQ: %s", eqMode);
    if (lblEQVal) lv_label_set_text(lblEQVal, buf);
}

void PlayerScreen::clearPlaylist() {
    lv_obj_clean(playlistScrollContainer);
    if (lblPlaylistCount) lv_label_set_text(lblPlaylistCount, "0 bài");
}

void PlayerScreen::addPlaylistItem(const PlaylistItem& item) {
    // 1. Create list card row
    lv_obj_t* row = lv_obj_create(playlistScrollContainer);
    lv_obj_set_size(row, 206, 32);
    lv_obj_set_style_bg_color(row, item.isPlaying ? CydTheme::getCardBorderColor() : CydTheme::getCardColor(), 0);
    lv_obj_set_style_bg_opa(row, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(row, CydTheme::getCardBorderColor(), 0);
    lv_obj_set_style_border_width(row, item.isPlaying ? 1 : 0, 0);
    lv_obj_set_style_radius(row, 4, 0);
    lv_obj_set_style_pad_all(row, 0, 0);
    lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Music/Audio index play indicator
    lv_obj_t* lblIndex = lv_label_create(row);
    if (item.isPlaying) {
        lv_label_set_text(lblIndex, LV_SYMBOL_VOLUME_MAX);
        CydTheme::applyTextFont(lblIndex, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    } else {
        lv_label_set_text(lblIndex, LV_SYMBOL_PLAY);
        CydTheme::applyTextFont(lblIndex, CydTheme::getFont12(), CydTheme::getTextMuted());
    }
    lv_obj_align(lblIndex, LV_ALIGN_LEFT_MID, 6, 0);

    // 3. Track information text
    lv_obj_t* lblInfo = lv_label_create(row);
    char infoBuf[64];
    sprintf(infoBuf, "%s - %s", item.title, item.artist);
    lv_label_set_text(lblInfo, infoBuf);
    CydTheme::applyTextFont(lblInfo, CydTheme::getFont12(), item.isPlaying ? CydTheme::getAccentGlowColor() : CydTheme::getTextSecondary());
    lv_obj_align(lblInfo, LV_ALIGN_LEFT_MID, 22, 0);
    lv_label_set_long_mode(lblInfo, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(lblInfo, 140);

    // 4. Duration text badge
    lv_obj_t* lblDur = lv_label_create(row);
    lv_label_set_text(lblDur, item.duration);
    CydTheme::applyTextFont(lblDur, CydTheme::getFont12(), CydTheme::getTextMuted());
    lv_obj_align(lblDur, LV_ALIGN_RIGHT_MID, -6, 0);

    // Update counter total size
    int childCount = lv_obj_get_child_cnt(playlistScrollContainer);
    char countBuf[16];
    sprintf(countBuf, "%d bài", childCount);
    if (lblPlaylistCount) lv_label_set_text(lblPlaylistCount, countBuf);
}

void PlayerScreen::tickSpectrumAnimation() {
    // Highly engaging dynamic sound waves simulation ticks
    for (int i = 0; i < 24; i++) {
        // Adjust the height smoothly using a slight delta
        int delta = (rand() % 16) - 8;
        barHeights[i] += delta;
        
        // Boundaries checks
        if (barHeights[i] < 2) barHeights[i] = 2;
        if (barHeights[i] > 30) barHeights[i] = 30;

        // Apply visual size transformations
        if (spectrumBars[i]) {
            lv_obj_set_height(spectrumBars[i], barHeights[i]);
            lv_obj_set_y(spectrumBars[i], 32 - barHeights[i]);
        }
    }
}