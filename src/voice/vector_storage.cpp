/**
 * vector_storage.cpp — Persistent storage for voice embeddings on SPIFFS
 * IoT Voice Command System
 */
#include "vector_storage.h"
#include <SPIFFS.h>
#include <Arduino.h>

static const char* get_path(int gpio, bool on_cmd, char* buf, size_t len) {
    snprintf(buf, len, "/cmd_%d_%s.bin", gpio, on_cmd ? "on" : "off");
    return buf;
}

bool vector_storage_init() {
    if (!SPIFFS.begin(true)) {
        Serial.println("[STORAGE] SPIFFS Mount Failed");
        return false;
    }
    Serial.println("[STORAGE] SPIFFS Initialized");
    return true;
}

bool vector_storage_save(int gpio, bool on_cmd, const float* vector) {
    char path[32];
    get_path(gpio, on_cmd, path, sizeof(path));
    
    File file = SPIFFS.open(path, "w");
    if (!file) {
        Serial.printf("[STORAGE] Failed to open %s for writing\n", path);
        return false;
    }
    
    size_t written = file.write((uint8_t*)vector, VECTOR_DIM * sizeof(float));
    file.close();
    
    return written == (VECTOR_DIM * sizeof(float));
}

bool vector_storage_load(int gpio, bool on_cmd, float* vector_out) {
    char path[32];
    get_path(gpio, on_cmd, path, sizeof(path));
    
    File file = SPIFFS.open(path, "r");
    if (!file) return false;
    
    size_t read_bytes = file.read((uint8_t*)vector_out, VECTOR_DIM * sizeof(float));
    file.close();
    
    return read_bytes == (VECTOR_DIM * sizeof(float));
}

bool vector_storage_exists(int gpio, bool on_cmd) {
    char path[32];
    get_path(gpio, on_cmd, path, sizeof(path));
    return SPIFFS.exists(path);
}

bool vector_storage_delete(int gpio, bool on_cmd) {
    char path[32];
    get_path(gpio, on_cmd, path, sizeof(path));
    return SPIFFS.remove(path);
}
