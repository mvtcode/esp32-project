#include "audio_player_service.h"
#include "storage_service.h"
#include "config_manager.h"
#include "log.h"
#include <AudioFileSource.h>
#include <AudioFileSourceBuffer.h>
#include <AudioGeneratorMP3a.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <SD.h>
#include <esp_task_wdt.h>
#include <driver/i2s.h>


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
        if (!f) f = SD.open(path, FILE_READ);

        // 2. Try without leading slash
        if (!f && path[0] == '/') {
            f = SD.open(&path[1], "r");
            if (!f) f = SD.open(&path[1], FILE_READ);
        }

        // 3. Try with leading slash
        if (!f && path[0] != '/') {
            char p[256];
            snprintf(p, sizeof(p), "/%s", path);
            f = SD.open(p, "r");
            if (!f) f = SD.open(p, FILE_READ);
        }

        // 4. Try stripping /sd prefix if present
        if (!f && strncmp(path, "/sd/", 4) == 0) {
            f = SD.open(&path[3], "r");
        }

        if (f) {
            uint32_t fsize = f.size();
            if (fsize < 4096) {
                f.close();
                return false;
            }

            // 1. Kiểm tra Magic Bytes chống file FAKE (M4A, MP4, FLAC, OGG, RIFF đổi đuôi thành .mp3)
            uint8_t magicBuf[16];
            if (f.read(magicBuf, sizeof(magicBuf)) < 16) {
                f.close();
                return false;
            }

            // Kiểm tra MP4 / M4A container ('ftyp' tại offset 4..7)
            if (magicBuf[4] == 'f' && magicBuf[5] == 't' && magicBuf[6] == 'y' && magicBuf[7] == 'p') {
                LOG_W("AudioPlayer", "REJECTED: '%s' is an MP4/M4A file (fake .mp3 extension).", path);
                f.close();
                return false;
            }
            // Kiểm tra FLAC container ('fLaC')
            if (magicBuf[0] == 'f' && magicBuf[1] == 'L' && magicBuf[2] == 'a' && magicBuf[3] == 'C') {
                LOG_W("AudioPlayer", "REJECTED: '%s' is a FLAC file (fake .mp3 extension).", path);
                f.close();
                return false;
            }
            // Kiểm tra OGG container ('OggS')
            if (magicBuf[0] == 'O' && magicBuf[1] == 'g' && magicBuf[2] == 'g' && magicBuf[3] == 'S') {
                LOG_W("AudioPlayer", "REJECTED: '%s' is an OGG file (fake .mp3 extension).", path);
                f.close();
                return false;
            }

            uint32_t startOffset = 0;

            // 2. Bỏ qua ID3v2 metadata nếu có
            if (magicBuf[0] == 'I' && magicBuf[1] == 'D' && magicBuf[2] == '3') {
                uint32_t id3Size = ((uint32_t)(magicBuf[6] & 0x7F) << 21) |
                                   ((uint32_t)(magicBuf[7] & 0x7F) << 14) |
                                   ((uint32_t)(magicBuf[8] & 0x7F) << 7)  |
                                   ((uint32_t)(magicBuf[9] & 0x7F));
                startOffset = 10 + id3Size;
                if (startOffset >= fsize) {
                    f.close();
                    return false;
                }
            }

            // 3. Quét tìm MP3 Frame Sync (0xFFE0 / 0xFFF0)
            f.seek(startOffset);
            uint8_t scanBuf[2048];
            size_t bytesRead = f.read(scanBuf, sizeof(scanBuf));
            bool foundSync = false;
            uint32_t syncOffset = startOffset;

            if (bytesRead >= 4) {
                for (size_t i = 0; i < bytesRead - 3; i++) {
                    if (scanBuf[i] == 0xFF && (scanBuf[i+1] & 0xE0) == 0xE0) {
                        uint8_t layer = (scanBuf[i+1] >> 1) & 0x03;
                        uint8_t bitrateIdx = (scanBuf[i+2] >> 4) & 0x0F;
                        uint8_t sampleIdx = (scanBuf[i+2] >> 2) & 0x03;
                        
                        if (layer != 0 && bitrateIdx != 0 && bitrateIdx != 15 && sampleIdx != 3) {
                            foundSync = true;
                            syncOffset = startOffset + i;
                            break;
                        }
                    }
                }
            }

            // Nếu không tìm thấy Frame Sync MP3 hợp lệ nào -> Từ chối mở file để bảo vệ decoder
            String pStr = String(path);
            pStr.toLowerCase();
            if (pStr.endsWith(".mp3") && !foundSync) {
                LOG_W("AudioPlayer", "REJECTED: '%s' contains no valid MP3 sync frames.", path);
                f.close();
                return false;
            }

            if (foundSync) {
                f.seek(syncOffset);
            } else {
                f.seek(startOffset);
            }

            m_cur_pos = f.position();
            m_bytes_since_sample = 0;
            return true;
        }

        return false;
    }


    virtual uint32_t read(void* data, uint32_t len) override {
        if (!f) return 0;
        esp_task_wdt_reset();
        uint32_t bytesRead = f.read((uint8_t*)data, len);
        m_cur_pos += bytesRead;
        return bytesRead;
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
uint32_t AudioPlayerService::seekOffsetSec = 0;
volatile bool AudioPlayerService::trackJustFinished = false;

class SafeAudioOutputDAC : public AudioOutputI2S {
public:
    SafeAudioOutputDAC(int dma_buf_count = 16) : AudioOutputI2S(0, INTERNAL_DAC, dma_buf_count) {}
    
    virtual bool SetRate(int hz) override {
        // Gán tần số an toàn (mặc định 44.1kHz nếu hz không hợp lệ) để tránh crash chia cho 0 trong ESP-IDF
        if (hz < 8000 || hz > 96000) {
            hz = 44100;
        }
        return AudioOutputI2S::SetRate(hz);
    }

    virtual bool SetChannels(int channels) override {
        if (channels < 1 || channels > 2) {
            channels = 2;
        }
        return AudioOutputI2S::SetChannels(channels);
    }

    virtual bool stop() override {
        if (!i2sOn) return false;
        // Xóa sạch buffer âm thanh để ngắt tiếng lập tức, không uninstall driver
        // Giúp loại bỏ 100% rủi ro lỗi "Error malloc dma buffer" khi chuyển bài
        i2s_zero_dma_buffer((i2s_port_t)portNo);
        return true;
    }
};



class FixedAudioGeneratorMP3a : public AudioGeneratorMP3a {
public:
    FixedAudioGeneratorMP3a() : AudioGeneratorMP3a(), errorCount(0) {}

    int errorCount;

    void resetPlaybackState() {
        running = false;
        buffValid = 0;
        lastFrameEnd = 0;
        validSamples = 0;
        curSample = 0;
        lastRate = 0;
        lastChannels = 0;
        errorCount = 0;
        memset(buff, 0, sizeof(buff));
        memset(outSample, 0, sizeof(outSample));
    }

    void flushAfterSeek() {
        buffValid = 0;
        lastFrameEnd = 0;
        validSamples = 0;
        curSample = 0;
        errorCount = 0;
        lastChannels = 2; // Default to stereo fallback
        memset(buff, 0, sizeof(buff));
    }

    virtual bool begin(AudioFileSource *source, AudioOutput *output) override {
        resetPlaybackState();
        return AudioGeneratorMP3a::begin(source, output);
    }

    virtual bool loop() override {
        if (!running) return false;
        esp_task_wdt_reset();

        // 1. Gửi các mẫu âm thanh đã giải mã trước đó ra I2S DAC
        while (validSamples > 0) {
            lastSample[0] = outSample[curSample * 2];
            lastSample[1] = outSample[curSample * 2 + 1];
            if (!output || !output->ConsumeSample(lastSample)) {
                goto done; // DMA buffer đầy -> nhường CPU và quay lại lần sau
            }
            validSamples--;
            curSample++;
        }

        // 2. Nạp và giải mã Frame MP3 tiếp theo
        if (FillBufferWithValidFrame()) {
            unsigned char* inBuff = reinterpret_cast<unsigned char*>(buff);
            int bytesLeft = buffValid;
            int ret = MP3Decode(hMP3Decoder, &inBuff, &bytesLeft, outSample, 0);
            if (ret != 0) {
                // Lỗi decode frame MP3 (ví dụ dữ liệu nhiễu hoặc header sai)
                errorCount++;
                if (errorCount > 30) {
                    running = false;
                    goto done;
                }
            } else {
                errorCount = 0;
                lastFrameEnd = buffValid - bytesLeft;
                MP3FrameInfo fi;
                MP3GetLastFrameInfo(hMP3Decoder, &fi);

                // Kiểm tra tần số mẫu hợp lệ (tránh 0Hz gây lỗi chia)
                if (fi.samprate >= 8000 && fi.samprate <= 96000 && (int)fi.samprate != (int)lastRate) {
                    if (output) output->SetRate(fi.samprate);
                    lastRate = fi.samprate;
                }

                // Bảo vệ số kênh âm thanh (CHỐNG LỖI CHIA CHO 0 IntegerDivideByZero khi seek cuối file)
                int chans = fi.nChans;
                if (chans < 1 || chans > 2) {
                    chans = (lastChannels >= 1 && lastChannels <= 2) ? lastChannels : 2;
                }
                if (chans != lastChannels) {
                    if (output) output->SetChannels(chans);
                    lastChannels = chans;
                }

                curSample = 0;
                validSamples = (lastChannels > 0) ? (fi.outputSamps / lastChannels) : 0;
            }
        } else {
            // Hết file (EOF) hoặc không còn frame MP3 hợp lệ nào
            running = false;
        }

    done:
        if (file) file->loop();
        if (output) output->loop();
        return running;
    }
};

static uint8_t s_audio_buffer[8192];
static AudioFileSourceSafeSD* audioFile = nullptr;
static AudioFileSourceBuffer* audioBuff = nullptr;
static FixedAudioGeneratorMP3a* mp3Decoder = nullptr;
static AudioGeneratorWAV* wavDecoder = nullptr;
static SafeAudioOutputDAC* audioOut = nullptr;

bool AudioPlayerService::isInitialized() {
    return initialized;
}

void AudioPlayerService::init() {
    if (initialized) return;

    audioLogger = &Serial;

    audioMutex = xSemaphoreCreateMutex();
    volume = ConfigManager::getDefaultVolume();
    if (volume == 0) volume = 50;

    // Preallocate MP3 and WAV decoders while memory is clean and unfragmented
    if (!mp3Decoder) {
        mp3Decoder = new FixedAudioGeneratorMP3a();
    }
    if (!wavDecoder) {
        wavDecoder = new AudioGeneratorWAV();
    }

    // Initialize Audio Output (CYD Onboard DAC on GPIO 26 / DAC channel 2, 16 DMA buffers)
    audioOut = new SafeAudioOutputDAC(16);
    audioOut->SetOutputModeMono(true);
    audioOut->SetGain(((float)volume / 100.0f) * 1.4f);
    audioOut->begin(); // Cài đặt I2S DMA buffers 1 lần duy nhất

    xTaskCreatePinnedToCore(
        audioTask,
        "AudioTask",
        6144,
        NULL,
        3, // Priority 3: trên IDLE0 nhưng có vTaskDelay nhường CPU hợp lý
        &audioTaskHandle,
        0
    );

    initialized = true;
    LOG_I("AudioPlayer", "Audio Engine initialized on Core 0 (DAC Output, Helix Preallocated)");
}

void AudioPlayerService::scanMusicFiles() {
    playlistCount = 0;
    currentTrackIdx = -1;
    state = STATE_STOPPED;

    if (!StorageService::isMounted()) {
        LOG_W("AudioPlayer", "Cannot scan SD card - Not mounted.");
        return;
    }

    LOG_I("AudioPlayer", "Scanning FULL SD card for music files (recursive)...");
    scanDirectory("/", 4);

    LOG_I("AudioPlayer", "Found %d tracks in SD playlist.", playlistCount);
    if (playlistCount > 0) {
        // Khôi phục bài hát đã phát gần nhất từ NVS
        String lastPath = ConfigManager::getLastAudioTrackPath();
        int matchedIdx = -1;
        if (lastPath.length() > 0) {
            for (int i = 0; i < playlistCount; i++) {
                if (strcasecmp(playlist[i].path, lastPath.c_str()) == 0) {
                    matchedIdx = i;
                    break;
                }
            }
        }

        if (matchedIdx >= 0) {
            currentTrackIdx = matchedIdx;
            LOG_I("AudioPlayer", "Restored last played track [%d]: %s (%s)", 
                  currentTrackIdx, playlist[currentTrackIdx].title, playlist[currentTrackIdx].path);
        } else {
            int lastIdx = ConfigManager::getLastAudioTrackIndex();
            if (lastIdx >= 0 && lastIdx < playlistCount) {
                currentTrackIdx = lastIdx;
                LOG_I("AudioPlayer", "Restored last played index [%d]: %s", 
                      currentTrackIdx, playlist[currentTrackIdx].title);
            } else {
                currentTrackIdx = 0;
            }
        }
    }
}

// -------------------------------------------------------------
// HELPER: Tính thời lượng (Duration) thực tế của file MP3 & WAV
// -------------------------------------------------------------
static int parseMp3Duration(File& f, uint32_t fileSize, int& outBitrate) {
    outBitrate = 128;
    if (fileSize < 4096) return -1;

    uint8_t buf[512];
    f.seek(0);
    int bytesRead = f.read(buf, sizeof(buf));
    if (bytesRead < 32) return -1;

    // 1. Kiểm tra Magic Bytes chống file FAKE (M4A, MP4, FLAC, OGG đổi đuôi thành .mp3)
    if (buf[4] == 'f' && buf[5] == 't' && buf[6] == 'y' && buf[7] == 'p') {
        return -1; // Fake MP4/M4A container
    }
    if (buf[0] == 'f' && buf[1] == 'L' && buf[2] == 'a' && buf[3] == 'C') {
        return -1; // Fake FLAC
    }
    if (buf[0] == 'O' && buf[1] == 'g' && buf[2] == 'g' && buf[3] == 'S') {
        return -1; // Fake OGG
    }

    // 2. Kiểm tra header ID3v2 để bỏ qua phần metadata
    uint32_t audioDataOffset = 0;
    if (buf[0] == 'I' && buf[1] == 'D' && buf[2] == '3') {
        uint32_t id3Size = ((uint32_t)(buf[6] & 0x7F) << 21) |
                           ((uint32_t)(buf[7] & 0x7F) << 14) |
                           ((uint32_t)(buf[8] & 0x7F) << 7)  |
                           (uint32_t)(buf[9] & 0x7F);
        audioDataOffset = 10 + id3Size;
        if (audioDataOffset < fileSize) {
            f.seek(audioDataOffset);
            bytesRead = f.read(buf, sizeof(buf));
        }
    }

    uint32_t effectiveAudioSize = (fileSize > audioDataOffset) ? (fileSize - audioDataOffset) : fileSize;

    // 3. Tìm frame header MP3 hợp lệ đầu tiên (sync word 0xFFE0)
    for (int i = 0; i <= bytesRead - 4; i++) {
        if (buf[i] == 0xFF && (buf[i + 1] & 0xE0) == 0xE0) {
            uint8_t version = (buf[i + 1] >> 3) & 0x03; // 3 = MPEG 1, 2 = MPEG 2
            uint8_t layer = (buf[i + 1] >> 1) & 0x03;   // 1 = Layer III
            uint8_t bitrateIdx = (buf[i + 2] >> 4) & 0x0F;
            uint8_t sampleRateIdx = (buf[i + 2] >> 2) & 0x03;
            uint8_t channelMode = (buf[i + 3] >> 6) & 0x03;

            if (version == 1 || layer == 0 || bitrateIdx == 0 || bitrateIdx == 15 || sampleRateIdx == 3) {
                continue;
            }

            // MPEG-1 / MPEG-2 Layer 3 bitrates (kbps)
            static const int bitratesMPEG1_L3[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 0};
            static const int bitratesMPEG2_L3[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 0};
            static const int sampleRatesMPEG1[] = {44100, 48000, 32000, 0};
            static const int sampleRatesMPEG2[] = {22050, 24000, 16000, 0};

            int bitrate = (version == 3) ? bitratesMPEG1_L3[bitrateIdx] : bitratesMPEG2_L3[bitrateIdx];
            int sampleRate = (version == 3) ? sampleRatesMPEG1[sampleRateIdx] : sampleRatesMPEG2[sampleRateIdx];

            if (sampleRate == 0 || bitrate == 0) continue;

            outBitrate = bitrate;

            // Kiểm tra Xing / Info VBR Header (chứa tổng số frame chính xác 100%)
            int xingOffset = (channelMode == 3) ? (i + 4 + 17) : (i + 4 + 32);
            if (xingOffset + 12 < bytesRead) {
                if ((buf[xingOffset] == 'X' && buf[xingOffset + 1] == 'i' && buf[xingOffset + 2] == 'n' && buf[xingOffset + 3] == 'g') ||
                    (buf[xingOffset] == 'I' && buf[xingOffset + 1] == 'n' && buf[xingOffset + 2] == 'f' && buf[xingOffset + 3] == 'o')) {
                    uint32_t flags = ((uint32_t)buf[xingOffset + 4] << 24) |
                                     ((uint32_t)buf[xingOffset + 5] << 16) |
                                     ((uint32_t)buf[xingOffset + 6] << 8)  |
                                     (uint32_t)buf[xingOffset + 7];
                    if (flags & 0x0001) { // Có trường Frame Count
                        uint32_t totalFrames = ((uint32_t)buf[xingOffset + 8] << 24) |
                                               ((uint32_t)buf[xingOffset + 9] << 16) |
                                               ((uint32_t)buf[xingOffset + 10] << 8) |
                                               (uint32_t)buf[xingOffset + 11];
                        int samplesPerFrame = (version == 3) ? 1152 : 576;
                        int dur = (int)((uint64_t)totalFrames * samplesPerFrame / sampleRate);
                        if (dur > 0) return dur;
                    }
                }
            }

            // Kiểm tra VBRI Header
            int vbriOffset = i + 4 + 32;
            if (vbriOffset + 18 < bytesRead) {
                if (buf[vbriOffset] == 'V' && buf[vbriOffset + 1] == 'B' && buf[vbriOffset + 2] == 'R' && buf[vbriOffset + 3] == 'I') {
                    uint32_t totalFrames = ((uint32_t)buf[vbriOffset + 14] << 24) |
                                           ((uint32_t)buf[vbriOffset + 15] << 16) |
                                           ((uint32_t)buf[vbriOffset + 16] << 8)  |
                                           (uint32_t)buf[vbriOffset + 17];
                    int samplesPerFrame = (version == 3) ? 1152 : 576;
                    int dur = (int)((uint64_t)totalFrames * samplesPerFrame / sampleRate);
                    if (dur > 0) return dur;
                }
            }

            // CBR: Tính chính xác theo bitrate thực tế của file (bytes / (bitrate_kbps * 125 bytes/sec))
            uint32_t bytesPerSec = (uint32_t)bitrate * 125;
            if (bytesPerSec > 0) {
                return (int)(effectiveAudioSize / bytesPerSec);
            }
        }
    }

    // Không tìm thấy bất kỳ frame MP3 hợp lệ nào -> Báo lỗi file fake/corrupt
    return -1;
}

static int parseWavDuration(File& f, uint32_t fileSize, int& outBitrate) {
    outBitrate = 1411;
    if (fileSize < 44) return 0;
    uint8_t buf[44];
    f.seek(0);
    if (f.read(buf, 44) == 44) {
        if (buf[0] == 'R' && buf[1] == 'I' && buf[2] == 'F' && buf[3] == 'F') {
            uint32_t byteRate = (uint32_t)buf[28] | ((uint32_t)buf[29] << 8) |
                                ((uint32_t)buf[30] << 16) | ((uint32_t)buf[31] << 24);
            if (byteRate > 0) {
                outBitrate = (int)(byteRate * 8 / 1000);
                return (int)((fileSize - 44) / byteRate);
            }
        }
    }
    return (int)(fileSize / 176400); // 44.1kHz 16-bit stereo
}

void AudioPlayerService::scanDirectory(const char* dirPath, int maxDepth) {
    if (maxDepth <= 0 || playlistCount >= MAX_AUDIO_TRACKS) return;

    LOG_D("AudioPlayer", "Opening directory: '%s'...", dirPath);
    File root = SD.open(dirPath);
    if (!root && (strcmp(dirPath, "/") == 0 || strlen(dirPath) == 0)) {
        root = SD.open("");
        if (!root) {
            root = SD.open("/sd");
        }
    }
    if (!root) {
        LOG_W("AudioPlayer", "Cannot open dir: '%s' (SD.open failed).", dirPath);
        return;
    }
    if (!root.isDirectory()) {
        LOG_W("AudioPlayer", "'%s' is not a directory.", dirPath);
        root.close();
        return;
    }
    LOG_D("AudioPlayer", "Scanning directory: '%s'", dirPath);

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
            file.close();
            if (!filename.startsWith(".") && !filename.equalsIgnoreCase("System Volume Information")) {
                scanDirectory(fullPath.c_str(), maxDepth - 1);
            }
        } else {
            String lower = filename;
            lower.toLowerCase();

            if (lower.endsWith(".mp3") || lower.endsWith(".wav") || lower.endsWith(".aac")) {
                if (!filename.startsWith("._") && fsize > 4096) {
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

                        // Parse filename as clean title (giữ trọn vẹn phần đầu có ý nghĩa, thay '_' bằng ' ')
                        int dotIdx = filename.lastIndexOf('.');
                        String rawTitle = (dotIdx > 0) ? filename.substring(0, dotIdx) : filename;

                        String displayTitle = rawTitle;
                        displayTitle.replace('_', ' ');
                        displayTitle.trim();
                        while (displayTitle.indexOf("  ") >= 0) {
                            displayTitle.replace("  ", " ");
                        }

                        strncpy(track.title, displayTitle.c_str(), sizeof(track.title) - 1);
                        strncpy(track.artist, "SD Music", sizeof(track.artist) - 1);

                        // Tính thời lượng và kiểm tra tính hợp lệ của file âm thanh
                        int duration = -1;
                        int bitrate = 128;
                        if (lower.endsWith(".mp3")) {
                            strncpy(track.format, "MP3", sizeof(track.format) - 1);
                            duration = parseMp3Duration(file, fsize, bitrate);
                        } else if (lower.endsWith(".wav")) {
                            strncpy(track.format, "WAV", sizeof(track.format) - 1);
                            duration = parseWavDuration(file, fsize, bitrate);
                        }

                        // Nếu phát hiện file FAKE (M4A/AAC/FLAC/OGG đổi đuôi .mp3) hoặc file hỏng -> Bỏ qua
                        if (duration <= 0) {
                            LOG_W("AudioPlayer", "REJECTED fake/corrupt file (skipped): %s", fullPath.c_str());
                            file.close();
                            file = root.openNextFile();
                            continue;
                        }

                        track.durationSec = duration;
                        track.bitrateKbps = bitrate;

                        playlistCount++;
                        LOG_I("AudioPlayer", "Added track [%d]: %s - %s (%s %d kbps, %u bytes, %02d:%02d:%02d)", 
                              playlistCount, track.artist, track.title, track.format, track.bitrateKbps, track.fileSize,
                              track.durationSec / 3600, (track.durationSec % 3600) / 60, track.durationSec % 60);
                    }
                }
            }
            file.close();
        }
        file = root.openNextFile();
    }
    root.close();
}

