#include "UIManager.h"

UIManager::UIManager()
    : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {}

void UIManager::begin() {
  Wire.begin(OLED_SDA, OLED_SCL);
  if (!display.begin(SCREEN_ADDRESS, true)) {
    // Serial.println("OLED Init Failed"); // Assuming Serial is available or
    // handle error
  }
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("ESP32 Audio Player");
  display.display();
}

void UIManager::showOff() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(30, 25);
  display.println("OFF");
  display.display();
}

void UIManager::showMessage(String msg) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println(msg);
  display.display();
}

void UIManager::update(const PlayerStatus &status) {
  static AudioMode lastMode = (AudioMode)-1;
  static bool lastPlay = false;
  static int lastVol = -1;
  static String lastTitle = "";
  static String lastArtist = "";
  static int lastTrack = -1;
  static int lastBuffer = -1;
  static unsigned long lastUpdateSec = 0;

  // Calculate current seconds to check if time/display needs update
  unsigned long currentMs = 0;
  if (status.isPlaying)
    currentMs = millis() - status.trackStartTime - status.trackPausedTime;
  else
    currentMs =
        status.lastPauseStart - status.trackStartTime - status.trackPausedTime;

  unsigned long currentSec = currentMs / 1000;

  // Dirty Check
  bool needsRedraw = false;

  if (status.mode != lastMode)
    needsRedraw = true;
  if (status.isPlaying != lastPlay)
    needsRedraw = true;
  if (status.volume != lastVol)
    needsRedraw = true;
  if (status.title != lastTitle)
    needsRedraw = true;
  if (status.artist != lastArtist)
    needsRedraw = true;
  if (status.currentTrack != lastTrack)
    needsRedraw = true;
  if (status.bufferLevel != lastBuffer)
    needsRedraw = true;
  if (currentSec != lastUpdateSec)
    needsRedraw = true;

  if (!needsRedraw)
    return;

  // Update State
  lastMode = status.mode;
  lastPlay = status.isPlaying;
  lastVol = status.volume;
  lastTitle = status.title;
  lastArtist = status.artist;
  lastTrack = status.currentTrack;
  lastBuffer = status.bufferLevel;
  lastUpdateSec = currentSec;

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  int iconX = 118;
  if (status.isPlaying) {
    display.fillTriangle(iconX, 0, iconX, 8, iconX + 5, 4, SH110X_WHITE);
  } else {
    display.fillRect(iconX, 0, 2, 8, SH110X_WHITE);
    display.fillRect(iconX + 3, 0, 2, 8, SH110X_WHITE);
  }

  display.setCursor(0, 0);
  switch (status.mode) {
  case MODE_BT:
    display.print("BLUETOOTH");
    break;
  case MODE_MP3:
    display.print("MP3 PLAYER");
    break;
  case MODE_RADIO:
    display.print("RADIO");
    break;
  }

  display.drawLine(0, 10, 128, 10, SH110X_WHITE);

  display.setCursor(0, 20);
  if (status.mode == MODE_BT) {
    display.println(status.title);
    display.println(status.artist);
  } else {
    // Truncate long titles
    String t = status.title;

    // Show Track X/Y if MP3
    if (status.mode == MODE_MP3) {
      display.setCursor(0, 20);
      display.printf("%d/%d", status.currentTrack, status.totalTracks);
      display.setCursor(0, 30); // Move Title down
    } else {
      display.setCursor(0, 20);
    }

    if (t.length() > 20)
      t = t.substring(0, 17) + "...";
    display.println(t);

    if (status.mode == MODE_RADIO) {
      display.printf("Buf: %d", status.bufferLevel);
    }

    // MP3 Progress Bar
    if (status.mode == MODE_MP3 && status.isPlaying) {
      uint32_t size = status.mp3Size;
      uint32_t pos = status.mp3Pos;

      if (size > 0) {
        // Bar
        int barWidth = 128;
        int barHeight = 4;
        int barY = 40;
        int filled = (pos * barWidth) / size;

        display.drawRect(0, barY, barWidth, barHeight, SH110X_WHITE);
        display.fillRect(0, barY, filled, barHeight, SH110X_WHITE);

        // Time logic (reused from local var, removed static cache logic inside
        // drawing to keep it simple or keep it?) The previous logic was
        // calculating total time based on bytes/sec estimation. I will keep the
        // time display logic simple or copy the estimation logic? Let's copy
        // the estimation logic but use safe static vars or just recalculate.
        // Actually, the previous code had static variables INSIDE the if block.
        // It's better to keep it if it works.

        static unsigned long cachedTotalMs = 0;
        static bool estimationLocked = false;

        if (pos > 0) {
          if (currentMs < 2000) {
            estimationLocked = false;
            cachedTotalMs = 0;
          }
          if (!estimationLocked && currentMs > 5000) {
            cachedTotalMs =
                (unsigned long)((float)currentMs / (float)pos * (float)size);
            estimationLocked = true;
          } else if (!estimationLocked) {
            cachedTotalMs =
                (unsigned long)((float)currentMs / (float)pos * (float)size);
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
  display.printf("VOL: %d%%", status.volume);

  display.display();
}

String UIManager::formatTime(unsigned long ms) {
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
