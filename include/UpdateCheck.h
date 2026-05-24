#ifndef UPDATE_CHECK_H
#define UPDATE_CHECK_H

#include <chrono>               // system_clock (cache TTL)
#include <filesystem>           // path, exists, create_directories
#include <fstream>              // ifstream, ofstream
#include <iostream>             // cerr
#include <sstream>              // istringstream
#include <string>               // string, stoi
#include "Platform.h"           // platform::popenRead, getHomeDir, nullDevice
#include "nlohmann/json.hpp"    // nlohmann::json (API response parsing)

namespace updateCheck {

    struct SemVer {
        int major = 0, minor = 0, patch = 0;

        bool operator>(const SemVer& o) const {
            if (major != o.major) return major > o.major;
            if (minor != o.minor) return minor > o.minor;
            return patch > o.patch;
        }
    };

    inline SemVer parseSemVer(const std::string& v) {
        SemVer sv;
        std::string s = v;
        // Strip leading 'v' if present
        if (!s.empty() && s[0] == 'v') s = s.substr(1);
        try {
            auto dot1 = s.find('.');
            if (dot1 == std::string::npos) return sv;
            sv.major = std::stoi(s.substr(0, dot1));
            auto dot2 = s.find('.', dot1 + 1);
            if (dot2 == std::string::npos) {
                sv.minor = std::stoi(s.substr(dot1 + 1));
                return sv;
            }
            sv.minor = std::stoi(s.substr(dot1 + 1, dot2 - dot1 - 1));
            sv.patch = std::stoi(s.substr(dot2 + 1));
        } catch (...) {}
        return sv;
    }

    inline std::string execCurl() {
        std::string cmd = std::string("curl -sf --max-time 2 "
            "https://api.github.com/repos/jona62/mac/releases/latest ") + platform::nullDevice();
        return platform::popenRead(cmd.c_str());
    }

    inline void checkForUpdate() {
        try {
            std::filesystem::path cacheDir = platform::installDir();
            std::filesystem::path cacheFile = cacheDir / "update_check";

            auto now = std::chrono::system_clock::now();
            auto nowEpoch = std::chrono::duration_cast<std::chrono::seconds>(
                now.time_since_epoch()).count();

            // Check cache — skip API call if checked within 24 hours
            std::string cachedVersion;
            if (std::filesystem::exists(cacheFile)) {
                std::ifstream in(cacheFile);
                std::string tsLine, verLine;
                if (std::getline(in, tsLine) && std::getline(in, verLine)) {
                    try {
                        long long cachedTs = std::stoll(tsLine);
                        if (nowEpoch - cachedTs < 86400) {
                            cachedVersion = verLine;
                        }
                    } catch (...) {}
                }
            }

            std::string latestVersion;
            if (!cachedVersion.empty()) {
                latestVersion = cachedVersion;
            } else {
                // Fetch from GitHub API
                auto response = execCurl();
                if (response.empty()) return;

                auto j = nlohmann::json::parse(response, nullptr, false);
                if (j.is_discarded() || !j.contains("tag_name")) return;

                latestVersion = j["tag_name"].get<std::string>();
                // Strip leading 'v'
                if (!latestVersion.empty() && latestVersion[0] == 'v') {
                    latestVersion = latestVersion.substr(1);
                }

                // Write cache
                std::filesystem::create_directories(cacheDir);
                std::ofstream out(cacheFile);
                out << nowEpoch << "\n" << latestVersion << "\n";
            }

            auto current = parseSemVer(MAC_VERSION);
            auto latest = parseSemVer(latestVersion);

            if (latest > current) {
                std::cerr << "\033[2mUpdate available: v" << MAC_VERSION
                          << " \xe2\x86\x92 v" << latestVersion
                          << " \xe2\x80\x94 run: curl -fsSL https://macstudio.meme/install.sh | bash\033[0m"
                          << std::endl;
            }
        } catch (...) {
            // Silent failure — never block startup
        }
    }

} // namespace updateCheck

#endif // UPDATE_CHECK_H
