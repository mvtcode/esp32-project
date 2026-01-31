#include <WiFi.h>

// Thay đổi thông tin WiFi của bạn tại đây
const char* ssid = "HPSTAR";
const char* password = "0964335688";

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n=== ESP32-S3 Super Mini - WiFi Test ===");
  Serial.print("Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  Serial.println("\nConnecting to WiFi...");
  Serial.print("SSID: ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int retry = 0;
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    retry++;
    if (retry > 30) {
      Serial.println("\n[ERROR] Failed to connect WiFi after 15 seconds");
      Serial.println("Please check:");
      Serial.println("  - SSID and password are correct");
      Serial.println("  - WiFi router is powered on");
      Serial.println("  - ESP32 is in range of WiFi signal");
      return;
    }
  }

  Serial.println("\n[SUCCESS] WiFi connected!");
  Serial.println("=== Connection Info ===");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.print("Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("Subnet Mask: ");
  Serial.println(WiFi.subnetMask());
  Serial.print("DNS: ");
  Serial.println(WiFi.dnsIP());
  Serial.print("Signal Strength (RSSI): ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
  Serial.println("========================");
}

void loop() {
  // Kiểm tra kết nối WiFi mỗi 5 giây
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WARNING] WiFi lost, reconnecting...");
    WiFi.reconnect();
  } else {
    // Hiển thị thông tin kết nối
    Serial.print("WiFi OK | IP: ");
    Serial.print(WiFi.localIP());
    Serial.print(" | RSSI: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
  }
  delay(5000);
}
