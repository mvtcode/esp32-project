#include <Arduino.h>
#include <Wire.h>
#include <Preferences.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include "btAudio.h"

// Audio libraries for MP3/Radio
#include <AudioGeneratorMP3.h>
#include <AudioFileSourceSD.h>
#include <AudioFileSourceID3.h>
#include <AudioFileSourceICYStream.h>
#include <AudioFileSourceBuffer.h>
#include <AudioOutputI2S.h>
#include "driver/i2s.h"

// WiFi for Radio
#include <WiFi.h>

// --- HARDWARE PIN DEFINITIONS ---
// I2S (PCM5102)
#define I2S_BCK     26
#define I2S_WS      27
#define I2S_DOUT    25

// OLED (I2C)
#define OLED_SDA    21
#define OLED_SCL    22
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

// SD Card (SPI)
#define SD_CS       5
#define SD_MOSI     23
#define SD_MISO     19
#define SD_SCK      18

// Buttons
#define BTN_MODE    32
#define BTN_PLAY    33
#define BTN_PREV_VOL_MINUS 34 // Pin 34: Vol- (Short) / Prev (Long)
#define BTN_NEXT_VOL_PLUS  35 // Pin 35: Vol+ (Short) / Next (Long)

// --- CONSTANTS & ENUMS ---
enum AudioMode {
  MODE_BT = 0,
  MODE_MP3,
  MODE_RADIO
};

#define DEBOUNCE_DELAY 50
#define RADIO_URL "http://stream.radioreklama.bg:80/radio1rock128" // Example URL

// --- GLOBALS ---
static Preferences preferences;
static Adafruit_SH1106G display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Audio Objects
static btAudio bt("ESP32-Audio-Player");

// MP3/Radio Objects
static AudioGeneratorMP3 *mp3 = NULL;
static AudioFileSourceSD *sourceSD = NULL;
static AudioFileSourceICYStream *sourceStream = NULL;
static AudioFileSourceBuffer *buff = NULL;
static AudioOutputI2S *out = NULL;

// State Variables
static int currentMode = MODE_BT;
static int volume = 80; // 0-100
static bool isPlaying = false;
static String currentTitle = "";
static String currentArtist = "";

// Tasks
static TaskHandle_t TaskAudioHandle = NULL;
static TaskHandle_t TaskUIHandle = NULL;
static SemaphoreHandle_t audioMutex = NULL;

// Timing & Progress (Shared, Thread-safe)
static unsigned long trackStartTime = 0;
static unsigned long trackPausedTime = 0;
static unsigned long lastPauseStart = 0;
volatile uint32_t g_mp3_Pos = 0;
volatile uint32_t g_mp3_Size = 0;
static int totalTracks = 0;
static int currentTrackIndex = 0;
static AudioFileSourceID3 *sourceID3 = NULL;

// Button States
struct ButtonState {
  uint8_t pin;
  bool isPressed;
  unsigned long pressTime;
  bool longPressHandled;
};

static ButtonState btnMode = {BTN_MODE, false, 0, false};
static ButtonState btnPlay = {BTN_PLAY, false, 0, false};
static ButtonState btnPrevVolMinus = {BTN_PREV_VOL_MINUS, false, 0, false};
static ButtonState btnNextVolPlus  = {BTN_NEXT_VOL_PLUS, false, 0, false};

