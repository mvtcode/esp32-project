Dưới đây là nội dung toàn bộ tệp `refactor.md` được tổng hợp vào một khối mã duy nhất để bạn có thể sao chép nhanh chóng. Giải pháp này tập trung vào việc thay đổi công thức âm lượng sang hệ lũy thừa và sử dụng bộ đệm RAM (Buffer) để triệt tiêu tiếng POP khi đang phát nhạc.

````markdown
# Giải Pháp Tổng Thể: Khắc Phục Volume và Tiếng POP (ESP32 Audio Player)

Tệp này chứa toàn bộ mã nguồn cải tiến cho các thành phần chính của dự án.

---

### 1. AudioManager.cpp (Xử lý Âm lượng & Chống POP)

Sử dụng công thức lũy thừa để làm mịn dải âm lượng và thêm `AudioFileSourceBuffer` để tạo "vùng đệm an toàn" cho dữ liệu MP3 từ thẻ SD.

```cpp
// --- src/AudioManager.cpp ---

// 1. Cải tiến Volume: Chuyển sang hàm mũ để mức 5% thực sự nhỏ
void AudioManager::setVolume(int v) {
    if (xSemaphoreTake(mutex, portMAX_DELAY)) {
        volume = v;
        // Công thức: Gain = (vol/100)^2. Mức 5% sẽ là 0.0025 (rất nhỏ)
        float floatVol = v / 100.0f;
        float logVol = floatVol * floatVol;

        if (currentMode == MODE_BT) {
            bt.volume(logVol);
        } else {
            if (out) out->SetGain(logVol);
        }
        xSemaphoreGive(mutex);
    }
}

// 2. Chống POP khi đang phát: Thêm bộ đệm RAM 16KB
void AudioManager::setupAudio() {
    stopAudio(); // Reset driver I2S để đảm bảo sạch sẽ

    if (currentMode == MODE_BT) {
        bt.begin();
        bt.reconnect();
        bt.I2S(I2S_BCK, I2S_DOUT, I2S_WS);
        bt.volume((volume / 100.0f) * (volume / 100.0f));
    } else if (currentMode == MODE_MP3) {
        if (!ensureSD()) return;

        // Tăng DMA Buffer lên 64 để giảm thiểu rủi ro trễ bus
        out = new AudioOutputI2S(0, 0, 64);
        out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);
        out->SetGain((volume / 100.0f) * (volume / 100.0f));

        String path = "/" + currentTitle;
        sourceSD = new AudioFileSourceSD(path.c_str());
        sourceID3 = new AudioFileSourceID3(sourceSD);

        // CẢI TIẾN QUAN TRỌNG: Buffer RAM lưu trữ dữ liệu đọc trước từ SD
        // Giúp giải mã MP3 không bị gián đoạn khi thẻ SD phản hồi chậm
        buff = new AudioFileSourceBuffer(sourceID3, 16384);

        mp3 = new AudioGeneratorMP3();
        mp3->begin(buff, out); // Chạy MP3 thông qua Buffer

        isPlaying = true;
        mp3Size = sourceSD->getSize();
    }
}
```
````

---

### 2. UIManager.cpp (Giảm tải Bus I2C)

Sử dụng cơ chế "Dirty Check" để chỉ vẽ lại OLED khi dữ liệu thực sự thay đổi, tránh việc Bus I2C chặn CPU quá lâu gây gián đoạn âm thanh.

```cpp
// --- src/UIManager.cpp ---

void UIManager::update(const PlayerStatus &status) {
    static AudioMode lastMode = (AudioMode)-1;
    static bool lastPlay = false;
    static int lastVol = -1;
    static String lastTitle = "";
    static unsigned long lastUpdateSec = 0;

    // Tính toán giây hiện tại để cập nhật đồng hồ
    unsigned long currentMs = (status.isPlaying) ?
        (millis() - status.trackStartTime - status.trackPausedTime) :
        (status.lastPauseStart - status.trackStartTime - status.trackPausedTime);
    unsigned long currentSec = currentMs / 1000;

    // Chỉ thực hiện vẽ lại nếu có thay đổi quan trọng
    bool needsRedraw = (status.mode != lastMode) || (status.isPlaying != lastPlay) ||
                       (status.volume != lastVol) || (status.title != lastTitle) ||
                       (currentSec != lastUpdateSec);

    if (!needsRedraw) return; // Thoát nếu không có gì mới để vẽ

    lastMode = status.mode; lastPlay = status.isPlaying;
    lastVol = status.volume; lastTitle = status.title; lastUpdateSec = currentSec;

    display.clearDisplay();
    // ... (Thực hiện các lệnh vẽ nội dung của bạn) ...
    display.display(); // Lệnh này tốn ~30ms, nay chỉ gọi khi cần
}

```

---

### 3. main.cpp (Cô lập Task Audio)

Đưa Audio Task lên mức ưu tiên cao nhất trên Core 0 để không bị các tiến trình khác làm gián đoạn.

```cpp
// --- src/main.cpp ---

void setup() {
    // ... Khởi tạo các thành phần ...

    // Audio Task: Core 0, Ưu tiên cao (5) để bảo vệ luồng âm thanh
    xTaskCreatePinnedToCore(processAudioTask, "AudioTask", 20000, NULL, 5, &TaskAudioHandle, 0);

    // UI Task: Core 1, Ưu tiên thấp (1) để xử lý màn hình và nút bấm
    xTaskCreatePinnedToCore(processUITask, "UITask", 5000, NULL, 1, &TaskUIHandle, 1);
}

void processAudioTask(void *parameter) {
    audio.setMode(currentMode);
    while (true) {
        audio.update(); // Luôn lấy dữ liệu mượt mà từ Buffer RAM
        vTaskDelay(1); // Trả lại một chút thời gian cho hệ thống
    }
}

void processUITask(void *parameter) {
    while (true) {
        InputAction action = input.update(); // Kiểm tra nút nhấn
        if (action != ACTION_NONE) {
            // Xử lý chuyển bài, tăng/giảm âm lượng...
        }

        PlayerStatus status = audio.getStatus();
        ui.update(status); // Cập nhật màn hình dựa trên Dirty Check

        vTaskDelay(pdMS_TO_TICKS(100)); // Kiểm tra định kỳ mỗi 100ms
    }
}

```

```

```
