Bây giờ chúng ta sẽ tập trung vào việc mở rộng `AudioManager` để nhận diện và giải mã thêm định dạng **.wav**

Dưới đây là tệp `refactor.md` tổng hợp các thay đổi cần thiết để tích hợp bộ giải mã WAV vào hệ thống hiện tại của bạn:

````markdown
# Cập nhật Audio Player: Hỗ trợ định dạng .wav

Bản cập nhật này thay đổi cách quản lý bộ giải mã (Generator) để hệ thống tự động nhận diện đuôi file và chọn bộ giải mã phù hợp (MP3 hoặc WAV).

---

### 1. Thay đổi trong `include/AudioManager.h`

Chúng ta cần khai báo thêm thư viện WAV và chuyển con trỏ bộ giải mã sang lớp cơ sở `AudioGenerator` để dùng chung cho cả hai loại file.

```cpp
// Thêm thư viện giải mã WAV
#include <AudioGeneratorWAV.h>

class AudioManager {
    // ... (các phần khác giữ nguyên)
private:
    void setupAudio();
    void stopAudio();
    // Đổi tên hàm để bao quát cả MP3 và WAV
    String getNextTrack(String current, bool next);
    int countTracks();

    // CẢI TIẾN: Sử dụng lớp cha AudioGenerator thay vì chỉ định đích danh MP3
    AudioGenerator *gen;
    AudioFileSourceSD *sourceSD;
    AudioFileSourceID3 *sourceID3;
    AudioFileSourceBuffer *buff;
    AudioOutputI2S *out;
    // ...
};
```
````

---

### 2. Thay đổi trong `src/AudioManager.cpp`

Đây là nơi chúng ta xử lý việc nhận diện đuôi file và khởi tạo bộ giải mã tương ứng.

#### Cập nhật hàm `setupAudio()`

Hàm này sẽ kiểm tra tên file, nếu là `.wav` thì dùng `AudioGeneratorWAV`, nếu là `.mp3` thì dùng `AudioGeneratorMP3`.

```cpp
void AudioManager::setupAudio() {
    stopAudio(); // Luôn dọn dẹp trước khi bắt đầu bài mới

    if (currentMode == MODE_MP3) {
        if (!ensureSD()) return;

        // Giữ nguyên phần cấu hình Output (đã có chống POP của bạn)
        out = new AudioOutputI2S(0, 0, 64);
        out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);
        out->SetGain((volume / 100.0f) * (volume / 100.0f)); // Volume logarit của bạn

        String path = "/" + currentTitle;
        if (SD.exists(path)) {
            sourceSD = new AudioFileSourceSD(path.c_str());

            // Giữ nguyên Buffer chống POP mà bạn đã implement
            buff = new AudioFileSourceBuffer(sourceSD, 16384);

            // KIỂM TRA ĐUÔI FILE ĐỂ CHỌN BỘ GIẢI MÃ
            if (currentTitle.endsWith(".wav") || currentTitle.endsWith(".WAV")) {
                gen = new AudioGeneratorWAV(); // Dùng cho file WAV
                gen->begin(buff, out);
            } else {
                // MP3 thường cần ID3 để bỏ qua dữ liệu metadata ở đầu file
                sourceID3 = new AudioFileSourceID3(buff);
                gen = new AudioGeneratorMP3(); // Dùng cho file MP3
                gen->begin(sourceID3, out);
            }

            isPlaying = true;
            audioSize = sourceSD->getSize();
        }
    }
    // ... (các mode khác giữ nguyên)
}

```

#### Cập nhật hàm quét file `countTracks()` và `getNextTrack()`

Chúng ta cần cho phép hệ thống tìm thấy cả các file có đuôi `.wav`.

```cpp
int AudioManager::countTracks() {
    int count = 0;
    File root = SD.open("/");
    File file = root.openNextFile();
    while (file) {
        String fileName = String(file.name());
        // Thêm điều kiện nhận diện file .wav
        if (!file.isDirectory() &&
            (fileName.endsWith(".mp3") || fileName.endsWith(".MP3") ||
             fileName.endsWith(".wav") || fileName.endsWith(".WAV"))) {
            count++;
        }
        file = root.openNextFile();
    }
    return count;
}

// Tương tự, cập nhật logic trong getNextTrack để không bỏ qua file WAV

```

#### Cập nhật hàm `update()`

Sử dụng con trỏ `gen` (lớp cha) để gọi hàm `loop()` và `isRunning()`.

```cpp
void AudioManager::update() {
    if (xSemaphoreTake(mutex, 10 / portTICK_PERIOD_MS)) {
        if (gen && gen->isRunning()) {
            if (gen->loop()) {
                if (sourceSD) audioPos = sourceSD->getPos();
            } else {
                gen->stop();
                xSemaphoreGive(mutex);
                nextTrack(); // Tự động chuyển bài khi hết
                return;
            }
        }
        xSemaphoreGive(mutex);
    }
}

```

```

### Điểm quan trọng cần lưu ý:
1.  **Dùng chung Buffer**: Cả MP3 và WAV đều được hưởng lợi từ `AudioFileSourceBuffer` mà bạn đã cài đặt để tránh tiếng "pop" do trễ đọc thẻ SD.
2.  **AudioGenerator**: Việc đổi từ `AudioGeneratorMP3*` sang `AudioGenerator*` cho phép mã nguồn xử lý linh hoạt bất kỳ định dạng nào mà thư viện hỗ trợ chỉ bằng một biến duy nhất.
3.  **Thứ tự xóa**: Trong hàm `stopAudio()`, hãy đảm bảo bạn xóa `gen` trước khi xóa `buff` và `sourceSD` để tránh lỗi truy cập vùng nhớ đã giải phóng.

Bạn có muốn tôi kiểm tra lại logic chuyển bài tự động (Auto-next) khi danh sách bài hát có trộn lẫn cả MP3 và WAV không?

```
