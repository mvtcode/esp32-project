#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include "config_manager.h"

// Web server and AP mode functions
void setupAPMode();
void setupWebServer();
void stopWebServer();

// Global web server instance
extern AsyncWebServer* server;

#endif // WEB_SERVER_H
