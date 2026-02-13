#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "Constants.h"
#include <Arduino.h>
#include <Preferences.h>

class StorageManager {
public:
  StorageManager();
  void begin();
  void saveSettings(int mode, int volume);
  void loadSettings(int &mode, int &volume);
  void saveLastTrack(String filename);
  String getLastTrack();

private:
  Preferences preferences;
};

#endif // STORAGE_MANAGER_H
