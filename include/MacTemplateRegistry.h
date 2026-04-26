#ifndef MAC_TEMPLATE_REGISTRY_H
#define MAC_TEMPLATE_REGISTRY_H

#include <string>
#include <vector>

namespace mac_catalog {

    struct TemplateDefinition {
        std::string id;
        std::string name;
        std::string category;
        std::string description;
        std::string bestFor;
        std::string assetPath;
    };

    inline const std::vector<TemplateDefinition>& builtInTemplates() {
        static const std::vector<TemplateDefinition> templates = {
            {"two_panel", "Two Panel", "canvas",
                "Classic split-screen for before-and-after or expectation-vs-reality.",
                "Pairing two ideas with a fast payoff.",
                "assets/templates/two_panel.png"},
            {"three_panel", "Three Panel", "canvas",
                "Three beats with escalating energy.",
                "Setups that need a beginning, middle, and spike.",
                "assets/templates/three_panel.png"},
            {"bottom_text", "Bottom Text", "canvas",
                "Poster-style with a heavy caption block.",
                "One-liners, announcements, and dramatic reveals.",
                "assets/templates/bottom_text.png"},
            {"blank", "Blank", "canvas",
                "Plain white canvas.",
                "Simple text-led stills and punchy cut-ins.",
                "assets/templates/blank.png"},
            {"dark", "Dark", "canvas",
                "Dark background for neon and high-contrast text.",
                "Night-mode cards, neon captions, dramatic overlays.",
                "assets/templates/dark.png"},
            {"wide", "Wide (16:9)", "canvas",
                "Landscape format for thumbnails and banners.",
                "Video covers and wide-format studio cards.",
                "assets/templates/wide.png"},
            {"tall", "Tall (9:16)", "canvas",
                "Portrait format for stories and reels.",
                "Storyboards, vertical promos, and mobile-first ideas.",
                "assets/templates/tall.png"},
            {"square", "Square (1:1)", "canvas",
                "Square format for social posts.",
                "Feed cards and balanced editorial compositions.",
                "assets/templates/square.png"},
            {"four_panel", "Four Panel", "canvas",
                "2x2 grid with dividers.",
                "Comparison sets and multi-beat punchlines.",
                "assets/templates/four_panel.png"},
            {"caption_bar", "Caption Bar", "canvas",
                "Image-heavy composition with a clean caption zone.",
                "Poster-like cards and tidy social callouts.",
                "assets/templates/caption_bar.png"},
        };
        return templates;
    }

} // namespace mac_catalog

#endif // MAC_TEMPLATE_REGISTRY_H
