#include "sd_card.h"
#include "log.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

SPIClass g_spiSD(HSPI);

static bool s_sd_mounted = false;
static PlaylistItem *s_playlist[MAX_PLAYLIST_TRACKS] = {nullptr};
static int s_track_count = 0;

// ---------------------------------------------------------------------------
// Vietnamese Diacritics Sanitizer to ASCII
// ---------------------------------------------------------------------------
void sd_sanitize_title(const char *raw_name, char *out_title, size_t max_len) {
    if (!raw_name || !out_title || max_len == 0) return;

    // 1. Extract only filename (strip directory path)
    const char *p = raw_name;
    const char *last_slash = strrchr(p, '/');
    if (last_slash) {
        p = last_slash + 1;
    }

    // 2. Keep full filename including extension (.mp3, .wav, etc.)
    size_t name_len = strlen(p);

    // 3. Process characters with UTF-8 decoding for Vietnamese diacritics
    char temp[128];
    size_t out_idx = 0;
    size_t in_idx = 0;

    while (in_idx < name_len && out_idx < sizeof(temp) - 1) {
        unsigned char c = (unsigned char)p[in_idx];

        if (c < 0x80) {
            // Standard ASCII
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                c == ' ' || c == '-' || c == '_' || c == '(' || c == ')' || c == '[' || c == ']' ||
                c == '.' || c == '&' || c == '+' || c == ',') {
                temp[out_idx++] = (char)c;
            } else if (c == '\t' || c == '\r' || c == '\n') {
                temp[out_idx++] = ' ';
            } else {
                temp[out_idx++] = ' ';
            }
            in_idx++;
        } else if ((c & 0xE0) == 0xC0) {
            // 2-byte UTF-8
            if (in_idx + 1 < name_len) {
                unsigned char c2 = (unsigned char)p[in_idx + 1];
                uint16_t code = ((uint16_t)c << 8) | c2;

                char rep = ' ';
                switch (code) {
                    // a / A
                    case 0xC3A1: case 0xC3A0: case 0xC3A3: case 0xC3A2: case 0xC483: rep = 'a'; break;
                    case 0xC381: case 0xC380: case 0xC383: case 0xC382: case 0xC482: rep = 'A'; break;
                    // d / D
                    case 0xC491: rep = 'd'; break;
                    case 0xC490: rep = 'D'; break;
                    // e / E
                    case 0xC3A9: case 0xC3A8: case 0xC3AA: rep = 'e'; break;
                    case 0xC389: case 0xC388: case 0xC38A: rep = 'E'; break;
                    // i / I
                    case 0xC3AD: case 0xC3AC: rep = 'i'; break;
                    case 0xC38D: case 0xC38C: rep = 'I'; break;
                    // o / O
                    case 0xC3B3: case 0xC3B2: case 0xC3B5: case 0xC3B4: case 0xC6A1: rep = 'o'; break;
                    case 0xC393: case 0xC392: case 0xC395: case 0xC394: case 0xC6A0: rep = 'O'; break;
                    // u / U
                    case 0xC3BA: case 0xC3B9: case 0xC3BC: case 0xC6B0: rep = 'u'; break;
                    case 0xC39A: case 0xC399: case 0xC39C: case 0xC6AF: rep = 'U'; break;
                    // y / Y
                    case 0xC3BD: rep = 'y'; break;
                    case 0xC39D: rep = 'Y'; break;
                    default: rep = ' '; break;
                }
                if (rep != ' ') temp[out_idx++] = rep;
                in_idx += 2;
            } else {
                in_idx++;
            }
        } else if ((c & 0xF0) == 0xE0) {
            // 3-byte UTF-8 (Vietnamese tone marks in U+1EA0..U+1EF9)
            if (in_idx + 2 < name_len) {
                unsigned char c2 = (unsigned char)p[in_idx + 1];
                unsigned char c3 = (unsigned char)p[in_idx + 2];
                uint32_t code = ((uint32_t)c << 16) | ((uint32_t)c2 << 8) | c3;

                char rep = ' ';
                // Range for Vietnamese diacritics under 0xE1BAB0 - 0xE1BFBF
                if (c == 0xE1 && (c2 == 0xBA || c2 == 0xBB)) {
                    if (c2 == 0xBA) {
                        if (c3 >= 0x80 && c3 <= 0x9B) {
                            rep = (c3 % 2 == 0) ? 'A' : 'a';
                        } else if (c3 >= 0x9C && c3 <= 0xB9) {
                            rep = (c3 % 2 == 0) ? 'E' : 'e';
                        } else if (c3 >= 0xBA && c3 <= 0xBF) {
                            rep = (c3 % 2 == 0) ? 'I' : 'i';
                        }
                    } else if (c2 == 0xBB) {
                        if (c3 >= 0x80 && c3 <= 0x99) {
                            rep = (c3 % 2 == 0) ? 'O' : 'o';
                        } else if (c3 >= 0x9A && c3 <= 0xB1) {
                            rep = (c3 % 2 == 0) ? 'U' : 'u';
                        } else if (c3 >= 0xB2 && c3 <= 0xB9) {
                            rep = (c3 % 2 == 0) ? 'Y' : 'y';
                        }
                    }
                }
                if (rep != ' ') temp[out_idx++] = rep;
                in_idx += 3;
            } else {
                in_idx++;
            }
        } else {
            // Skip 4-byte UTF8 or invalid
            in_idx++;
        }
    }

    temp[out_idx] = '\0';

    // 4. Remove consecutive duplicate spaces and trim leading/trailing spaces
    char cleaned[128];
    size_t c_idx = 0;
    bool prev_space = true; // suppress leading spaces
    for (size_t i = 0; i < out_idx; i++) {
        char ch = temp[i];
        if (ch == ' ') {
            if (!prev_space) {
                cleaned[c_idx++] = ' ';
                prev_space = true;
            }
        } else {
            cleaned[c_idx++] = ch;
            prev_space = false;
        }
    }
    // Remove trailing space
    if (c_idx > 0 && cleaned[c_idx - 1] == ' ') {
        c_idx--;
    }
    cleaned[c_idx] = '\0';

    // 5. Copy full title without truncating
    strncpy(out_title, cleaned, max_len - 1);
    out_title[max_len - 1] = '\0';

    // Fallback if title becomes empty
    if (strlen(out_title) == 0) {
        strncpy(out_title, "Track.mp3", max_len - 1);
        out_title[max_len - 1] = '\0';
    }
}

