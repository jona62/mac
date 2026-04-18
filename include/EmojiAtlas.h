#ifndef EMOJI_ATLAS_H
#define EMOJI_ATLAS_H

#include <cstdio>               // snprintf
#include <filesystem>           // path, exists
#include <fstream>              // ifstream
#include <string>               // string
#include <unordered_map>        // unordered_map
#include <vector>               // vector

#include "nlohmann/json.hpp"    // JSON parsing for emoji metadata
#include "stb/stb_image.h"      // stbi_load

namespace meme {

class EmojiAtlas {
public:
    static EmojiAtlas& instance(const std::string& assetsDir = "") {
        static EmojiAtlas atlas;
        if (!atlas.attempted_ && !assetsDir.empty()) {
            atlas.attempted_ = true;
            atlas.load(assetsDir);
        }
        return atlas;
    }

    bool isLoaded() const { return loaded_; }

    static bool isEmojiStart(int cp) {
        return (cp >= 0x203C && cp <= 0x3299) ||
               (cp >= 0x1F000 && cp <= 0x1FAFF) ||
               (cp >= 0xFE00 && cp <= 0xFE0F) ||
               (cp >= 0x2600 && cp <= 0x27BF) ||
               (cp >= 0xE0020 && cp <= 0xE007F) ||
               cp == 0x200D || cp == 0x20E3 ||
               (cp >= 0x2300 && cp <= 0x23FF) ||
               (cp >= 0x2B05 && cp <= 0x2B55);
    }

    std::string matchEmoji(const std::vector<int>& cps, size_t& i) const {
        if (!loaded_ || i >= cps.size()) return "";

        std::string key = cpHex(cps[i]);
        std::string bestKey = sprites_.count(key) ? key : "";
        size_t bestEnd = i + 1;

        size_t j = i + 1;
        std::string tryKey = key;
        while (j < cps.size()) {
            if (cps[j] == 0xFE0F || cps[j] == 0xFE0E) {
                std::string withVS = tryKey + "-" + cpHex(cps[j]);
                if (sprites_.count(withVS)) { tryKey = withVS; bestKey = tryKey; bestEnd = j + 1; }
                ++j;
                continue;
            }
            if (cps[j] == 0x200D && j + 1 < cps.size()) {
                tryKey += "-200D-" + cpHex(cps[j + 1]);
                j += 2;
                if (sprites_.count(tryKey)) { bestKey = tryKey; bestEnd = j; }
                continue;
            }
            if (cps[j] == 0x20E3) {
                tryKey += "-20E3";
                ++j;
                if (sprites_.count(tryKey)) { bestKey = tryKey; bestEnd = j; }
                continue;
            }
            if (cps[j] >= 0x1F3FB && cps[j] <= 0x1F3FF) {
                tryKey += "-" + cpHex(cps[j]);
                ++j;
                if (sprites_.count(tryKey)) { bestKey = tryKey; bestEnd = j; }
                continue;
            }
            break;
        }

        if (!bestKey.empty()) {
            i = bestEnd - 1;
            return bestKey;
        }
        return "";
    }

    float emojiAdvance(int targetHeight) const {
        return static_cast<float>(targetHeight);
    }

    void blitEmoji(const std::string& key,
                   std::vector<unsigned char>& pixels,
                   int imgW, int imgH,
                   int x, int y,
                   int targetHeight) const {
        auto it = sprites_.find(key);
        if (it == sprites_.end()) return;

        int srcX = it->second.col * cellSize_;
        int srcY = it->second.row * cellSize_;
        float scale = static_cast<float>(targetHeight) / cellSize_;
        int tw = targetHeight;

        for (int dy = 0; dy < targetHeight; ++dy) {
            for (int dx = 0; dx < tw; ++dx) {
                int dstX = x + dx, dstY = y + dy;
                if (dstX < 0 || dstX >= imgW || dstY < 0 || dstY >= imgH) continue;
                int sX = srcX + static_cast<int>(dx / scale);
                int sY = srcY + static_cast<int>(dy / scale);
                if (sX >= atlasW_ || sY >= atlasH_) continue;
                int si = (sY * atlasW_ + sX) * 4;
                unsigned char sa = atlasPixels_[si + 3];
                if (sa == 0) continue;
                int di = (dstY * imgW + dstX) * 4;
                float srcA = sa / 255.0f;
                float dstA = pixels[di + 3] / 255.0f;
                float outA = srcA + dstA * (1.0f - srcA);
                if (outA > 0) {
                    pixels[di+0] = static_cast<unsigned char>((atlasPixels_[si+0]*srcA + pixels[di+0]*dstA*(1.0f-srcA)) / outA);
                    pixels[di+1] = static_cast<unsigned char>((atlasPixels_[si+1]*srcA + pixels[di+1]*dstA*(1.0f-srcA)) / outA);
                    pixels[di+2] = static_cast<unsigned char>((atlasPixels_[si+2]*srcA + pixels[di+2]*dstA*(1.0f-srcA)) / outA);
                    pixels[di+3] = static_cast<unsigned char>(outA * 255.0f);
                }
            }
        }
    }

private:
    struct Sprite { int col; int row; };

    bool loaded_ = false;
    bool attempted_ = false;
    int cellSize_ = 72;
    int atlasW_ = 0, atlasH_ = 0;
    std::vector<unsigned char> atlasPixels_;
    std::unordered_map<std::string, Sprite> sprites_;

    EmojiAtlas() = default;

    void load(const std::string& assetsDir) {
        std::string atlasPath = assetsDir + "/emoji/emoji_atlas.png";
        std::string metaPath = assetsDir + "/emoji/emoji_meta.json";

        if (!std::filesystem::exists(atlasPath) || !std::filesystem::exists(metaPath)) return;

        int c;
        unsigned char* data = stbi_load(atlasPath.c_str(), &atlasW_, &atlasH_, &c, 4);
        if (!data) return;
        atlasPixels_.assign(data, data + atlasW_ * atlasH_ * 4);
        stbi_image_free(data);

        try {
            std::ifstream f(metaPath);
            if (!f.is_open()) return;
            nlohmann::json meta = nlohmann::json::parse(f);
            cellSize_ = meta.value("cell", 72);
            for (auto& [key, val] : meta["sprites"].items()) {
                sprites_[key] = {val[0].get<int>(), val[1].get<int>()};
            }
        } catch (...) {
            sprites_.clear();
            return;
        }

        loaded_ = true;
    }

    static std::string cpHex(int cp) {
        char buf[16];
        std::snprintf(buf, sizeof(buf), "%X", cp);
        return buf;
    }
};

} // namespace meme

#endif // EMOJI_ATLAS_H
