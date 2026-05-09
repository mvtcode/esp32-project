Chúng ta định nghĩa 4 màn hình chính:
sẽ có 1 biến để đánh dấu, với mỗi màn hình sẽ có 1 hàm để vẽ và các trạng thái buttons ở màn hình đó.

Màn hình 128x64 pixel, IC driver màn hình: sh1106, giao tiếp i2c
Chúng ta sẽ vẽ thủ công từng màn hình bằng thư viện u8g2

Màn hình thường có 2 vùng:

- Phía trên (header) hiển thị tên màn hình, ví dụ "LISTENING", "GPIO SETTING", ...
- Phía dưới là nội dung của màn hình đó

1. màn hình home (alias: màn hình 1)
   Đây là màn hình chính của ứng dụng. ở màn hình này sẽ listen theo câu khẩu hiệu "Hey Google" hoặc nhấn button boot để đánh thức.

Phía trên là listen, phía dưới, nếu có người dùng nói sẽ có một virtual sóng nhạc thể hiện cường độ to nhỏ của người nói và thể hiện hệ thống đang hoạt động.
Khi để 10s không có voice command thì sẽ chuyển sang chế độ sleep để tiết kiệm điện đồng thời làm tối màn hình.

Ở chế độ sleep thì listen wake word liên tục, hoặc khi nhấn nút boot thì sẽ chuyển sang chế độ listen command.

Ở màn hình này, các button:

- up: không có chức năng (có thể sau này có thể gán cho chức năng nào đó, ví dụ on/off relay 1)
- down: không có chức năng (có thể sau này có thể gán cho chức năng nào đó, ví dụ on/off relay 2)
- enter: vào chế độ menu cài đặt GPIO (vào màn hình 2)
- back: nếu ở chế độ wake thì chuyển sang chế độ sleep, nếu ở chế độ listen thì không làm gì.
- boot: chuyển từ sleep sang chế độ wake up, nếu wake up rồi thì không làm gì.
- Microphone: lắng nghe lệnh wake word (nếu ở chế độ sleep), lắng nghe lệnh (nếu ở chế độ listen)

2. màn hình cài đặt GPIO (alias: màn hình 2)
   Tại màn hình này, sẽ liệt kê các gpio, chúng ta sẽ load một file config.json để biết được gpio này dùng vào việc gì. Ví dụ:

```json
{
  "gpios": [
    {
      "gpio": 1,
      "name": "QUAT TRAN"
    },
    {
      "gpio": 3,
      "name": "DEN BAN"
    },
    {
      "gpio": 4,
      "name": "DEN NGU"
    },
    {
      "gpio": 5,
      "name": "BOM NUOC"
    },
    {
      "gpio": 6,
      "name": "DEN VUON"
    },
    {
      "gpio": 7,
      "name": "QUAT NGU"
    }
  ]
}
```

> **Quy ước lưu trữ Vector Âm thanh (MFCC) trên SPIFFS:**
>
> Để tối ưu hóa RAM và tăng tốc độ xử lý, dữ liệu vector giọng nói sẽ **KHÔNG** được lưu vào `config.json` hay nạp sẵn toàn bộ vào RAM. Thay vào đó, chúng ta sẽ lưu động trực tiếp thành các file nhị phân (`.bin`) trên phân vùng SPIFFS/LittleFS theo một quy ước đặt tên cố định dựa vào ID của GPIO:
>
> - **Lệnh ON:** `/spiffs/cmd_<gpio>_on.bin` (Ví dụ: `/spiffs/cmd_1_on.bin`)
> - **Lệnh OFF:** `/spiffs/cmd_<gpio>_off.bin` (Ví dụ: `/spiffs/cmd_1_off.bin`)
>
> **Cơ chế hoạt động:**
>
> 1. **Kiểm tra trạng thái (Cho UI):** Để biết một lệnh đã được cài đặt hay chưa (có hiển thị `--` hay không), hệ thống chỉ việc kiểm tra xem file có tồn tại không (Ví dụ: `LittleFS.exists("/spiffs/cmd_1_on.bin")`). Không cần phải lưu thêm field nào vào JSON.
> 2. **Ghi đè dễ dàng:** Mỗi GPIO + Lệnh (ON/OFF) chỉ có 1 file duy nhất. Khi người dùng chọn "Change Voice" và ghi âm lại, ESP32 sẽ trực tiếp mở file đó ở chế độ Write để ghi đè dữ liệu mới, không làm thay đổi file `config.json`.
> 3. **Load động (Dynamic Loading):** Khi bắt đầu chế độ LISTEN (chờ lệnh), hệ thống mới quét các GPIO đang được ENABLE, lấy vector từ SPIFFS đẩy vào engine so sánh của ESP-SR. Nhận diện xong hoặc khi thoát chế độ, RAM lập tức được giải phóng.

