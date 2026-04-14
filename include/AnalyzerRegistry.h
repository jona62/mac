#ifndef ANALYZER_REGISTRY_H
#define ANALYZER_REGISTRY_H

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "AnalyzerTypes.h"
#include "Token.h"

namespace analyzer {

    // Populates the analyzer's scope, native names, properties, and signatures.
    // Called by the MacAnalyzer constructor.

    using DefFn = std::function<void(const token::Token&, const std::string&,
                                     const std::string&, const std::string&)>;

    inline void registerNatives(
        std::unordered_set<std::string>& nativeNames,
        AnalysisResult& result,
        DefFn define
    ) {
        auto reg = [&](const std::string& name, const std::string& desc,
                        const std::string& type = "fun(1)") {
            nativeNames.insert(name);
            token::Token tok(token::TokenType::IDENTIFIER, token::TokenValue(name), 0);
            define(tok, "native", type, desc);
        };
        auto sig = [&](const std::string& name, std::vector<std::string> params,
                        const std::string& ret, const std::string& desc = "") {
            result.signatures.push_back({name, ret, desc, std::move(params)});
        };

        // Signatures for param hints + signature help
        sig("blur",{"radius"},"Meme -> Meme"); sig("pixelate",{"blockSize"},"Meme -> Meme");
        sig("noise",{"amount"},"Meme -> Meme"); sig("saturate",{"factor"},"Meme -> Meme");
        sig("contrast",{"factor"},"Meme -> Meme"); sig("brightness",{"factor"},"Meme -> Meme");
        sig("jpeg",{"quality"},"Meme -> Meme");
        sig("map",{"array","fn"},"[T]"); sig("filter",{"array","fn"},"[T]");
        sig("reduce",{"array","fn","initial"},"T"); sig("find",{"array","fn"},"T");
        sig("zip",{"array1","array2"},"[(A, B)]");
        sig("take",{"array","n"},"[T]"); sig("drop",{"array","n"},"[T]");
        sig("join",{"array","separator"},"string");
        sig("replace",{"str","from","to"},"string"); sig("substr",{"str","start","length"},"string");
        sig("split",{"str","delimiter"},"[string]");
        sig("pad",{"meme","pixels"},"Meme"); sig("border",{"meme","pixels"},"Meme");
        sig("beside",{"meme1","meme2"},"Meme"); sig("stack",{"meme1","meme2"},"Meme");
        sig("animate",{"memes","duration"},"Gif"); sig("toGrid",{"memes","cols","rows"},"Meme");
        sig("save",{"target","path"},"bool"); sig("pow",{"base","exponent"},"number");
        sig("len",{"value"},"number"); sig("type",{"value"},"string");

        // Native function definitions
        reg("clock","Current time."); reg("len","Length of string/array.");
        reg("substr","Substring."); reg("split","Split string."); reg("type","Type name.");
        reg("sqrt","Square root."); reg("abs","Absolute value.");
        reg("pow","Exponentiation."); reg("floor","Round down."); reg("ceil","Round up.");
        reg("push","Append to array."); reg("pop","Remove last.");
        reg("map","Map function over array."); reg("filter","Filter by predicate.");
        reg("input","Read input line.");
        reg("range","Generate number array."); reg("reduce","Fold with accumulator.");
        reg("zip","Pair two arrays."); reg("enumerate","Pair with indices.");
        reg("each","Side effects."); reg("flatten","Flatten one level.");
        reg("flatMap","Map then flatten."); reg("sort","Sort."); reg("reverse","Reverse.");
        reg("find","First match."); reg("any","Any match?"); reg("all","All match?");
        reg("take","First n."); reg("drop","Skip first n."); reg("join","Join to string.");
        reg("upper","Uppercase."); reg("lower","Lowercase.");
        reg("trim","Strip whitespace."); reg("replace","Replace all.");
        reg("animate","Memes to GIF."); reg("toGrid","Memes to grid."); reg("save","Save to file.");
        // Effects
        for (auto& n : {"blur","pixelate","noise","saturate","contrast","brightness","jpeg",
                        "invert","sepia","sharpen","vignette"})
            reg(n, std::string(n) + ". Meme -> Meme.", "Meme -> Meme");
        // Layout + timeline internals
        reg("beside","Side-by-side."); reg("stack","Vertical stack.");
        reg("grid","Grid layout."); reg("pad","Padding."); reg("border","Border.");
        reg("timeline","Internal.");
        for (auto& n : {"_resolve_template","_meme_save","_gif_save","_save_rendered",
                        "_apply_effect","_compose_layout","_add_padding","_add_border",
                        "_timeline_keyframe","_timeline_transition","_timeline_hold",
                        "_timeline_loop","_timeline_render"})
            reg(n, "Internal.");
    }

