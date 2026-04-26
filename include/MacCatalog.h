#ifndef MAC_CATALOG_H
#define MAC_CATALOG_H

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "GifLimits.h"
#include "MacClass.h"
#include "MacTemplateRegistry.h"
#include "NativeRegistry.h"
#include "nlohmann/json.hpp"

#ifndef MAC_VERSION
#define MAC_VERSION "unknown"
#endif

namespace mac_catalog {

    using json = nlohmann::json;

    struct TemplateEntry {
        std::string id;
        std::string name;
        std::string category;
        std::string description;
        std::string bestFor;
        std::string source;
        std::string assetCategory;
        std::string assetPath;
    };

    inline std::string lower(std::string value) {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
            return static_cast<char>(std::tolower(c));
        });
        return value;
    }

    inline std::string titleizeStem(std::string value) {
        std::replace(value.begin(), value.end(), '_', ' ');
        bool nextUpper = true;
        for (auto& ch : value) {
            unsigned char c = static_cast<unsigned char>(ch);
            if (std::isspace(c) || ch == '-') {
                nextUpper = true;
                continue;
            }
            ch = static_cast<char>(nextUpper ? std::toupper(c) : std::tolower(c));
            nextUpper = false;
        }
        return value;
    }

    inline bool isAllowedImageExt(const std::filesystem::path& path) {
        static const std::unordered_set<std::string> exts = {
            ".jpg", ".jpeg", ".png", ".gif",
        };
        return exts.count(lower(path.extension().string())) > 0;
    }

    inline json templateToJson(const TemplateEntry& entry) {
        json item = {
            {"id", entry.id},
            {"name", entry.name},
            {"category", entry.category},
            {"description", entry.description},
            {"bestFor", entry.bestFor},
            {"source", entry.source},
            {"assetPath", entry.assetPath},
        };
        if (!entry.assetCategory.empty()) {
            item["assetCategory"] = entry.assetCategory;
        }
        return item;
    }

    inline std::vector<TemplateEntry> assetTemplateEntries(const std::string& binaryDir) {
        std::vector<TemplateEntry> out;
        auto root = std::filesystem::path(binaryDir) / "assets" / "templates";
        if (!std::filesystem::is_directory(root)) return out;

        std::vector<std::filesystem::path> categories;
        for (const auto& entry : std::filesystem::directory_iterator(root)) {
            if (entry.is_directory()) categories.push_back(entry.path());
        }
        std::sort(categories.begin(), categories.end());

        for (const auto& categoryPath : categories) {
            auto category = categoryPath.filename().string();
            std::vector<std::filesystem::path> images;
            for (const auto& entry : std::filesystem::directory_iterator(categoryPath)) {
                if (entry.is_regular_file() && isAllowedImageExt(entry.path())) {
                    images.push_back(entry.path());
                }
            }
            std::sort(images.begin(), images.end());

            for (const auto& image : images) {
                auto stem = image.stem().string();
                auto filename = image.filename().string();
                out.push_back({
                    category + "." + stem,
                    titleizeStem(stem),
                    category,
                    "Meme template: " + stem,
                    "Reaction shots and recognizable meme beats.",
                    "asset",
                    category,
                    "assets/templates/" + category + "/" + filename,
                });
            }
        }

        return out;
    }

    inline std::vector<TemplateEntry> templateEntries(const std::string& binaryDir) {
        std::vector<TemplateEntry> out;
        for (const auto& tmpl : builtInTemplates()) {
            out.push_back({
                tmpl.id,
                tmpl.name,
                tmpl.category,
                tmpl.description,
                tmpl.bestFor,
                "builtin",
                "",
                tmpl.assetPath,
            });
        }
        auto assets = assetTemplateEntries(binaryDir);
        out.insert(out.end(), assets.begin(), assets.end());
        return out;
    }

    inline json templatesJson(const std::string& binaryDir) {
        json items = json::array();
        for (const auto& entry : templateEntries(binaryDir)) {
            items.push_back(templateToJson(entry));
        }
        return items;
    }

    inline json assetsJson(const std::string& binaryDir, const std::string& onlyCategory = "") {
        json grouped = json::object();
        for (const auto& entry : assetTemplateEntries(binaryDir)) {
            if (!onlyCategory.empty() && entry.assetCategory != onlyCategory) continue;
            if (!grouped.contains(entry.assetCategory)) grouped[entry.assetCategory] = json::array();
            grouped[entry.assetCategory].push_back(templateToJson(entry));
        }
        return grouped;
    }

    inline json effectsJson() {
        return json::array({
            {{"id", "none"}, {"name", "None"}, {"description", "Leave the scene clean."}, {"expression", nullptr}},
            {{"id", "sepia"}, {"name", "Sepia"}, {"description", "Warm sepia tone."}, {"expression", "sepia"}},
            {{"id", "vintage"}, {"name", "Vintage"}, {"description", "Sepia with reduced brightness."}, {"expression", "sepia >> brightness(0.9)"}},
            {{"id", "glitch"}, {"name", "Glitch"}, {"description", "Pixelation, contrast, and noise."}, {"expression", "pixelate(4) >> contrast(1.8) >> noise(0.2)"}},
            {{"id", "deepfry"}, {"name", "Deep Fry"}, {"description", "High saturation and compression chaos."}, {"expression", "saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1)"}},
            {{"id", "cyberpunk"}, {"name", "Cyberpunk"}, {"description", "Hue shift, chromatic aberration, glow."}, {"expression", "hueShift(180) >> contrast(1.5) >> chromatic(3) >> glow(4)"}},
            {{"id", "comic"}, {"name", "Comic"}, {"description", "Posterize, contrast, sharpen."}, {"expression", "posterize(5) >> contrast(1.4) >> sharpen"}},
            {{"id", "retro"}, {"name", "Retro"}, {"description", "Grayscale with texture and contrast."}, {"expression", "grayscale >> contrast(1.3) >> noise(0.1)"}},
            {{"id", "grayscale"}, {"name", "Grayscale"}, {"description", "Convert to grayscale."}, {"expression", "grayscale"}},
            {{"id", "invert"}, {"name", "Invert"}, {"description", "Invert all colors."}, {"expression", "invert"}},
            {{"id", "vignette"}, {"name", "Vignette"}, {"description", "Darken the outer edges."}, {"expression", "vignette"}},
            {{"id", "sharpen"}, {"name", "Sharpen"}, {"description", "Sharpen the scene."}, {"expression", "sharpen"}},
        });
    }

    inline json effectDefinitionsJson() {
        return json::array({
            {{"id", "blur"}, {"name", "Blur"}, {"param", "radius"}, {"min", 1}, {"max", 20}, {"default", 5}, {"step", 1}},
            {{"id", "pixelate"}, {"name", "Pixelate"}, {"param", "blockSize"}, {"min", 1}, {"max", 32}, {"default", 4}, {"step", 1}},
            {{"id", "noise"}, {"name", "Noise"}, {"param", "amount"}, {"min", 0.01}, {"max", 1.0}, {"default", 0.2}, {"step", 0.05}},
            {{"id", "saturate"}, {"name", "Saturate"}, {"param", "factor"}, {"min", 0.0}, {"max", 5.0}, {"default", 1.5}, {"step", 0.1}},
            {{"id", "contrast"}, {"name", "Contrast"}, {"param", "factor"}, {"min", 0.1}, {"max", 3.0}, {"default", 1.5}, {"step", 0.1}},
            {{"id", "brightness"}, {"name", "Brightness"}, {"param", "factor"}, {"min", 0.1}, {"max", 3.0}, {"default", 1.0}, {"step", 0.1}},
            {{"id", "jpeg"}, {"name", "JPEG Artifacts"}, {"param", "quality"}, {"min", 1}, {"max", 100}, {"default", 10}, {"step", 1}},
            {{"id", "hueShift"}, {"name", "Hue Shift"}, {"param", "degrees"}, {"min", 0}, {"max", 360}, {"default", 90}, {"step", 10}},
            {{"id", "glow"}, {"name", "Glow"}, {"param", "radius"}, {"min", 1}, {"max", 20}, {"default", 4}, {"step", 1}},
            {{"id", "posterize"}, {"name", "Posterize"}, {"param", "levels"}, {"min", 2}, {"max", 16}, {"default", 5}, {"step", 1}},
            {{"id", "chromatic"}, {"name", "Chromatic"}, {"param", "offset"}, {"min", 1}, {"max", 20}, {"default", 3}, {"step", 1}},
            {{"id", "threshold"}, {"name", "Threshold"}, {"param", "level"}, {"min", 0}, {"max", 255}, {"default", 128}, {"step", 1}},
            {{"id", "sepia"}, {"name", "Sepia"}, {"param", nullptr}},
            {{"id", "invert"}, {"name", "Invert"}, {"param", nullptr}},
            {{"id", "sharpen"}, {"name", "Sharpen"}, {"param", nullptr}},
            {{"id", "vignette"}, {"name", "Vignette"}, {"param", nullptr}},
            {{"id", "grayscale"}, {"name", "Grayscale"}, {"param", nullptr}},
        });
    }

    inline json layoutsJson() {
        return json::array({
            {{"id", "single"}, {"name", "Single"}, {"slotCount", 1}, {"description", "One full-canvas scene."}},
            {{"id", "beside"}, {"name", "Beside"}, {"slotCount", 2}, {"description", "Two slots side by side."}},
            {{"id", "stack"}, {"name", "Stack"}, {"slotCount", 2}, {"description", "Two slots stacked vertically."}},
            {{"id", "grid2x2"}, {"name", "Grid 2x2"}, {"slotCount", 4}, {"description", "Four slots in a 2x2 comparison grid."}},
        });
    }

    inline json stylePresetsJson() {
        return json::array({
            {{"id", "cinematic"}, {"name", "Cinematic"}, {"description", "Soft white type with a restrained shadow."},
                {"style", {{"color", "#FFFFFF"}, {"outline", 2}, {"outlineColor", "#111111"}, {"shadow", 4}, {"shadowColor", "#00000088"}, {"fontSize", "sm"}}}},
            {{"id", "panic"}, {"name", "Panic"}, {"description", "Red alert styling with heavier edges."},
                {"style", {{"color", "#FF0000"}, {"outline", 5}, {"outlineColor", "#440000"}, {"shadow", 3}, {"shadowColor", "#00000099"}, {"fontSize", "sm"}}}},
            {{"id", "chill"}, {"name", "Chill"}, {"description", "Terminal green with crisp outline."},
                {"style", {{"color", "#00FF41"}, {"outline", 3}, {"outlineColor", "#003300"}, {"fontSize", "sm"}}}},
            {{"id", "shout"}, {"name", "Shout"}, {"description", "Loud white all-caps energy."},
                {"style", {{"color", "#FFFFFF"}, {"outline", 4}, {"outlineColor", "#000000"}, {"shadow", 3}, {"shadowColor", "#00000099"}, {"fontSize", "sm"}}}},
            {{"id", "whisper"}, {"name", "Whisper"}, {"description", "Muted grey with a fine outline."},
                {"style", {{"color", "#CCCCCC"}, {"outline", 1}, {"outlineColor", "#333333"}, {"fontSize", "sm"}}}},
        });
    }

    inline json limitsJson() {
        return {
            {"width", {{"min", 240}, {"max", 1200}}},
            {"height", {{"min", 240}, {"max", 1200}}},
            {"duration", {{"min", 80}, {"max", 2500}}},
            {"padding", {{"min", 0}, {"max", 60}}},
            {"border", {{"min", 0}, {"max", 24}}},
            {"outline", {{"min", 0}, {"max", 10}}},
            {"shadow", {{"min", 0}, {"max", 10}}},
            {"fontSizePx", {{"min", 8}, {"max", 240}}},
            {"maxScenes", 24},
            {"maxGifFrames", meme::MAX_GIF_FRAMES},
        };
    }

    inline json idList(const json& items) {
        json out = json::array();
        for (const auto& item : items) out.push_back(item.at("id"));
        return out;
    }

    inline json nativeNamesJson() {
        json out = json::array();
        for (const auto& native : native_registry::all()) {
            if (native.visibility == native_registry::NativeVisibility::Public) {
                out.push_back(native.name);
            }
        }
        return out;
    }

    inline json allowedNamesJson(const std::string& binaryDir) {
        json assets = json::object();
        auto assetGroups = assetsJson(binaryDir);
        for (auto it = assetGroups.begin(); it != assetGroups.end(); ++it) {
            assets[it.key()] = idList(it.value());
        }
        return {
            {"templates", idList(templatesJson(binaryDir))},
            {"assets", assets},
            {"effects", idList(effectsJson())},
            {"effect_definitions", idList(effectDefinitionsJson())},
            {"layouts", idList(layoutsJson())},
            {"style_presets", idList(stylePresetsJson())},
            {"natives", nativeNamesJson()},
        };
    }

    inline uint64_t fnv1a(const std::string& text) {
        uint64_t hash = 1469598103934665603ULL;
        for (unsigned char c : text) {
            hash ^= c;
            hash *= 1099511628211ULL;
        }
        return hash;
    }

    inline std::string hex64(uint64_t value) {
        std::ostringstream ss;
        ss << std::hex << std::setw(16) << std::setfill('0') << value;
        return ss.str();
    }

    inline json assetFingerprintStats(const std::string& binaryDir) {
        json stats = json::array();
        auto root = std::filesystem::path(binaryDir) / "assets" / "templates";
        if (!std::filesystem::is_directory(root)) return stats;

        std::vector<std::filesystem::path> images;
        for (const auto& dir : std::filesystem::directory_iterator(root)) {
            if (!dir.is_directory()) continue;
            for (const auto& entry : std::filesystem::directory_iterator(dir.path())) {
                if (entry.is_regular_file() && isAllowedImageExt(entry.path())) {
                    images.push_back(entry.path());
                }
            }
        }
        std::sort(images.begin(), images.end());

        for (const auto& image : images) {
            std::error_code relEc;
            std::error_code timeEc;
            std::error_code sizeEc;
            auto rel = std::filesystem::relative(image, std::filesystem::path(binaryDir), relEc);
            auto mtime = std::filesystem::last_write_time(image, timeEc);
            auto size = std::filesystem::file_size(image, sizeEc);
            stats.push_back({
                {"path", relEc ? image.string() : rel.generic_string()},
                {"size", sizeEc ? 0 : size},
                {"mtime", timeEc ? 0 : mtime.time_since_epoch().count()},
            });
        }
        return stats;
    }

    inline std::string catalogFingerprint(const std::string& binaryDir) {
        json payload = {
            {"mac_version", MAC_VERSION},
            {"templates", templatesJson(binaryDir)},
            {"effects", effectsJson()},
            {"effect_definitions", effectDefinitionsJson()},
            {"layouts", layoutsJson()},
            {"style_presets", stylePresetsJson()},
            {"limits", limitsJson()},
            {"allowed_names", allowedNamesJson(binaryDir)},
            {"asset_stats", assetFingerprintStats(binaryDir)},
        };
        return hex64(fnv1a(payload.dump()));
    }

    inline std::vector<std::string> splitSelectors(const std::string& selectorText) {
        std::vector<std::string> out;
        std::stringstream ss(selectorText);
        std::string item;
        while (std::getline(ss, item, ',')) {
            item.erase(item.begin(), std::find_if(item.begin(), item.end(), [](unsigned char c) {
                return !std::isspace(c);
            }));
            item.erase(std::find_if(item.rbegin(), item.rend(), [](unsigned char c) {
                return !std::isspace(c);
            }).base(), item.end());
            if (!item.empty()) out.push_back(item);
        }
        return out;
    }

    inline std::vector<std::string> assetCategories(const std::string& binaryDir) {
        std::set<std::string> categories;
        for (const auto& entry : assetTemplateEntries(binaryDir)) {
            categories.insert(entry.assetCategory);
        }
        return {categories.begin(), categories.end()};
    }

    inline std::vector<std::string> availableSelectors(const std::string& binaryDir) {
        std::vector<std::string> selectors = {
            "templates",
            "assets",
            "effects",
            "effect_definitions",
            "layouts",
            "style_presets",
            "limits",
            "allowed_names",
        };
        for (const auto& category : assetCategories(binaryDir)) {
            selectors.push_back("assets:" + category);
        }
        return selectors;
    }

    inline bool includeTopLevel(json& out, const std::string& binaryDir, const std::string& selector) {
        if (selector == "templates") out["templates"] = templatesJson(binaryDir);
        else if (selector == "assets") out["assets"] = assetsJson(binaryDir);
        else if (selector == "effects") out["effects"] = effectsJson();
        else if (selector == "effect_definitions") out["effect_definitions"] = effectDefinitionsJson();
        else if (selector == "layouts") out["layouts"] = layoutsJson();
        else if (selector == "style_presets") out["style_presets"] = stylePresetsJson();
        else if (selector == "limits") out["limits"] = limitsJson();
        else if (selector == "allowed_names") out["allowed_names"] = allowedNamesJson(binaryDir);
        else return false;
        return true;
    }

    inline json catalogJson(const std::string& binaryDir, const std::string& selectorText = "") {
        auto fullSelectors = std::vector<std::string>{
            "templates",
            "assets",
            "effects",
            "effect_definitions",
            "layouts",
            "style_presets",
            "limits",
            "allowed_names",
        };
        auto selectors = selectorText.empty() || selectorText == "all"
            ? fullSelectors
            : splitSelectors(selectorText);

        json out = {
            {"schema_version", 1},
            {"mac_version", MAC_VERSION},
            {"catalog_fingerprint", catalogFingerprint(binaryDir)},
            {"included", json::array()},
        };

        auto available = availableSelectors(binaryDir);
        std::unordered_set<std::string> availableSet(available.begin(), available.end());
        std::unordered_set<std::string> seen;
        for (const auto& selector : selectors) {
            if (!availableSet.count(selector)) {
                return {
                    {"ok", false},
                    {"error", "unknown_catalog_selector"},
                    {"selector", selector},
                    {"available", available},
                };
            }
            if (seen.count(selector)) continue;
            seen.insert(selector);
            out["included"].push_back(selector);

            if (selector.rfind("assets:", 0) == 0) {
                auto category = selector.substr(std::string("assets:").size());
                if (!out.contains("assets")) out["assets"] = json::object();
                out["assets"][category] = assetsJson(binaryDir, category).value(category, json::array());
            } else {
                includeTopLevel(out, binaryDir, selector);
            }
        }

        return out;
    }

    inline int printCatalog(const std::string& binaryDir, const std::string& selectorText = "") {
        auto result = catalogJson(binaryDir, selectorText);
        std::cout << result.dump() << std::endl;
        return result.value("ok", true) ? 0 : 2;
    }

} // namespace mac_catalog

#endif // MAC_CATALOG_H