static int s_playlist_capacity = 0;

// ---------------------------------------------------------------------------
// SD Card Init & Deinit
// ---------------------------------------------------------------------------
bool sd_card_init() {
    if (s_sd_mounted) return true;

    LOG_I("SD", "Initializing SPI bus for MicroSD: CS=%d, SCK=%d, MOSI=%d, MISO=%d",
          SD_PIN_CS, SD_PIN_SCK, SD_PIN_MOSI, SD_PIN_MISO);

    pinMode(SD_PIN_CS, OUTPUT);
    digitalWrite(SD_PIN_CS, HIGH);
    delay(30);

    static bool s_spi_inited = false;
    if (!s_spi_inited) {
        g_spiSD.begin(SD_PIN_SCK, SD_PIN_MISO, SD_PIN_MOSI, SD_PIN_CS);
        s_spi_inited = true;
    }

    // Multi-speed mounting strategy: 20MHz -> 10MHz -> 4MHz (clean SD.begin without raw transfer collisions)
    bool mounted = SD.begin(SD_PIN_CS, g_spiSD, 20000000);
    if (!mounted) {
        LOG_W("SD", "SD mount at 20MHz failed, retrying at 10MHz...");
        delay(40);
        mounted = SD.begin(SD_PIN_CS, g_spiSD, 10000000);
    }
    if (!mounted) {
        LOG_W("SD", "SD mount at 10MHz failed, retrying at 4MHz...");
        delay(40);
        mounted = SD.begin(SD_PIN_CS, g_spiSD, 4000000);
    }

    if (!mounted) {
        LOG_E("SD", "MicroSD Card mount failed!");
        s_sd_mounted = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        LOG_E("SD", "No SD card attached!");
        SD.end();
        s_sd_mounted = false;
        return false;
    }

    s_sd_mounted = true;
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    LOG_I("SD", "MicroSD Card mounted successfully! Size: %llu MB", cardSize);
    return true;
}

void sd_card_free_playlist() {
    for (int i = 0; i < MAX_PLAYLIST_TRACKS; i++) {
        if (s_playlist[i]) {
            free(s_playlist[i]);
            s_playlist[i] = nullptr;
        }
    }
    s_track_count = 0;
}

void sd_card_deinit() {
    if (!s_sd_mounted) return;
    sd_card_free_playlist();
    SD.end();
    s_sd_mounted = false;
    LOG_I("SD", "MicroSD Card unmounted to free RAM.");
}

bool sd_card_is_mounted() {
    return s_sd_mounted;
}

