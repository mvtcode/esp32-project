# Giao Thức Giao Tiếp & Bắt Tay (Handshake) XiaoZhi

Tài liệu này tổng hợp chi tiết về quy trình bắt tay (Handshake), cơ chế trao đổi Speech-to-Text (STT), Text-to-Speech (TTS), và truyền nhận âm thanh trong hệ sinh thái XiaoZhi (áp dụng cho cả ESP32 firmware và Python client `py-xiaozhi`).

---

## 1. Xác thực & Kết nối (HTTP Upgrade Handshake)

Trước khi gửi dữ liệu thoại, Client mở kết nối WebSocket tới Server (`wss://` hoặc `ws://`) kèm theo các **HTTP Headers**:

```http
GET / HTTP/1.1
Host: api.xiaozhi.me
Upgrade: websocket
Connection: Upgrade
Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
Sec-WebSocket-Version: 13
Authorization: Bearer <WEBSOCKET_ACCESS_TOKEN>
Protocol-Version: 1
Device-Id: <MAC_ADDRESS_HOAC_DEVICE_ID>
Client-Id: <UUID_HOAC_CLIENT_ID>
```

> **Ghi chú Token:**
> - `WEBSOCKET_ACCESS_TOKEN`: Được server cấp tự động qua luồng OTA / Kích hoạt thiết bị (`/ota/` API) hoặc cấu hình thủ công.
> - Server kiểm tra Header `Authorization`, `Device-Id`, nếu hợp lệ sẽ đồng ý nâng cấp kết nối lên WebSocket (HTTP 101 Switching Protocols).

---

## 2. Ứng dụng Bắt Tay (Application Hello Handshake)

Ngay sau khi kết nối WebSocket thành công, Client và Server gửi bản tin chào hỏi (`hello`) dạng JSON qua WebSocket Text frame:

### Bước 1: Client gửi `hello`
```json
{
  "type": "hello",
  "version": 1,
  "features": {
    "mcp": true,
    "aec": true
  },
  "transport": "websocket",
  "audio_params": {
    "format": "opus",
    "sample_rate": 16000,
    "channels": 1,
    "frame_duration": 60
  }
}
```
* **`transport`**: `"websocket"` (thông báo truyền tải âm thanh trực tiếp qua WebSocket binary frame).
* **`audio_params`**: Cấu hình âm thanh Opus (16kHz, mono 1 channel, 60ms mỗi frame).
* **`features`**: Các tính năng hỗ trợ như `mcp` (Model Context Protocol - điều khiển IoT), `aec` (Acoustic Echo Cancellation phía server).

### Bước 2: Server phản hồi `hello`
```json
{
  "type": "hello",
  "transport": "websocket",
  "session_id": "9f7b3c2a-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "audio_params": {
    "format": "opus",
    "sample_rate": 16000,
    "channels": 1,
    "frame_duration": 60
  }
}
```
* Client nhận được gói này, lưu lại `session_id`, mở luồng Audio Channel và chuyển sang trạng thái sẵn sàng lắng nghe (`Listening`).

---

## 3. Luồng STT (Speech-to-Text) và Gửi Voice Mic

### 3.1. Bắt đầu lắng nghe (Listen Start)
Khi người dùng bấm nút hoặc phát hiện Wake Word (từ khóa đánh thức), Client gửi lệnh báo hiệu:
```json
{
  "session_id": "xxx",
  "type": "listen",
  "state": "start",
  "mode": "auto"
}
```
*(Các chế độ `mode`: `"auto"` - server tự phát hiện kết thúc câu bằng VAD; `"manual"` - giữ nút để nói; `"realtime"`).*

### 3.2. Truyền âm thanh Micro (Uplink Audio)
- Client ghi âm từ micro (I2S/ADC) -> mã hóa **Opus** (khung 60ms) -> gửi trực tiếp dưới dạng **WebSocket Binary Frame**.

