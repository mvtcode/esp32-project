#include "audio_player_service.h"
#include "storage_service.h"
#include "config_manager.h"
#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3a.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <SD.h>
#include <esp_task_wdt.h>

class AudioFileSourceSafeSD : public AudioFileSource {
public:
    AudioFileSourceSafeSD() : m_bytes_since_sample(0), m_cur_pos(0) {}
    AudioFileSourceSafeSD(const char* path) : m_bytes_since_sample(0), m_cur_pos(0) { open(path); }
    virtual ~AudioFileSourceSafeSD() override { close(); }

    virtual bool open(const char* path) override {
        if (!path || strlen(path) == 0) return false;
        close();
        
        // 1. Try exact path
        f = SD.open(path, "r");
        if (f) { m_bytes_since_sample = 0; m_cur_pos = 0; return true; }
        
        f = SD.open(path, FILE_READ);
        if (f) { m_bytes_since_sample = 0; m_cur_pos = 0; return true; }

        // 2. Try without leading slash
        if (path[0] == '/') {
            f = SD.open(&path[1], "r");
            if (f) { m_bytes_since_sample = 0; m_cur_pos = 0; return true; }
            f = SD.open(&path[1], FILE_READ);
            if (f) { m_bytes_since_sample = 0; m_cur_pos = 0; return true; }
        }

        // 3. Try with leading slash
        if (path[0] != '/') {
            String p = String("/") + path;
            f = SD.open(p.c_str(), "r");
            if (f) { m_bytes_since_sample = 0; m_cur_pos = 0; return true; }
            f = SD.open(p.c_str(), FILE_READ);
            if (f) { m_bytes_since_sample = 0; m_cur_pos = 0; return true; }
        }

        // 4. Try stripping /sd prefix if present
        if (strncmp(path, "/sd/", 4) == 0) {
            f = SD.open(&path[3], "r");
            if (f) { m_bytes_since_sample = 0; m_cur_pos = 0; return true; }
        }

        return false;
    }

    virtual uint32_t read(void* data, uint32_t len) override {
        if (!f) return 0;
        esp_task_wdt_reset();
        return f.read((uint8_t*)data, len);
    }

    virtual bool seek(int32_t pos, int dir) override {
        if (!f) return false;
        SeekMode mode = SeekSet;
        if (dir == 1) mode = SeekCur;
        else if (dir == 2) mode = SeekEnd;
        bool ok = f.seek(pos, mode);
        if (ok) {
            m_cur_pos = f.position();
        }
        return ok;
    }

    virtual bool close() override {
        if (f) f.close();
        m_cur_pos = 0;
        m_bytes_since_sample = 0;
        return true;
    }

    virtual bool isOpen() override {
        return f ? true : false;
    }

    virtual uint32_t getSize() override {
        return f ? f.size() : 0;
    }

    virtual uint32_t getPos() override {
        return m_cur_pos;
    }

    void reset_sample_bytes() {
        m_bytes_since_sample = 0;
    }

private:
    File f;
    uint32_t m_bytes_since_sample;
    uint32_t m_cur_pos;
};

bool AudioPlayerService::initialized = false;
TaskHandle_t AudioPlayerService::audioTaskHandle = nullptr;
SemaphoreHandle_t AudioPlayerService::audioMutex = nullptr;

AudioTrack AudioPlayerService::playlist[MAX_AUDIO_TRACKS];
int AudioPlayerService::playlistCount = 0;
int AudioPlayerService::currentTrackIdx = -1;
AudioPlaybackState AudioPlayerService::state = STATE_STOPPED;
uint8_t AudioPlayerService::volume = 50;
bool AudioPlayerService::shuffle = false;
AudioRepeatMode AudioPlayerService::repeatMode = REPEAT_MODE_ALL;

uint32_t AudioPlayerService::playStartMillis = 0;
uint32_t AudioPlayerService::pausedElapsedMillis = 0;
uint32_t AudioPlayerService::pauseStartMillis = 0;
bool AudioPlayerService::trackJustFinished = false;

