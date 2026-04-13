#include "resource_loader.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <windows.h>
#include <direct.h>
#include <process.h>
#else
#include <unistd.h>
#endif

resource_manager_t g_resource_manager = {0};

#define PAK_MAGIC "VLPAK"
#define PAK_VERSION 1

typedef struct {
    char* path;
    uint32_t offset;
    uint32_t size;
} pak_file_entry_t;

typedef struct {
    pak_file_entry_t* entries;
    uint32_t count;
} pak_header_t;

static uint8_t* load_exe_data(uint32_t* out_size) {
#ifdef _WIN32
    HMODULE hModule = GetModuleHandle(NULL);
    if (!hModule) return NULL;
    
    // Get the module base address
    uint8_t* base = (uint8_t*)hModule;
    
    // Read PE header to find image size
    IMAGE_DOS_HEADER* dos = (IMAGE_DOS_HEADER*)base;
    if (dos->e_magic != 0x5A4D) return NULL; // 'MZ'
    
    IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + dos->e_lfanew);
    uint32_t image_size = nt->OptionalHeader.SizeOfImage;
    
    // Read all exe memory
    uint8_t* exe_data = (uint8_t*)malloc(image_size + 10000000); // Extra space for PAK
    if (!exe_data) return NULL;
    
    memcpy(exe_data, base, image_size);
    *out_size = image_size;
    return exe_data;
#else
    // On Linux, read the executable file itself
    char path[1024];
    if (readlink("/proc/self/exe", path, sizeof(path) - 1) < 0) {
        return NULL;
    }
    
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    uint32_t size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    uint8_t* data = (uint8_t*)malloc(size);
    if (!data) {
        fclose(f);
        return NULL;
    }
    
    if (fread(data, 1, size, f) != size) {
        free(data);
        fclose(f);
        return NULL;
    }
    
    fclose(f);
    *out_size = size;
    return data;
#endif
}

static pak_header_t* load_pak_from_data(uint8_t* data, uint32_t size) {
    if (size < 13) return NULL; // Too small (4 bytes size + 4 bytes marker + at least 5 bytes PAK magic)
    
    // Look for marker "PAKX" at the very end
    if (memcmp(data + size - 4, "PAKX", 4) != 0) {
        printf("[ResourceLoader] No PAKX marker found\n");
        return NULL;
    }
    
    // Read PAK size from before the marker
    uint32_t pak_size;
    memcpy(&pak_size, data + size - 8, 4);
    
    printf("[ResourceLoader] Found PAK marker, size: %u bytes\n", pak_size);
    
    if (pak_size < 10 || pak_size > size - 8) {
        printf("[ResourceLoader] Invalid PAK size: %u\n", pak_size);
        return NULL;
    }
    
    // PAK data starts at (size - 8 - pak_size)
    uint8_t* pak_start = data + size - 8 - pak_size;
    
    // Check magic
    if (memcmp(pak_start, "VLPAK", 5) != 0) {
        printf("[ResourceLoader] Invalid PAK magic\n");
        return NULL;
    }
    
    // Parse header
    pak_header_t* header = (pak_header_t*)malloc(sizeof(pak_header_t));
    if (!header) return NULL;
    
    uint8_t* ptr = pak_start + 5; // Skip magic
    
    uint32_t file_count;
    memcpy(&file_count, ptr, 4);
    ptr += 4;
    
    printf("[ResourceLoader] PAK contains %u files\n", file_count);
    
    header->entries = (pak_file_entry_t*)malloc(file_count * sizeof(pak_file_entry_t));
    header->count = file_count;
    
    if (!header->entries) {
        free(header);
        return NULL;
    }
    
    // Read file table - simpler format: [name_len][name][data_len][data]
    uint32_t data_offset = 0;
    for (uint32_t i = 0; i < file_count; i++) {
        uint32_t namelen;
        memcpy(&namelen, ptr, 4);
        ptr += 4;
        
        if (namelen > 512) {
            printf("[ResourceLoader] Invalid name length: %u\n", namelen);
            free(header->entries);
            free(header);
            return NULL;
        }
        
        char* name = (char*)malloc(namelen + 1);
        memcpy(name, ptr, namelen);
        name[namelen] = '\0';
        ptr += namelen;
        
        uint32_t datalen;
        memcpy(&datalen, ptr, 4);
        ptr += 4;
        
        // data_offset is absolute: pak_start + current position in ptr
        data_offset = (uint32_t)(ptr - pak_start);
        
        header->entries[i].path = name;
        header->entries[i].size = datalen;
        header->entries[i].offset = data_offset;
        
        // Skip to next file
        ptr += datalen;
    }
    
    // Store pak_start for later data access
    g_resource_manager.pak_base = pak_start;
    
    return header;
}