// Function Prototypes
void saveSettings();
void loadSettings();
void setupAudio();
void stopAudio();
void handleButtons();
void updateDisplay();
void processAudioTask(void *parameter);
void processUITask(void *parameter);
void enterDeepSleep();
void nextTrack();
void prevTrack();
void togglePlayPause();
String getNextMP3(String current, bool next);
String formatTime(unsigned long ms);
int countTracks();

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  
  audioMutex = xSemaphoreCreateMutex();
  
  // Init Buttons
  pinMode(BTN_MODE, INPUT_PULLUP);
  pinMode(BTN_PLAY, INPUT_PULLUP);
  pinMode(BTN_PREV_VOL_MINUS, INPUT); // Requires external pull-up
  pinMode(BTN_NEXT_VOL_PLUS, INPUT);  // Requires external pull-up

  // Check Wakeup Reason (Deep Sleep)
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke up from Deep Sleep!");
  }

  // Load NVS
  loadSettings();

  // Init OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SCREEN_ADDRESS, true)) {
    Serial.println("OLED Init Failed");
  }
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 Audio Player");
  display.display();
  delay(1000);

  // Init SD Card if needed for MP3
  if (currentMode == MODE_MP3) {
    if(!SD.begin(SD_CS)) {
      Serial.println("SD Card Init Failed!");
      display.println("SD Fail!");
      display.display();
      delay(2000);
    }
  }

  // Connect WiFi if needed for Radio
  if (currentMode == MODE_RADIO) {
    WiFi.begin("HPSTAR", "0964335688"); // WiFi Creds
    Serial.print("Connecting WiFi");
     int tryCount = 0;
    while (WiFi.status() != WL_CONNECTED && tryCount < 20) {
      delay(500);
      Serial.print(".");
      tryCount++;
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("Connected");
    } else {
        Serial.println("WiFi Fail");
    }
  }

  // Create Tasks
  // Create Tasks
  xTaskCreatePinnedToCore(
    processAudioTask,   "AudioTask",   20000,  NULL,  3,  &TaskAudioHandle,  0 // Core 0 (High Priority)
  );

  xTaskCreatePinnedToCore(
    processUITask,      "UITask",      4096,   NULL,  1,  &TaskUIHandle,     1 // Core 1 (Low Priority)
  );

  // Configure Wakeup Source for Deep Sleep
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BTN_MODE, 0); // Wake on Low
}

void loop() {
  vTaskDelete(NULL); // Loop is not used
}

// --- TASKS ---

void processAudioTask(void *parameter) {
  Serial.println("Audio Task Started");
  
  if (xSemaphoreTake(audioMutex, portMAX_DELAY)) {
      setupAudio();
      xSemaphoreGive(audioMutex);
  }
  
  while(true) {
    if (xSemaphoreTake(audioMutex, 10 / portTICK_PERIOD_MS)) {
        if (currentMode == MODE_BT) {
          if (isPlaying) {
             static unsigned long lastMeta = 0;
             if (millis() - lastMeta > 1000) {
               lastMeta = millis();
               bt.updateMeta();
             }
          }
        } 
        else if (currentMode == MODE_MP3 || currentMode == MODE_RADIO) {
          if (mp3 && mp3->isRunning()) {
            
            // Only loop if playing (Paused MP3 should not consume data)
            // Radio also pauses by stopping loop? No, Radio should stop/start stream.
            // For MP3:
            if (currentMode == MODE_MP3 && !isPlaying) {
                // Do nothing (Paused)
            } else {
                if (mp3->loop()) {
                   // Update shared progress occasionally (e.g., every 10 calls) to reduce partial overhead
                   // But since it's just a memory write, every loop is fine.
                   // However, getPos() might be slow? NO, AudioFileSourceSD::getPos() calls f.position().
                   // Let's do it every 100ms equivalent or so? 
                   // Actually, safer to do it every loop but we must ensure getPos doesn't block.
                   // The SD library is not thread safe, but here we OWN the mutex.
                   // The issue was updateDisplay accessing it WITHOUT mutex.
                   // So we update the shared volatile variable here.
                   if (sourceSD) {
                       g_mp3_Pos = sourceSD->getPos(); 
                   }
                } else {
                  mp3->stop();
                  Serial.println("Track Ended");
                  if (currentMode == MODE_MP3) {
                      xSemaphoreGive(audioMutex); // Give it up temporarily
                      nextTrack(); 
                      continue; 
                  }
                }
            }
          }
        }
        xSemaphoreGive(audioMutex);
    }
    delay(10); 
  }
}

void processUITask(void *parameter) {
  unsigned long lastUpdate = 0;
  while(true) {
    handleButtons();
    
    // Throttle Display Update to 5Hz (200ms) to save bus bandwidth
    if (millis() - lastUpdate > 200) {
        updateDisplay();
        lastUpdate = millis();
    }
    
    delay(50); // Keep button polling responsive
  }
}