static uint8_t s_audio_buffer[8192];
static AudioFileSourceSafeSD* audioFile = nullptr;
static AudioFileSourceBuffer* audioBuff = nullptr;
static AudioGeneratorMP3a* mp3Decoder = nullptr;
static AudioGeneratorWAV* wavDecoder = nullptr;
static AudioOutputI2S* audioOut = nullptr;

bool AudioPlayerService::isInitialized() {
    return initialized;
}

void AudioPlayerService::init() {
    if (initialized) return;

    audioLogger = &Serial;

    audioMutex = xSemaphoreCreateMutex();
    volume = ConfigManager::getDefaultVolume();
    if (volume == 0) volume = 50;

    // Initialize Audio Output (CYD Onboard DAC on GPIO 26 / DAC channel 2, 16 DMA buffers for smooth streaming)
    audioOut = new AudioOutputI2S(0, AudioOutputI2S::INTERNAL_DAC, 16);
    audioOut->SetOutputModeMono(true);
    audioOut->SetGain(((float)volume / 100.0f) * 1.4f);
    audioOut->begin(); // Install I2S DMA buffers early while heap is clean

    // Preallocate MP3 and WAV decoders while memory is unfragmented
    if (!mp3Decoder) {
        mp3Decoder = new AudioGeneratorMP3a();
    }
    if (!wavDecoder) {
        wavDecoder = new AudioGeneratorWAV();
    }

    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        6144,
        NULL,
        5,
        &audioTaskHandle,
        0
    );

    initialized = true;
    Serial.println("[AudioPlayer] Audio Engine initialized on Core 0 (DAC Output, Helix Decoder Preallocated)");
}

void AudioPlayerService::scanMusicFiles() {
    playlistCount = 0;
    currentTrackIdx = -1;
    state = STATE_STOPPED;

    if (!StorageService::isMounted()) {
        Serial.println("[AudioPlayer] Cannot scan SD card - Not mounted.");
        return;
    }

    Serial.println("[AudioPlayer] Scanning FULL SD card for music files (recursive)...");
    scanDirectory("/", 4);

    Serial.printf("[AudioPlayer] Found %d tracks in SD playlist.\n", playlistCount);
    if (playlistCount > 0) {
        currentTrackIdx = 0;
    }
}

