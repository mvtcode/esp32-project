#ifndef NTP_MANAGER_H
#define NTP_MANAGER_H

#include <Arduino.h>

void initNTP();
bool isTimeSynced();
void printLocalTime();
void getLocalTimeStr(char* buffer, size_t maxLen);

#endif
