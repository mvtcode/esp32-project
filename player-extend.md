# ESP32 Audio Player - Extension Module (.wav, .aac, .flac, .m4a & Error Handling)

Tệp này cung cấp các đoạn mã cần thiết để nâng cấp trình phát nhạc hiện tại của bạn, giải quyết triệt để 4 vấn đề: Tên file Tiếng Việt/Dài, File lỗi, và hỗ trợ đa định dạng âm thanh.

## 1. Cấu hình PlatformIO (platformio.ini)

Đảm bảo bạn đã thêm các thư viện cần thiết để hỗ trợ giải mã.

```ini
lib_deps =
    earlephilhower/ESP8266Audio @ ^1.9.7  ; Thư viện lõi

```

---

## 2. Khai báo mở rộng (include/AudioManager.h)

Chúng ta chuyển sang sử dụng lớp cơ sở `AudioGenerator` để có thể điều khiển linh hoạt mọi loại định dạng.

```cpp
#include <AudioGenerator.h>
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>
#include <AudioGeneratorAAC.h>
#include <AudioGeneratorFLAC.h>

class AudioManager {
private:
    AudioGenerator *gen;             // Con trỏ đa năng cho mọi loại file
    unsigned long lastTrackStartTime; // Để kiểm tra file lỗi (skip nếu dừng quá sớm)

    String sanitizeFilename(String filename); // Xử lý Tiếng Việt & Độ dài
    bool isSupportedFile(String fileName);    // Kiểm tra đuôi file
};

```

---

## 3. Logic xử lý lỗi và Đa định dạng (src/AudioManager.cpp)

### A. Xử lý Tên file (Tiếng Việt & Marquee thay thế)

Hàm này ngăn chặn việc ESP32 bị reboot do ký tự lạ và rút gọn tên file để không tràn màn hình.

```cpp
String AudioManager::sanitizeFilename(String filename) {
    String cleanName = "";
    // 1. Lọc ký tự ASCII (Chống reboot do Tiếng Việt/Unicode lạ)
    for (int i = 0; i < filename.length(); i++) {
        char c = filename[i];
        if (c >= 32 && c <= 126) cleanName += c;
        else cleanName += "?";
    }

    // 2. Rút gọn tên dài (Phương án: <8 ký tự đầu>...<7 ký tự cuối>)
    if (cleanName.length() > 20) {
        return cleanName.substring(0, 8) + "..." + cleanName.substring(cleanName.length() - 7);
    }
    return cleanName;
}

```

### B. Nhận diện và Khởi tạo Giải mã (MP3, WAV, AAC, FLAC)

Hệ thống tự động chọn "vũ khí" phù hợp cho từng đuôi file và bỏ qua file lỗi ngay lập tức.

```cpp
void AudioManager::setupAudio() {
    stopAudio();
    if (!ensureSD()) return;

    // Lọc file rác hệ thống (._ hoặc file ẩn)
    if (currentTitle.startsWith("._") || currentTitle.startsWith(".")) {
        nextTrack(); return;
    }

    String path = "/" + currentTitle;
    sourceSD = new AudioFileSourceSD(path.c_str());
    buff = new AudioFileSourceBuffer(sourceSD, 16384); // Buffer RAM chống POP

    String ext = currentTitle;
    ext.toLowerCase();

    // CHỌN GENERATOR PHÙ HỢP
    if (ext.endsWith(".wav"))      gen = new AudioGeneratorWAV();
    else if (ext.endsWith(".flac")) gen = new AudioGeneratorFLAC();
    else if (ext.endsWith(".aac") || ext.endsWith(".m4a")) gen = new AudioGeneratorAAC();
    else {
        sourceID3 = new AudioFileSourceID3(buff);
        gen = new AudioGeneratorMP3();
    }

    // Kiểm tra file lỗi ngay khi bắt đầu
    AudioFileSource *src = (ext.endsWith(".mp3")) ? (AudioFileSource*)sourceID3 : (AudioFileSource*)buff;
    if (!gen->begin(src, out)) {
        Serial.println("File hỏng, bỏ qua...");
        nextTrack();
        return;
    }

    // Sanitize title để Task UI hiển thị an toàn
    currentTitle = sanitizeFilename(currentTitle);
    isPlaying = true;
    lastTrackStartTime = millis();
}

```

### C. Tự động bỏ qua file "chết" (update loop)

Nếu file lỗi khiến bộ giải mã dừng ngay lập tức (dưới 1.5 giây), hệ thống sẽ tự động chuyển bài.

```cpp
void AudioManager::update() {
    if (xSemaphoreTake(mutex, 10 / portTICK_PERIOD_MS)) {
        if (gen && gen->isRunning()) {
            if (!gen->loop()) {
                gen->stop();
                // Nếu chơi chưa được 1.5s đã ngắt -> File bị lỗi dữ liệu
                if (millis() - lastTrackStartTime < 1500) {
                    Serial.println("Phát hiện file lỗi trong khi chơi. Đang chuyển bài...");
                }
                xSemaphoreGive(mutex);
                nextTrack();
                return;
            }
        }
        xSemaphoreGive(mutex);
    }
}

```

---

## 4. Hiển thị UI ổn định (src/UIManager.cpp)

Đảm bảo tên file được hiển thị trên 1 dòng duy nhất mà không làm nghẽn bus I2C.

```cpp
void UIManager::update(const PlayerStatus &status) {
    // ... (Giữ logic Dirty Check để tiết kiệm bus I2C)

    display.clearDisplay();
    display.setCursor(0, 25);
    // Hiển thị tên đã xử lý (Ví dụ: "MySong_...mp3")
    display.print(status.title);

    // ... (Các thành phần khác)
    display.display();
}

```

```

---

### Điểm nhấn kỹ thuật:
* **Tính khả dụng**: Hỗ trợ các định dạng .m4a (Apple), .flac (Lossless), .aac và .wav.
* **Độ tin cậy**: Hàm `sanitizeFilename` là lớp bảo vệ cuối cùng để tránh lỗi truy cập vùng nhớ khi gặp ký tự UTF-8 phức tạp của Tiếng Việt.
* **Trải nghiệm**: Cơ chế `lastTrackStartTime` giúp thiết bị của bạn không bao giờ bị "kẹt" ở một file lỗi, nó sẽ tự động tìm đến bài hát tiếp theo có thể chơi được.

Bạn có muốn tôi bổ sung thêm đoạn code để hiển thị một thông báo **"FILE ERROR"** nhấp nháy trên màn hình mỗi khi nó bỏ qua một file bị hỏng không?

```
