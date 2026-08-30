#include "player_screen.h"
#include "../cyd_theme.h"
#include "../../../services/audio_player_service.h"
#include "log.h"
#include <stdio.h>
#include <stdlib.h>
#include <esp_heap_caps.h>


PlayerScreen::PlayerScreen(lv_obj_t* parent)
    : rootContainer(nullptr),
      lblSongTitle(nullptr),
      lblSongArtist(nullptr),
      lblQualityChip(nullptr),
      imgAlbumCover(nullptr),
      lblAudioCodec(nullptr),
      lblAudioSampleRate(nullptr),
      lblAudioFileSize(nullptr),
      spectrumCanvas(nullptr),
      canvasBuf(nullptr),
      seekSlider(nullptr),
      lblCurrentTime(nullptr),
      lblTotalTime(nullptr),
      btnShuffle(nullptr),
      btnPrev(nullptr),
      btnPlayPause(nullptr),
      lblPlayPauseSymbol(nullptr),
      btnNext(nullptr),
      btnRepeat(nullptr),
      lblRepeatSymbol(nullptr),
      volSlider(nullptr),
      lblVolVal(nullptr),
      lblEQVal(nullptr),
      objEQVisualizer(nullptr),
      playlistScrollContainer(nullptr),
      lblPlaylistCount(nullptr),
      isSeeking(false) {
    // 1. Create root screen container
    rootContainer = lv_obj_create(parent);
    lv_obj_set_size(rootContainer, 480, 282);
    lv_obj_set_style_bg_opa(rootContainer, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(rootContainer, 0, 0);
    lv_obj_set_style_pad_all(rootContainer, 0, 0);
    lv_obj_clear_flag(rootContainer, LV_OBJ_FLAG_SCROLLABLE);

    // 2. Khởi tạo dữ liệu Spectrum nếu được bật
#if ENABLE_SPECTRUM_CANVAS
    static lv_color_t s_spectrum_canvas_buf[200 * 24];
    canvasBuf = s_spectrum_canvas_buf;
    for (int i = 0; i < 24; i++) {
        barHeights[i] = 2 + (rand() % 10);
    }
#else
    canvasBuf = nullptr;
#endif

    // 3. Build left and right layout panes
    createPlayerControlsPane(rootContainer);
    createPlaylistPane(rootContainer);
}


PlayerScreen::~PlayerScreen() {
    if (rootContainer) {
        lv_obj_del(rootContainer);
        rootContainer = nullptr;
        spectrumCanvas = nullptr;
        lblAudioCodec = nullptr;
        lblAudioSampleRate = nullptr;
        lblAudioFileSize = nullptr;
    }
    canvasBuf = nullptr;
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
    lv_obj_set_style_bg_color(imgAlbumCover, lv_color_make(18, 12, 34), 0);
    lv_obj_set_style_bg_opa(imgAlbumCover, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(imgAlbumCover, CydTheme::getAccentColor(), 0);
    lv_obj_set_style_border_width(imgAlbumCover, 1, 0);
    lv_obj_set_style_radius(imgAlbumCover, 8, 0);
    lv_obj_set_style_pad_all(imgAlbumCover, 4, 0);
    lv_obj_clear_flag(imgAlbumCover, LV_OBJ_FLAG_SCROLLABLE);

    // Dòng 1: Tên bài hát (Font 14 Trắng, Marquee cuộn tròn nếu tên dài)
    lblSongTitle = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblSongTitle, "Chưa chọn bài");
    CydTheme::applyTextFont(lblSongTitle, CydTheme::getFont14(), CydTheme::getWhiteColor());
    lv_obj_align(lblSongTitle, LV_ALIGN_TOP_LEFT, 6, 6);
    lv_obj_set_width(lblSongTitle, 196);
    lv_label_set_long_mode(lblSongTitle, LV_LABEL_LONG_SCROLL_CIRCULAR);

    // Dòng 2: Nghệ sĩ (Chỉ hiển thị khi có tên nghệ sĩ THỰC SỰ)
    lblSongArtist = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblSongArtist, "");
    CydTheme::applyTextFont(lblSongArtist, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    lv_obj_align(lblSongArtist, LV_ALIGN_TOP_LEFT, 6, 30);

#if ENABLE_SPECTRUM_CANVAS
    // [CHẾ ĐỘ SÓNG NHẠC]: 24-Bar Spectrum Visualizer (Kích thước 200x24)
    spectrumCanvas = lv_canvas_create(imgAlbumCover);
    lv_obj_set_size(spectrumCanvas, 200, 24);
    lv_obj_align(spectrumCanvas, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(spectrumCanvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(spectrumCanvas, LV_OBJ_FLAG_CLICKABLE);

    if (canvasBuf) {
        lv_canvas_set_buffer(spectrumCanvas, canvasBuf, 200, 24, LV_IMG_CF_TRUE_COLOR);
        lv_canvas_fill_bg(spectrumCanvas, lv_color_make(18, 12, 34), LV_OPA_COVER);
        lv_draw_rect_dsc_t barDsc;
        lv_draw_rect_dsc_init(&barDsc);
        barDsc.bg_color  = CydTheme::getAccentGlowColor();
        barDsc.bg_opa    = LV_OPA_COVER;
        barDsc.radius    = 2;
        barDsc.border_width = 0;
        for (int i = 0; i < 24; i++) {
            lv_canvas_draw_rect(spectrumCanvas, i * 8 + 4, 24 - barHeights[i], 5, barHeights[i], &barDsc);
        }
    } else {
        spectrumCanvas = nullptr;
    }
#else
    // [CHẾ ĐỘ THÔNG SỐ TEXT THẬT 100%]: Tinh gọn, không chữ thừa, không fake
    // Dòng 3: Định dạng chuẩn
    lblAudioCodec = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblAudioCodec, "Định dạng: MP3 (MPEG Layer 3)");
    CydTheme::applyTextFont(lblAudioCodec, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblAudioCodec, LV_ALIGN_TOP_LEFT, 6, 54);

    // Dòng 4: Tần số lấy mẫu & Kênh âm thanh
    lblAudioSampleRate = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblAudioSampleRate, "Tần số: 44.1 kHz • Stereo 16-bit");
    CydTheme::applyTextFont(lblAudioSampleRate, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblAudioSampleRate, LV_ALIGN_TOP_LEFT, 6, 78);

    // Dòng 5: Dung lượng file thực tế trên thẻ nhớ SD
    lblAudioFileSize = lv_label_create(imgAlbumCover);
    lv_label_set_text(lblAudioFileSize, "Dung lượng: -- MB");
    CydTheme::applyTextFont(lblAudioFileSize, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_align(lblAudioFileSize, LV_ALIGN_TOP_LEFT, 6, 102);
#endif



    // 3. Playback progress seek bar (Shifted up with comfortable margin)
    seekSlider = lv_slider_create(leftCard);
    lv_obj_set_size(seekSlider, 206, 8);
    lv_obj_align(seekSlider, LV_ALIGN_BOTTOM_MID, 0, -90);
    CydTheme::applySliderStyle(seekSlider, CydTheme::getAccentGlowColor());
    lv_slider_set_range(seekSlider, 0, 100);
    lv_slider_set_value(seekSlider, 0, LV_ANIM_OFF);
    lv_obj_add_event_cb(seekSlider, seek_slider_cb, LV_EVENT_ALL, this);

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
    lblRepeatSymbol = lv_label_create(btnRepeat);
    lv_label_set_text(lblRepeatSymbol, LV_SYMBOL_REFRESH); // Repeat representation
    CydTheme::applyTextFont(lblRepeatSymbol, CydTheme::getFont12(), CydTheme::getTextSecondary());
    lv_obj_clear_flag(lblRepeatSymbol, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_center(lblRepeatSymbol);
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
        lv_label_set_text(lblSongTitle, title);
    }
    if (lblSongArtist) {
        if (artist && strlen(artist) > 0 && strcmp(artist, "SD Music") != 0 && strcmp(artist, "SD Card") != 0) {
            char buf[48];
            snprintf(buf, sizeof(buf), "Nghệ sĩ: %s", artist);
            lv_label_set_text(lblSongArtist, buf);
        } else {
            lv_label_set_text(lblSongArtist, "");
        }
    }
    
    if (lblQualityChip) lv_label_set_text(lblQualityChip, qualityStr);

#if !ENABLE_SPECTRUM_CANVAS
    if (lblAudioCodec) {
        AudioTrack cur = AudioPlayerService::getCurrentTrack();
        char buf[48];
        const char* fmt = (strlen(cur.format) > 0) ? cur.format : ((qualityStr && strlen(qualityStr) > 0) ? qualityStr : "MP3");
        int br = (cur.bitrateKbps > 0) ? cur.bitrateKbps : 128;
        snprintf(buf, sizeof(buf), "Định dạng: %s • %d kbps", fmt, br);
        lv_label_set_text(lblAudioCodec, buf);
    }

    if (lblAudioFileSize) {
        AudioTrack cur = AudioPlayerService::getCurrentTrack();
        if (cur.fileSize > 0) {
            char buf[36];
            if (cur.fileSize >= 1048576) {
                float mb = (float)cur.fileSize / (1024.0f * 1024.0f);
                snprintf(buf, sizeof(buf), "Dung lượng: %.1f MB", mb);
            } else {
                snprintf(buf, sizeof(buf), "Dung lượng: %u KB", cur.fileSize / 1024);
            }
            lv_label_set_text(lblAudioFileSize, buf);
        } else {
            lv_label_set_text(lblAudioFileSize, "Dung lượng: --");
        }
    }
#endif
}

void PlayerScreen::updatePlaybackProgress(int currentTimeSecs, int totalTimeSecs) {
    char buf[16];
    
    // Update seek slider value percentage (only if user is not actively scrubbing)
    if (!isSeeking && seekSlider && !lv_obj_has_state(seekSlider, LV_STATE_PRESSED)) {
        if (totalTimeSecs > 0) {
            int pct = (currentTimeSecs * 100) / totalTimeSecs;
            lv_slider_set_value(seekSlider, pct, LV_ANIM_OFF);
        } else {
            lv_slider_set_value(seekSlider, 0, LV_ANIM_OFF);
        }
    }

    // Format current elapsed time (HH:mm:ss if >= 3600s, else mm:ss)
    if (!isSeeking && lblCurrentTime) {
        if (currentTimeSecs >= 3600 || totalTimeSecs >= 3600) {
            int h = currentTimeSecs / 3600;
            int m = (currentTimeSecs % 3600) / 60;
            int s = currentTimeSecs % 60;
            snprintf(buf, sizeof(buf), "%02d:%02d:%02d", h, m, s);
        } else {
            snprintf(buf, sizeof(buf), "%02d:%02d", currentTimeSecs / 60, currentTimeSecs % 60);
        }
        lv_label_set_text(lblCurrentTime, buf);
    }

    // Format total song duration (HH:mm:ss if >= 3600s, else mm:ss)
    if (lblTotalTime) {
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
}

void PlayerScreen::setPlayState(bool isPlaying) {
    if (lblPlayPauseSymbol) {
        lv_label_set_text(lblPlayPauseSymbol, isPlaying ? LV_SYMBOL_PAUSE : LV_SYMBOL_PLAY);
    }
}

void PlayerScreen::updatePlaybackMode(bool shuffleActive, int repeatMode) {
    if (shuffleActive) {
        lv_obj_set_style_bg_color(btnShuffle, CydTheme::getAccentColor(), 0);
        lv_obj_set_style_text_color(btnShuffle, CydTheme::getWhiteColor(), 0);
    } else {
        lv_obj_set_style_bg_color(btnShuffle, CydTheme::getCardBorderColor(), 0);
        lv_obj_set_style_text_color(btnShuffle, CydTheme::getTextSecondary(), 0);
    }

    if (lblRepeatSymbol && btnRepeat) {
        if (repeatMode == 0) { // REPEAT_MODE_OFF (Tắt lặp lại)
            lv_obj_set_style_bg_color(btnRepeat, CydTheme::getCardBorderColor(), 0);
            lv_obj_set_style_text_color(btnRepeat, CydTheme::getTextSecondary(), 0);
            lv_label_set_text(lblRepeatSymbol, LV_SYMBOL_REFRESH);
            CydTheme::applyTextFont(lblRepeatSymbol, CydTheme::getFont12(), CydTheme::getTextSecondary());
        } else if (repeatMode == 1) { // REPEAT_MODE_ALL (Lặp lại tất cả - Nút tím)
            lv_obj_set_style_bg_color(btnRepeat, CydTheme::getAccentColor(), 0);
            lv_obj_set_style_text_color(btnRepeat, CydTheme::getWhiteColor(), 0);
            lv_label_set_text(lblRepeatSymbol, LV_SYMBOL_REFRESH);
            CydTheme::applyTextFont(lblRepeatSymbol, CydTheme::getFont12(), CydTheme::getWhiteColor());
        } else if (repeatMode == 2) { // REPEAT_MODE_ONE (Lặp lại 1 bài - Nút vàng số 1)
            lv_obj_set_style_bg_color(btnRepeat, CydTheme::getGoldColor(), 0);
            lv_obj_set_style_text_color(btnRepeat, lv_color_make(18, 12, 34), 0);
            lv_label_set_text(lblRepeatSymbol, "1");
            CydTheme::applyTextFont(lblRepeatSymbol, CydTheme::getFont14(), lv_color_make(18, 12, 34));
        }
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
    
    // Bảo vệ OOM: Nếu free heap < 25KB, không tạo thêm widget để tránh crash hệ thống
    if (ESP.getFreeHeap() < 25000) {
        return;
    }

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
        if (trackIndex >= 0) {
            snprintf(infoBuf, sizeof(infoBuf), "%s %d. %s", icon, trackIndex + 1, shortTitle);
        } else {
            snprintf(infoBuf, sizeof(infoBuf), "%s  %s", icon, shortTitle);
        }
        
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
#if !ENABLE_SPECTRUM_CANVAS
    return; // Đã tắt sóng nhạc -> Bỏ qua 100% render load, FPS tối đa 60 mượt mà
#else
    if (!spectrumCanvas) return;

    bool isPlaying = AudioPlayerService::isPlaying();
    static int targetHeights[24] = {};
    static int frameCount = 0;
    static bool wasIdle = false;

    if (!isPlaying) {
        bool allFlat = true;
        for (int i = 0; i < 24; i++) {
            targetHeights[i] = 2;
            if (barHeights[i] > 2) {
                barHeights[i] -= 1;
                allFlat = false;
            }
        }
        if (allFlat) {
            if (wasIdle) return; // Đã phẳng hoàn toàn, không cần vẽ lại
            wasIdle = true;
        }
    } else {
        wasIdle = false;
        // Đặt target mới mỗi 3 ticks (~75ms) để animation nhịp nhàng theo nhạc
        if ((frameCount % 3) == 0) {
            for (int i = 0; i < 24; i++) {
                int center = 12;
                int distFromCenter = abs(i - center);
                int maxH = 22 - distFromCenter * 1;
                if (maxH < 6) maxH = 6;
                targetHeights[i] = 3 + (rand() % maxH);
            }
        }
        frameCount++;

        // Smooth lerp: di chuyển barHeights về target (tốc độ 40%)
        for (int i = 0; i < 24; i++) {
            int diff = targetHeights[i] - barHeights[i];
            barHeights[i] += diff * 2 / 5;
            if (barHeights[i] < 2)  barHeights[i] = 2;
            if (barHeights[i] > 22) barHeights[i] = 22;
        }
    }

    // --- Vẽ lại toàn bộ canvas trong 1 lần invalidate duy nhất ---
    lv_canvas_fill_bg(spectrumCanvas, lv_color_make(18, 12, 34), LV_OPA_COVER);

    lv_draw_rect_dsc_t barDsc;
    lv_draw_rect_dsc_init(&barDsc);
    barDsc.bg_opa       = LV_OPA_COVER;
    barDsc.radius       = 2;
    barDsc.border_width = 0;

    for (int i = 0; i < 24; i++) {
        // Gradient màu theo chiều cao: thấp = teal, cao = cyan sáng
        uint8_t t = (uint8_t)((barHeights[i] * 255) / 22);
        barDsc.bg_color = lv_color_make(
            0,
            (uint8_t)(180 + (t * 75) / 255),  // G: 180→255
            (uint8_t)(200 + (t * 55) / 255)   // B: 200→255
        );

        lv_coord_t x = (lv_coord_t)(i * 8 + 4);
        lv_coord_t y = (lv_coord_t)(24 - barHeights[i]);
        lv_canvas_draw_rect(spectrumCanvas, x, y, 5, barHeights[i], &barDsc);
    }
#endif
}



void PlayerScreen::syncCurrentTrackUI() {
    AudioTrack cur = AudioPlayerService::getCurrentTrack();
    int currentIdx = AudioPlayerService::getCurrentTrackIndex();
    int totalCount = AudioPlayerService::getTrackCount();
    bool isPlaying = AudioPlayerService::isPlaying();

    updateTrackInfo(cur.title, cur.artist, "SD Music", cur.format);
    updatePlaybackProgress(AudioPlayerService::getCurrentElapsedSec(), cur.durationSec);
    setPlayState(isPlaying);

    // Cập nhật số thứ tự bài trên tiêu đề Playlist: "Bài 5 / 21"
    if (lblPlaylistCount && totalCount > 0) {
        char countBuf[24];
        snprintf(countBuf, sizeof(countBuf), "Bài %d / %d", (currentIdx >= 0) ? (currentIdx + 1) : 1, totalCount);
        lv_label_set_text(lblPlaylistCount, countBuf);
        CydTheme::applyTextFont(lblPlaylistCount, CydTheme::getFont12(), CydTheme::getAccentGlowColor());
    }

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
                    snprintf(infoBuf, sizeof(infoBuf), "%s %d. %s", icon, rowIdx + 1, shortTitle);
                    lv_label_set_text(lbl, infoBuf);
                    CydTheme::applyTextFont(lbl, CydTheme::getFont12(), isActive ? CydTheme::getAccentGlowColor() : CydTheme::getTextSecondary());
                }
            }

            // Tự động cuộn danh sách đến đúng bài đang phát để người dùng luôn nhìn thấy
            if (isActive) {
                lv_obj_scroll_to_view(row, LV_ANIM_ON);
            }
        }
    }
}