bool resource_manager_init(void) {
    printf("[ResourceLoader] Initializing resource manager...\n");
    
#ifdef _WIN32
    // Try to load PAK from the exe file itself
    // Get the exe path first
    char exe_path[MAX_PATH];
    if (!GetModuleFileNameA(NULL, exe_path, MAX_PATH)) {
        printf("[ResourceLoader] ERROR: Could not get exe path\n");
        return false;
    }
    
    printf("[ResourceLoader] Loading from exe: %s\n", exe_path);
    
    // Open exe file and look for PAKX marker at end
    FILE* exe_file = fopen(exe_path, "rb");
    if (!exe_file) {
        printf("[ResourceLoader] ERROR: Could not open exe file\n");
        return false;
    }
    
    // Get file size
    fseek(exe_file, 0, SEEK_END);
    uint32_t exe_size = ftell(exe_file);
    
    // Read last 4 bytes (should be "PAKX")
    fseek(exe_file, exe_size - 4, SEEK_SET);
    uint8_t marker[4];
    if (fread(marker, 1, 4, exe_file) != 4 || memcmp(marker, "PAKX", 4) != 0) {
        printf("[ResourceLoader] ERROR: No PAKX marker found in exe\n");
        fclose(exe_file);
        return false;
    }
    
    // Read PAK size (4 bytes before PAKX marker)
    fseek(exe_file, exe_size - 8, SEEK_SET);
    uint32_t pak_size;
    if (fread(&pak_size, 4, 1, exe_file) != 1) {
        printf("[ResourceLoader] ERROR: Could not read PAK size\n");
        fclose(exe_file);
        return false;
    }
    
    printf("[ResourceLoader] Found PAK: size=%u bytes\n", pak_size);
    
    // Seek to PAK start
    uint32_t pak_offset = exe_size - pak_size - 8;
    fseek(exe_file, pak_offset, SEEK_SET);
    
    // Read PAK magic
    uint8_t pak_magic[5];
    if (fread(pak_magic, 1, 5, exe_file) != 5 || memcmp(pak_magic, "VLPAK", 5) != 0) {
        printf("[ResourceLoader] ERROR: Invalid PAK magic\n");
        fclose(exe_file);
        return false;
    }
    
    printf("[ResourceLoader] Valid PAK found!\n");
    fseek(exe_file, pak_offset, SEEK_SET);
    
    // Load entire PAK into memory
    uint8_t* pak_data = (uint8_t*)malloc(pak_size);
    if (!pak_data) {
        printf("[ResourceLoader] ERROR: Failed to allocate memory for PAK\n");
        fclose(exe_file);
        return false;
    }
    
    if (fread(pak_data, 1, pak_size, exe_file) != pak_size) {
        printf("[ResourceLoader] ERROR: Failed to read PAK data\n");
        free(pak_data);
        fclose(exe_file);
        return false;
    }
    
    fclose(exe_file);
    
    g_resource_manager.pak_data = pak_data;
    g_resource_manager.pak_base = pak_data;
    g_resource_manager.pak_size = pak_size;
    g_resource_manager.loaded = true;
    
    // Count files and verify PAK is valid
    uint8_t* ptr = pak_data + 5; // Skip magic
    uint32_t file_count;
    memcpy(&file_count, ptr, 4);
    
    printf("[ResourceLoader] Successfully loaded PAK with %u files from exe\n", file_count);
    
    if (file_count == 0) {
        printf("[ResourceLoader] ERROR: PAK contains 0 files! This is invalid.\n");
        free(pak_data);
        g_resource_manager.pak_data = NULL;
        g_resource_manager.pak_base = NULL;
        return false;
    }
    
    return true;
#else
    printf("[ResourceLoader] PAK loading not supported on this platform\n");
    return false;
#endif
}

void resource_manager_cleanup(void) {
    if (g_resource_manager.pak_data) {
        free(g_resource_manager.pak_data);
        g_resource_manager.pak_data = NULL;
    }
}

static char* get_appdata_resource_path(const char* filename) {
    static char path[512];
#ifdef _WIN32
    const char* appdata = getenv("LOCALAPPDATA");
    if (appdata) {
        snprintf(path, sizeof(path), "%s\\Zlither\\app\\res\\%s", appdata, filename);
        // Convert backslashes to forward slashes for consistency
        for (int i = 0; path[i]; i++) {
            if (path[i] == '\\') path[i] = '/';
        }
        return path;
    }
#endif
    return NULL;
}

