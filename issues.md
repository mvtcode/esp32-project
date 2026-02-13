tôi cần xử lý thêm case của file:

- Tên file có Tiếng Việt (trước tôi gặp case này, esp bị reboot liên tục)
- Tên file dài
- File .mp3 bị lỗi
- File .wav bị lỗi
  Có thể đưa ra phương án giải quyết từng case được không?

---

Dưới đây là tệp `refactor.md` tổng hợp toàn bộ phương án giải quyết cho các vấn đề: **Tên file Tiếng Việt**, **Tên file dài**, **File lỗi (MP3/WAV)** và **Hiển thị tên file tối ưu** (dạng `<Đầu>...<Cuối>`).

Giải pháp này được thiết kế để bảo vệ hệ thống khỏi bị reboot, xử lý linh hoạt đa định dạng và giữ cho âm thanh không bị nhiễu "POP".

````markdown
# ESP32 Audio Player - Refactor: Xử lý File & Hiển thị Thông minh

Tệp này tập trung vào việc gia cố hệ thống (Hardening) để xử lý các tệp tin không chuẩn và tối ưu hóa hiển thị OLED mà không làm ảnh hưởng đến chất lượng âm thanh.

---

## 1. Cập nhật include/AudioManager.h

Cần thay đổi kiểu dữ liệu bộ giải mã và thêm các hàm hỗ trợ kiểm soát lỗi file.

```cpp
#include <AudioGenerator.h>    // Lớp cơ sở cho MP3/WAV
#include <AudioGeneratorMP3.h>
#include <AudioGeneratorWAV.h>

class AudioManager {
public:
    // ... (giữ các hàm public cũ)
private:
    void setupAudio();
    void stopAudio();
    // Đổi tên hàm quét file để hỗ trợ cả WAV
    String getNextTrack(String current, bool next);

    // HÀM MỚI: Xử lý tên file Tiếng Việt và rút gọn tên dài
    String sanitizeFilename(String filename);

    // BIẾN MỚI: Kiểm soát file lỗi
    AudioGenerator *gen;             // Dùng con trỏ lớp cha
    unsigned long trackStartTime;    // Lưu mốc thời gian bắt đầu phát
    // ...
};
```
````

---

## 2. Cập nhật src/AudioManager.cpp

Triển khai logic nhận diện định dạng, lọc ký tự lạ và tự động bỏ qua file hỏng.

```cpp
// 1. Xử lý Tên file: Chống crash do Tiếng Việt & Rút gọn tên dài (Phương án 2)
String AudioManager::sanitizeFilename(String filename) {
    String cleanName = "";
    // Lọc bỏ ký tự không phải ASCII để tránh reboot khi xử lý Unicode Tiếng Việt
    for (int i = 0; i < filename.length(); i++) {
        char c = filename[i];
        if (c >= 32 && c <= 126) cleanName += c;
        else cleanName += "?";
    }

    // Nếu tên quá dài, rút gọn theo dạng: 8 ký tự đầu ... 7 ký tự cuối (kèm đuôi file)
    if (cleanName.length() > 20) {
        return cleanName.substring(0, 8) + "..." + cleanName.substring(cleanName.length() - 7);
    }
    return cleanName;
}

// 2. Setup Audio: Hỗ trợ WAV và xử lý file lỗi ngay khi khởi tạo
void AudioManager::setupAudio() {
    stopAudio();
    if (!ensureSD()) return;

    // Lọc bỏ file rác hệ thống (thường bắt đầu bằng dấu chấm)
    if (currentTitle.startsWith("._") || currentTitle.startsWith(".")) {
        nextTrack(); return;
    }

    out = new AudioOutputI2S(0, 0, 64);
    out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);
    out->SetGain((volume / 100.0f) * (volume / 100.0f));

    String path = "/" + currentTitle;
    sourceSD = new AudioFileSourceSD(path.c_str());

    // Sử dụng Buffer RAM đã có để chống POP
    buff = new AudioFileSourceBuffer(sourceSD, 16384);

    // Tự động chọn bộ giải mã dựa trên đuôi file
    if (currentTitle.endsWith(".wav") || currentTitle.endsWith(".WAV")) {
        gen = new AudioGeneratorWAV();
    } else {
        sourceID3 = new AudioFileSourceID3(buff);
        gen = new AudioGeneratorMP3();
    }

    // Kiểm tra khởi tạo thành công (Xử lý file lỗi cấu trúc nặng)
    AudioFileSource *src = (currentTitle.endsWith(".wav")) ? (AudioFileSource*)buff : (AudioFileSource*)sourceID3;
    if (!gen->begin(src, out)) {
        nextTrack(); // Bỏ qua nếu file không thể mở
        return;
    }

    // Lưu tên đã được rút gọn an toàn để UI Task lấy dữ liệu
    currentTitle = sanitizeFilename(currentTitle);
    isPlaying = true;
    trackStartTime = millis();
}

// 3. Update loop: Xử lý file lỗi (tự ngắt trong 1.5 giây đầu)
void AudioManager::update() {
    if (xSemaphoreTake(mutex, 10 / portTICK_PERIOD_MS)) {
        if (gen && gen->isRunning()) {
            if (gen->loop()) {
                // ... cập nhật vị trí nhạc ...
            } else {
                gen->stop();
                // Nếu bài nhạc kết thúc quá nhanh (<1.5s), coi như file lỗi và skip
                if (millis() - trackStartTime < 1500) {
                    xSemaphoreGive(mutex);
                    nextTrack();
                    return;
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

## 3. Cập nhật src/UIManager.cpp

Đảm bảo hiển thị tên file trên một dòng duy nhất, không sử dụng Marquee để tiết kiệm CPU và tránh tiếng POP.

```cpp
void UIManager::update(const PlayerStatus &status) {
    // ... (Giữ logic Dirty Check để tiết kiệm bus I2C) ...

    display.clearDisplay();
    display.setTextSize(1);

    // Hiển thị Title trên 1 dòng duy nhất (đã được rút gọn ở AudioManager)
    display.setCursor(0, 25);
    display.print(status.title); // Ví dụ hiển thị: "Tinhca_...wav"

    // Vẽ Volume và thanh tiến trình ở các dòng tiếp theo
    // ...
    display.display();
}

```

---

## 4. Cấu trúc Task (main.cpp)

Đảm bảo Audio Task chiếm Core 0 với ưu tiên cao nhất để việc giải mã WAV (bitrate cao) không bị gián đoạn.

```cpp
void processAudioTask(void *parameter) {
    audio.setMode(currentMode);
    while (true) {
        audio.update(); // Giải mã liên tục
        vTaskDelay(1);
    }
}

```

```

### Tổng kết phương án:
* **Tiếng Việt & Tên dài:** Xử lý qua `sanitizeFilename` để đảm bảo chuỗi ký tự an toàn trước khi đi vào các hàm xử lý chuỗi của hệ thống hoặc hiển thị OLED.
* **File lỗi:** Kiểm tra lỗi ở 2 cấp độ: ngay khi mở file (`gen->begin`) và trong lúc phát (nếu kết thúc bất thường trong 1.5 giây đầu).
* **Hiển thị:** Chọn phương án 2 (Rút gọn tên) để bảo vệ chất lượng âm thanh, tránh xung đột bus I2C do hiệu ứng Marquee gây ra.

Bạn có muốn tôi hỗ trợ thêm việc liệt kê danh sách các file lỗi vào một tệp log trên thẻ SD để bạn dễ dàng kiểm tra sau này không?

```
