#ifndef MAC_MEME_H
#define MAC_MEME_H

#include <algorithm>            // transform, min
#include <filesystem>           // path, exists (template resolution)
#include <iomanip>              // hex formatting
#include <memory>               // shared_ptr
#include <string>               // string
#include <sstream>              // ostringstream (toString)
#include <unordered_map>        // unordered_map (template registry)
#include <vector>               // vector (positioned texts, panels)

namespace meme {

    // Text rendering style — used by MemeRenderer and MacMeme
    struct TextStyle {
        unsigned char textR = 255, textG = 255, textB = 255, textA = 255;
        unsigned char outlineR = 0, outlineG = 0, outlineB = 0;
        int outlineWidth = 3;
        int shadowOffsetX = 0, shadowOffsetY = 0;
        unsigned char shadowR = 0, shadowG = 0, shadowB = 0, shadowA = 128;
        float fontSizeOverride = 0;
        unsigned char bgR = 0, bgG = 0, bgB = 0, bgA = 0;  // 0 alpha = no background
        bool uppercase = true;   // false = preserve original case
        bool bold = true;        // false = normal weight (thinner strokes)
    };

    struct PositionedText {
        std::string content;
        int x = 0, y = 0;  // pixel coordinates from top-left
        float fontSizeOverride = 0;
    };

    // Forward declare MemeRenderer -- included only in method bodies below
    class MemeRenderer;

    class MacMeme {
    public:
        std::string templateName;
        std::string topText;
        std::string centerText;
        std::string bottomText;
        std::string imagePath;  // resolved path to template image
        int width = 0;          // 0 = use template's native size
        int height = 0;
        TextStyle style;        // text rendering style
        std::vector<PositionedText> positionedTexts;

        MacMeme(const std::string& tmpl, const std::string& top, const std::string& bottom,
                const std::string& center = "")
            : templateName(tmpl), topText(top), centerText(center), bottomText(bottom) {}

        // Known template map + custom templates
        static std::unordered_map<std::string, std::string>& templateMap() {
            static std::unordered_map<std::string, std::string> map = {
                {"two_panel",      "assets/templates/two_panel.png"},
                {"three_panel",    "assets/templates/three_panel.png"},
                {"bottom_text",    "assets/templates/bottom_text.png"},
                {"blank",          "assets/templates/blank.png"},
                {"caption_bar",    "assets/templates/caption_bar.png"},
                {"four_panel",     "assets/templates/four_panel.png"},
                {"wide",           "assets/templates/wide.png"},
                {"tall",           "assets/templates/tall.png"},
                {"square",         "assets/templates/square.png"},
                {"dark",           "assets/templates/dark.png"},
            };
            return map;
        }

        // Binary directory for resolving assets — set by main()
        static std::string& binaryDir() {
            static std::string dir = ".";
            return dir;
        }

        // Script file directory for resolving relative @"path" templates
        static std::string& scriptDir() {
            static std::string dir = ".";
            return dir;
        }

        // Resolve a template name to an image file path
        // Check if a string contains path traversal sequences
        static bool hasTraversal(const std::string& s) {
            return s.find("..") != std::string::npos ||
                   s.find('/') != std::string::npos ||
                   s.find('\\') != std::string::npos;
        }