// ---------------------------------------------------------------------------
// Recursive Audio File Scanning & Anti-Fake MP3 Validation
// ---------------------------------------------------------------------------
static bool is_supported_audio(const char *name) {
    if (!name || name[0] == '.') return false; // Ignore hidden files / Apple double ._*

    const char *dot = strrchr(name, '.');
    if (!dot) return false;

    if (strcasecmp(dot, ".mp3") == 0 || strcasecmp(dot, ".wav") == 0) {
        return true;
    }
    return false;
}

bool sd_card_validate_file(File &file, const char *path) {
    if (!file) return false;
    uint32_t fsize = file.size();
    if (fsize < 4096) {
        LOG_W("SD", "REJECTED: '%s' too small (%u bytes, min 4KB).", path, fsize);
        return false;
    }

    file.seek(0);
    uint8_t magicBuf[16];
    size_t read_bytes = file.read(magicBuf, sizeof(magicBuf));
    if (read_bytes < 16) {
        LOG_W("SD", "REJECTED: '%s' unable to read 16-byte header.", path);
        return false;
    }

    // 1. Kiểm tra Magic Bytes chống file FAKE (M4A, MP4, FLAC, OGG, RIFF đổi đuôi thành .mp3)
    // Kiểm tra MP4 / M4A container ('ftyp' tại offset 4..7)
    if (magicBuf[4] == 'f' && magicBuf[5] == 't' && magicBuf[6] == 'y' && magicBuf[7] == 'p') {
        LOG_W("SD", "REJECTED: '%s' is an MP4/M4A file (fake .mp3 extension).", path);
        return false;
    }
    // Kiểm tra FLAC container ('fLaC')
    if (magicBuf[0] == 'f' && magicBuf[1] == 'L' && magicBuf[2] == 'a' && magicBuf[3] == 'C') {
        LOG_W("SD", "REJECTED: '%s' is a FLAC file (fake .mp3 extension).", path);
        return false;
    }
    // Kiểm tra OGG container ('OggS')
    if (magicBuf[0] == 'O' && magicBuf[1] == 'g' && magicBuf[2] == 'g' && magicBuf[3] == 'S') {
        LOG_W("SD", "REJECTED: '%s' is an OGG file (fake .mp3 extension).", path);
        return false;
    }

    const char *dot = strrchr(path, '.');
    if (!dot) return false;

    if (strcasecmp(dot, ".wav") == 0) {
        if (magicBuf[0] == 'R' && magicBuf[1] == 'I' && magicBuf[2] == 'F' && magicBuf[3] == 'F' &&
            magicBuf[8] == 'W' && magicBuf[9] == 'A' && magicBuf[10] == 'V' && magicBuf[11] == 'E') {
            return true;
        }
        LOG_W("SD", "REJECTED: '%s' is not a valid RIFF/WAVE audio file.", path);
        return false;
    }

    if (strcasecmp(dot, ".mp3") == 0) {
        uint32_t startOffset = 0;

        // 2. Bỏ qua ID3v2 metadata nếu có
        if (magicBuf[0] == 'I' && magicBuf[1] == 'D' && magicBuf[2] == '3') {
            uint32_t id3Size = ((uint32_t)(magicBuf[6] & 0x7F) << 21) |
                               ((uint32_t)(magicBuf[7] & 0x7F) << 14) |
                               ((uint32_t)(magicBuf[8] & 0x7F) << 7)  |
                               ((uint32_t)(magicBuf[9] & 0x7F));
            startOffset = 10 + id3Size;
            if (startOffset >= fsize) {
                LOG_W("SD", "REJECTED: '%s' has invalid ID3 size (%u >= %u).", path, startOffset, fsize);
                return false;
            }
        }

        // 3. Quét tìm MP3 Frame Sync (0xFFE0 / 0xFFF0)
        file.seek(startOffset);
        uint8_t scanBuf[512];
        size_t bytesRead = file.read(scanBuf, sizeof(scanBuf));
        if (bytesRead >= 4) {
            for (size_t i = 0; i < bytesRead - 3; i++) {
                if (scanBuf[i] == 0xFF && (scanBuf[i+1] & 0xE0) == 0xE0) {
                    uint8_t layer = (scanBuf[i+1] >> 1) & 0x03;
                    uint8_t bitrateIdx = (scanBuf[i+2] >> 4) & 0x0F;
                    uint8_t sampleIdx = (scanBuf[i+2] >> 2) & 0x03;

                    if (layer != 0 && bitrateIdx != 0 && bitrateIdx != 15 && sampleIdx != 3) {
                        return true;
                    }
                }
            }
        }
        LOG_W("SD", "REJECTED: '%s' contains no valid MP3 sync frames.", path);
        return false;
    }

    return false;
}

