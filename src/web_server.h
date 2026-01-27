#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config_manager.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Web server and AP mode functions
void setupAPMode();
void setupWebServer();
void stopWebServer();
void setupCaptivePortal();
void handleDNS(); // Call this in loop when in AP mode

// Global web server instance
extern AsyncWebServer *server;

#endif // WEB_SERVER_H
