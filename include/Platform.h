#ifndef PLATFORM_H
#define PLATFORM_H

#include <cstdlib>              // getenv
#include <filesystem>           // path, canonical, temp_directory_path
#include <string>               // string

#ifdef _WIN32
#include <io.h>                 // _isatty, _fileno
#include <process.h>            // _getpid
#include <windows.h>            // GetModuleFileNameA, SetConsoleOutputCP, etc.
#include <cstdio>               // _popen, _pclose, fgets
#else
#include <unistd.h>             // isatty, fileno, getpid
#include <cstdio>               // popen, pclose, fgets
#ifdef __APPLE__
#include <mach-o/dyld.h>        // _NSGetExecutablePath
#endif
#endif

namespace platform {

    inline std::string getHomeDir() {
#ifdef _WIN32
        const char* home = std::getenv("USERPROFILE");
#else
        const char* home = std::getenv("HOME");
#endif
        return home ? std::string(home) : ".";
    }

    inline std::string getExePath() {
        std::filesystem::path exe;
#ifdef _WIN32
        char buf[MAX_PATH];
        DWORD len = GetModuleFileNameA(nullptr, buf, MAX_PATH);
        if (len > 0 && len < MAX_PATH) {
            exe = std::filesystem::canonical(buf);
        }
#elif defined(__APPLE__)
        char buf[1024];
        uint32_t size = sizeof(buf);
        if (_NSGetExecutablePath(buf, &size) == 0) {
            exe = std::filesystem::canonical(buf);
        }
#elif defined(__linux__)
        exe = std::filesystem::canonical("/proc/self/exe");
#endif
        if (!exe.empty()) return exe.parent_path().string();
        return ".";
    }

    inline bool isInteractiveTTY() {
#ifdef _WIN32
        return _isatty(_fileno(stdin));
#else
        return isatty(fileno(stdin));
#endif
    }

    inline std::string getTempDir() {
        return std::filesystem::temp_directory_path().string();
    }

    inline int getProcessId() {
#ifdef _WIN32
        return _getpid();
#else
        return getpid();
#endif
    }

    inline std::string popenRead(const char* cmd) {
#ifdef _WIN32
        FILE* pipe = _popen(cmd, "r");
#else
        FILE* pipe = popen(cmd, "r");
#endif
        if (!pipe) return "";
        std::string result;
        char buf[4096];
        while (fgets(buf, sizeof(buf), pipe)) {
            result += buf;
        }
#ifdef _WIN32
        int status = _pclose(pipe);
#else
        int status = pclose(pipe);
#endif
        if (status != 0) return "";
        return result;
    }

    inline const char* nullDevice() {
#ifdef _WIN32
        return "2>NUL";
#else
        return "2>/dev/null";
#endif
    }

    inline void enableAnsiEscapes() {
#ifdef _WIN32
        SetConsoleOutputCP(CP_UTF8);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if (hOut != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (GetConsoleMode(hOut, &mode)) {
                SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        }
        HANDLE hErr = GetStdHandle(STD_ERROR_HANDLE);
        if (hErr != INVALID_HANDLE_VALUE) {
            DWORD mode = 0;
            if (GetConsoleMode(hErr, &mode)) {
                SetConsoleMode(hErr, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
            }
        }
#endif
    }

    inline std::string installDir() {
        return getHomeDir() + "/.mac";
    }

    inline std::string historyFile() {
        return getHomeDir() + "/.mac_history";
    }

    inline std::string outputDir() {
        return getHomeDir() + "/mac/output";
    }

#ifdef _WIN32
    inline std::string binLink() {
        return getHomeDir() + "/.mac/mac.exe";
    }
#else
    inline std::string binLink() {
        return getHomeDir() + "/.local/bin/mac";
    }
#endif

} // namespace platform

#endif // PLATFORM_H
