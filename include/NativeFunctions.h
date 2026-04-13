#ifndef NATIVE_FUNCTIONS_H
#define NATIVE_FUNCTIONS_H

#include <chrono>
#include <cmath>
#include <iostream>
#include <string>
#include "MacCallable.h"
#include "MacArray.h"
#include "MacMap.h"
#include "MacMeme.h"
#include "MacGif.h"

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

} // namespace callable

#endif // NATIVE_FUNCTIONS_H