void setupAudio() {
  Serial.println("setupAudio() called");
  stopAudio(); 

  if (currentMode == MODE_BT) {
    bt.begin();
    bt.reconnect();
    bt.I2S(I2S_BCK, I2S_DOUT, I2S_WS);
    bt.volume(volume / 100.0);
    isPlaying = true;
    currentTitle = "Waiting for BT...";
  } 
  else if (currentMode == MODE_MP3) {
    Serial.println("Setting up MP3...");
    // 0 = Port 0
    // 0 = EXTERNAL_I2S
    // 32 = DMA Buffer Count (Increased from default 8 to reduce popping)
    out = new AudioOutputI2S(0, 0, 32); 
    out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);
    out->SetGain(volume / 100.0);
    
    if (currentTitle == "" || currentTitle == "No MP3 Files") {
// ...
// ... (inside setupAudio)
        Serial.println("Scanning for first MP3...");
        totalTracks = countTracks();
        String first = getNextMP3("", true);
        if (first != "") currentTitle = first.substring(1);
        else currentTitle = "No MP3 Files";
    }
    
    Serial.printf("Loading MP3: %s\n", currentTitle.c_str());

    if (currentTitle != "No MP3 Files" && currentTitle != "") {
        String path = "/" + currentTitle;
        if (SD.exists(path)) {
            sourceSD = new AudioFileSourceSD(path.c_str());
            sourceID3 = new AudioFileSourceID3(sourceSD);
            mp3 = new AudioGeneratorMP3();
            mp3->begin(sourceID3, out); // Use ID3 source
            isPlaying = true;
            // Reset Timing
            trackStartTime = millis();
            trackPausedTime = 0;
            // Update Size
            g_mp3_Size = sourceSD->getSize();
            
            Serial.println("MP3 Started");
        } else {
            Serial.println("File does not exist!");
            currentTitle = "File Error";
        }
    }
  }
  else if (currentMode == MODE_RADIO) {
    Serial.println("Setting up Radio...");
    out = new AudioOutputI2S(0, 0, 32);
    out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);
    out->SetGain(volume / 100.0);

    sourceStream = new AudioFileSourceICYStream(RADIO_URL);
    buff = new AudioFileSourceBuffer(sourceStream, 1024 * 16); 
    mp3 = new AudioGeneratorMP3();
    mp3->begin(buff, out);
    isPlaying = true;
    currentTitle = "Internet Radio";
  }
}

void stopAudio() {
  if (mp3) { mp3->stop(); delete mp3; mp3 = NULL; }
  if (buff) { delete buff; buff = NULL; }
  if (sourceStream) { delete sourceStream; sourceStream = NULL; }
  if (sourceID3) { delete sourceID3; sourceID3 = NULL; }
  if (sourceSD) { delete sourceSD; sourceSD = NULL; }
  if (out) { delete out; out = NULL; }
  
  if (currentMode == MODE_BT) {
      bt.end(); 
  }
  
  // Force uninstall I2S driver to ensure clean state
  i2s_driver_uninstall(I2S_NUM_0);
  delay(100);
}

// --- HELPER --
int countTracks() {
    int count = 0;
    File root = SD.open("/");
    if (!root) return 0;
    
    File file = root.openNextFile();
    while (file) {
        String fileName = String(file.name());
        if (!file.isDirectory() && (fileName.endsWith(".mp3") || fileName.endsWith(".MP3"))) {
            count++;
        }
        file = root.openNextFile();
    }
    return count;
}

String getNextMP3(String current, bool next) {
    File root = SD.open("/");
    if (!root) return "";
    
    String firstFile = "";
    String foundFile = "";
    String prevFile = "";
    bool foundCurrent = false;
    
    int index = 0;

    File file = root.openNextFile();
    while (file) {
        String fileName = String(file.name());
        if (!file.isDirectory() && (fileName.endsWith(".mp3") || fileName.endsWith(".MP3"))) {
            index++;
            if (firstFile == "") firstFile = "/" + fileName;
            String fullPath = "/" + fileName;
            
            if (next) {
                if (foundCurrent) {
                    currentTrackIndex = index;
                    return fullPath;
                }
                if (fullPath == current) foundCurrent = true;
            } else {
                if (fullPath == current) {
                    if (prevFile != "") {
                        currentTrackIndex = index - 1;
                        return prevFile;
                    }
                }
                prevFile = fullPath;
            }
        }
        file = root.openNextFile();
    }
    
    if (next) {
        if (foundCurrent) {
            currentTrackIndex = 1;
            return firstFile;
        }
        if (current == "") {
            currentTrackIndex = 1;
            return firstFile;
        }
    } else {
        // Wrap to last file if prev from first
        if (prevFile != "" && current == firstFile) {
            currentTrackIndex = totalTracks; 
            return prevFile;
        }
    }
    
    currentTrackIndex = 1; // Default
    return firstFile; 
}

