#include "AudioManager.h"

AudioManager::AudioManager()
    : bt("ESP32-Audio-Player"), mp3(NULL), sourceSD(NULL), sourceID3(NULL),
      sourceStream(NULL), buff(NULL), out(NULL), currentMode(MODE_BT),
      volume(80), isPlaying(false), currentTrackIndex(0), totalTracks(0),
      trackStartTime(0), trackPausedTime(0), lastPauseStart(0), mp3Pos(0),
      mp3Size(0) {
  mutex = xSemaphoreCreateMutex();
}

void AudioManager::begin(int initialMode, int initialVolume) {
  currentMode = initialMode;
  volume = initialVolume;

  // Start Audio Task external?
  // Here we just prepare logic.
  // Initial setup if not in task?
  // Usually setupAudio is heavy, better call it in the task via flag or just
  // mutex protected. For now we assume begin is called in Setup, and real
  // setupAudio happens in first loop or explicitly. Actually main.cpp calls
  // setupAudio inside the task for the first time. We can just set state here.
}

bool AudioManager::ensureSD() {
  if (!SD.begin(SD_CS)) {
    return false;
  }
  return true;
}

void AudioManager::setupAudio() {
  stopAudio();

  if (currentMode == MODE_BT) {
    bt.begin();
    bt.reconnect();
    bt.I2S(I2S_BCK, I2S_DOUT, I2S_WS);
    float volFloat = volume / 100.0f;
    bt.volume(volFloat * volFloat);
    isPlaying = true;
    currentTitle = "Waiting for BT...";
    currentArtist = "";
  } else if (currentMode == MODE_MP3) {
    if (!ensureSD()) {
      currentTitle = "SD Init Failed";
      return;
    }

    // Increase DMA buffers to 64 for stability
    out = new AudioOutputI2S(0, 0, 64);
    out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);

    float volFloat = volume / 100.0f;
    out->SetGain(volFloat * volFloat);

    if (currentTitle == "" || currentTitle == "No MP3 Files" ||
        currentTitle == "File Error" || currentTitle == "SD Init Failed") {
      totalTracks = countTracks();
      String first = getNextMP3("", true);
      if (first != "")
        currentTitle = first.substring(1);
      else
        currentTitle = "No MP3 Files";
    }

    if (currentTitle != "No MP3 Files" && currentTitle != "") {
      String path = "/" + currentTitle;
      if (SD.exists(path)) {
        sourceSD = new AudioFileSourceSD(path.c_str());
        sourceID3 = new AudioFileSourceID3(sourceSD);

        // Use Buffer for MP3 to prevent pops
        buff = new AudioFileSourceBuffer(sourceID3, 16384);

        mp3 = new AudioGeneratorMP3();
        mp3->begin(buff, out); // Use Buffer

        isPlaying = true;
        // Reset Timing
        trackStartTime = millis();
        trackPausedTime = 0;
        lastPauseStart = 0;
        // Update Size
        mp3Size = sourceSD->getSize();
      } else {
        currentTitle = "File Error";
        isPlaying = false;
      }
    }
  } else if (currentMode == MODE_RADIO) {
    out = new AudioOutputI2S(0, 0, 64);
    out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);

    float volFloat = volume / 100.0f;
    out->SetGain(volFloat * volFloat);

    sourceStream = new AudioFileSourceICYStream(RADIO_URL);
    buff = new AudioFileSourceBuffer(sourceStream, 1024 * 16);
    mp3 = new AudioGeneratorMP3();
    mp3->begin(buff, out);
    isPlaying = true;
    currentTitle = "Internet Radio";
    currentArtist = "";
  }
}

void AudioManager::stopAudio() {
  if (mp3) {
    mp3->stop();
    delete mp3;
    mp3 = NULL;
  }
  if (buff) {
    delete buff;
    buff = NULL;
  }
  if (sourceStream) {
    delete sourceStream;
    sourceStream = NULL;
  }
  if (sourceID3) {
    delete sourceID3;
    sourceID3 = NULL;
  }
  if (sourceSD) {
    delete sourceSD;
    sourceSD = NULL;
  }
  if (out) {
    delete out;
    out = NULL;
  }

  if (currentMode == MODE_BT) {
    bt.end();
  }

  i2s_driver_uninstall(I2S_NUM_0);
  delay(100);
}