### 3.3. Server trả về kết quả STT (Speech-to-Text)
Sau khi Server nhận các khung Opus và nhận diện giọng nói, Server gửi bản tin text JSON về Client:
```json
{
  "session_id": "xxx",
  "type": "stt",
  "text": "Hôm nay thời tiết thế nào?"
}
```
* **Client xử lý:** Lấy chuỗi `text` hiển thị lên màn hình (Subtitle / Chat bubble / TUI) cho người dùng thấy câu mình vừa nói.

---

## 4. Luồng Phản Hồi LLM & TTS (Text-to-Speech)

Sau khi có kết quả STT, AI (LLM) trên Server xử lý và trả về phản hồi:

### 4.1. Server gửi Emotion / Biểu cảm (LLM)
```json
{
  "session_id": "xxx",
  "type": "llm",
  "emotion": "happy",
  "text": "😀"
}
```
* Client cập nhật biểu cảm mắt/khuôn mặt trên màn hình (mặt cười, buồn, bất ngờ, ...).

### 4.2. Bắt đầu TTS (TTS Start)
```json
{
  "session_id": "xxx",
  "type": "tts",
  "state": "start"
}
```
* Client chuyển máy trạng thái sang `Speaking` (đang nói), dừng mic và chuẩn bị bộ giải mã Opus để phát loa.

### 4.3. Hiển thị phụ đề từng câu (TTS Subtitle)
```json
{
  "session_id": "xxx",
  "type": "tts",
  "state": "sentence_start",
  "text": "Hôm nay trời nắng ráo, nhiệt độ khoảng 28 độ C."
}
```
* Client hiển thị câu trả lời dạng chữ lên màn hình.

### 4.4. Nhận âm thanh TTS (Downlink Audio)
* Server gửi liên tiếp các **WebSocket Binary Frame** chứa âm thanh Opus.
* Client nhận các frame nhị phân -> giải mã (Opus Decode) -> đẩy qua I2S / Loa để phát ra tiếng.

### 4.5. Kết thúc TTS (TTS Stop)
```json
{
  "session_id": "xxx",
  "type": "tts",
  "state": "stop"
}
```
* Nếu đang ở chế độ hội thoại liên tục (`auto`), Client tự động chuyển lại về `Listening` để chờ người dùng nói tiếp; nếu không sẽ chuyển về `Idle` (nghỉ).

---

## 5. Ngắt câu / Hủy phản hồi (Abort)

Nếu Server đang phát TTS mà người dùng ngắt lời (bấm nút hoặc nói từ khóa đánh thức):
- Client gửi lệnh `abort`:
```json
{
  "session_id": "xxx",
  "type": "abort",
  "reason": "wake_word_detected"
}
```
- Client xóa buffer âm thanh loa ngay lập tức và chuyển ngay sang `Listening`.

---

## 6. Tổng kết Sơ đồ luồng Giao Tiếp (Message Sequence)

```text
Client (ESP32 / Python)                               Server (XiaoZhi AI)
      |                                                        |
      | ------------- 1. HTTP Upgrade (Bearer Token) --------> |
      | <------------ 2. HTTP 101 Switching Protocols -------- |
      |                                                        |
      | ------------- 3. JSON {"type": "hello"} -------------> |
      | <------------ 4. JSON {"type": "hello"} -------------- |  [Handshake Xong]
      |                                                        |
      | ------------- 5. JSON {"type": "listen", "start"} ---> |
      | ===== Binary Opus Frames (Mic Data) =================> |  [Người dùng nói]
      |                                                        |
      | <------------ 6. JSON {"type": "stt", "text": "..."} - |  [STT - Nhận diện giọng]
      | <------------ 7. JSON {"type": "llm", "emotion": ...}- |  [Emotion]
      | <------------ 8. JSON {"type": "tts", "start"} ------- |
      | <------------ 9. JSON {"type": "tts", "sentence_start"}|  [Phụ đề câu trả lời]
      | <==== Binary Opus Frames (TTS Data) ================== |  [Phát loa]
      | <------------ 10. JSON {"type": "tts", "stop"} ------- |  [Kết thúc phản hồi]
      |                                                        |
```