// --- INPUT & UI ---

void handleButtons() {
    // Read Buttons
    bool modeState = digitalRead(BTN_MODE) == LOW; 
    bool playState = digitalRead(BTN_PLAY) == LOW;
    bool volMinusState = digitalRead(BTN_PREV_VOL_MINUS) == LOW;
    bool volPlusState  = digitalRead(BTN_NEXT_VOL_PLUS) == LOW;
    
    unsigned long now = millis();



    // --- MODE ---
    if (modeState && !btnMode.isPressed) {
        Serial.println("BTN_MODE Pressed");
        btnMode.isPressed = true;
        btnMode.pressTime = now;
        btnMode.longPressHandled = false;
    } else if (modeState && btnMode.isPressed && (now - btnMode.pressTime > 2000) && !btnMode.longPressHandled) {
        Serial.println("BTN_MODE Long Press (Sleep)");
        btnMode.longPressHandled = true;
        enterDeepSleep();
    } else if (!modeState && btnMode.isPressed) {
        if (!btnMode.longPressHandled) {
             Serial.println("BTN_MODE Release (Next Mode)");
             currentMode++;
             if (currentMode > MODE_RADIO) currentMode = MODE_BT;
             saveSettings();
             ESP.restart(); 
        }
        btnMode.isPressed = false;
    }

    // --- PLAY ---
    if (playState && !btnPlay.isPressed) {
        Serial.println("BTN_PLAY Pressed");
        btnPlay.isPressed = true;
        togglePlayPause(); 
    } else if (!playState) btnPlay.isPressed = false;

    // --- Vol- / PREV ---
    if (volMinusState && !btnPrevVolMinus.isPressed) {
        Serial.println("BTN_MINUS Pressed");
        btnPrevVolMinus.isPressed = true;
        btnPrevVolMinus.pressTime = now;
        btnPrevVolMinus.longPressHandled = false;
    } else if (volMinusState && btnPrevVolMinus.isPressed && (now - btnPrevVolMinus.pressTime > 600) && !btnPrevVolMinus.longPressHandled) {
        Serial.println("BTN_MINUS Long Press (Prev Track)");
        if (currentMode == MODE_MP3) prevTrack();
        btnPrevVolMinus.longPressHandled = true; 
    } else if (!volMinusState && btnPrevVolMinus.isPressed) {
        if (!btnPrevVolMinus.longPressHandled) {
             Serial.println("BTN_MINUS Release (Vol -)");
             volume = max(0, volume - 5);
             if (out) out->SetGain(volume / 100.0);
             if (currentMode == MODE_BT) bt.volume(volume / 100.0);
             saveSettings();
        }
        btnPrevVolMinus.isPressed = false;
    }

    // --- Vol+ / NEXT ---
    if (volPlusState && !btnNextVolPlus.isPressed) {
        Serial.println("BTN_PLUS Pressed");
        btnNextVolPlus.isPressed = true;
        btnNextVolPlus.pressTime = now;
        btnNextVolPlus.longPressHandled = false;
    } else if (volPlusState && btnNextVolPlus.isPressed && (now - btnNextVolPlus.pressTime > 600) && !btnNextVolPlus.longPressHandled) {
        Serial.println("BTN_PLUS Long Press (Next Track)");
        if (currentMode == MODE_MP3) nextTrack();
        btnNextVolPlus.longPressHandled = true;
    } else if (!volPlusState && btnNextVolPlus.isPressed) {
        if (!btnNextVolPlus.longPressHandled) {
             Serial.println("BTN_PLUS Release (Vol +)");
             volume = min(100, volume + 5);
             if (out) out->SetGain(volume / 100.0);
             if (currentMode == MODE_BT) bt.volume(volume / 100.0);
             saveSettings();
        }
        btnNextVolPlus.isPressed = false;
    }
}