static void scan_directory_recursive(File dir, int depth) {
    if (depth > 6 || s_track_count >= MAX_PLAYLIST_TRACKS) return;

    File file = dir.openNextFile();
    while (file && s_track_count < MAX_PLAYLIST_TRACKS) {
        // Use file.path() if available (returns full path e.g. /Fixed/song.mp3), else fallback to file.name()
        const char *full_path = file.path();
        if (!full_path || full_path[0] == '\0') {
            full_path = file.name();
        }

        // Skip hidden files & directories
        const char *basename = strrchr(full_path, '/');
        basename = basename ? (basename + 1) : full_path;

        if (basename[0] != '.') {
            if (file.isDirectory()) {
                scan_directory_recursive(file, depth + 1);
            } else {
                if (is_supported_audio(basename)) {
                    if (sd_card_validate_file(file, basename)) {
                        if (s_track_count < MAX_PLAYLIST_TRACKS) {
                            if (s_playlist[s_track_count] == nullptr) {
                                s_playlist[s_track_count] = (PlaylistItem*)malloc(sizeof(PlaylistItem));
                            }
                            PlaylistItem *item = s_playlist[s_track_count];
                            if (item) {
                                // Ensure full path has leading slash
                                if (full_path[0] == '/') {
                                    strncpy(item->path, full_path, sizeof(item->path) - 1);
                                } else {
                                    snprintf(item->path, sizeof(item->path), "/%s", full_path);
                                }
                                item->path[sizeof(item->path) - 1] = '\0';

                                // Generate clean display title
                                sd_sanitize_title(basename, item->title, sizeof(item->title));

                                LOG_I("SD", "[%d] %s -> '%s'", s_track_count + 1, item->path, item->title);
                                s_track_count++;
                            }
                        }
                    }
                }
            }
        }
        file.close();
        file = dir.openNextFile();
    }
}

// Case-insensitive natural alphabetical comparator for PlaylistItem
static int compare_playlist_items(const void *a, const void *b) {
    const PlaylistItem *item_a = *(const PlaylistItem**)a;
    const PlaylistItem *item_b = *(const PlaylistItem**)b;

    const char *sa = item_a->title;
    const char *sb = item_b->title;

    while (*sa && *sb) {
        if (isdigit((unsigned char)*sa) && isdigit((unsigned char)*sb)) {
            unsigned long na = strtoul(sa, (char**)&sa, 10);
            unsigned long nb = strtoul(sb, (char**)&sb, 10);
            if (na != nb) {
                return (na < nb) ? -1 : 1;
            }
        } else {
            char ca = tolower((unsigned char)*sa);
            char cb = tolower((unsigned char)*sb);
            if (ca != cb) {
                return (unsigned char)ca - (unsigned char)cb;
            }
            sa++;
            sb++;
        }
    }
    return tolower((unsigned char)*sa) - tolower((unsigned char)*sb);
}

int sd_card_scan_tracks() {
    if (!s_sd_mounted) {
        if (!sd_card_init()) return 0;
    }

    s_track_count = 0;
    File root = SD.open("/");
    if (!root || !root.isDirectory()) {
        LOG_E("SD", "Failed to open root directory '/'");
        return 0;
    }

    LOG_I("SD", "Scanning MicroSD card for audio files...");
    scan_directory_recursive(root, 0);
    root.close();

    // Sort playlist alphabetically (A-Z / Natural Sort)
    if (s_track_count > 1) {
        qsort(s_playlist, s_track_count, sizeof(PlaylistItem*), compare_playlist_items);
        LOG_I("SD", "Tracks sorted alphabetically (A-Z):");
        for (int i = 0; i < s_track_count; i++) {
            LOG_I("SD", "  [%02d] %s", i + 1, s_playlist[i]->title);
        }
    }

    // Free unused allocated pointers from previous larger scans
    for (int i = s_track_count; i < MAX_PLAYLIST_TRACKS; i++) {
        if (s_playlist[i]) {
            free(s_playlist[i]);
            s_playlist[i] = nullptr;
        }
    }

    LOG_I("SD", "Scan completed: %d audio tracks found (Heap: %u, MaxBlock: %u)",
          s_track_count, (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
    return s_track_count;
}

int sd_card_get_track_count() {
    return s_track_count;
}

const PlaylistItem* sd_card_get_track(int index) {
    if (index < 0 || index >= s_track_count) return nullptr;
    return s_playlist[index];
}
