#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>

SoftwareSerial mySerial(4, 5); // RX, TX

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

// Cấu hình chân
const int touchPin = 2;        // Chân ngắt - Touch sensor (Verify)
const int enrollPin = 3;       // Chân ngắt - Nút ENROLL
const int deletePin = 6;       // Chân polling - Nút DELETE

// Biến cờ hiệu ngắt
volatile bool fingerTouched = false;  // Cờ verify
volatile bool enrollRequested = false; // Cờ enroll
volatile bool deleteRequested = false; // Cờ delete

// Debounce cho các nút
volatile unsigned long lastEnrollTime = 0;
const unsigned long debounceDelay = 300; // 300ms debounce

// Biến quản lý ID vân tay
uint8_t nextID = 1; // ID tiếp theo để đăng ký

// Khai báo trước các hàm (forward declaration)
void touchISR();
void enrollISR();
int verifyFingerprint();
uint8_t enrollFingerprint();
void deleteFingerprint();

void setup() {
  Serial.begin(9600);
  
  // Cấu hình chân
  pinMode(touchPin, INPUT);    // Touch sensor (3.3V từ module)
  pinMode(enrollPin, INPUT_PULLUP);  // Nút ENROLL (nhấn = LOW)
  pinMode(deletePin, INPUT_PULLUP);  // Nút DELETE (nhấn = LOW)
  
  // Thiết lập ngắt
  attachInterrupt(digitalPinToInterrupt(touchPin), touchISR, RISING);  // Touch sensor
  attachInterrupt(digitalPinToInterrupt(enrollPin), enrollISR, FALLING); // Nút ENROLL

  // Khởi tạo cảm biến vân tay
  finger.begin(57600);
  
  Serial.println("=== HE THONG VAN TAY BIOSEC TA0702 ===");
  Serial.println("- Dat ngon tay len sensor: VERIFY");
  Serial.println("- Nhan nut ENROLL (Pin 3): THEM van tay");
  Serial.println("- Nhan nut DELETE (Pin 6): XOA van tay");
  Serial.println("San sang...\n");
}

void loop() {
  // Kiểm tra nút DELETE (polling vì không có ngắt ngoài)
  if (digitalRead(deletePin) == LOW && !deleteRequested) {
    deleteRequested = true;
    delay(50); // Debounce
  }
  
  // Chế độ 1: ENROLL - Thêm vân tay mới
  if (enrollRequested) {
    Serial.println("\n>>> CHE DO THEM VAN TAY <<<");
    enrollRequested = false; // Reset để tránh lặp
    fingerTouched = false;   // Clear cờ verify
    deleteRequested = false; // Clear cờ delete
    
    // TẮT NGẮT trong lúc enroll để tránh conflict
    detachInterrupt(digitalPinToInterrupt(touchPin));
    detachInterrupt(digitalPinToInterrupt(enrollPin));
    
    uint8_t result = enrollFingerprint();
    if (result == FINGERPRINT_OK) {
      Serial.println("==> THEM VAN TAY THANH CONG!");
      nextID++; // Tăng ID cho lần sau
    } else {
      Serial.println("==> LOI: Khong them duoc van tay!");
    }
    
    // BẬT LẠI NGẮT sau khi xong
    attachInterrupt(digitalPinToInterrupt(touchPin), touchISR, RISING);
    attachInterrupt(digitalPinToInterrupt(enrollPin), enrollISR, FALLING);
    
    // Clear lại tất cả các cờ sau khi xong
    fingerTouched = false;
    deleteRequested = false;
    enrollRequested = false;
    Serial.println("San sang...\n");
  }
  
  // Chế độ 2: DELETE - Xóa vân tay
  else if (deleteRequested) {
    Serial.println("\n>>> CHE DO XOA VAN TAY <<<");
    deleteRequested = false; // Reset
    fingerTouched = false;   // Clear cờ verify
    enrollRequested = false; // Clear cờ enroll
    
    // TẮT NGẮT trong lúc delete
    detachInterrupt(digitalPinToInterrupt(touchPin));
    detachInterrupt(digitalPinToInterrupt(enrollPin));
    
    deleteFingerprint();
    
    // BẬT LẠI NGẮT sau khi xong
    attachInterrupt(digitalPinToInterrupt(touchPin), touchISR, RISING);
    attachInterrupt(digitalPinToInterrupt(enrollPin), enrollISR, FALLING);
    
    // Clear lại tất cả các cờ sau khi xong
    fingerTouched = false;
    enrollRequested = false;
    deleteRequested = false;
    Serial.println("San sang...\n");
  }
  
  // Chế độ 3: VERIFY - Kiểm tra vân tay
  else if (fingerTouched) {
    Serial.println("Phat hien co ngon tay! Dang quet...");
    fingerTouched = false;   // Reset ngay để tránh lặp
    
    int result = verifyFingerprint();
    
    if (result == FINGERPRINT_OK) {
      Serial.println("==> MO CUA THANH CONG!");
      delay(2000);
    } else {
      Serial.println("==> VAN TAY KHONG HOP LE!");
    }
    
    Serial.println("Cho tiep...\n");
  }
}

