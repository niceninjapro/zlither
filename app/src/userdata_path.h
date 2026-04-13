#ifndef USERDATA_PATH_H
#define USERDATA_PATH_H

#ifdef _WIN32
    #include <windows.h>
    #include <shlobj.h>
    #include <string>
    #include <filesystem>

    namespace fs = std::filesystem;

    inline std::string GetUserDataPath() {
        char appDataPath[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, appDataPath))) {
            std::string zlitherDataPath = std::string(appDataPath) + "\\Zlither";
            
            // Create directory if it doesn't exist
            try {
                if (!fs::exists(zlitherDataPath)) {
                    fs::create_directories(zlitherDataPath);
                }
            } catch (...) {
                // If creation fails, fall back to relative path
                return ".\\data";
            }
            
            return zlitherDataPath;
        }
        
        // Fallback
        return ".\\data";
    }
#else
    #include <string>
    #include <filesystem>
    #include <cstdlib>

    namespace fs = std::filesystem;

    inline std::string GetUserDataPath() {
        std::string homeDir = std::getenv("HOME");
        std::string zlitherDataPath = homeDir + "/.local/share/zlither";
        
        try {
            if (!fs::exists(zlitherDataPath)) {
                fs::create_directories(zlitherDataPath);
            }
        } catch (...) {
            return "./data";
        }
        
        return zlitherDataPath;
    }
#endif

#endif // USERDATA_PATH_H