void togglePlayPause() {
    isPlaying = !isPlaying;
    
    if (currentMode == MODE_BT) {
        if (isPlaying) {
             esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_PRESSED);
             delay(40);
             esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PLAY, ESP_AVRC_PT_CMD_STATE_RELEASED);
        } else {
             esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_PRESSED);
             delay(40);
             esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PAUSE, ESP_AVRC_PT_CMD_STATE_RELEASED);
        }
    } 
    else if (currentMode == MODE_MP3) {
        if (!isPlaying) {
             // Paused
             lastPauseStart = millis();
        } else {
             // Resumed
             if (lastPauseStart > 0) {
                 trackPausedTime += (millis() - lastPauseStart);
                 lastPauseStart = 0;
             }
        }
    }
    else if (currentMode == MODE_RADIO) {
        if (!isPlaying) {
            if (xSemaphoreTake(audioMutex, portMAX_DELAY)) {
                stopAudio();
                currentTitle = "Stopped";
                xSemaphoreGive(audioMutex);
            }
        } else {
            if (xSemaphoreTake(audioMutex, portMAX_DELAY)) {
                setupAudio(); 
                xSemaphoreGive(audioMutex);
            }
        }
    }
}

void nextTrack() {
    if (currentMode != MODE_MP3) return;
    String next = getNextMP3(currentTitle.startsWith("/") ? currentTitle : "/" + currentTitle, true);
    if (next != "") {
        if (xSemaphoreTake(audioMutex, portMAX_DELAY)) {
            stopAudio();
            out = new AudioOutputI2S();
            out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);
            out->SetGain(volume / 100.0);
            
            if (!next.startsWith("/")) next = "/" + next;
            
            sourceSD = new AudioFileSourceSD(next.c_str());
            sourceID3 = new AudioFileSourceID3(sourceSD);
            mp3 = new AudioGeneratorMP3();
            mp3->begin(sourceID3, out);
            isPlaying = true;
            currentTitle = next.substring(1);
            
            // Stats
            trackStartTime = millis();
            trackPausedTime = 0;
            g_mp3_Size = sourceSD->getSize();

            xSemaphoreGive(audioMutex);
        }
    }
}

void prevTrack() {
    if (currentMode != MODE_MP3) return;
    String prev = getNextMP3(currentTitle.startsWith("/") ? currentTitle : "/" + currentTitle, false);
    if (prev != "") {
        if (xSemaphoreTake(audioMutex, portMAX_DELAY)) {
            stopAudio();
            // ... (setup out)
            // 0 = Port 0
            // 0 = EXTERNAL_I2S
            // 32 = DMA Buffer Count
            out = new AudioOutputI2S(0, 0, 32); 
            out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);
            out->SetGain(volume / 100.0);
            
            if (!prev.startsWith("/")) prev = "/" + prev;
            
            sourceSD = new AudioFileSourceSD(prev.c_str());
            sourceID3 = new AudioFileSourceID3(sourceSD);
            mp3 = new AudioGeneratorMP3();
            mp3->begin(sourceID3, out);
            isPlaying = true;
            currentTitle = prev.substring(1);
            
            // Stats
            trackStartTime = millis();
            trackPausedTime = 0;
            g_mp3_Size = sourceSD->getSize();

            xSemaphoreGive(audioMutex);
        }
    } else {
        nextTrack();
    }
}