    inline void registerPreludeTypes(
        std::unordered_set<std::string>& nativeNames,
        DefFn define
    ) {
        auto reg = [&](const std::string& name, const std::string& kind,
                        const std::string& type, const std::string& desc) {
            nativeNames.insert(name);
            token::Token tok(token::TokenType::IDENTIFIER, token::TokenValue(name), 0);
            define(tok, kind, type, desc);
        };
        reg("Size","class","class Size","Pixel dimensions.");
        reg("Duration","class","class Duration","Time in milliseconds.");
        reg("Position","class","class Position","Text position.");
        reg("Format","class","class Format","Output format.");
        reg("Template","class","class Template","Meme template image.");
        reg("Meme","class","class Meme","Meme builder.");
        reg("Frame","class","class Frame","Animation frame.");
        reg("Gif","class","class Gif","GIF builder.");
        reg("Timeline","class","class Timeline","Animation timeline.");
        reg("Top","variable","Position","Top position.");
        reg("Bottom","variable","Position","Bottom position.");
        reg("Center","variable","Position","Center position.");
        reg("PNG","variable","Format","PNG format.");
        reg("JPG","variable","Format","JPG format.");
        reg("GIF","variable","Format","GIF format.");
        reg("deepfry","variable","Meme -> Meme","Composed effect preset.");
        for (auto& n : {"crossfade","slideLeft","slideRight","slideUp","slideDown","wipe"})
            reg(n, "variable", "string", "Transition type.");
    }

    inline void registerProperties(
        std::unordered_map<std::string, PropInfo>& props
    ) {
        auto p = [&](const std::string& owner, const std::string& name,
                      const std::string& kind, const std::string& desc) {
            props[owner + "." + name] = {owner, kind, desc};
        };
        p("Meme","text","method",".text(position, str) — Add text. Returns Meme. Chainable.");
        p("Meme","save","method",".save(format, path) — Save meme to file.");
        p("Meme","resize","method",".resize(size) — Returns resized Meme.");
        p("Meme","_top","field","Top text string.");
        p("Meme","_bottom","field","Bottom text string.");
        p("Meme","_template","field","Template used by this meme.");
        p("Gif","frame","method",".frame(meme, duration) — Add frame. Returns Gif. Chainable.");
        p("Gif","save","method",".save(path) — Render and save as animated GIF.");
        p("Timeline","frame","method",".frame(meme, duration) — Add keyframe. Returns Timeline. Chainable.");
        p("Timeline","transition","method",".transition(type, duration) — Set transition. Chainable.");
        p("Timeline","loop","method",".loop(count) — Set loop count (0 = infinite). Chainable.");
        p("Timeline","render","method",".render(path) — Render to animated GIF.");
        p("Timeline","save","method",".save(path) — Alias for render().");
        p("Template","name","field","Template name or path.");
        p("Template","path","field","Resolved file path.");
        p("Size","width","field","Width in pixels.");
        p("Size","height","field","Height in pixels.");
        p("Duration","ms","field","Duration in milliseconds.");
        p("Frame","meme","field","The Meme for this frame.");
        p("Frame","duration","field","The Duration for this frame.");
    }

} // namespace analyzer

#endif // ANALYZER_REGISTRY_H