bool resource_ensure_appdata_extracted(void) {
    // Check if resources already exist in AppData
    const char* appdata = getenv("LOCALAPPDATA");
    if (!appdata) {
        printf("[ResourceLoader] ERROR: LOCALAPPDATA env var not found\n");
        return false;  // Will use embedded PAK as fallback
    }
    
    char appdata_res_dir[1024];
    snprintf(appdata_res_dir, sizeof(appdata_res_dir), "%s\\Zlither\\app\\res", appdata);
    
    // Check if resources have already been extracted by verifying a known file exists
    char test_file_path[1024];
    snprintf(test_file_path, sizeof(test_file_path), "%s\\fonts\\mono_regular.ttf", appdata_res_dir);
    
    FILE* test_file = fopen(test_file_path, "rb");
    if (test_file) {
        fclose(test_file);
        printf("[ResourceLoader] AppData resources already extracted at %s\n", appdata_res_dir);
        return true;
    }
    
    // Resources missing or not yet extracted - extract from embedded PAK
    if (!g_resource_manager.pak_base || !g_resource_manager.pak_data) {
        printf("[ResourceLoader] WARNING: No embedded PAK - cannot extract to AppData, will use embedded PAK\n");
        return false;  // Will use embedded PAK as fallback
    }
    
    printf("[ResourceLoader] Resources not found - attempting extraction from embedded PAK...\n");
    
    // Create the parent Vlither directory
    char parent_dir[1024];
    snprintf(parent_dir, sizeof(parent_dir), "%s\\Vlither", appdata);
    
    char create_dir_cmd[2048];
    snprintf(create_dir_cmd, sizeof(create_dir_cmd), "if not exist \"%s\" mkdir \"%s\"", parent_dir, parent_dir);
    int result = system(create_dir_cmd);
    if (result != 0) {
        printf("[ResourceLoader] WARNING: Failed to create Zlither directory, will use embedded PAK\n");
        return false;  // Will use embedded PAK as fallback
    }
    
    // Extract files directly from PAK to AppData
    uint8_t* pak_start = g_resource_manager.pak_base;
    uint8_t* ptr = pak_start + 5; // Skip "VLPAK"
    uint32_t file_count_pak;
    memcpy(&file_count_pak, ptr, 4);
    ptr += 4;
    
    printf("[ResourceLoader] Extracting %u files from embedded PAK to AppData...\n", file_count_pak);
    
    uint32_t extracted_count = 0;
    uint32_t failed_count = 0;
    
    // Extract each file from PAK
    for (uint32_t i = 0; i < file_count_pak; i++) {
        uint32_t namelen;
        memcpy(&namelen, ptr, 4);
        ptr += 4;
        
        char filename[512];
        memcpy(filename, ptr, namelen);
        filename[namelen] = '\0';
        ptr += namelen;
        
        uint32_t datalen;
        memcpy(&datalen, ptr, 4);
        ptr += 4;
        
        // Normalize forward slashes to backslashes for Windows
        for (int j = 0; filename[j]; j++) {
            if (filename[j] == '/') filename[j] = '\\';
        }
        
        // Build full output path in AppData
        char output_path[1024];
        snprintf(output_path, sizeof(output_path), "%s\\%s", appdata_res_dir, filename);
        
        // Create subdirectories
        char* last_slash = strrchr(output_path, '\\');
        if (last_slash) {
            *last_slash = '\0';
            char mkdir_cmd[1024];
            snprintf(mkdir_cmd, sizeof(mkdir_cmd), "if not exist \"%s\" mkdir \"%s\"", output_path, output_path);
            int mkdir_result = system(mkdir_cmd);
            if (mkdir_result != 0) {
                printf("[ResourceLoader] WARNING: Failed to create directory for %s\n", output_path);
                failed_count++;
            }
            *last_slash = '\\';
        }
        
        // Write file from PAK
        FILE* out = fopen(output_path, "wb");
        if (out) {
            size_t written = fwrite(ptr, 1, datalen, out);
            if (written == datalen) {
                extracted_count++;
            } else {
                printf("[ResourceLoader] WARNING: Incomplete write to %s\n", output_path);
                failed_count++;
            }
            fclose(out);
        } else {
            printf("[ResourceLoader] WARNING: Could not write to %s\n", output_path);
            failed_count++;
        }
        
        ptr += datalen;
    }
    
    printf("[ResourceLoader] Extracted %u of %u files (%u failed), will use embedded PAK for missing files\n", 
           extracted_count, file_count_pak, failed_count);
    
    return true;  // Return true even if partial - embedded PAK will provide fallback
}