void AudioPlayerService::cleanupCurrentPlayback() {
    if (mp3Decoder && mp3Decoder->isRunning()) {
        mp3Decoder->stop();
        // Reset hoàn toàn internal state của decoder (buff, outSample, lastRate, lastChannels...)
        // để không mang dữ liệu rác từ bài cũ sang bài mới gây crash khi begin() lần sau
        mp3Decoder->resetPlaybackState();
    }
    if (wavDecoder && wavDecoder->isRunning()) {
        wavDecoder->stop();
    }
    // Xóa audioBuff TRƯỚC audioFile (AudioFileSourceBuffer giữ pointer tới audioFile)
    if (audioBuff) {
        delete audioBuff;
        audioBuff = nullptr;
    }
    if (audioFile) {
        delete audioFile;
        audioFile = nullptr;
    }
}

bool AudioPlayerService::playTrack(int index) {
    if (index < 0 || index >= playlistCount) return false;

    // 1. Tạm dừng state ngay lập tức để audioTask không decode dở dang
    state = STATE_STOPPED;

    if (audioMutex && xSemaphoreTake(audioMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        cleanupCurrentPlayback();

        currentTrackIdx = index;
        const AudioTrack& track = playlist[currentTrackIdx];
        LOG_I("AudioPlayer", "Starting playback: %s (Size: %u bytes)", track.path, track.fileSize);

        audioFile = new AudioFileSourceSafeSD(track.path);
        if (!audioFile || !audioFile->isOpen()) {
            LOG_E("AudioPlayer", "Failed to open audio file: %s", track.path);
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
                mp3Decoder = new FixedAudioGeneratorMP3a();
            }
            if (!mp3Decoder || !mp3Decoder->begin(audioBuff, audioOut)) {
                LOG_E("AudioPlayer", "Failed to start MP3 decoder.");
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
                LOG_E("AudioPlayer", "Failed to start WAV decoder.");
                cleanupCurrentPlayback();
                state = STATE_STOPPED;
                xSemaphoreGive(audioMutex);
                return false;
            }
        }

        playStartMillis = millis();
        pausedElapsedMillis = 0;
        seekOffsetSec = 0;
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
    state = STATE_STOPPED;
    if (audioMutex && xSemaphoreTake(audioMutex, pdMS_TO_TICKS(300)) == pdTRUE) {
        cleanupCurrentPlayback();
        pausedElapsedMillis = 0;
        seekOffsetSec = 0;
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

bool AudioPlayerService::seekToPercent(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    if (currentTrackIdx < 0 || currentTrackIdx >= playlistCount) return false;
    const AudioTrack& track = playlist[currentTrackIdx];
    if (track.fileSize == 0) return false;

    if (audioMutex && xSemaphoreTake(audioMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        if (state == STATE_STOPPED || !audioBuff || !audioFile) {
            xSemaphoreGive(audioMutex);
            return false;
        }

        // Bảo vệ biên cuối file: giữ lại tối thiểu 16KB trước EOF để decoder có đủ buffer frame
        uint32_t maxSafeOffset = (track.fileSize > 16384) ? (track.fileSize - 16384) : 0;
        uint32_t targetByte = (uint32_t)(((uint64_t)track.fileSize * percent) / 100);
        if (targetByte > maxSafeOffset) {
            targetByte = maxSafeOffset;
        }

        String pathStr = String(track.path);
        pathStr.toLowerCase();

        if (pathStr.endsWith(".mp3")) {
            if (mp3Decoder) {
                mp3Decoder->flushAfterSeek();
            }
            audioBuff->seek(targetByte, SEEK_SET);
        } else {
            // WAV seek align 4 bytes for 16-bit stereo sample
            uint32_t headerOffset = 44;
            if (targetByte < headerOffset) targetByte = headerOffset;
            targetByte = (targetByte / 4) * 4;
            audioBuff->seek(targetByte, SEEK_SET);
        }

        int targetSec = (track.durationSec > 0) ? (int)(((int64_t)track.durationSec * percent) / 100) : 0;
        uint32_t now = millis();
        seekOffsetSec = (uint32_t)targetSec;
        playStartMillis = now;
        pausedElapsedMillis = 0;
        if (state == STATE_PAUSED) {
            pauseStartMillis = now;
        }

        xSemaphoreGive(audioMutex);
        LOG_D("AudioPlayer", "Seeked to %d%% (%d s, byte %u)", percent, targetSec, targetByte);
        return true;
    }
    return false;
}

bool AudioPlayerService::seekToSec(int targetSec) {
    int total = getCurrentTotalSec();
    if (total <= 0) return false;
    int pct = (targetSec * 100) / total;
    return seekToPercent(pct);
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
    uint32_t elapsedFromPlay = 0;
    if (now >= playStartMillis + pausedElapsedMillis) {
        elapsedFromPlay = (now - playStartMillis - pausedElapsedMillis) / 1000;
    }
    int totalElapsed = (int)(seekOffsetSec + elapsedFromPlay);
    int totalDuration = getCurrentTotalSec();
    if (totalDuration > 0 && totalElapsed > totalDuration) {
        totalElapsed = totalDuration;
    }
    return totalElapsed;
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
        // Guard: không auto-advance nếu user vừa thủ công đổi bài
        // (state sẽ là STATE_PLAYING nếu playTrack() đã được gọi từ UI trước đó)
        if (state == STATE_PLAYING) return;
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
    while (true) {
        if (state == STATE_PLAYING) {
            bool isDecoded = false;
            if (audioMutex && xSemaphoreTake(audioMutex, pdMS_TO_TICKS(10)) == pdTRUE) {
                // Double-check state sau khi lấy mutex (tránh race nếu playTrack() vừa thay đổi state)
                if (state == STATE_PLAYING && audioBuff != nullptr) {
                    if (mp3Decoder && mp3Decoder->isRunning()) {
                        if (!mp3Decoder->loop()) {
                            mp3Decoder->stop();
                            trackJustFinished = true;
                            state = STATE_STOPPED;
                        }
                        isDecoded = true;
                    } else if (wavDecoder && wavDecoder->isRunning()) {
                        if (!wavDecoder->loop()) {
                            wavDecoder->stop();
                            trackJustFinished = true;
                            state = STATE_STOPPED;
                        }
                        isDecoded = true;
                    }
                }
                xSemaphoreGive(audioMutex);
            }

            if (isDecoded) {
                // Nhường 1ms (1 tick) sau mỗi frame decode:
                // 1) Cho phép IDLE0 chạy mượt mà để feed Watchdog Timer (tránh TG1WDT reset)
                // 2) Tốc độ 44.1kHz đủ thời gian phát từ DMA buffer (2048 samples = ~46ms)
                vTaskDelay(pdMS_TO_TICKS(1));
            } else {
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        } else {
            vTaskDelay(pdMS_TO_TICKS(30));
        }
    }
}