Nếu không có file config này thì hiển thị default GPIO1...GPIO6.

ở màn hình này, chúng ta hiển thị các GPIO ở dạng list dọc:
ở 1 line sẽ hiển thị thông tin:

- vị trí con trỏ (sẽ có 1 hình tam giác chỉ vào vị trí con trỏ)
- trạng thái ON/OFF/-- (với -- là chưa được set command voice)
- tên của GPIO

```
┌──────────────────────────────────────────────┐
│           [GPIO SETTING]                     │
├──────────────────────────────────────────────┤
│                                              │
│ > [ON]  QUẠT TRẦN                            │
│   [OFF] ĐÈN BÀN                              │
│   [--]  ĐÈN NGỦ                              │
│   [--]  BƠM NƯỚC                             │
│                                              │
└──────────────────────────────────────────────┘
```

Ví dụ chỉ hiển thị được 4 gpio, nếu có nhiều hơn 4 gpio thì hiển thị chế độ cuộn, khi cuộn thì sẽ cập nhật dòng hiển thị con trỏ. khi tới đầu hoặc đáy:

- vị trí top: nhấn phí up thì về cuối danh sách, nhấn down thì tiếp tục scroll lên
- vị trí cuối: nhấn phí down thì về đầu danh sách, nhấn up thì tiếp tục scroll xuống

ở màn hình này, các button:

- up: di chuyển con trỏ lên
- down: di chuyển con trỏ xuống
- enter: vào chế độ cài đặt voice cho GPIO tại vị trí con trỏ (vào màn hình 3)
- back: quay về màn hình 1
- boot: quay về màn hình 1
- Microphone: không có tác dụng

3. màn hình cài đặt voice cho GPIO (alias: màn hình 3)
   Tại màn hình này, chúng ta sẽ có menu con options gồm có:

- Bật/Tắt GPIO (ON/OFF)
- Cài đặt voice command
- DISABLE sẽ chuyển sang trạng thái --

tại màn hình này cũng có con trỏ lên/xuống giống màn hình 2.
Line sẽ có các thành phần:

- Con trỏ
- Tên command
- Trạng thái (nếu có)

```
Ví dụ: quạt trần đã enable và trạng thái đang bật, cài đặt voice tắt chưa được set, nhưng cài đặt voice mở đã được set thì UI như sau:
┌──────────────────────────────────────────────┐
│         [QUẠT TRẦN - ON]                     │
├──────────────────────────────────────────────┤
│                                              │
│ > OFF                                        │
│   Cài đặt voice tắt (--)                     │
│   Cài đặt voice mở                           │
│   DISABLE                                    │
│                                              │
└──────────────────────────────────────────────┘

Ví dụ: quạt trần đã enable và trạng thái đang tắt, cài đặt voice tắt và mở đều được set:
┌──────────────────────────────────────────────┐
│         [QUẠT TRẦN - OFF]                    │
├──────────────────────────────────────────────┤
│                                              │
│ > ON                                         │
│   Cài đặt voice tắt                          │
│   Cài đặt voice mở                           │
│   DISABLE                                    │
│                                              │
└──────────────────────────────────────────────┘

Ví dụ: bơm nước chưa enable, chỉ chọn được ENABLE, các menu khác không chọn được (biểu tượng [x] ở cuối menu là không thể chọn)
┌──────────────────────────────────────────────┐
│         [BƠM NƯỚC --]                        │
├──────────────────────────────────────────────┤
│                                              │
│   OFF                                   [x]  │
│   Cài đặt voice tắt (--)                [x]  │
│   Cài đặt voice mở (--)                 [x]  │
│ > ENABLE                                     │
│                                              │
└──────────────────────────────────────────────┘
```

Các button ở màn hình này:

- up: di chuyển con trỏ lên
- down: di chuyển con trỏ xuống
- enter: chọn
- back: quay về màn hình 2
- boot: về màn hình 1
- Microphone: không có tác dụng

Nếu chọn, nhấn ENTER để vào màn hình phía trong (ở đây chúng ra còn 2 màn hình con: màn hình cài đặt ON/OFF và màn hình cài đặt voice command).

Màn hình 3.1: cài đặt voice command ON
Màn hình 3.2: cài đặt voice command OFF

Tại màn hình này sẽ hiển thị

```
┌──────────────────────────────────────────────┐
│         [BƠM NƯỚC - VOICE ON]                │
├──────────────────────────────────────────────┤
│                                              │
│ > CHANGE VOICE                               │
│   DELETE VOICE                               │
│                                              │
└──────────────────────────────────────────────┘
```

Nếu chọn change voice, màn hình như sau (alias: màn hình 3.1.1 | màn hình 3.2.1)