resource_t* resource_load(const char* path) {
    if (!path) return NULL;
    
    resource_t* res = (resource_t*)malloc(sizeof(resource_t));
    res->from_pak = false;
    
    // Normalize the requested path - strip "app/res/" prefix if present
    const char* search_path = path;
    if (strncmp(path, "app/res/", 8) == 0) {
        search_path = path + 8;  // Skip "app/res/"
    }
    
    printf("[ResourceLoader] Attempting to load: %s\n", search_path);
    
    // Priority 1: Try AppData path first (extracted resources)
    char* appdata_path = get_appdata_resource_path(search_path);
    if (appdata_path) {
        FILE* f = fopen(appdata_path, "rb");
        if (f) {
            printf("[ResourceLoader] Found in AppData: %s\n", search_path);
            fseek(f, 0, SEEK_END);
            uint32_t filesize = ftell(f);
            fseek(f, 0, SEEK_SET);
            
            uint8_t* file_data = (uint8_t*)malloc(filesize);
            if (fread(file_data, 1, filesize, f) == filesize) {
                fclose(f);
                res->data = file_data;
                res->size = filesize;
                res->from_pak = false;
                printf("[ResourceLoader] Loaded from AppData: %s (%u bytes)\n", search_path, filesize);
                return res;
            }
            fclose(f);
            free(file_data);
            printf("[ResourceLoader] AppData file incomplete/corrupt: %s\n", search_path);
        }
        // AppData file doesn't exist or is corrupted, fall through to PAK
        printf("[ResourceLoader] AppData file not found: %s, trying embedded PAK...\n", search_path);
    }
    
    // Priority 2: Try from embedded PAK (fallback for first-run or missing files)
    if (!g_resource_manager.pak_base || !g_resource_manager.pak_data) {
        printf("[ResourceLoader] ERROR: No embedded PAK available for fallback\n");
        free(res);
        return NULL;
    }
    
    printf("[ResourceLoader] Searching embedded PAK for: %s\n", search_path);
    uint8_t* pak_start = g_resource_manager.pak_base;
    
    // Skip magic and read file count
    uint8_t* ptr = pak_start + 5; // Skip "VLPAK"
    uint32_t file_count;
    memcpy(&file_count, ptr, 4);
    ptr += 4;
    
    printf("[ResourceLoader] PAK has %u files, searching...\n", file_count);
    
    // Search file table
    for (uint32_t i = 0; i < file_count; i++) {
        uint32_t namelen;
        memcpy(&namelen, ptr, 4);
        ptr += 4;
        
        char temp_name[512];
        memcpy(temp_name, ptr, namelen);
        temp_name[namelen] = '\0';
        ptr += namelen;
        
        uint32_t datalen;
        memcpy(&datalen, ptr, 4);
        ptr += 4;
        
        // Normalize path comparison (forward slashes)
        char norm_path[512];
        strcpy(norm_path, search_path);
        for (int k = 0; norm_path[k]; k++) {
            if (norm_path[k] == '\\') norm_path[k] = '/';
        }
        
        // Compare: PAK contains "textures/background_4k.png", we search for same
        if (strcmp(temp_name, norm_path) == 0) {
            // Found it! Data is right after the length
            printf("[ResourceLoader] SUCCESS: Found in PAK: %s (%u bytes)\n", search_path, datalen);
            uint8_t* file_data = (uint8_t*)malloc(datalen);
            memcpy(file_data, ptr, datalen);
            
            res->data = file_data;
            res->size = datalen;
            res->from_pak = true;
            
            return res;
        }
        
        // Move to next file
        ptr += datalen;
    }
    
    printf("[ResourceLoader] FATAL ERROR: Could not load resource '%s' - not found in AppData or embedded PAK\n", path);
    printf("[ResourceLoader] Search path was: %s\n", search_path);
    free(res);
    return NULL;
}

void resource_free(resource_t* res) {
    if (res) {
        if (res->data) free(res->data);
        free(res);
    }
}

bool resource_exists(const char* path) {
    // Try disk first
    FILE* f = fopen(path, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    
    char full_path[512];
    snprintf(full_path, sizeof(full_path), "app/res/%s", path);
    f = fopen(full_path, "rb");
    if (f) {
        fclose(f);
        return true;
    }
    
    return false;
}
