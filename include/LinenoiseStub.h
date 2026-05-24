#ifndef LINENOISE_STUB_H
#define LINENOISE_STUB_H

#ifdef _WIN32

#include <cstdlib>              // malloc, free
#include <cstring>              // strdup
#include <iostream>             // cout, cin, getline
#include <string>               // string

inline char* linenoise(const char* prompt) {
    std::cout << prompt << std::flush;
    std::string line;
    if (!std::getline(std::cin, line)) return nullptr;
    return _strdup(line.c_str());
}

inline void linenoiseFree(void* ptr) {
    free(ptr);
}

inline int linenoiseHistoryAdd(const char*) { return 0; }
inline int linenoiseHistorySetMaxLen(int) { return 0; }
inline int linenoiseHistorySave(const char*) { return 0; }
inline int linenoiseHistoryLoad(const char*) { return 0; }
inline void linenoiseClearScreen(void) {
    std::cout << "\033[2J\033[H" << std::flush;
}
inline void linenoiseSetMultiLine(int) {}

#endif // _WIN32
#endif // LINENOISE_STUB_H