```
┌──────────────────────────────────────────────┐
│         [BƠM NƯỚC - VOICE ON - CHANGE]       │
├──────────────────────────────────────────────┤
│                                              │
│                 Đang ghi...                  │
│                                              │
│            (biểu tượng sóng âm)              │
│                                              │
│          Enter kết thúc | Back hủy           │
└──────────────────────────────────────────────┘
```

Các button ở màn hình này:

- up: không có tác dụng
- down: không có tác dụng
- enter: kết thúc ghi và save lại
- back: quay về màn hình 3.1 | 3.2 (tùy vào màn hình đang hiển thị)
- boot: về màn hình 1
- Microphone: lắng nghe để ghi

Nếu ở màn hình này, user không nói trong 10s sẽ tự động back.
Nếu microphone lỗi cũng tự động back về màn hình 3.1 | 3.2 (tùy vào màn hình đang hiển thị)

### 5. Cấu trúc Quản lý State Machine cho các màn hình (C/C++)

Việc thiết kế luồng quản lý màn hình và button dựa trên cấu hình khai báo sẵn như ý tưởng của bạn là rất chuẩn. Tuy nhiên, nếu dùng chuỗi JSON (`"go_screen_2"`, `"scroll_up"`) để ánh xạ vào thời gian chạy (runtime), code C/C++ sẽ buộc phải dùng hàng chục câu `if/else` và `strcmp()` để gọi đúng hàm, gây rối mã nguồn và chậm thiết bị.

**Giải pháp mã giả (C/C++ Pseudo-code) sử dụng Máy trạng thái (State Machine) & Con trỏ hàm (Function Pointers):**

Cách này vẫn giữ tính chất "cấu hình từ 1 nơi duy nhất" mà bạn muốn, nhưng bỏ đi mọi lệnh `if/else` rườm rà.

```cpp
// 1. Khai báo các hàm xử lý hành động (Action Functions)
void go_screen_2();
void go_screen_1();
void go_sleep_mode();
void toggle_wake_word();
void scroll_up();
void scroll_down();
void go_screen_3();

void draw_home_screen();
void draw_gpio_screen();

// 2. Định nghĩa cấu trúc cho một Màn hình
typedef struct {
    void (*on_up)();         // Hàm chạy khi ấn phím UP
    void (*on_down)();       // Hàm chạy khi ấn phím DOWN
    void (*on_enter)();      // Hàm chạy khi ấn phím ENTER
    void (*on_back)();
    void (*on_boot)();
    void (*on_mic)();        // Hàm chạy khi Mic bắt được sự kiện
    void (*draw_ui)();       // Hàm vẽ màn hình này bằng thư viện u8g2
} ScreenConfig;

// 3. Khởi tạo danh sách màn hình (Quản lý 1 nơi duy nhất, không dùng JSON string)
const ScreenConfig app_screens[] = {
    // Màn hình 1: HOME (index = 0)
    {
        .on_up = nullptr,
        .on_down = nullptr,
        .on_enter = go_screen_2,
        .on_back = go_sleep_mode,
        .on_boot = toggle_wake_word,
        .on_mic = listen_wake_word,
        .draw_ui = draw_home_screen
    },
    // Màn hình 2: GPIO SETTING (index = 1)
    {
        .on_up = scroll_up,
        .on_down = scroll_down,
        .on_enter = go_screen_3,
        .on_back = go_screen_1,
        .on_boot = go_screen_1,
        .on_mic = nullptr,
        .draw_ui = draw_gpio_screen
    }
    // ... Định nghĩa các màn hình 3, 3.1, 3.2 xuống phía dưới tương tự
};

// 4. Biến toàn cục lưu trạng thái màn hình hiện tại
uint8_t current_screen_id = 0; // Mặc định boot vào HOME

// --- KHI PHẦN CỨNG BẮT ĐƯỢC SỰ KIỆN NÚT BẤM --- //

// Ví dụ khi người dùng bấm phím ENTER:
void handle_button_enter_press() {
    // Không cần if/else kiểm tra mình đang ở màn hình nào!
    // Trỏ thẳng đến cấu hình của màn hình hiện tại và thực thi
    if (app_screens[current_screen_id].on_enter != nullptr) {
        app_screens[current_screen_id].on_enter(); // O(1) complexity
    }
}

// Vòng lặp chính UI của ESP32
void loop() {
    // Engine UI: Tự động lấy hàm vẽ tương ứng với màn hình hiện tại
    if (app_screens[current_screen_id].draw_ui != nullptr) {
         app_screens[current_screen_id].draw_ui();
    }
}
```

Cách thiết kế này giúp mọi module quản lý màn hình được "cứng hóa" ngay từ lúc compile, tránh phân mảnh bộ nhớ của JSON và mã nguồn của bạn hoàn toàn thoát khỏi ma trận `if ... else if ... else`.
