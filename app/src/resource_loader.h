#ifndef RESOURCE_LOADER_H
#define RESOURCE_LOADER_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    uint8_t* data;
    uint32_t size;
    bool from_pak;
} resource_t;

typedef struct {
    uint8_t* pak_data;
    uint8_t* pak_base;  // Pointer to PAK start within pak_data
    uint32_t pak_size;
    bool loaded;
} resource_manager_t;

extern resource_manager_t g_resource_manager;

// Initialize resource manager (load PAK from exe if available, else load from disk)
bool resource_manager_init(void);
void resource_manager_cleanup(void);

// Extract embedded PAK to AppData on first run
// Returns true if already extracted or successfully extracted
bool resource_ensure_appdata_extracted(void);

// Load resource by path (path like "fonts/Arial.ttf" or "shaders/bin/shader.spv")
// Load priority: AppData > embedded PAK > app/res > current directory
// Returns allocated data that must be freed with resource_free()
resource_t* resource_load(const char* path);
void resource_free(resource_t* res);

// Check if resource exists
bool resource_exists(const char* path);

#endif