void updateDisplay() {
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setCursor(0, 0);
  int iconX = 118;
  if (isPlaying) {
      display.fillTriangle(iconX, 0, iconX, 8, iconX+5, 4, SH110X_WHITE);
  } else {
      display.fillRect(iconX, 0, 2, 8, SH110X_WHITE);
      display.fillRect(iconX+3, 0, 2, 8, SH110X_WHITE);
  }
  
  display.setCursor(0, 0);
  switch(currentMode) {
      case MODE_BT: display.print("BLUETOOTH"); break;
      case MODE_MP3: display.print("MP3 PLAYER"); break;
      case MODE_RADIO: display.print("RADIO"); break;
  }
  
  display.drawLine(0, 10, 128, 10, SH110X_WHITE);
  
  display.setCursor(0, 20);
  if (currentMode == MODE_BT) {
      display.println(bt.title);
      display.println(bt.artist);
  } else {
      // Truncate long titles
      String t = currentTitle;
      
      // Show Track X/Y if MP3
      if (currentMode == MODE_MP3) {
          display.setCursor(0, 20);
          display.printf("%d/%d", currentTrackIndex, totalTracks);
          display.setCursor(0, 30); // Move Title down
      } else {
          display.setCursor(0, 20);
      }
      
      if (t.length() > 20) t = t.substring(0, 17) + "...";
      display.println(t);
      
      if (currentMode == MODE_RADIO && buff) {
          display.printf("Buf: %d", buff->getFillLevel());
      }
      
      // MP3 Progress Bar
      if (currentMode == MODE_MP3 && isPlaying) {
           uint32_t size = g_mp3_Size;
           uint32_t pos = g_mp3_Pos;
           
           if (size > 0) {
               // Bar
               int barWidth = 128;
               int barHeight = 4;
               int barY = 40;
               int filled = (pos * barWidth) / size;
               
               display.drawRect(0, barY, barWidth, barHeight, SH110X_WHITE);
               display.fillRect(0, barY, filled, barHeight, SH110X_WHITE);
               
                // Time
                unsigned long currentMs = 0;
                if (isPlaying) currentMs = millis() - trackStartTime - trackPausedTime;
                else currentMs = lastPauseStart - trackStartTime - trackPausedTime;
                
                // One-time Stable Total Time Calculation
                static unsigned long cachedTotalMs = 0;
                static bool estimationLocked = false;
                
                // Reset cache when track changes (detect by pos reset or big jump?)
                // Actually `cachedTotalMs` needs to be global or reset when track starts.
                // Better approach: Calculate once at 5 seconds and lock it.
                // Or better: Just calculate it continuously but use a VERY slow filter?
                // User said: "Read exact info when start... dont lose CPU".
                // Since we don't have exact info (no header parsing), we latch the estimate
                // after the stream is stable (e.g. > 5 seconds).
                
                if (pos > 0) {
                    // Check if we need to reset (new track)
                    // This static var approach is tricky with track changes.
                    // Let's use a simple global or just purely based on time.
                    // If currentMs is small (< 1000), reset lock.
                    if (currentMs < 2000) {
                        estimationLocked = false;
                        cachedTotalMs = 0;
                    }
                    
                    if (!estimationLocked && currentMs > 5000) {
                        cachedTotalMs = (unsigned long)((float)currentMs / (float)pos * (float)size);
                        estimationLocked = true; // Lock it
                    }
                    else if (!estimationLocked) {
                        // Show provisional
                        cachedTotalMs = (unsigned long)((float)currentMs / (float)pos * (float)size);
                    }
                }

                display.setCursor(0, 48);
                display.print(formatTime(currentMs));
                
                if (cachedTotalMs > 0) {
                    display.print(" / ");
                    display.print(formatTime(cachedTotalMs));
                }
            }
       }
  }

  display.drawLine(0, 54, 128, 54, SH110X_WHITE);
  display.setCursor(0, 56);
  display.printf("VOL: %d%%", volume);
  
  display.display();
}

String formatTime(unsigned long ms) {
    unsigned long seconds = ms / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long hours = minutes / 60;
    minutes %= 60;
    seconds %= 60;
    char buffer[20];
    if (hours > 0) {
        sprintf(buffer, "%02lu:%02lu:%02lu", hours, minutes, seconds);
    } else {
        sprintf(buffer, "%02lu:%02lu", minutes, seconds);
    }
    return String(buffer);
}

void enterDeepSleep() {
    display.clearDisplay();
    display.setTextSize(2);
    display.setCursor(30, 25);
    display.println("OFF");
    display.display();
    delay(1000);
    display.display(); 
    
    stopAudio();
    saveSettings();
    esp_deep_sleep_start();
}

void saveSettings() {
    preferences.begin("audio-config", false);
    preferences.putInt("mode", currentMode);
    preferences.putInt("volume", volume);
    preferences.end();
}

void loadSettings() {
    preferences.begin("audio-config", true);
    currentMode = preferences.getInt("mode", MODE_BT);
    volume = preferences.getInt("volume", 80);
    preferences.end();
}