#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "config_manager.h"
#include <Arduino.h>
#include <ESPAsyncWebServer.h>

// Web server and AP mode functions
void setupAPSTAMode(); // Setup both AP and STA mode simultaneously
void setupWebServer();
void stopWebServer();
void setupCaptivePortal();
void handleDNS(); // Call this in loop when in AP mode

// Authentication
bool authenticateRequest(AsyncWebServerRequest *request,
                         const String &adminPassword);

// Global web server instance
extern AsyncWebServer *server;
extern ConfigData globalConfig; // Need access to config for auth

#endif // WEB_SERVER_H
