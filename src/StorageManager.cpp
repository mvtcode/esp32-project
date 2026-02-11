#include "StorageManager.h"

StorageManager::StorageManager() {}

void StorageManager::begin() {
  // Preferences does not need explicit begin in constructor usually,
  // but good to have a setup method if needed.
}

void StorageManager::saveSettings(int mode, int volume) {
  preferences.begin("audio-config", false);
  preferences.putInt("mode", mode);
  preferences.putInt("volume", volume);
  preferences.end();
}

void StorageManager::loadSettings(int &mode, int &volume) {
  preferences.begin("audio-config", true);
  mode = preferences.getInt("mode", MODE_BT);
  volume = preferences.getInt("volume", 80);
  preferences.end();
}
