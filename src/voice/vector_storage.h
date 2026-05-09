/**
 * vector_storage.h — Persistent storage for voice embeddings on SPIFFS
 * IoT Voice Command System
 */
#ifndef VECTOR_STORAGE_H
#define VECTOR_STORAGE_H

#include "vector_math.h"
#include <stdbool.h>

/**
 * Initialize LittleFS/SPIFFS
 */
bool vector_storage_init();

/**
 * Save a 128-dim vector to SPIFFS
 * Path: /spiffs/cmd_<gpio>_<on/off>.bin
 */
bool vector_storage_save(int gpio, bool on_cmd, const float* vector);

/**
 * Load a 128-dim vector from SPIFFS
 */
bool vector_storage_load(int gpio, bool on_cmd, float* vector_out);

/**
 * Check if a voice command exists for this GPIO
 */
bool vector_storage_exists(int gpio, bool on_cmd);

/**
 * Delete a voice command
 */
bool vector_storage_delete(int gpio, bool on_cmd);

#endif // VECTOR_STORAGE_H