// ============= CÁC HÀM XỬ LÝ NGẮT (ISR) =============
void touchISR() {
  fingerTouched = true; 
}

void enrollISR() {
  // Debounce: chỉ trigger nếu đã qua 300ms từ lần nhấn trước
  unsigned long currentTime = millis();
  if (currentTime - lastEnrollTime > debounceDelay) {
    enrollRequested = true;
    lastEnrollTime = currentTime;
  }
}

// ============= VERIFY - KIỂM TRA VÂN TAY =============
int verifyFingerprint() {
  uint8_t p = finger.getImage();
  if (p != FINGERPRINT_OK) return -1;
  
  p = finger.image2Tz();
  if (p != FINGERPRINT_OK) return -1;
  
  p = finger.fingerFastSearch();
  return p;
}

// ============= ENROLL - THÊM VÂN TAY MỚI =============
uint8_t enrollFingerprint() {
  uint8_t id = nextID;
  
  Serial.print("Dang ky van tay voi ID #");
  Serial.println(id);
  Serial.println("Vui long dat ngon tay len sensor...");
  
  // Bước 1: Chụp ảnh lần 1
  int p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      // Đợi ngón tay
    } else if (p == FINGERPRINT_OK) {
      Serial.println("Anh 1 OK!");
    } else {
      Serial.print("Loi chup anh 1, ma loi: 0x");
      Serial.println(p, HEX);
      return p;
    }
  }
  
  // Chuyển đổi ảnh sang template
  p = finger.image2Tz(1);
  if (p != FINGERPRINT_OK) {
    Serial.println("Loi chuyen doi anh 1");
    return p;
  }
  
  Serial.println("Nha ngon tay ra...");
  delay(2000);
  
  // Đợi nhả tay
  p = 0;
  while (p != FINGERPRINT_NOFINGER) {
    p = finger.getImage();
  }
  
  Serial.println("Dat lai ngon tay lan 2...");
  
  // Bước 2: Chụp ảnh lần 2
  p = -1;
  while (p != FINGERPRINT_OK) {
    p = finger.getImage();
    if (p == FINGERPRINT_NOFINGER) {
      // Đợi ngón tay
    } else if (p == FINGERPRINT_OK) {
      Serial.println("Anh 2 OK!");
    } else {
      Serial.println("Loi chup anh 2");
      return p;
    }
  }
  
  // Chuyển đổi ảnh sang template
  p = finger.image2Tz(2);
  if (p != FINGERPRINT_OK) {
    Serial.println("Loi chuyen doi anh 2");
    return p;
  }
  
  // Tạo model từ 2 template
  Serial.println("Dang tao model...");
  p = finger.createModel();
  if (p != FINGERPRINT_OK) {
    Serial.println("Loi: 2 anh khong khop!");
    return p;
  }
  
  // Lưu model vào database
  Serial.print("Luu vao ID #");
  Serial.println(id);
  p = finger.storeModel(id);
  if (p == FINGERPRINT_OK) {
    Serial.println("Da luu thanh cong!");
  } else {
    Serial.println("Loi luu model!");
    return p;
  }
  
  return FINGERPRINT_OK;
}

// ============= DELETE - XÓA VÂN TAY =============
void deleteFingerprint() {
  Serial.println("Chon che do xoa:");
  Serial.println("1. Xoa tat ca van tay");
  Serial.println("2. Xoa theo ID cu the");
  Serial.println("Gui '1' hoac '2' qua Serial...");
  
  // Đợi input từ Serial (timeout 10 giây)
  unsigned long timeout = millis() + 10000;
  while (!Serial.available() && millis() < timeout) {
    delay(10);
  }
  
  if (!Serial.available()) {
    Serial.println("Het thoi gian! Huy thao tac.");
    return;
  }
  
  char choice = Serial.read();
  
  if (choice == '1') {
    // Xóa tất cả
    Serial.println("Xoa tat ca van tay...");
    finger.emptyDatabase();
    Serial.println("==> DA XOA TAT CA VAN TAY!");
    nextID = 1; // Reset ID counter
  } 
  else if (choice == '2') {
    // Xóa theo ID
    Serial.println("Nhap ID can xoa (1-127):");
    
    timeout = millis() + 10000;
    while (!Serial.available() && millis() < timeout) {
      delay(10);
    }
    
    if (!Serial.available()) {
      Serial.println("Het thoi gian! Huy thao tac.");
      return;
    }
    
    uint8_t id = Serial.parseInt();
    
    if (id >= 1 && id <= 127) {
      Serial.print("Xoa van tay ID #");
      Serial.println(id);
      
      uint8_t p = finger.deleteModel(id);
      if (p == FINGERPRINT_OK) {
        Serial.println("==> DA XOA THANH CONG!");
      } else {
        Serial.println("==> LOI: Khong xoa duoc!");
      }
    } else {
      Serial.println("ID khong hop le!");
    }
  } 
  else {
    Serial.println("Lua chon khong hop le!");
  }
}