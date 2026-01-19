/*
  Tác giả: 芳芳
  Phiên bản: 1.0
  Mô tả: Điều khiển module vân tay TZM1026 bằng Arduino Nano
*/
#include <Arduino.h>
#include <SoftwareSerial.h>

// Module TX -> D2, Module RX -> D3
SoftwareSerial mySerial(2, 3);

int vo = 2;

/*
  ZLJ: Danh sách lệnh HEX gửi tới module vân tay
  Các comment bên dưới mô tả chức năng chi tiết của từng lệnh
*/
const char *ZLJ[] = {

    "F5090000000009F5", // 0. Lấy số lượng người dùng
    "F5600000000060F5", // 1. Lấy ID (mã định danh) duy nhất
    "F52D000001002CF5", // 2. Thiết lập mức độ so khớp (độ nghiêm ngặt)
    "F5260000000026F5", // 3. Thiết lập chế độ đăng ký vân tay
    "F528000200002AF5", // 4. Thiết lập cấp phân quyền người dùng
    "F5050000000005F5", // 5. Xóa toàn bộ dữ liệu người dùng
    "F52D000000002DF5", // 6. Cho phép hoặc cấm đăng ký người dùng mới
    "F5280002000000F5", // 7. Thiết lập lại mức độ so khớp
    "F53F0000000000F5", // 8. Thiết lập chế độ đăng ký
    "F53F0001000000F5", // 9. Thiết lập mức xác thực cùng cấp
    "F53F000201003CF5", // 10. Thiết lập mức xác thực cùng cấp (mức 2)
    "F53F000303003FF5", // 11. Thiết lập tần suất thu thập vân tay
    "F53F004000007FF5", // 12. Thiết lập cấp độ cho người dùng chỉ định
    "F53F000401003AF5", // 13. Thiết lập chức năng điều khiển bằng nút bấm
    "F52B000000002BF5", // 14. Lấy toàn bộ dữ liệu người dùng
    "F5010000010000F5", // 15. Đăng ký bằng nhiều lần nhấn (chế độ 3CNR)
    "F5010000010000F5", // 16. Đăng ký bằng nhiều lần nhấn (chế độ NCNR)
    "F50B0000000000F5", // 17. So khớp vân tay 1-1
    "F50C0000000000F5", // 18. So khớp vân tay 1:N
    "F5240000000024F5", // 19. Lấy ảnh vân tay
    "F5B80000000000F5", // 20. Hủy / ngắt thao tác hiện tại
};

void setup() {
    Serial.begin(115200);
    mySerial.begin(115200);

    Serial.println("Khoi dong module van tay TZM1026");
}

/*
  Hàm gửi chuỗi HEX sang module
*/
void FSSJ(char ZL) {
    static byte SJ[8];
    static int ca2[2];
    static int ca3 = 0;
    static int SJBJ = 0;

    int ca4 = 0;
    char ca = ZL;

    if (ca >= '0' && ca <= 'F') {
        if (ca <= '9') ca4 = ca - 48;
        else ca4 = (ca - 'A') + 10;

        ca2[ca3] = ca4;

        if (ca3 == 1) {
            SJ[SJBJ] = ca2[0] * 16 + ca2[1];
            ca3 = 0;
            SJBJ++;
        } else ca3++;
    }

    if (SJBJ == 8) {
        for (int i = 0; i < 8; i++) {
            mySerial.write(SJ[i]);
            Serial.print(SJ[i], HEX);
        }
        Serial.println();
        SJBJ = 0;
    }
}