void AudioPlayerService::scanDirectory(const char* dirPath, int maxDepth) {
    if (maxDepth <= 0 || playlistCount >= MAX_AUDIO_TRACKS) return;

    Serial.printf("[AudioPlayer] Opening directory: '%s'...\n", dirPath);
    File root = SD.open(dirPath);
    if (!root && (strcmp(dirPath, "/") == 0 || strlen(dirPath) == 0)) {
        root = SD.open("");
        if (!root) {
            root = SD.open("/sd");
        }
    }
    if (!root) {
        Serial.printf("[AudioPlayer] Cannot open dir: '%s' (SD.open failed).\n", dirPath);
        return;
    }
    if (!root.isDirectory()) {
        Serial.printf("[AudioPlayer] '%s' is not a directory.\n", dirPath);
        root.close();
        return;
    }
    Serial.printf("[AudioPlayer] Scanning directory: '%s'\n", dirPath);

    File file = root.openNextFile();
    while (file && playlistCount < MAX_AUDIO_TRACKS) {
        String rawName = file.name();
        bool isDir = file.isDirectory();
        size_t fsize = file.size();

        // Extract pure filename component
        int lastSlash = rawName.lastIndexOf('/');
        String filename = (lastSlash >= 0) ? rawName.substring(lastSlash + 1) : rawName;

        // Construct proper full path: dirPath + filename
        String fullPath = String(dirPath);
        if (!fullPath.endsWith("/")) {
            fullPath += "/";
        }
        fullPath += filename;

        if (isDir) {
            bool isSystemDir = filename.startsWith(".") ||
                               filename.equalsIgnoreCase("System Volume Information") ||
                               filename.equalsIgnoreCase("$RECYCLE.BIN") ||
                               filename.equalsIgnoreCase(".Spotlight-V100") ||
                               filename.equalsIgnoreCase(".fseventsd") ||
                               filename.equalsIgnoreCase(".Trashes");

            if (maxDepth > 1 && !isSystemDir) {
                scanDirectory(fullPath.c_str(), maxDepth - 1);
            }
        } else {
            // Ignore hidden files and AppleDouble metadata
            if (!filename.startsWith(".")) {
                String lower = filename;
                lower.toLowerCase();
                if (lower.endsWith(".mp3") || lower.endsWith(".wav") || lower.endsWith(".aac")) {
                    // Avoid duplicate entries
                    bool exists = false;
                    for (int i = 0; i < playlistCount; i++) {
                        if (strcasecmp(playlist[i].path, fullPath.c_str()) == 0) {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists && playlistCount < MAX_AUDIO_TRACKS) {
                        AudioTrack& track = playlist[playlistCount];
                        memset(&track, 0, sizeof(AudioTrack));
                        strncpy(track.path, fullPath.c_str(), sizeof(track.path) - 1);
                        track.fileSize = fsize;

                        // Parse filename as title
                        int dotIdx = filename.lastIndexOf('.');
                        String rawTitle = (dotIdx > 0) ? filename.substring(0, dotIdx) : filename;

                        // Check for "Artist - Title" format
                        int dashIdx = rawTitle.indexOf('-');
                        if (dashIdx > 0 && dashIdx < (int)rawTitle.length() - 1) {
                            String artistStr = rawTitle.substring(0, dashIdx);
                            artistStr.trim();
                            String titleStr = rawTitle.substring(dashIdx + 1);
                            titleStr.trim();
                            strncpy(track.artist, artistStr.c_str(), sizeof(track.artist) - 1);
                            strncpy(track.title, titleStr.c_str(), sizeof(track.title) - 1);
                        } else {
                            strncpy(track.title, rawTitle.c_str(), sizeof(track.title) - 1);
                            track.artist[0] = '\0';
                        }

                        if (lower.endsWith(".mp3")) {
                            strncpy(track.format, "MP3", sizeof(track.format) - 1);
                            track.durationSec = (int)(track.fileSize / 16000);
                        } else if (lower.endsWith(".wav")) {
                            strncpy(track.format, "WAV", sizeof(track.format) - 1);
                            track.durationSec = (int)(track.fileSize / 176400);
                        } else {
                            strncpy(track.format, "AAC", sizeof(track.format) - 1);
                            track.durationSec = (int)(track.fileSize / 16000);
                        }

                        if (track.durationSec < 5) track.durationSec = 180;

                        playlistCount++;
                        Serial.printf("[AudioPlayer] Added track [%d]: %s - %s (%s, %u bytes)\n", 
                                      playlistCount, track.artist, track.title, track.path, track.fileSize);
                    }
                }
            }
        }
        file.close();
        file = root.openNextFile();
    }
    root.close();
}

void AudioPlayerService::cleanupCurrentPlayback() {
    if (mp3Decoder && mp3Decoder->isRunning()) {
        mp3Decoder->stop();
    }
    if (wavDecoder && wavDecoder->isRunning()) {
        wavDecoder->stop();
    }
    if (audioBuff) {
        audioBuff->close();
        delete audioBuff;
        audioBuff = nullptr;
    }
    if (audioFile) {
        if (audioFile->isOpen()) audioFile->close();
        delete audioFile;
        audioFile = nullptr;
    }
}

bool AudioPlayerService::playTrack(int index) {
    if (index < 0 || index >= playlistCount) return false;

    if (audioMutex && xSemaphoreTake(audioMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        cleanupCurrentPlayback();

        currentTrackIdx = index;
        const AudioTrack& track = playlist[currentTrackIdx];
        Serial.printf("[AudioPlayer] Starting playback: %s (Size: %u bytes)\n", track.path, track.fileSize);

        audioFile = new AudioFileSourceSafeSD(track.path);
        if (!audioFile || !audioFile->isOpen()) {
            Serial.printf("[AudioPlayer] Failed to open audio file: %s\n", track.path);
            cleanupCurrentPlayback();
            state = STATE_STOPPED;
            xSemaphoreGive(audioMutex);
            return false;
        }

        audioBuff = new AudioFileSourceBuffer(audioFile, s_audio_buffer, sizeof(s_audio_buffer));

        String pathStr = String(track.path);
        pathStr.toLowerCase();
        if (pathStr.endsWith(".mp3")) {
            if (!mp3Decoder) {
                mp3Decoder = new AudioGeneratorMP3a();
            }
            if (!mp3Decoder || !mp3Decoder->begin(audioBuff, audioOut)) {
                Serial.println("[AudioPlayer] Failed to start MP3 decoder.");
                cleanupCurrentPlayback();
                state = STATE_STOPPED;
                xSemaphoreGive(audioMutex);
                return false;
            }
        } else {
            if (!wavDecoder) {
                wavDecoder = new AudioGeneratorWAV();
            }
            if (!wavDecoder || !wavDecoder->begin(audioBuff, audioOut)) {
                Serial.println("[AudioPlayer] Failed to start WAV decoder.");
                cleanupCurrentPlayback();
                state = STATE_STOPPED;
                xSemaphoreGive(audioMutex);
                return false;
            }
        }

        playStartMillis = millis();
        pausedElapsedMillis = 0;
        state = STATE_PLAYING;
        trackJustFinished = false;
        xSemaphoreGive(audioMutex);
        return true;
    }
    return false;
}

bool AudioPlayerService::play() {
    if (state == STATE_PLAYING) return true;
    if (state == STATE_PAUSED) return resume();
    if (currentTrackIdx >= 0 && currentTrackIdx < playlistCount) {
        return playTrack(currentTrackIdx);
    }
    if (playlistCount > 0) {
        return playTrack(0);
    }
    return false;
}

bool AudioPlayerService::pause() {
    if (state != STATE_PLAYING) return false;
    state = STATE_PAUSED;
    pauseStartMillis = millis();
    return true;
}

bool AudioPlayerService::resume() {
    if (state != STATE_PAUSED) return false;
    pausedElapsedMillis += (millis() - pauseStartMillis);
    state = STATE_PLAYING;
    return true;
}

bool AudioPlayerService::togglePlay() {
    if (state == STATE_PLAYING) return pause();
    return play();
}

void AudioPlayerService::stop() {
    if (audioMutex && xSemaphoreTake(audioMutex, pdMS_TO_TICKS(200)) == pdTRUE) {
        cleanupCurrentPlayback();
        state = STATE_STOPPED;
        pausedElapsedMillis = 0;
        xSemaphoreGive(audioMutex);
    }
}

int AudioPlayerService::getNextIndex() {
    if (playlistCount == 0) return -1;
    if (shuffle && playlistCount > 1) {
        int r = rand() % playlistCount;
        if (r == currentTrackIdx) r = (r + 1) % playlistCount;
        return r;
    }
    return (currentTrackIdx + 1) % playlistCount;
}

int AudioPlayerService::getPrevIndex() {
    if (playlistCount == 0) return -1;
    if (shuffle && playlistCount > 1) {
        int r = rand() % playlistCount;
        if (r == currentTrackIdx) r = (r - 1 + playlistCount) % playlistCount;
        return r;
    }
    return (currentTrackIdx - 1 + playlistCount) % playlistCount;
}

void AudioPlayerService::next() {
    if (playlistCount == 0) return;
    int nextIdx = getNextIndex();
    playTrack(nextIdx);
}

void AudioPlayerService::prev() {
    if (playlistCount == 0) return;
    // If playing more than 3 seconds, restart current track
    if (getCurrentElapsedSec() > 3) {
        playTrack(currentTrackIdx);
    } else {
        int prevIdx = getPrevIndex();
        playTrack(prevIdx);
    }
}

void AudioPlayerService::setVolume(uint8_t volumePercent) {
    if (volumePercent > 100) volumePercent = 100;
    volume = volumePercent;
    if (audioOut) {
        float gain = ((float)volume / 100.0f) * 1.4f;
        audioOut->SetGain(gain);
    }
    ConfigManager::setDefaultVolume(volume);
}

uint8_t AudioPlayerService::getVolume() {
    return volume;
}

void AudioPlayerService::setShuffle(bool enabled) {
    shuffle = enabled;
}

bool AudioPlayerService::isShuffle() {
    return shuffle;
}

void AudioPlayerService::setRepeatMode(AudioRepeatMode mode) {
    repeatMode = mode;
}

AudioRepeatMode AudioPlayerService::getRepeatMode() {
    return repeatMode;
}

AudioPlaybackState AudioPlayerService::getState() {
    return state;
}

bool AudioPlayerService::isPlaying() {
    return state == STATE_PLAYING;
}

int AudioPlayerService::getCurrentTrackIndex() {
    return currentTrackIdx;
}

AudioTrack AudioPlayerService::getCurrentTrack() {
    if (currentTrackIdx >= 0 && currentTrackIdx < playlistCount) {
        return playlist[currentTrackIdx];
    }
    AudioTrack dummy;
    memset(&dummy, 0, sizeof(AudioTrack));
    strncpy(dummy.title, "Chưa chọn bài", sizeof(dummy.title) - 1);
    strncpy(dummy.artist, "SD Card", sizeof(dummy.artist) - 1);
    strncpy(dummy.format, "--", sizeof(dummy.format) - 1);
    return dummy;
}

const AudioTrack* AudioPlayerService::getTrack(int index) {
    if (index >= 0 && index < playlistCount) {
        return &playlist[index];
    }
    return nullptr;
}

int AudioPlayerService::getCurrentElapsedSec() {
    if (state == STATE_STOPPED) return 0;
    uint32_t now = (state == STATE_PAUSED) ? pauseStartMillis : millis();
    if (now < playStartMillis + pausedElapsedMillis) return 0;
    return (int)((now - playStartMillis - pausedElapsedMillis) / 1000);
}

int AudioPlayerService::getCurrentTotalSec() {
    if (currentTrackIdx >= 0 && currentTrackIdx < playlistCount) {
        return playlist[currentTrackIdx].durationSec;
    }
    return 0;
}

int AudioPlayerService::getTrackCount() {
    return playlistCount;
}

bool AudioPlayerService::isSdReady() {
    return StorageService::isMounted();
}

void AudioPlayerService::update() {
    if (trackJustFinished) {
        trackJustFinished = false;
        if (repeatMode == REPEAT_MODE_ONE) {
            playTrack(currentTrackIdx);
        } else if (repeatMode == REPEAT_MODE_ALL || shuffle) {
            next();
        } else {
            // REPEAT_MODE_OFF
            if (currentTrackIdx + 1 < playlistCount) {
                next();
            } else {
                stop();
            }
        }
    }
}

void AudioPlayerService::audioTask(void* parameter) {
    uint32_t loopCount = 0;
    while (true) {
        if (state == STATE_PLAYING) {
            if (audioMutex && xSemaphoreTake(audioMutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                if (mp3Decoder && mp3Decoder->isRunning()) {
                    if (!mp3Decoder->loop()) {
                        mp3Decoder->stop();
                        trackJustFinished = true;
                    }
                } else if (wavDecoder && wavDecoder->isRunning()) {
                    if (!wavDecoder->loop()) {
                        wavDecoder->stop();
                        trackJustFinished = true;
                    }
                }
                xSemaphoreGive(audioMutex);
            }

            loopCount++;
            if ((loopCount % 64) == 0) {
                esp_task_wdt_reset();
                vTaskDelay(pdMS_TO_TICKS(1));
            } else {
                taskYIELD();
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
}