void AudioManager::update() {
  if (xSemaphoreTake(mutex, 10 / portTICK_PERIOD_MS)) {
    // Initialization check logic (handled by whoever switches mode)
    // If pointers are null but we should be playing?
    // We assume setupAudio is called on mode change.

    if (currentMode == MODE_BT) {
      if (isPlaying) {
        static unsigned long lastMeta = 0;
        if (millis() - lastMeta > 1000) {
          lastMeta = millis();
          bt.updateMeta();
          // BT Meta updated
          // We can read bt.title/artist directly in getStatus
          // or cache it here if we want.
          // bt.title is public.
        }
      }
    } else if (currentMode == MODE_MP3 || currentMode == MODE_RADIO) {
      if (mp3 && mp3->isRunning()) {
        if (currentMode == MODE_MP3 && !isPlaying) {
          // Paused
        } else {
          if (mp3->loop()) {
            if (sourceSD) {
              mp3Pos = sourceSD->getPos();
            }
          } else {
            mp3->stop();
            if (currentMode == MODE_MP3) {
              // Loop Next Track logic
              // We need to release mutex to call nextTrack which takes mutex?
              // No, nextTrack takes mutex.
              // We are holding mutex. Call internal function or recursive
              // mutex? Simple solution:
              xSemaphoreGive(mutex);
              nextTrack();
              return; // Done
            }
          }
        }
      }
    }
    xSemaphoreGive(mutex);
  }
}

void AudioManager::setMode(int mode) {
  if (xSemaphoreTake(mutex, portMAX_DELAY)) {
    currentMode = mode;
    setupAudio(); // Switch
    xSemaphoreGive(mutex);
  }
}

void AudioManager::setVolume(int v) {
  if (xSemaphoreTake(mutex, portMAX_DELAY)) {
    volume = v;
    // Logarithmic volume control: (v/100)^2
    float volFloat = volume / 100.0f;
    float volLog = volFloat * volFloat;

    if (currentMode == MODE_BT) {
      bt.volume(volLog);
    } else {
      if (out)
        out->SetGain(volLog);
    }
    xSemaphoreGive(mutex);
  }
}

void AudioManager::togglePlayPause() {
  // Logic from main.cpp
  if (currentMode == MODE_BT) {
    isPlaying = !isPlaying;
    if (isPlaying) {
      esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PLAY,
                                       ESP_AVRC_PT_CMD_STATE_PRESSED);
      delay(40);
      esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PLAY,
                                       ESP_AVRC_PT_CMD_STATE_RELEASED);
    } else {
      esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PAUSE,
                                       ESP_AVRC_PT_CMD_STATE_PRESSED);
      delay(40);
      esp_avrc_ct_send_passthrough_cmd(0, ESP_AVRC_PT_CMD_PAUSE,
                                       ESP_AVRC_PT_CMD_STATE_RELEASED);
    }
  } else if (currentMode == MODE_MP3) {
    isPlaying = !isPlaying;
    if (!isPlaying) {
      lastPauseStart = millis();
    } else {
      if (lastPauseStart > 0) {
        trackPausedTime += (millis() - lastPauseStart);
        lastPauseStart = 0;
      }
    }
  } else if (currentMode == MODE_RADIO) {
    if (isPlaying) {
      // Stop
      isPlaying = false;
      if (xSemaphoreTake(mutex, portMAX_DELAY)) {
        stopAudio();
        currentTitle = "Stopped";
        xSemaphoreGive(mutex);
      }
    } else {
      // Play
      if (xSemaphoreTake(mutex, portMAX_DELAY)) {
        setupAudio();
        xSemaphoreGive(mutex);
      }
    }
  }
}

void AudioManager::nextTrack() {
  if (currentMode != MODE_MP3)
    return;

  // Determine next file - accessing SD requires mutex?
  // getNextMP3 accesses SD. Better protect it.
  // If called from update() we gave up mutex.

  if (xSemaphoreTake(mutex, portMAX_DELAY)) {
    String next = getNextMP3(
        currentTitle.startsWith("/") ? currentTitle : "/" + currentTitle, true);
    if (next != "") {
      stopAudio();
      out = new AudioOutputI2S(0, 0, 64);
      out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);

      float volFloat = volume / 100.0f;
      out->SetGain(volFloat * volFloat);

      if (!next.startsWith("/"))
        next = "/" + next;

      sourceSD = new AudioFileSourceSD(next.c_str());
      sourceID3 = new AudioFileSourceID3(sourceSD);
      buff = new AudioFileSourceBuffer(sourceID3, 16384);
      mp3 = new AudioGeneratorMP3();
      mp3->begin(buff, out);
      isPlaying = true;
      currentTitle = next.substring(1);

      trackStartTime = millis();
      trackPausedTime = 0;
      lastPauseStart = 0;
      mp3Size = sourceSD->getSize();
    }
    xSemaphoreGive(mutex);
  }
}

