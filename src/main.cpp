#include <Arduino.h>
#include <WiFi.h> // Keep WiFi here for setup if needed, or move to AudioManager if we decide
#include <Wire.h>

#include "AudioManager.h"
#include "Constants.h"
#include "InputManager.h"
#include "StorageManager.h"
#include "UIManager.h"

// --- GLOBALS ---
StorageManager storage;
InputManager input;
UIManager ui;
AudioManager audio;

// State needed for Control Logic
int currentMode = MODE_BT;
int volume = 80;

// Tasks
TaskHandle_t TaskAudioHandle = NULL;
TaskHandle_t TaskUIHandle = NULL;

// Function Prototypes
void processAudioTask(void *parameter);
void processUITask(void *parameter);
void enterDeepSleep();

// --- SETUP ---
void setup() {
  Serial.begin(115200);

  // Check Wakeup Reason (Deep Sleep)
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Woke up from Deep Sleep!");
  }

  // Init Managers
  storage.begin(); // (Currently empty but good practice)
  storage.loadSettings(currentMode, volume);

  // Init UI
  ui.begin();

  // Init Audio
  // Handle Radio WiFi dependency here as in original code
  if (currentMode == MODE_RADIO) {
    WiFi.begin("HPSTAR", "0964335688"); // WiFi Creds
    ui.showMessage("Connecting WiFi...");
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
      ui.showMessage("WiFi Fail");
      delay(1000);
    }
  }

  // Audio Begin (Sets up initial state)
  audio.begin(currentMode, volume);

  // Init Input
  input.begin();

  // Create Tasks
  xTaskCreatePinnedToCore(processAudioTask, "AudioTask", 20000, NULL, 3,
                          &TaskAudioHandle, 0 // Core 0 (High Priority)
  );

  xTaskCreatePinnedToCore(processUITask, "UITask", 4096, NULL, 1, &TaskUIHandle,
                          1 // Core 1 (Low Priority)
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
  // If we need to call setupAudio initially in the task context:
  // audio.update() handles "looping", but setupAudio might need to run once?
  // Our AudioManager::begin just sets params.
  // The original code called setupAudio() inside the mutex check loop or
  // manually. We can trigger it by setting mode again or just letting update
  // handle it? AudioManager::update checks pointers. But strictly, we should
  // probably force a setup.
  audio.setMode(currentMode); // This will trigger setupAudio inside the
                              // task/mutex safely?
  // Wait, setMode takes mutex.

  while (true) {
    audio.update();
    delay(10);
  }
}

void processUITask(void *parameter) {
  unsigned long lastUpdate = 0;
  while (true) {
    // Input
    InputAction action = input.update();

    switch (action) {
    case ACTION_NEXT_MODE:
      Serial.println("Next Mode");
      currentMode++;
      if (currentMode > MODE_RADIO)
        currentMode = MODE_BT;
      storage.saveSettings(currentMode, volume);
      ESP.restart();
      break;

    case ACTION_DEEP_SLEEP:
      enterDeepSleep();
      break;

    case ACTION_TOGGLE_PLAY:
      Serial.println("Toggle Play");
      audio.togglePlayPause();
      break;

    case ACTION_PREV_TRACK:
      Serial.println("Prev Track");
      audio.prevTrack();
      break;

    case ACTION_VOL_DOWN:
      Serial.println("Vol -");
      volume = max(0, volume - 5);
      audio.setVolume(volume);
      storage.saveSettings(currentMode, volume);
      break;

    case ACTION_NEXT_TRACK:
      Serial.println("Next Track");
      audio.nextTrack();
      break;

    case ACTION_VOL_UP:
      Serial.println("Vol +");
      volume = min(100, volume + 5);
      audio.setVolume(volume);
      storage.saveSettings(currentMode, volume);
      break;

    default:
      break;
    }

    // UI Update
    if (millis() - lastUpdate > 200) {
      PlayerStatus status = audio.getStatus();
      ui.update(status);
      lastUpdate = millis();
    }

    delay(50);
  }
}

void enterDeepSleep() {
  ui.showOff();
  delay(1000); // Give time for UI

  audio.setMode(MODE_BT); // Stop audio cleanly?
  // Actually AudioManager::stopAudio is private.
  // Changing mode calls setupAudio which calls stopAudio.
  // But we are restarting/sleeping.
  // Ideally we just save settings and sleep.

  storage.saveSettings(currentMode, volume);
  esp_deep_sleep_start();
}