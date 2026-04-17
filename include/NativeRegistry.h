#ifndef NATIVE_REGISTRY_H
#define NATIVE_REGISTRY_H

#include <functional>           // function (factory callbacks)
#include <memory>               // shared_ptr, make_shared
#include <string>               // string
#include <vector>               // vector (overloads, definitions)
#include "NativeFunctions.h"    // callable::* (all native function classes)

namespace native_registry {

    enum class NativeVisibility {
        Public,
        Internal,
    };

    struct NativeOverload {
        std::vector<std::string> params;
        std::string returnType;
        std::string description;
    };

    struct NativeDefinition {
        std::string name;
        NativeVisibility visibility;
        std::string symbolType;
        std::string description;
        std::vector<NativeOverload> overloads;
        std::function<std::shared_ptr<callable::MacCallable>()> factory;
    };

    inline const std::vector<NativeDefinition>& all() {
        static const std::vector<NativeDefinition> defs = {
            {"clock", NativeVisibility::Public, "fun(0)", "Current time in seconds.",
                {{{}, "number", "Return the current wall-clock time in seconds."}},
                [] { return std::make_shared<callable::ClockFunction>(); }},
            {"len", NativeVisibility::Public, "fun(1)", "Length of a string or array.",
                {{{"value"}, "number", "Return the number of characters or elements."}},
                [] { return std::make_shared<callable::LenFunction>(); }},
            {"substr", NativeVisibility::Public, "fun(3)", "Extract a substring.",
                {{{"str", "start", "length"}, "string", "Return part of a string."}},
                [] { return std::make_shared<callable::SubstrFunction>(); }},
            {"split", NativeVisibility::Public, "fun(2)", "Split a string by a delimiter.",
                {{{"str", "delimiter"}, "[string]", "Split a string into an array of substrings."}},
                [] { return std::make_shared<callable::SplitFunction>(); }},
            {"type", NativeVisibility::Public, "fun(1)", "Return the runtime type name.",
                {{{"value"}, "string", "Return the runtime type name for a value."}},
                [] { return std::make_shared<callable::TypeFunction>(); }},
            {"sqrt", NativeVisibility::Public, "fun(1)", "Square root.",
                {{{"value"}, "number", "Return the square root of a number."}},
                [] { return std::make_shared<callable::SqrtFunction>(); }},
            {"abs", NativeVisibility::Public, "fun(1)", "Absolute value.",
                {{{"value"}, "number", "Return the absolute value of a number."}},
                [] { return std::make_shared<callable::AbsFunction>(); }},
            {"pow", NativeVisibility::Public, "fun(2)", "Exponentiation.",
                {{{"base", "exponent"}, "number", "Raise a number to a power."}},
                [] { return std::make_shared<callable::PowFunction>(); }},
            {"floor", NativeVisibility::Public, "fun(1)", "Round down.",
                {{{"value"}, "number", "Round a number down to the nearest integer."}},
                [] { return std::make_shared<callable::FloorFunction>(); }},
            {"ceil", NativeVisibility::Public, "fun(1)", "Round up.",
                {{{"value"}, "number", "Round a number up to the nearest integer."}},
                [] { return std::make_shared<callable::CeilFunction>(); }},
            {"push", NativeVisibility::Public, "fun(2)", "Append to an array.",
                {{{"array", "value"}, "nil", "Append a value to an array in place."}},
                [] { return std::make_shared<callable::PushFunction>(); }},
            {"pop", NativeVisibility::Public, "fun(1)", "Remove and return the last array element.",
                {{{"array"}, "T", "Remove and return the last element from an array."}},
                [] { return std::make_shared<callable::PopFunction>(); }},
            {"map", NativeVisibility::Public, "fun(2)", "Transform each array element.",
                {{{"array", "fn"}, "[T]", "Apply a callable to each element and return a new array."}},
                [] { return std::make_shared<callable::MapArrayFunction>(); }},
            {"filter", NativeVisibility::Public, "fun(2)", "Keep array elements that match a predicate.",
                {{{"array", "fn"}, "[T]", "Return a new array containing only truthy matches."}},
                [] { return std::make_shared<callable::FilterFunction>(); }},
            {"input", NativeVisibility::Public, "fun(1)", "Read one line of user input.",
                {{{"prompt"}, "string", "Print a prompt and return the entered line."}},
                [] { return std::make_shared<callable::InputFunction>(); }},

            {"blur", NativeVisibility::Public, "fun(1)", "Create a blur effect.",
                {{{"radius"}, "Meme -> Meme", "Return a reusable blur effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("blur"); }},
            {"pixelate", NativeVisibility::Public, "fun(1)", "Create a pixelation effect.",
                {{{"blockSize"}, "Meme -> Meme", "Return a reusable pixelation effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("pixelate"); }},
            {"noise", NativeVisibility::Public, "fun(1)", "Create a noise effect.",
                {{{"amount"}, "Meme -> Meme", "Return a reusable noise effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("noise"); }},
            {"saturate", NativeVisibility::Public, "fun(1)", "Create a saturation effect.",
                {{{"factor"}, "Meme -> Meme", "Return a reusable saturation effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("saturate"); }},
            {"contrast", NativeVisibility::Public, "fun(1)", "Create a contrast effect.",
                {{{"factor"}, "Meme -> Meme", "Return a reusable contrast effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("contrast"); }},
            {"brightness", NativeVisibility::Public, "fun(1)", "Create a brightness effect.",
                {{{"factor"}, "Meme -> Meme", "Return a reusable brightness effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("brightness"); }},
            {"jpeg", NativeVisibility::Public, "fun(1)", "Create a JPEG-artifact effect.",
                {{{"quality"}, "Meme -> Meme", "Return a reusable JPEG compression effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("jpeg"); }},
            {"invert", NativeVisibility::Public, "Meme -> Meme", "Invert image colors.",
                {{{"meme"}, "Meme", "Apply the invert effect to a meme."}},
                [] { return std::make_shared<callable::DirectEffect>("invert"); }},
            {"sepia", NativeVisibility::Public, "Meme -> Meme", "Apply a sepia tone.",
                {{{"meme"}, "Meme", "Apply the sepia effect to a meme."}},
                [] { return std::make_shared<callable::DirectEffect>("sepia"); }},
            {"sharpen", NativeVisibility::Public, "Meme -> Meme", "Sharpen a meme image.",
                {{{"meme"}, "Meme", "Apply the sharpen effect to a meme."}},
                [] { return std::make_shared<callable::DirectEffect>("sharpen"); }},
            {"vignette", NativeVisibility::Public, "Meme -> Meme", "Darken the edges of an image.",
                {{{"meme"}, "Meme", "Apply the vignette effect to a meme."}},
                [] { return std::make_shared<callable::DirectEffect>("vignette"); }},
            {"grayscale", NativeVisibility::Public, "Meme -> Meme", "Convert to grayscale.",
                {{{"meme"}, "Meme", "Convert a meme to grayscale."}},
                [] { return std::make_shared<callable::DirectEffect>("grayscale"); }},

            {"hueShift", NativeVisibility::Public, "Meme -> Meme", "Rotate hue by degrees (0-360).",
                {{{"degrees"}, "Meme -> Meme", "Return a reusable hue shift effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("hueShift"); }},
            {"glow", NativeVisibility::Public, "Meme -> Meme", "Add bloom/glow effect.",
                {{{"radius"}, "Meme -> Meme", "Return a reusable glow effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("glow"); }},
            {"posterize", NativeVisibility::Public, "Meme -> Meme", "Reduce to N color levels.",
                {{{"levels"}, "Meme -> Meme", "Return a reusable posterize effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("posterize"); }},
            {"chromatic", NativeVisibility::Public, "Meme -> Meme", "RGB channel displacement.",
                {{{"offset"}, "Meme -> Meme", "Return a reusable chromatic aberration effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("chromatic"); }},
            {"threshold", NativeVisibility::Public, "Meme -> Meme", "Black/white binarize at level.",
                {{{"level"}, "Meme -> Meme", "Return a reusable threshold effect function."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("threshold"); }},
            {"tint", NativeVisibility::Public, "Meme -> Meme", "Apply a color tint overlay.",
                {{{"hexColor"}, "Meme -> Meme", "Return a reusable tint effect function. Pass hex as number."}},
                [] { return std::make_shared<callable::ParamEffectCreator>("tint"); }},

            {"beside", NativeVisibility::Public, "fun(2)", "Compose two memes side by side.",
                {{{"left", "right"}, "Meme", "Place two memes next to each other."}},
                [] { return std::make_shared<callable::BesideFunction>(); }},
            {"stack", NativeVisibility::Public, "fun(2)", "Compose two memes vertically.",
                {{{"top", "bottom"}, "Meme", "Stack two memes vertically."}},
                [] { return std::make_shared<callable::StackFunction>(); }},
            {"grid", NativeVisibility::Public, "fun(2)", "Compose a grid of memes.",
                {{{"cols", "memes"}, "Meme", "Arrange memes into a grid."}},
                [] { return std::make_shared<callable::GridFunction>(); }},
            {"pad", NativeVisibility::Public, "fun(2)", "Add padding around a meme.",
                {{{"meme", "pixels"}, "Meme", "Add white padding around a meme or rendered result."}},
                [] { return std::make_shared<callable::PadFunction>(); }},
            {"border", NativeVisibility::Public, "fun(2)", "Add a border around a meme.",
                {{{"meme", "pixels"}, "Meme", "Add a black border around a meme or rendered result."}},
                [] { return std::make_shared<callable::BorderFunction>(); }},

            {"range", NativeVisibility::Public, "fun(*)", "Generate a numeric range.",
                {
                    {{"end"}, "[number]", "Generate numbers from 0 up to end."},
                    {{"start", "end"}, "[number]", "Generate numbers from start up to end."},
                    {{"start", "end", "step"}, "[number]", "Generate numbers from start to end with a custom step."},
                },
                [] { return std::make_shared<callable::RangeFunction>(); }},
            {"reduce", NativeVisibility::Public, "fun(3)", "Fold an array with an accumulator.",
                {{{"array", "fn", "initial"}, "T", "Reduce an array to a single value."}},
                [] { return std::make_shared<callable::ReduceFunction>(); }},
            {"zip", NativeVisibility::Public, "fun(2)", "Pair two arrays together.",
                {{{"left", "right"}, "[(A, B)]", "Combine two arrays into pairs."}},
                [] { return std::make_shared<callable::ZipFunction>(); }},
            {"enumerate", NativeVisibility::Public, "fun(1)", "Pair array items with indices.",
                {{{"array"}, "[(number, T)]", "Return index-value pairs for an array."}},
                [] { return std::make_shared<callable::EnumerateFunction>(); }},
            {"each", NativeVisibility::Public, "fun(2)", "Run a callback for side effects.",
                {{{"array", "fn"}, "nil", "Call a function for each element and return nil."}},
                [] { return std::make_shared<callable::EachFunction>(); }},
            {"flatten", NativeVisibility::Public, "fun(1)", "Flatten one array nesting level.",
                {{{"array"}, "[T]", "Flatten one level of nested arrays."}},
                [] { return std::make_shared<callable::FlattenFunction>(); }},
            {"flatMap", NativeVisibility::Public, "fun(2)", "Map and flatten one level.",
                {{{"array", "fn"}, "[T]", "Map each element, then flatten one nesting level."}},
                [] { return std::make_shared<callable::FlatMapFunction>(); }},
            {"sort", NativeVisibility::Public, "fun(*)", "Sort an array.",
                {
                    {{"array"}, "[T]", "Sort an array using the natural order for numbers or strings."},
                    {{"array", "compare"}, "[T]", "Sort an array using a custom comparator."},
                },
                [] { return std::make_shared<callable::SortFunction>(); }},
            {"reverse", NativeVisibility::Public, "fun(1)", "Reverse an array.",
                {{{"array"}, "[T]", "Return a reversed copy of an array."}},
                [] { return std::make_shared<callable::ReverseFunction>(); }},
            {"find", NativeVisibility::Public, "fun(2)", "Find the first matching array element.",
                {{{"array", "fn"}, "T", "Return the first element whose predicate result is truthy."}},
                [] { return std::make_shared<callable::FindFunction>(); }},
            {"any", NativeVisibility::Public, "fun(2)", "Check whether any array element matches.",
                {{{"array", "fn"}, "bool", "Return true when any element passes the predicate."}},
                [] { return std::make_shared<callable::AnyFunction>(); }},
            {"all", NativeVisibility::Public, "fun(2)", "Check whether all array elements match.",
                {{{"array", "fn"}, "bool", "Return true when every element passes the predicate."}},
                [] { return std::make_shared<callable::AllFunction>(); }},
            {"take", NativeVisibility::Public, "fun(2)", "Take the first n array elements.",
                {{{"array", "count"}, "[T]", "Return the first count elements from an array."}},
                [] { return std::make_shared<callable::TakeFunction>(); }},
            {"drop", NativeVisibility::Public, "fun(2)", "Drop the first n array elements.",
                {{{"array", "count"}, "[T]", "Return an array without the first count elements."}},
                [] { return std::make_shared<callable::DropFunction>(); }},
            {"join", NativeVisibility::Public, "fun(2)", "Join array values into a string.",
                {{{"array", "separator"}, "string", "Join stringified array values with a separator."}},
                [] { return std::make_shared<callable::JoinFunction>(); }},
            {"takeWhile", NativeVisibility::Public, "fun(2)", "Take elements while predicate is true.",
                {{{"array", "fn"}, "[T]", "Take elements from the front while the predicate returns true."}},
                [] { return std::make_shared<callable::TakeWhileFunction>(); }},
            {"dropWhile", NativeVisibility::Public, "fun(2)", "Drop elements while predicate is true.",
                {{{"array", "fn"}, "[T]", "Drop elements from the front while the predicate returns true."}},
                [] { return std::make_shared<callable::DropWhileFunction>(); }},
            {"partition", NativeVisibility::Public, "fun(2)", "Split array by predicate.",
                {{{"array", "fn"}, "[[T], [T]]", "Split into matching and non-matching arrays."}},
                [] { return std::make_shared<callable::PartitionFunction>(); }},
            {"groupBy", NativeVisibility::Public, "fun(2)", "Group elements by key function.",
                {{{"array", "fn"}, "{string: [T]}", "Group elements by the string key returned by fn."}},
                [] { return std::make_shared<callable::GroupByFunction>(); }},
            {"unique", NativeVisibility::Public, "fun(1)", "Remove duplicate values.",
                {{{"array"}, "[T]", "Remove duplicates, preserving order of first occurrence."}},
                [] { return std::make_shared<callable::UniqueFunction>(); }},
            {"chunk", NativeVisibility::Public, "fun(2)", "Split array into groups of n.",
                {{{"array", "size"}, "[[T]]", "Split array into chunks of the given size."}},
                [] { return std::make_shared<callable::ChunkFunction>(); }},
            {"scan", NativeVisibility::Public, "fun(3)", "Reduce keeping intermediate values.",
                {{{"array", "fn", "initial"}, "[T]", "Like reduce but returns all intermediate accumulated values."}},
                [] { return std::make_shared<callable::ScanFunction>(); }},
            {"upper", NativeVisibility::Public, "fun(1)", "Convert text to uppercase.",
                {{{"str"}, "string", "Return an uppercase copy of a string."}},
                [] { return std::make_shared<callable::UpperFunction>(); }},
            {"lower", NativeVisibility::Public, "fun(1)", "Convert text to lowercase.",
                {{{"str"}, "string", "Return a lowercase copy of a string."}},
                [] { return std::make_shared<callable::LowerFunction>(); }},
            {"trim", NativeVisibility::Public, "fun(1)", "Trim surrounding whitespace.",
                {{{"str"}, "string", "Return a string without surrounding whitespace."}},
                [] { return std::make_shared<callable::TrimFunction>(); }},
            {"replace", NativeVisibility::Public, "fun(3)", "Replace all substring matches.",
                {{{"str", "from", "to"}, "string", "Replace all instances of one substring with another."}},
                [] { return std::make_shared<callable::ReplaceFunction>(); }},
            {"animate", NativeVisibility::Public, "fun(2)", "Turn an array of memes into a GIF.",
                {{{"memes", "duration"}, "Gif", "Create a GIF by giving each meme the same duration."}},
                [] { return std::make_shared<callable::AnimateFunction>(); }},
            {"toGrid", NativeVisibility::Public, "fun(3)", "Arrange memes into a grid.",
                {{{"memes", "cols", "rows"}, "Meme", "Compose memes into a grid-shaped rendered result."}},
                [] { return std::make_shared<callable::ToGridFunction>(); }},
            {"save", NativeVisibility::Public, "fun(2)", "Save an exportable value to a file.",
                {{{"target", "path"}, "bool", "Save a Meme, Gif, Timeline, or rendered result to disk."}},
                [] { return std::make_shared<callable::SaveFunction>(); }},

            {"_resolve_template", NativeVisibility::Internal, "fun(1)", "Internal template resolver.",
                {{{"nameOrPath"}, "string", "Resolve a template id or file path."}},
                [] { return std::make_shared<callable::ResolveTemplateFunction>(); }},
            {"_meme_save", NativeVisibility::Internal, "fun(7)", "Internal meme saver.",
                {{{"templatePath", "topText", "bottomText", "centerText", "width", "height", "path"}, "bool", "Render and save a meme image."}},
                [] { return std::make_shared<callable::MemeRenderSaveFunction>(); }},
            {"_gif_save", NativeVisibility::Internal, "fun(2)", "Internal GIF saver.",
                {{{"frames", "path"}, "bool", "Render and save an animated GIF."}},
                [] { return std::make_shared<callable::GifRenderSaveFunction>(); }},
            {"_gif_save_raw", NativeVisibility::Internal, "fun(2)", "Save a raw MacGif.",
                {{{"gif", "path"}, "bool", "Save a raw MacGif to an animated GIF file."}},
                [] { return std::make_shared<callable::GifSaveRawFunction>(); }},
            {"_apply_effect", NativeVisibility::Internal, "fun(8)", "Internal effect helper.",
                {{{"renderedPath", "templatePath", "topText", "bottomText", "width", "height", "effectName", "param"}, "string", "Apply an effect and return a rendered temp path."}},
                [] { return std::make_shared<callable::ApplyEffectFunction>(); }},
            {"_compose_layout", NativeVisibility::Internal, "fun(14)", "Internal layout helper.",
                {{{"layoutName", "rendered1", "template1", "top1", "bottom1", "width1", "height1", "rendered2", "template2", "top2", "bottom2", "width2", "height2", "extra"}, "string", "Compose two rendered memes and return a temp path."}},
                [] { return std::make_shared<callable::ComposeLayoutFunction>(); }},
            {"_add_padding", NativeVisibility::Internal, "fun(7)", "Internal padding helper.",
                {{{"renderedPath", "templatePath", "topText", "bottomText", "width", "height", "pixels"}, "string", "Add padding and return a temp path."}},
                [] { return std::make_shared<callable::AddPaddingFunction>(); }},
            {"_add_border", NativeVisibility::Internal, "fun(7)", "Internal border helper.",
                {{{"renderedPath", "templatePath", "topText", "bottomText", "width", "height", "pixels"}, "string", "Add a border and return a temp path."}},
                [] { return std::make_shared<callable::AddBorderFunction>(); }},
            {"_save_rendered", NativeVisibility::Internal, "fun(2)", "Internal rendered-image saver.",
                {{{"tempPath", "path"}, "bool", "Copy a rendered temp image to a real output path."}},
                [] { return std::make_shared<callable::SaveRenderedFunction>(); }},
            {"_timeline_render", NativeVisibility::Internal, "fun(2)", "Internal timeline render helper.",
                {{{"timeline", "path"}, "bool", "Render a native timeline to an animated GIF."}},
                [] { return std::make_shared<callable::TimelineRenderFunction>(); }},
        };
        return defs;
    }

    inline const NativeDefinition* find(const std::string& name) {
        for (const auto& def : all()) {
            if (def.name == name) return &def;
        }
        return nullptr;
    }

    inline bool isPublic(const NativeDefinition& def) {
        return def.visibility == NativeVisibility::Public;
    }

    inline std::string visibilityName(NativeVisibility visibility) {
        return visibility == NativeVisibility::Public ? "public" : "internal";
    }

} // namespace native_registry

#endif // NATIVE_REGISTRY_H