void AudioManager::prevTrack() {
  if (currentMode != MODE_MP3)
    return;

  if (xSemaphoreTake(mutex, portMAX_DELAY)) {
    String prev = getNextMP3(currentTitle.startsWith("/") ? currentTitle
                                                          : "/" + currentTitle,
                             false);
    if (prev != "") {
      stopAudio();
      out = new AudioOutputI2S(0, 0, 64);
      out->SetPinout(I2S_BCK, I2S_WS, I2S_DOUT);

      float volFloat = volume / 100.0f;
      out->SetGain(volFloat * volFloat);

      if (!prev.startsWith("/"))
        prev = "/" + prev;

      sourceSD = new AudioFileSourceSD(prev.c_str());
      sourceID3 = new AudioFileSourceID3(sourceSD);
      buff = new AudioFileSourceBuffer(sourceID3, 16384);
      mp3 = new AudioGeneratorMP3();
      mp3->begin(buff, out);
      isPlaying = true;
      currentTitle = prev.substring(1);

      trackStartTime = millis();
      trackPausedTime = 0;
      lastPauseStart = 0;
      mp3Size = sourceSD->getSize();
    } else {
      // Logic in main.cpp said "nextTrack()" if prev fails?
      // "else { nextTrack(); }"
      // I'll skip that for now or replicate?
      // Replicating:
      xSemaphoreGive(mutex); // Give before calling nextTrack
      nextTrack();
      return;
    }
    xSemaphoreGive(mutex);
  }
}

PlayerStatus AudioManager::getStatus() {
  PlayerStatus status;
  // We should take mutex to read consistent state?
  // UI task is low priority.
  // Reading basic types is atomic enough on ESP32 usually, but Strings...
  if (xSemaphoreTake(mutex, 10 / portTICK_PERIOD_MS)) {
    status.mode = (AudioMode)currentMode;
    status.isPlaying = isPlaying;
    status.volume = volume;
    if (currentMode == MODE_BT) {
      status.title = String(bt.title);
      status.artist = String(bt.artist);
    } else {
      status.title = currentTitle;
      status.artist = currentArtist;
    }
    status.currentTrack = currentTrackIndex;
    status.totalTracks = totalTracks;
    status.mp3Size = mp3Size;
    status.mp3Pos = mp3Pos;
    status.trackStartTime = trackStartTime;
    status.trackPausedTime = trackPausedTime;
    status.lastPauseStart = lastPauseStart;
    if (buff)
      status.bufferLevel = buff->getFillLevel();
    else
      status.bufferLevel = 0;

    xSemaphoreGive(mutex);
  } else {
    // Fallback? or just return default/partial
    status.mode = (AudioMode)currentMode;
    status.isPlaying = isPlaying;
    status.volume = volume;
  }
  return status;
}

// --- Helpers ---
int AudioManager::countTracks() {
  // Assumes mutex held or SD access safe (usually single threaded access with
  // mutex)
  int count = 0;
  File root = SD.open("/");
  if (!root)
    return 0;

  File file = root.openNextFile();
  while (file) {
    String fileName = String(file.name());
    if (!file.isDirectory() &&
        (fileName.endsWith(".mp3") || fileName.endsWith(".MP3"))) {
      count++;
    }
    file = root.openNextFile();
  }
  return count;
}

String AudioManager::getNextMP3(String current, bool next) {
  File root = SD.open("/");
  if (!root)
    return "";

  String firstFile = "";
  String foundFile = "";
  String prevFile = "";
  bool foundCurrent = false;

  int index = 0;

  File file = root.openNextFile();
  while (file) {
    String fileName = String(file.name());
    if (!file.isDirectory() &&
        (fileName.endsWith(".mp3") || fileName.endsWith(".MP3"))) {
      index++;
      if (firstFile == "")
        firstFile = "/" + fileName;
      String fullPath = "/" + fileName;

      if (next) {
        if (foundCurrent) {
          currentTrackIndex = index;
          return fullPath;
        }
        if (fullPath == current)
          foundCurrent = true;
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
    if (prevFile != "" && current == firstFile) {
      currentTrackIndex = totalTracks;
      return prevFile;
    }
  }

  currentTrackIndex = 1;
  return firstFile;
}
