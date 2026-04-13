#ifndef MAC_MEME_H
#define MAC_MEME_H

#include <iomanip>
#include <memory>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace meme {

    // Forward declare MemeRenderer -- included only in method bodies below
    class MemeRenderer;

    class MacMeme {
    public:
        std::string templateName;
        std::string topText;
        std::string bottomText;
        std::string imagePath;  // resolved path to template image
        int width = 0;          // 0 = use template's native size
        int height = 0;

        MacMeme(const std::string& tmpl, const std::string& top, const std::string& bottom)
            : templateName(tmpl), topText(top), bottomText(bottom) {}

        // Known template map + custom templates
        static std::unordered_map<std::string, std::string>& templateMap() {
            static std::unordered_map<std::string, std::string> map = {
                {"drake",          "assets/templates/drake.png"},
                {"distracted",     "assets/templates/distracted.png"},
                {"change_my_mind", "assets/templates/change_my_mind.png"},
                {"blank",          "assets/templates/blank.png"}
            };
            return map;
        }

        // Resolve a template name to an image file path
        static std::string resolveTemplate(const std::string& name) {
            auto& map = templateMap();
            auto it = map.find(name);
            if (it != map.end()) return it->second;
            // Treat as a direct file path
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
            auto copy = std::make_shared<MacMeme>(templateName, topText, bottomText);
            copy->imagePath = imagePath;
            copy->width = w;
            copy->height = h;
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
            out << "|" << std::string(w, ' ') << "|\n";
            out << "| " << std::left << std::setw(w - 2) << bottomText << " |\n";
            out << "+" << border << "+";
            return out.str();
        }

        std::shared_ptr<MacMeme> remix(const std::string& newTop, const std::string& newBottom) const {
            auto copy = std::make_shared<MacMeme>(templateName, newTop, newBottom);
            copy->imagePath = imagePath;
            copy->width = width;
            copy->height = height;
            return copy;
        }
    };

} // namespace meme

// Include MemeRenderer so the render/save implementations work
#include "MemeRenderer.h"

inline std::vector<unsigned char> meme::MacMeme::render() const {
    return meme::MemeRenderer::render(imagePath, topText, bottomText, width, height);
}

inline std::vector<unsigned char> meme::MacMeme::render(int& outW, int& outH) const {
    return meme::MemeRenderer::render(imagePath, topText, bottomText, width, height, outW, outH);
}

inline bool meme::MacMeme::save(const std::string& outputPath) const {
    int w, h;
    auto pixels = render(w, h);
    return meme::MemeRenderer::saveImage(pixels, w, h, outputPath);
}

#endif // MAC_MEME_H