// -------------------------------------------------------------
// EVENT CALLBACKS
// -------------------------------------------------------------
void PlayerScreen::play_pause_click_cb(lv_event_t* e) {
    LOG_D("PlayerUI", "Play/Pause clicked");
    AudioPlayerService::togglePlay();
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->syncCurrentTrackUI();
    }
}

void PlayerScreen::prev_click_cb(lv_event_t* e) {
    LOG_D("PlayerUI", "Prev clicked");
    AudioPlayerService::prev();
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->syncCurrentTrackUI();
    }
}

void PlayerScreen::next_click_cb(lv_event_t* e) {
    LOG_D("PlayerUI", "Next clicked");
    AudioPlayerService::next();
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->syncCurrentTrackUI();
    }
}

void PlayerScreen::shuffle_click_cb(lv_event_t* e) {
    bool newShuf = !AudioPlayerService::isShuffle();
    AudioPlayerService::setShuffle(newShuf);
    LOG_D("PlayerUI", "Shuffle toggled: %d", newShuf);
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->updatePlaybackMode(newShuf, (int)AudioPlayerService::getRepeatMode());
    }
}

void PlayerScreen::repeat_click_cb(lv_event_t* e) {
    AudioRepeatMode cur = AudioPlayerService::getRepeatMode();
    AudioRepeatMode nextMode = (cur == REPEAT_MODE_OFF) ? REPEAT_MODE_ALL : (cur == REPEAT_MODE_ALL ? REPEAT_MODE_ONE : REPEAT_MODE_OFF);
    AudioPlayerService::setRepeatMode(nextMode);
    LOG_D("PlayerUI", "Repeat mode: %d", nextMode);
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    if (screen) {
        screen->updatePlaybackMode(AudioPlayerService::isShuffle(), (int)nextMode);
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

void PlayerScreen::seek_slider_cb(lv_event_t* e) {
    lv_event_code_t code = lv_event_get_code(e);
    PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
    lv_obj_t* slider = lv_event_get_current_target(e);
    if (!screen || !slider) return;

    if (code == LV_EVENT_PRESSED) {
        screen->isSeeking = true;
    } else if (code == LV_EVENT_VALUE_CHANGED) {
        // User đang kéo seek bar -> Cập nhật nhãn thời gian tức thì để xem trước vị trí tua
        int pct = lv_slider_get_value(slider);
        int totalSec = AudioPlayerService::getCurrentTotalSec();
        if (totalSec > 0 && screen->lblCurrentTime) {
            int previewSec = (int)(((int64_t)totalSec * pct) / 100);
            char buf[16];
            if (totalSec >= 3600) {
                snprintf(buf, sizeof(buf), "%02d:%02d:%02d", previewSec / 3600, (previewSec % 3600) / 60, previewSec % 60);
            } else {
                snprintf(buf, sizeof(buf), "%02d:%02d", previewSec / 60, previewSec % 60);
            }
            lv_label_set_text(screen->lblCurrentTime, buf);
        }
    } else if (code == LV_EVENT_RELEASED) {
        int pct = lv_slider_get_value(slider);
        LOG_D("PlayerUI", "Seek slider released at %d%%", pct);
        AudioPlayerService::seekToPercent(pct);
        screen->isSeeking = false;
        int elapsed = AudioPlayerService::getCurrentElapsedSec();
        int total = AudioPlayerService::getCurrentTotalSec();
        screen->updatePlaybackProgress(elapsed, total);
    }
}

void PlayerScreen::playlist_item_click_cb(lv_event_t* e) {
    lv_obj_t* target = lv_event_get_current_target(e);
    int trackIdx = (int)(intptr_t)lv_obj_get_user_data(target);
    LOG_D("PlayerUI", "Playlist item clicked: track index %d", trackIdx);
    if (trackIdx >= 0) {
        AudioPlayerService::playTrack(trackIdx);
        PlayerScreen* screen = (PlayerScreen*)lv_event_get_user_data(e);
        if (screen) {
            screen->syncCurrentTrackUI();
        }
    }
}