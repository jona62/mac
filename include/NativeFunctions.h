#ifndef NATIVE_FUNCTIONS_H
#define NATIVE_FUNCTIONS_H

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "MacCallable.h"
#include "MacArray.h"
#include "MacMap.h"
#include "MacMeme.h"
#include "MacGif.h"
#include "MacTimeline.h"
#include "MemeEffects.h"
#include "MemeLayout.h"
#include "MacInstance.h"

namespace callable {

    // --- Time ---

    class ClockFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue>) override {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000.0;
        }
        int arity() override { return 0; }
        std::string toString() override { return "<native fn>"; }
    };

    // --- String functions ---

    class LenFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            if (std::holds_alternative<std::string>(args[0])) {
                return static_cast<double>(std::get<std::string>(args[0]).size());
            }
            if (std::holds_alternative<std::shared_ptr<collection::MacArray>>(args[0])) {
                return static_cast<double>(std::get<std::shared_ptr<collection::MacArray>>(args[0])->elements.size());
            }
            throw std::runtime_error("len() expects a string or array.");
        }
        int arity() override { return 1; }
        std::string toString() override { return "<native fn>"; }
    };

    class SubstrFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto str = std::get<std::string>(args[0]);
            int start = static_cast<int>(std::get<double>(args[1]));
            int len = static_cast<int>(std::get<double>(args[2]));
            return str.substr(start, len);
        }
        int arity() override { return 3; }
        std::string toString() override { return "<native fn>"; }
    };

    class SplitFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto str = std::get<std::string>(args[0]);
            auto delim = std::get<std::string>(args[1]);
            auto arr = std::make_shared<collection::MacArray>();
            size_t pos = 0;
            while ((pos = str.find(delim)) != std::string::npos) {
                arr->elements.push_back(value::MacValue(str.substr(0, pos)));
                str.erase(0, pos + delim.length());
            }
            arr->elements.push_back(value::MacValue(str));
            return value::MacValue(arr);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    class TypeFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto& val = args[0];
            if (std::holds_alternative<std::string>(val)) return std::string("string");
            if (std::holds_alternative<double>(val)) return std::string("number");
            if (std::holds_alternative<bool>(val)) return std::string("bool");
            if (std::holds_alternative<std::monostate>(val)) return std::string("nil");
            if (std::holds_alternative<std::shared_ptr<MacCallable>>(val)) return std::string("function");
            if (std::holds_alternative<std::shared_ptr<instance::MacInstance>>(val)) return std::string("instance");
            if (std::holds_alternative<std::shared_ptr<collection::MacArray>>(val)) return std::string("array");
            if (std::holds_alternative<std::shared_ptr<collection::MacMap>>(val)) return std::string("map");
            if (std::holds_alternative<std::shared_ptr<meme::MacMeme>>(val)) return std::string("meme");
            if (std::holds_alternative<std::shared_ptr<meme::MacGif>>(val)) return std::string("gif");
            if (std::holds_alternative<std::shared_ptr<meme::MacTimeline>>(val)) return std::string("timeline");
            return std::string("unknown");
        }
        int arity() override { return 1; }
        std::string toString() override { return "<native fn>"; }
    };

    // --- Math functions ---

    class SqrtFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            return std::sqrt(std::get<double>(args[0]));
        }
        int arity() override { return 1; }
        std::string toString() override { return "<native fn>"; }
    };

    class AbsFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            return std::abs(std::get<double>(args[0]));
        }
        int arity() override { return 1; }
        std::string toString() override { return "<native fn>"; }
    };

    class PowFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            return std::pow(std::get<double>(args[0]), std::get<double>(args[1]));
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    class FloorFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            return std::floor(std::get<double>(args[0]));
        }
        int arity() override { return 1; }
        std::string toString() override { return "<native fn>"; }
    };

    class CeilFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            return std::ceil(std::get<double>(args[0]));
        }
        int arity() override { return 1; }
        std::string toString() override { return "<native fn>"; }
    };

    // --- Array functions ---

    class PushFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto arr = std::get<std::shared_ptr<collection::MacArray>>(args[0]);
            arr->elements.push_back(args[1]);
            return std::monostate{};
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    class PopFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto arr = std::get<std::shared_ptr<collection::MacArray>>(args[0]);
            if (arr->elements.empty()) return value::MacValue(std::monostate{});
            auto val = arr->elements.back();
            arr->elements.pop_back();
            return val;
        }
        int arity() override { return 1; }
        std::string toString() override { return "<native fn>"; }
    };

    class MapArrayFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter> interp,
                             std::vector<value::MacValue> args) override {
            auto arr = std::get<std::shared_ptr<collection::MacArray>>(args[0]);
            auto fn = std::get<std::shared_ptr<MacCallable>>(args[1]);
            auto result = std::make_shared<collection::MacArray>();
            for (auto& elem : arr->elements) {
                result->elements.push_back(fn->call(interp, {elem}));
            }
            return value::MacValue(result);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    class FilterFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter> interp,
                             std::vector<value::MacValue> args) override {
            auto arr = std::get<std::shared_ptr<collection::MacArray>>(args[0]);
            auto fn = std::get<std::shared_ptr<MacCallable>>(args[1]);
            auto result = std::make_shared<collection::MacArray>();
            for (auto& elem : arr->elements) {
                auto val = fn->call(interp, {elem});
                bool truthy = !std::holds_alternative<std::monostate>(val) &&
                              !(std::holds_alternative<bool>(val) && !std::get<bool>(val));
                if (truthy) result->elements.push_back(elem);
            }
            return value::MacValue(result);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    // --- I/O ---

    class InputFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            std::cout << std::get<std::string>(args[0]);
            std::string line;
            std::getline(std::cin, line);
            return line;
        }
        int arity() override { return 1; }
        std::string toString() override { return "<native fn>"; }
    };

    // --- Typed API native functions ---

    class ResolveTemplateFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto name = std::get<std::string>(args[0]);
            return meme::MacMeme::resolveTemplate(name);
        }
        int arity() override { return 1; }
        std::string toString() override { return "<native fn>"; }
    };

    class MemeRenderSaveFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto templatePath = std::get<std::string>(args[0]);
            auto topText = std::get<std::string>(args[1]);
            auto bottomText = std::get<std::string>(args[2]);
            int width = static_cast<int>(std::get<double>(args[3]));
            int height = static_cast<int>(std::get<double>(args[4]));
            auto outputPath = std::get<std::string>(args[5]);
            auto m = std::make_shared<meme::MacMeme>("", topText, bottomText);
            m->imagePath = templatePath;
            m->width = width;
            m->height = height;
            return m->save(outputPath);
        }
        int arity() override { return 6; }
        std::string toString() override { return "<native fn>"; }
    };

    class GifRenderSaveFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto framesArr = std::get<std::shared_ptr<collection::MacArray>>(args[0]);
            auto outputPath = std::get<std::string>(args[1]);
            auto gif = std::make_shared<meme::MacGif>();
            for (auto& frameVal : framesArr->elements) {
                auto frameMap = std::get<std::shared_ptr<collection::MacMap>>(frameVal);
                auto path = std::get<std::string>(frameMap->get("path"));
                auto top = std::get<std::string>(frameMap->get("top"));
                auto bottom = std::get<std::string>(frameMap->get("bottom"));
                int w = static_cast<int>(std::get<double>(frameMap->get("width")));
                int h = static_cast<int>(std::get<double>(frameMap->get("height")));
                int dur = static_cast<int>(std::get<double>(frameMap->get("duration")));
                auto m = std::make_shared<meme::MacMeme>("", top, bottom);
                m->imagePath = path;
                m->width = w;
                m->height = h;
                gif->addFrame(m, dur);
            }
            return gif->save(outputPath);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    // =======================================================================
    // Effect & Layout Helpers
    // =======================================================================

    // Helper: extract meme rendering data from a MacInstance (Meme class)
    // Returns the rendered pixel data + dimensions. If _rendered is set (temp path),
    // loads from that. Otherwise renders from template + text.
    struct MemePixelData {
        std::vector<unsigned char> pixels;
        int width;
        int height;
    };

    static MemePixelData getMemePixels(const value::MacValue& val) {
        // Handle MacMap (rendered effect result)
        if (std::holds_alternative<std::shared_ptr<collection::MacMap>>(val)) {
            auto map = std::get<std::shared_ptr<collection::MacMap>>(val);
            if (map->has("_rendered")) {
                auto path = std::get<std::string>(map->get("_rendered"));
                int w, h, c;
                unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
                if (!data) throw std::runtime_error("Cannot load rendered image: " + path);
                MemePixelData result;
                result.pixels.assign(data, data + w * h * 4);
                result.width = w;
                result.height = h;
                stbi_image_free(data);
                return result;
            }
            throw std::runtime_error("Map does not contain rendered image data.");
        }

        if (!std::holds_alternative<std::shared_ptr<instance::MacInstance>>(val)) {
            throw std::runtime_error("Expected a Meme instance.");
        }
        auto inst = std::get<std::shared_ptr<instance::MacInstance>>(val);

        // Check for _rendered field (temp file path)
        token::Token renderedTok(token::TokenType::IDENTIFIER, token::TokenValue(std::string("_rendered")), 0);
        value::MacValue renderedVal;
        try {
            renderedVal = inst->get(renderedTok);
        } catch (...) {
            renderedVal = std::monostate{};
        }

        if (std::holds_alternative<std::string>(renderedVal)) {
            auto path = std::get<std::string>(renderedVal);
            int w, h, c;
            unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
            if (!data) {
                throw std::runtime_error("Cannot load rendered image: " + path);
            }
            MemePixelData result;
            result.pixels.assign(data, data + w * h * 4);
            result.width = w;
            result.height = h;
            stbi_image_free(data);
            return result;
        }

        // Render from template
        token::Token tplTok(token::TokenType::IDENTIFIER, token::TokenValue(std::string("_template")), 0);
        auto tplVal = inst->get(tplTok);
        auto tplInst = std::get<std::shared_ptr<instance::MacInstance>>(tplVal);

        token::Token pathTok(token::TokenType::IDENTIFIER, token::TokenValue(std::string("path")), 0);
        auto templatePath = std::get<std::string>(tplInst->get(pathTok));

        token::Token topTok(token::TokenType::IDENTIFIER, token::TokenValue(std::string("_top")), 0);
        token::Token botTok(token::TokenType::IDENTIFIER, token::TokenValue(std::string("_bottom")), 0);
        token::Token wTok(token::TokenType::IDENTIFIER, token::TokenValue(std::string("_width")), 0);
        token::Token hTok(token::TokenType::IDENTIFIER, token::TokenValue(std::string("_height")), 0);

        auto topText = std::get<std::string>(inst->get(topTok));
        auto bottomText = std::get<std::string>(inst->get(botTok));
        int w = static_cast<int>(std::get<double>(inst->get(wTok)));
        int h = static_cast<int>(std::get<double>(inst->get(hTok)));

        int outW, outH;
        auto pixels = meme::MemeRenderer::render(templatePath, topText, bottomText, w, h, outW, outH);
        return {pixels, outW, outH};
    }

    // Helper: save pixels to a temp file and return the path
    static int tempCounter = 0;
    static std::string saveTempImage(const std::vector<unsigned char>& pixels, int w, int h) {
        std::string tmpDir = "/tmp/mac_effects";
        std::filesystem::create_directories(tmpDir);
        std::string path = tmpDir + "/effect_" + std::to_string(tempCounter++) + ".png";
        meme::MemeRenderer::saveImage(pixels, w, h, path);
        return path;
    }

    // Helper: create a new Meme-like MacInstance with _rendered set
    // We actually create a MacMap that carries the rendered state,
    // but the prelude wrapper will handle creating a proper Meme.
    // Instead, the native returns the temp path as a string, and
    // the prelude wraps it.

    // =======================================================================
    // Apply Effect Native Function
    // _apply_effect(renderedPath_or_nil, templatePath, top, bottom, w, h, effectName, param)
    // Returns: temp file path (string)
    // =======================================================================

    class ApplyEffectFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            // args: renderedPath_or_nil, templatePath, top, bottom, w, h, effectName, param
            std::string effectName = std::get<std::string>(args[6]);
            double param = 0;
            if (args.size() > 7 && std::holds_alternative<double>(args[7])) {
                param = std::get<double>(args[7]);
            }

            // Load pixels
            std::vector<unsigned char> pixels;
            int w, h;

            if (std::holds_alternative<std::string>(args[0])) {
                // Load from rendered temp file
                auto path = std::get<std::string>(args[0]);
                int c;
                unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
                if (!data) throw std::runtime_error("Cannot load rendered image: " + path);
                pixels.assign(data, data + w * h * 4);
                stbi_image_free(data);
            } else {
                // Render from template
                auto templatePath = std::get<std::string>(args[1]);
                auto top = std::get<std::string>(args[2]);
                auto bottom = std::get<std::string>(args[3]);
                int tw = static_cast<int>(std::get<double>(args[4]));
                int th = static_cast<int>(std::get<double>(args[5]));
                pixels = meme::MemeRenderer::render(templatePath, top, bottom, tw, th, w, h);
            }

            // Apply named effect
            if (effectName == "blur") {
                effects::blur(pixels.data(), w, h, static_cast<int>(param));
            } else if (effectName == "pixelate") {
                effects::pixelate(pixels.data(), w, h, static_cast<int>(param));
            } else if (effectName == "noise") {
                effects::noise(pixels.data(), w, h, static_cast<float>(param));
            } else if (effectName == "saturate") {
                effects::saturate(pixels.data(), w, h, static_cast<float>(param));
            } else if (effectName == "contrast") {
                effects::contrast(pixels.data(), w, h, static_cast<float>(param));
            } else if (effectName == "brightness") {
                effects::brightness(pixels.data(), w, h, static_cast<float>(param));
            } else if (effectName == "jpeg") {
                effects::jpegQuality(pixels.data(), w, h, static_cast<int>(param));
            } else if (effectName == "invert") {
                effects::invertColors(pixels.data(), w, h);
            } else if (effectName == "sepia") {
                effects::sepia(pixels.data(), w, h);
            } else if (effectName == "sharpen") {
                effects::sharpen(pixels.data(), w, h);
            } else if (effectName == "vignette") {
                effects::vignette(pixels.data(), w, h);
            } else {
                throw std::runtime_error("Unknown effect: " + effectName);
            }

            return saveTempImage(pixels, w, h);
        }
        int arity() override { return 8; }
        std::string toString() override { return "<native fn>"; }
    };

    // =======================================================================
    // Compose Layout Native Function
    // _compose_layout(layoutName, rendered1_or_nil, tpl1, top1, bot1, w1, h1,
    //                              rendered2_or_nil, tpl2, top2, bot2, w2, h2, extraParam)
    // Returns: temp file path (string)
    // =======================================================================

    class ComposeLayoutFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto layoutName = std::get<std::string>(args[0]);

            // Load first image
            auto loadImage = [](const std::vector<value::MacValue>& a, int offset) -> MemePixelData {
                std::vector<unsigned char> pixels;
                int w, h;
                if (std::holds_alternative<std::string>(a[offset])) {
                    auto path = std::get<std::string>(a[offset]);
                    int c;
                    unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
                    if (!data) throw std::runtime_error("Cannot load image: " + path);
                    pixels.assign(data, data + w * h * 4);
                    stbi_image_free(data);
                } else {
                    auto templatePath = std::get<std::string>(a[offset + 1]);
                    auto top = std::get<std::string>(a[offset + 2]);
                    auto bottom = std::get<std::string>(a[offset + 3]);
                    int tw = static_cast<int>(std::get<double>(a[offset + 4]));
                    int th = static_cast<int>(std::get<double>(a[offset + 5]));
                    pixels = meme::MemeRenderer::render(templatePath, top, bottom, tw, th, w, h);
                }
                return {pixels, w, h};
            };

            auto img1 = loadImage(args, 1);  // offset 1: rendered1, tpl1, top1, bot1, w1, h1
            auto img2 = loadImage(args, 7);  // offset 7: rendered2, tpl2, top2, bot2, w2, h2

            double extraParam = 0;
            if (args.size() > 13 && std::holds_alternative<double>(args[13])) {
                extraParam = std::get<double>(args[13]);
            }

            std::vector<unsigned char> result;
            int outW, outH;

            if (layoutName == "beside") {
                result = layout::composeBeside(img1.pixels.data(), img1.width, img1.height,
                                               img2.pixels.data(), img2.width, img2.height,
                                               outW, outH);
            } else if (layoutName == "stack") {
                result = layout::composeStack(img1.pixels.data(), img1.width, img1.height,
                                              img2.pixels.data(), img2.width, img2.height,
                                              outW, outH);
            } else {
                throw std::runtime_error("Unknown layout: " + layoutName);
            }

            return saveTempImage(result, outW, outH);
        }
        int arity() override { return 14; }
        std::string toString() override { return "<native fn>"; }
    };

    // =======================================================================
    // Padding / Border native
    // _add_padding(rendered_or_nil, tpl, top, bot, w, h, padPx)
    // =======================================================================

    class AddPaddingFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            std::vector<unsigned char> pixels;
            int w, h;
            if (std::holds_alternative<std::string>(args[0])) {
                auto path = std::get<std::string>(args[0]);
                int c;
                unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
                if (!data) throw std::runtime_error("Cannot load image: " + path);
                pixels.assign(data, data + w * h * 4);
                stbi_image_free(data);
            } else {
                auto templatePath = std::get<std::string>(args[1]);
                auto top = std::get<std::string>(args[2]);
                auto bottom = std::get<std::string>(args[3]);
                int tw = static_cast<int>(std::get<double>(args[4]));
                int th = static_cast<int>(std::get<double>(args[5]));
                pixels = meme::MemeRenderer::render(templatePath, top, bottom, tw, th, w, h);
            }
            int padPx = static_cast<int>(std::get<double>(args[6]));
            int outW, outH;
            auto result = layout::addPadding(pixels.data(), w, h, padPx, outW, outH);
            return saveTempImage(result, outW, outH);
        }
        int arity() override { return 7; }
        std::string toString() override { return "<native fn>"; }
    };

    class AddBorderFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            std::vector<unsigned char> pixels;
            int w, h;
            if (std::holds_alternative<std::string>(args[0])) {
                auto path = std::get<std::string>(args[0]);
                int c;
                unsigned char* data = stbi_load(path.c_str(), &w, &h, &c, 4);
                if (!data) throw std::runtime_error("Cannot load image: " + path);
                pixels.assign(data, data + w * h * 4);
                stbi_image_free(data);
            } else {
                auto templatePath = std::get<std::string>(args[1]);
                auto top = std::get<std::string>(args[2]);
                auto bottom = std::get<std::string>(args[3]);
                int tw = static_cast<int>(std::get<double>(args[4]));
                int th = static_cast<int>(std::get<double>(args[5]));
                pixels = meme::MemeRenderer::render(templatePath, top, bottom, tw, th, w, h);
            }
            int borderPx = static_cast<int>(std::get<double>(args[6]));
            int outW, outH;
            auto result = layout::addBorder(pixels.data(), w, h, borderPx, outW, outH);
            return saveTempImage(result, outW, outH);
        }
        int arity() override { return 7; }
        std::string toString() override { return "<native fn>"; }
    };

    // =======================================================================
    // Save Rendered Native
    // _save_rendered(tempPath, outputPath)
    // =======================================================================

    class SaveRenderedFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto tempPath = std::get<std::string>(args[0]);
            auto outputPath = std::get<std::string>(args[1]);
            int w, h, c;
            unsigned char* data = stbi_load(tempPath.c_str(), &w, &h, &c, 4);
            if (!data) throw std::runtime_error("Cannot load rendered image: " + tempPath);
            std::vector<unsigned char> pixels(data, data + w * h * 4);
            stbi_image_free(data);
            bool ok = meme::MemeRenderer::saveImage(pixels, w, h, outputPath);
            return ok;
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    // =======================================================================
    // Partial Effect — a callable Meme -> Meme (well, Meme -> string temp path)
    // Created by parameterized effect creators (blur, pixelate, etc.)
    // =======================================================================

    class PartialEffect : public MacCallable {
    public:
        std::string effectName;
        double param;

        PartialEffect(const std::string& name, double p) : effectName(name), param(p) {}

        value::MacValue call(std::shared_ptr<interpreter::Interpreter> interp,
                             std::vector<value::MacValue> args) override {
            // args[0] is the meme instance — extract and process
            auto pd = getMemePixels(args[0]);

            if (effectName == "blur") {
                effects::blur(pd.pixels.data(), pd.width, pd.height, static_cast<int>(param));
            } else if (effectName == "pixelate") {
                effects::pixelate(pd.pixels.data(), pd.width, pd.height, static_cast<int>(param));
            } else if (effectName == "noise") {
                effects::noise(pd.pixels.data(), pd.width, pd.height, static_cast<float>(param));
            } else if (effectName == "saturate") {
                effects::saturate(pd.pixels.data(), pd.width, pd.height, static_cast<float>(param));
            } else if (effectName == "contrast") {
                effects::contrast(pd.pixels.data(), pd.width, pd.height, static_cast<float>(param));
            } else if (effectName == "brightness") {
                effects::brightness(pd.pixels.data(), pd.width, pd.height, static_cast<float>(param));
            } else if (effectName == "jpeg") {
                effects::jpegQuality(pd.pixels.data(), pd.width, pd.height, static_cast<int>(param));
            }

            auto tempPath = saveTempImage(pd.pixels, pd.width, pd.height);

            // Create a new rendered meme map with the result info
            auto result = std::make_shared<collection::MacMap>();
            result->set("_rendered", value::MacValue(tempPath));
            result->set("_width", value::MacValue(static_cast<double>(pd.width)));
            result->set("_height", value::MacValue(static_cast<double>(pd.height)));
            return value::MacValue(result);
        }
        int arity() override { return 1; }
        std::string toString() override { return "<" + effectName + " effect>"; }
    };

    // =======================================================================
    // Parameterized Effect Creator
    // blur(3) -> returns PartialEffect
    // meme |> blur(3) -> pipe prepends meme, calls with [meme, 3] -> apply directly
    // =======================================================================

    class ParamEffectCreator : public MacCallable {
    public:
        std::string effectName;

        ParamEffectCreator(const std::string& name) : effectName(name) {}

        value::MacValue call(std::shared_ptr<interpreter::Interpreter> interp,
                             std::vector<value::MacValue> args) override {
            if (args.size() == 1) {
                // Single arg: must be the parameter, return partial
                if (std::holds_alternative<double>(args[0])) {
                    auto partial = std::make_shared<PartialEffect>(effectName, std::get<double>(args[0]));
                    return value::MacValue(std::static_pointer_cast<MacCallable>(partial));
                }
                // Single arg is a meme instance -> error, need a parameter
                throw std::runtime_error(effectName + "() requires a numeric parameter.");
            }

            if (args.size() == 2) {
                // Two args: meme (from pipe) + parameter
                // Determine which is the meme and which is the parameter
                // In pipe: args[0] = meme (prepended), args[1] = original param
                double param;
                value::MacValue memeVal;

                if (std::holds_alternative<double>(args[1])) {
                    param = std::get<double>(args[1]);
                    memeVal = args[0];
                } else if (std::holds_alternative<double>(args[0])) {
                    param = std::get<double>(args[0]);
                    memeVal = args[1];
                } else {
                    throw std::runtime_error(effectName + "() requires a numeric parameter.");
                }

                auto pd = getMemePixels(memeVal);

                if (effectName == "blur") {
                    effects::blur(pd.pixels.data(), pd.width, pd.height, static_cast<int>(param));
                } else if (effectName == "pixelate") {
                    effects::pixelate(pd.pixels.data(), pd.width, pd.height, static_cast<int>(param));
                } else if (effectName == "noise") {
                    effects::noise(pd.pixels.data(), pd.width, pd.height, static_cast<float>(param));
                } else if (effectName == "saturate") {
                    effects::saturate(pd.pixels.data(), pd.width, pd.height, static_cast<float>(param));
                } else if (effectName == "contrast") {
                    effects::contrast(pd.pixels.data(), pd.width, pd.height, static_cast<float>(param));
                } else if (effectName == "brightness") {
                    effects::brightness(pd.pixels.data(), pd.width, pd.height, static_cast<float>(param));
                } else if (effectName == "jpeg") {
                    effects::jpegQuality(pd.pixels.data(), pd.width, pd.height, static_cast<int>(param));
                }

                auto tempPath = saveTempImage(pd.pixels, pd.width, pd.height);

                auto result = std::make_shared<collection::MacMap>();
                result->set("_rendered", value::MacValue(tempPath));
                result->set("_width", value::MacValue(static_cast<double>(pd.width)));
                result->set("_height", value::MacValue(static_cast<double>(pd.height)));
                return value::MacValue(result);
            }

            throw std::runtime_error(effectName + "() expects 1 or 2 arguments.");
        }
        int arity() override { return 1; }
        std::string toString() override { return "<" + effectName + ">"; }
    };

    // =======================================================================
    // Direct Effect (parameterless): invert, sepia, sharpen, vignette
    // invert(meme) -> apply and return rendered map
    // =======================================================================

    class DirectEffect : public MacCallable {
    public:
        std::string effectName;

        DirectEffect(const std::string& name) : effectName(name) {}

        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto pd = getMemePixels(args[0]);

            if (effectName == "invert") {
                effects::invertColors(pd.pixels.data(), pd.width, pd.height);
            } else if (effectName == "sepia") {
                effects::sepia(pd.pixels.data(), pd.width, pd.height);
            } else if (effectName == "sharpen") {
                effects::sharpen(pd.pixels.data(), pd.width, pd.height);
            } else if (effectName == "vignette") {
                effects::vignette(pd.pixels.data(), pd.width, pd.height);
            }

            auto tempPath = saveTempImage(pd.pixels, pd.width, pd.height);

            auto result = std::make_shared<collection::MacMap>();
            result->set("_rendered", value::MacValue(tempPath));
            result->set("_width", value::MacValue(static_cast<double>(pd.width)));
            result->set("_height", value::MacValue(static_cast<double>(pd.height)));
            return value::MacValue(result);
        }
        int arity() override { return 1; }
        std::string toString() override { return "<" + effectName + ">"; }
    };

    // =======================================================================
    // Layout Native Functions
    // These take meme instances directly, render, composite, return map
    // =======================================================================

    // beside(meme1, meme2) -> rendered map
    class BesideFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto pd1 = getMemePixels(args[0]);
            auto pd2 = getMemePixels(args[1]);
            int outW, outH;
            auto result = layout::composeBeside(pd1.pixels.data(), pd1.width, pd1.height,
                                                pd2.pixels.data(), pd2.width, pd2.height,
                                                outW, outH);
            auto tempPath = saveTempImage(result, outW, outH);
            auto map = std::make_shared<collection::MacMap>();
            map->set("_rendered", value::MacValue(tempPath));
            map->set("_width", value::MacValue(static_cast<double>(outW)));
            map->set("_height", value::MacValue(static_cast<double>(outH)));
            return value::MacValue(map);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<beside>"; }
    };

    // stack(meme1, meme2) -> rendered map
    class StackFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto pd1 = getMemePixels(args[0]);
            auto pd2 = getMemePixels(args[1]);
            int outW, outH;
            auto result = layout::composeStack(pd1.pixels.data(), pd1.width, pd1.height,
                                               pd2.pixels.data(), pd2.width, pd2.height,
                                               outW, outH);
            auto tempPath = saveTempImage(result, outW, outH);
            auto map = std::make_shared<collection::MacMap>();
            map->set("_rendered", value::MacValue(tempPath));
            map->set("_width", value::MacValue(static_cast<double>(outW)));
            map->set("_height", value::MacValue(static_cast<double>(outH)));
            return value::MacValue(map);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<stack>"; }
    };

    // grid(cols, memes_array) -> rendered map
    class GridFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            int cols = static_cast<int>(std::get<double>(args[0]));
            auto arr = std::get<std::shared_ptr<collection::MacArray>>(args[1]);

            std::vector<MemePixelData> images;
            for (auto& elem : arr->elements) {
                images.push_back(getMemePixels(elem));
            }

            std::vector<std::pair<const unsigned char*, std::pair<int, int>>> imgPtrs;
            for (auto& img : images) {
                imgPtrs.push_back({img.pixels.data(), {img.width, img.height}});
            }

            int outW, outH;
            auto result = layout::composeGrid(imgPtrs, cols, outW, outH);
            auto tempPath = saveTempImage(result, outW, outH);
            auto map = std::make_shared<collection::MacMap>();
            map->set("_rendered", value::MacValue(tempPath));
            map->set("_width", value::MacValue(static_cast<double>(outW)));
            map->set("_height", value::MacValue(static_cast<double>(outH)));
            return value::MacValue(map);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<grid>"; }
    };

    // pad(meme, px) -> rendered map
    class PadFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto pd = getMemePixels(args[0]);
            int padPx = static_cast<int>(std::get<double>(args[1]));
            int outW, outH;
            auto result = layout::addPadding(pd.pixels.data(), pd.width, pd.height, padPx, outW, outH);
            auto tempPath = saveTempImage(result, outW, outH);
            auto map = std::make_shared<collection::MacMap>();
            map->set("_rendered", value::MacValue(tempPath));
            map->set("_width", value::MacValue(static_cast<double>(outW)));
            map->set("_height", value::MacValue(static_cast<double>(outH)));
            return value::MacValue(map);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<pad>"; }
    };

    // border(meme, px) -> rendered map
    class BorderFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto pd = getMemePixels(args[0]);
            int borderPx = static_cast<int>(std::get<double>(args[1]));
            int outW, outH;
            auto result = layout::addBorder(pd.pixels.data(), pd.width, pd.height, borderPx, outW, outH);
            auto tempPath = saveTempImage(result, outW, outH);
            auto map = std::make_shared<collection::MacMap>();
            map->set("_rendered", value::MacValue(tempPath));
            map->set("_width", value::MacValue(static_cast<double>(outW)));
            map->set("_height", value::MacValue(static_cast<double>(outH)));
            return value::MacValue(map);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<border>"; }
    };

    // =======================================================================
    // Timeline Native Functions
    // =======================================================================

    // timeline() -> MacTimeline
    class TimelineCreateFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue>) override {
            return value::MacValue(std::make_shared<meme::MacTimeline>());
        }
        int arity() override { return 0; }
        std::string toString() override { return "<native fn>"; }
    };

    // _timeline_keyframe(timeline, meme) -> timeline
    class TimelineKeyframeFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto tl = std::get<std::shared_ptr<meme::MacTimeline>>(args[0]);
            auto pd = getMemePixels(args[1]);
            tl->addKeyframe(std::move(pd.pixels), pd.width, pd.height);
            return value::MacValue(tl);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    // _timeline_transition(timeline, durationMs, transitionType) -> timeline
    class TimelineTransitionFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto tl = std::get<std::shared_ptr<meme::MacTimeline>>(args[0]);
            int durationMs = static_cast<int>(std::get<double>(args[1]));
            auto transType = std::get<std::string>(args[2]);
            tl->setTransition(durationMs, transType);
            return value::MacValue(tl);
        }
        int arity() override { return 3; }
        std::string toString() override { return "<native fn>"; }
    };

    // _timeline_hold(timeline, durationMs) -> timeline
    class TimelineHoldFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto tl = std::get<std::shared_ptr<meme::MacTimeline>>(args[0]);
            int durationMs = static_cast<int>(std::get<double>(args[1]));
            tl->addHold(durationMs);
            return value::MacValue(tl);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    // _timeline_loop(timeline, count) -> timeline
    class TimelineLoopFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto tl = std::get<std::shared_ptr<meme::MacTimeline>>(args[0]);
            int count = static_cast<int>(std::get<double>(args[1]));
            tl->setLoop(count);
            return value::MacValue(tl);
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

    // _timeline_render(timeline, outputPath) -> bool
    class TimelineRenderFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue> args) override {
            auto tl = std::get<std::shared_ptr<meme::MacTimeline>>(args[0]);
            auto outputPath = std::get<std::string>(args[1]);
            auto frames = tl->renderFrames();
            if (frames.empty()) return false;

            int w = frames[0].width;
            int h = frames[0].height;
            meme::GifEncoder enc(outputPath, w, h);
            for (auto& frame : frames) {
                enc.addFrame(frame.pixels.data(), frame.delayCs);
            }
            enc.finish();
            return true;
        }
        int arity() override { return 2; }
        std::string toString() override { return "<native fn>"; }
    };

} // namespace callable

#endif // NATIVE_FUNCTIONS_H
