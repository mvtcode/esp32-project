/**
 * relay_controller.cpp — GPIO relay/output management
 * IoT Voice Command System
 */
#include "relay_controller.h"
#include "../hardware_config.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>

// ─── Static state ─────────────────────────────────────────────────────────────
static const int _pins[NUM_RELAYS] = {
    RELAY_1, RELAY_2, RELAY_3, RELAY_4, RELAY_5, RELAY_6
};

static bool _states[NUM_RELAYS] = { false };
static char _aliases[NUM_RELAYS][32];

// ─── Lifecycle ───────────────────────────────────────────────────────────────

void relay_init() {
    // Mount SPIFFS explicitly from the 'spiffs' partition
    if (!SPIFFS.begin(true, "/spiffs", 10, "spiffs")) {
        Serial.println("[RELAY] SPIFFS Mount Failed (label: spiffs)!");
    } else {
        Serial.println("[RELAY] SPIFFS Mounted successfully.");
    }

    // Khởi tạo GPIO
    for (int i = 0; i < NUM_RELAYS; i++) {
        pinMode(_pins[i], OUTPUT);
        digitalWrite(_pins[i], LOW);
        _states[i] = false;
        // Default alias
        snprintf(_aliases[i], sizeof(_aliases[i]), "GPIO %d", _pins[i]);
    }

    // Load aliases from config.json
    if (SPIFFS.exists("/config.json")) {
        File file = SPIFFS.open("/config.json", "r");
        if (file) {
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, file);
            if (!error) {
                JsonArray gpios = doc["gpios"];
                int idx = 0;
                for (JsonObject item : gpios) {
                    if (idx < NUM_RELAYS) {
                        const char* name = item["name"];
                        if (name) {
                            strncpy(_aliases[idx], name, sizeof(_aliases[idx]) - 1);
                            _aliases[idx][sizeof(_aliases[idx]) - 1] = '\0';
                            Serial.printf("[RELAY] Loaded Alias: ID %d -> %s\n", idx, _aliases[idx]);
                        }
                        idx++;
                    }
                }
            } else {
                Serial.printf("[RELAY] JSON Parse Error: %s\n", error.c_str());
            }
            file.close();
        }
    } else {
        Serial.println("[RELAY] /config.json NOT FOUND in SPIFFS.");
    }

    Serial.println("[RELAY] All relays initialized OFF");
}

// ─── Control ─────────────────────────────────────────────────────────────────

void relay_set(int idx, bool on) {
    if (idx < 0 || idx >= NUM_RELAYS) return;
    _states[idx] = on;
    digitalWrite(_pins[idx], on ? HIGH : LOW);
    Serial.printf("[RELAY] %s -> %s\n", _aliases[idx], on ? "ON" : "OFF");
}

void relay_toggle(int idx) {
    if (idx < 0 || idx >= NUM_RELAYS) return;
    relay_set(idx, !_states[idx]);
}

void relay_all_off() {
    for (int i = 0; i < NUM_RELAYS; i++) {
        relay_set(i, false);
    }
}

// ─── Query ───────────────────────────────────────────────────────────────────

bool relay_get(int idx) {
    if (idx < 0 || idx >= NUM_RELAYS) return false;
    return _states[idx];
}

const bool* relay_get_all() {
    return _states;
}

int relay_count() {
    return NUM_RELAYS;
}

const char* relay_get_alias(int idx) {
    if (idx < 0 || idx >= NUM_RELAYS) return "Unknown";
    return _aliases[idx];
}