        static std::string resolveTemplate(const std::string& name) {
            auto& map = templateMap();
            auto it = map.find(name);
            if (it != map.end()) {
                // Try binary-relative path first
                auto binPath = binaryDir() + "/" + it->second;
                if (std::filesystem::exists(binPath)) return binPath;
                return it->second;
            }

            // Dotted name -> subdirectory lookup (e.g. "meme.shrek_smirk")
            auto dot = name.find('.');
            if (dot != std::string::npos) {
                auto category = name.substr(0, dot);
                auto base = name.substr(dot + 1);
                // Don't treat "file.jpg" as a dotted category
                static const std::vector<std::string> imgExts =
                    {"png", "jpg", "jpeg", "gif", "bmp", "webp"};
                bool catIsExt = std::find(imgExts.begin(), imgExts.end(), category) != imgExts.end();
                bool baseIsExt = std::find(imgExts.begin(), imgExts.end(), base) != imgExts.end();
                if (!catIsExt && !baseIsExt && !hasTraversal(category) && !hasTraversal(base)) {
                    std::string dir = "assets/templates/" + category + "/";
                    for (auto& e : imgExts) {
                        auto path = dir + base + "." + e;
                        auto binPath = binaryDir() + "/" + path;
                        if (std::filesystem::exists(binPath)) return binPath;
                        if (std::filesystem::exists(path)) return path;
                    }
                }
            }

            // Direct file path - try script-relative, then binary-relative
            // For paths with directory components, validate they resolve under allowed dirs
            auto scrPath = scriptDir() + "/" + name;
            if (std::filesystem::exists(scrPath)) {
                auto resolved = std::filesystem::canonical(scrPath).string();
                auto scrBase = std::filesystem::canonical(scriptDir()).string();
                if (resolved.starts_with(scrBase)) return resolved;
            }
            auto binPath = binaryDir() + "/" + name;
            if (std::filesystem::exists(binPath)) {
                auto resolved = std::filesystem::canonical(binPath).string();
                auto binBase = std::filesystem::canonical(binaryDir()).string();
                if (resolved.starts_with(binBase)) return resolved;
            }
            return name;
        }

        // Register a custom template
        static void addTemplate(const std::string& name, const std::string& path) {
            templateMap()[name] = path;
        }

        // Render to RGBA pixel buffer (implemented after MemeRenderer include)
        std::vector<unsigned char> render() const;
        std::vector<unsigned char> render(int& outW, int& outH) const;

        // Save to file
        bool save(const std::string& outputPath) const;

        // Create resized copy
        std::shared_ptr<MacMeme> resize(int w, int h) const {
            auto copy = std::make_shared<MacMeme>(templateName, topText, bottomText, centerText);
            copy->imagePath = imagePath;
            copy->width = w;
            copy->height = h;
            copy->style = style;
            return copy;
        }

        std::string toString() const {
            std::ostringstream out;
            int w = 30;
            std::string border(w, '-');
            out << "+" << border << "+\n";
            out << "| " << std::left << std::setw(w - 2) << templateName << " |\n";
            out << "+" << border << "+\n";
            out << "| " << std::left << std::setw(w - 2) << topText << " |\n";
            if (!centerText.empty()) {
                out << "| " << std::left << std::setw(w - 2) << centerText << " |\n";
            }
            out << "|" << std::string(w, ' ') << "|\n";
            out << "| " << std::left << std::setw(w - 2) << bottomText << " |\n";
            out << "+" << border << "+";
            return out.str();
        }

        std::shared_ptr<MacMeme> remix(const std::string& newTop, const std::string& newBottom) const {
            auto copy = std::make_shared<MacMeme>(templateName, newTop, newBottom, centerText);
            copy->imagePath = imagePath;
            copy->width = width;
            copy->height = height;
            copy->style = style;
            return copy;
        }
    };

} // namespace meme

#include "MemeRenderer.h"       // meme::MemeRenderer (render/save implementations)

inline std::vector<unsigned char> meme::MacMeme::render() const {
    int w, h;
    return meme::MemeRenderer::renderWithPositions(imagePath, topText, bottomText, centerText, width, height, w, h, style, positionedTexts);
}

inline std::vector<unsigned char> meme::MacMeme::render(int& outW, int& outH) const {
    return meme::MemeRenderer::renderWithPositions(imagePath, topText, bottomText, centerText, width, height, outW, outH, style, positionedTexts);
}

inline bool meme::MacMeme::save(const std::string& outputPath) const {
    int w, h;
    auto pixels = render(w, h);
    return meme::MemeRenderer::saveImage(pixels, w, h, outputPath);
}

#endif // MAC_MEME_H
