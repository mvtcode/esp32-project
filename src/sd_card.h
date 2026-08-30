#pragma once
#ifndef SD_CARD_H
#define SD_CARD_H

#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>

// ---------------------------------------------------------------------------
// Hardware Pinout for MicroSD Card SPI Bus
// ---------------------------------------------------------------------------
#define SD_PIN_CS     5
#define SD_PIN_SCK    16
#define SD_PIN_MOSI   17
#define SD_PIN_MISO   35

#define MAX_PLAYLIST_TRACKS 100
#define TRACK_PATH_MAX_LEN   128
#define TRACK_TITLE_MAX_LEN  96

struct PlaylistItem {
    char path[TRACK_PATH_MAX_LEN];   // Full path on SD card (e.g. /music/track.mp3)
    char title[TRACK_TITLE_MAX_LEN]; // Sanitized display title for OLED UI
};

/**
 * @brief Initialize SPI bus and mount MicroSD Card.
 * @return true if mounted successfully, false otherwise.
 */
bool sd_card_init();

/**
 * @brief Unmount SD card and release SPI bus.
 */
void sd_card_deinit();

/**
 * @brief Free dynamically allocated playlist items.
 */
void sd_card_free_playlist();

/**
 * @brief Check if SD card is mounted and ready.
 */
bool sd_card_is_mounted();

/**
 * @brief Scan the SD card recursively for supported audio files (.mp3, .wav).
 * @return Number of tracks found and cached (capped at MAX_PLAYLIST_TRACKS).
 */
int sd_card_scan_tracks();

/**
 * @brief Get total number of tracks currently loaded in playlist.
 */
int sd_card_get_track_count();

/**
 * @brief Get track item at specified index (0 to track_count - 1).
 */
const PlaylistItem* sd_card_get_track(int index);

/**
 * @brief Sanitize a filename or path: strips path/extension, removes Vietnamese
 * diacritics, removes non-printable chars, and truncates if necessary.
 */
void sd_sanitize_title(const char *raw_name, char *out_title, size_t max_len);

/**
 * @brief Validate audio file integrity & protect against fake MP3/WAV files
 * (e.g. M4A/MP4, FLAC, OGG, corrupted headers, or missing MP3 sync frames).
 * @param file Open File handle to check.
 * @param path File path or filename for extension context.
 * @return true if valid audio file, false if fake or corrupt.
 */
bool sd_card_validate_file(File &file, const char *path);

/**
 * @brief Access to global SPIClass instance for SD.
 */
extern SPIClass g_spiSD;

#endif // SD_CARD_H