/*
  Nhận và hiển thị dữ liệu phản hồi từ module
*/
void FHSJ() {
    if (mySerial.available()) {
        byte response[8];
        int idx = 0;
        
        // Đọc tối đa 8 byte
        while (mySerial.available() && idx < 8) {
            response[idx++] = mySerial.read();
            delay(5); // Đợi byte tiếp theo
        }
        
        // Hiển thị raw data
        Serial.print("Phan hoi HEX: ");
        for (int i = 0; i < idx; i++) {
            if (response[i] < 0x10) Serial.print("0");
            Serial.print(response[i], HEX);
            Serial.print(" ");
        }
        Serial.println();
        
        // Kiểm tra gói tin hợp lệ (bắt đầu và kết thúc bằng 0xF5)
        if (idx >= 8 && response[0] == 0xF5 && response[7] == 0xF5) {
            byte cmd = response[1];
            byte data1 = response[2];
            byte data2 = response[3];
            byte data3 = response[4];
            byte data4 = response[5];
            byte status = data3; // Byte trạng thái thường ở vị trí này
            
            Serial.print(">> ");
            
            // Giải mã theo lệnh
            switch(cmd) {
                case 0x09: // Lấy số lượng người dùng
                    {
                        int userCount = (data1 << 8) | data2;
                        Serial.print("So luong nguoi dung: ");
                        Serial.println(userCount);
                    }
                    break;
                    
                case 0x60: // Lấy ID duy nhất
                    Serial.print("ID module: ");
                    Serial.print(data1, HEX);
                    Serial.print(data2, HEX);
                    Serial.print(data3, HEX);
                    Serial.println(data4, HEX);
                    break;
                    
                case 0x01: // Đăng ký vân tay
                    if (status == 0x00) {
                        Serial.println("DANG KY THANH CONG!");
                    } else if (status == 0x01) {
                        Serial.println("Dang cho cham ngon tay...");
                    } else if (status == 0x02) {
                        Serial.println("Dang xu ly anh van tay...");
                    } else if (status == 0x11) {
                        Serial.println("LOI: Van tay da ton tai!");
                    } else if (status == 0x12) {
                        Serial.println("LOI: Khong doc duoc van tay!");
                    } else {
                        Serial.print("Trang thai: 0x");
                        Serial.println(status, HEX);
                    }
                    break;
                    
                case 0x0B: // So khớp 1:1
                case 0x0C: // So khớp 1:N
                    if (status == 0x00) {
                        int userId = (data1 << 8) | data2;
                        Serial.print("XAC THUC THANH CONG! User ID: ");
                        Serial.println(userId);
                    } else if (status == 0x01) {
                        Serial.println("Dang cho cham ngon tay...");
                    } else if (status == 0x11) {
                        Serial.println("KHONG KHOP! Van tay khong tim thay.");
                    } else if (status == 0x12) {
                        Serial.println("LOI: Khong doc duoc van tay!");
                    } else {
                        Serial.print("Trang thai: 0x");
                        Serial.println(status, HEX);
                    }
                    break;
                    
                case 0x05: // Xóa toàn bộ
                    if (status == 0x00) {
                        Serial.println("XOA TOAN BO DU LIEU THANH CONG!");
                    } else {
                        Serial.println("LOI: Khong the xoa du lieu!");
                    }
                    break;
                    
                case 0x2B: // Lấy toàn bộ dữ liệu
                    Serial.print("Du lieu nguoi dung - Data1: ");
                    Serial.print(data1, HEX);
                    Serial.print(", Data2: ");
                    Serial.println(data2, HEX);
                    break;
                    
                case 0x2D: // Thiết lập mức so khớp
                case 0x26: // Thiết lập chế độ đăng ký
                case 0x28: // Thiết lập phân quyền
                case 0x3F: // Các thiết lập khác
                    if (status == 0x00) {
                        Serial.println("CAU HINH THANH CONG!");
                    } else {
                        Serial.print("Trang thai cau hinh: 0x");
                        Serial.println(status, HEX);
                    }
                    break;
                    
                case 0x38: // Hủy thao tác
                    Serial.println("DA HUY THAO TAC!");
                    break;
                    
                case 0x0D: // Lấy ảnh vân tay
                    if (status == 0x00) {
                        Serial.println("LAY ANH VAN TAY THANH CONG!");
                    } else {
                        Serial.println("LOI: Khong lay duoc anh!");
                    }
                    break;
                    
                default:
                    Serial.print("Lenh 0x");
                    Serial.print(cmd, HEX);
                    Serial.print(" - Trang thai: 0x");
                    Serial.println(status, HEX);
                    break;
            }
        } else if (idx > 0) {
            Serial.println(">> Goi tin khong hop le hoac chua day du!");
        }
    }
}

/*
  Gửi lệnh theo chỉ số trong mảng ZLJ
*/
void FF(int sum) {
    char *ZL = ZLJ[sum];
    for (int i = 0; ZL[i] != '\0'; i++) {
        FSSJ(ZL[i]);
    }
    FHSJ();
    delay(1500);
}

void loop() {
    if (Serial.available()) {
        int sum = Serial.parseInt();
        Serial.print("Dang thuc hien lenh so: ");
        Serial.println(sum);
        FF(sum);
    }
}