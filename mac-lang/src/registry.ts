// Mac Language Registry — native functions, properties, return types, prelude definitions

import { MacType, T_NUMBER, T_STRING, T_BOOL, T_NIL, T_UNKNOWN, PropertyInfo, SymbolKind } from "./types";

// ============================================================
// Native function definitions (name, arity, hover description)
// ============================================================

export interface NativeDef {
    name: string;
    arity: number;
    description: string;
}

export const NATIVE_FUNCTIONS: NativeDef[] = [
    { name: "clock", arity: 0, description: "Returns the current time in seconds since epoch." },
    { name: "len", arity: 1, description: "Returns the length of a string or array." },
    { name: "substr", arity: 3, description: "Returns a substring: substr(str, start, length)." },
    { name: "split", arity: 2, description: "Splits a string by a delimiter: split(str, delim)." },
    { name: "type", arity: 1, description: "Returns the type of a value as a string." },
    { name: "sqrt", arity: 1, description: "Returns the square root of a number." },
    { name: "abs", arity: 1, description: "Returns the absolute value of a number." },
    { name: "pow", arity: 2, description: "Returns base raised to exponent: pow(base, exp)." },
    { name: "floor", arity: 1, description: "Returns the floor of a number." },
    { name: "ceil", arity: 1, description: "Returns the ceiling of a number." },
    { name: "push", arity: 2, description: "Appends an element to an array: push(array, element)." },
    { name: "pop", arity: 1, description: "Removes and returns the last element of an array." },
    { name: "map", arity: 2, description: "Applies a function to each element: map(array, fn)." },
    { name: "filter", arity: 2, description: "Filters elements by predicate: filter(array, fn)." },
    { name: "input", arity: 1, description: "Reads a line of input, displaying the given prompt." },
    { name: "_resolve_template", arity: 1, description: "Internal: resolves template name to file path." },
    { name: "_meme_save", arity: 6, description: "Internal: renders and saves meme image." },
    { name: "_gif_save", arity: 2, description: "Internal: renders and saves animated GIF." },
    { name: "_save_rendered", arity: 2, description: "Internal: copies a rendered temp image to an output path." },
    { name: "_apply_effect", arity: 8, description: "Internal: applies a named effect to a meme image." },
    { name: "_compose_layout", arity: 14, description: "Internal: composes two meme images with a layout." },
    { name: "_add_padding", arity: 7, description: "Internal: adds white padding around a meme image." },
    { name: "_add_border", arity: 7, description: "Internal: adds a black border around a meme image." },
    { name: "range", arity: -1, description: "range(end), range(start, end), or range(start, end, step) — Generate an array of numbers." },
    { name: "reduce", arity: 3, description: "reduce(arr, fn, initial) — Fold array: result = fn(result, element) for each element." },
    { name: "zip", arity: 2, description: "zip(arr1, arr2) — Pair elements: [[a[0],b[0]], [a[1],b[1]], ...]. Pipeable." },
    { name: "enumerate", arity: 1, description: "enumerate(arr) — Returns [[0, elem0], [1, elem1], ...]. Pipeable." },
    { name: "each", arity: 2, description: "each(arr, fn) — Call fn(element) for side effects. Returns nil. Pipeable." },
    { name: "flatten", arity: 1, description: "flatten(arr) — Flatten one level: [[1,2],[3]] → [1,2,3]. Pipeable." },
    { name: "flatMap", arity: 2, description: "flatMap(arr, fn) — Map then flatten one level. Pipeable." },
    { name: "sort", arity: -1, description: "sort(arr) or sort(arr, fn) — Sort by natural order or comparator. Pipeable." },
    { name: "reverse", arity: 1, description: "reverse(arr) — Returns new reversed array. Pipeable." },
    { name: "find", arity: 2, description: "find(arr, fn) — First element where fn(elem) is truthy, or nil. Pipeable." },
    { name: "any", arity: 2, description: "any(arr, fn) — True if any fn(elem) is truthy. Pipeable." },
    { name: "all", arity: 2, description: "all(arr, fn) — True if all fn(elem) are truthy. Pipeable." },
    { name: "take", arity: 2, description: "take(arr, n) — First n elements. Pipeable." },
    { name: "drop", arity: 2, description: "drop(arr, n) — Array without first n elements. Pipeable." },
    { name: "join", arity: 2, description: "join(arr, sep) — Join elements into string with separator. Pipeable." },
    { name: "upper", arity: 1, description: "upper(str) — Returns uppercased string. Pipeable." },
    { name: "lower", arity: 1, description: "lower(str) — Returns lowercased string. Pipeable." },
    { name: "trim", arity: 1, description: "trim(str) — Strip leading/trailing whitespace. Pipeable." },
    { name: "replace", arity: 3, description: "replace(str, from, to) — Replace all occurrences. Pipeable." },
    { name: "animate", arity: 2, description: "animate(memesArray, duration) — Create animated GIF from array of Memes. Pipeable." },
    { name: "toGrid", arity: 3, description: "toGrid(memesArray, cols, rows) — Arrange memes in a grid layout. Pipeable." },
    { name: "save", arity: 2, description: "save(thing, path) — Save a Meme, Gif, Timeline, or effect result to a file." },
    { name: "blur", arity: 1, description: "blur(radius) — Box blur effect. Returns Meme → Meme. Pipeable." },
    { name: "pixelate", arity: 1, description: "pixelate(blockSize) — Pixelation effect. Returns Meme → Meme." },
    { name: "noise", arity: 1, description: "noise(amount) — Random noise effect. Returns Meme → Meme." },
    { name: "saturate", arity: 1, description: "saturate(amount) — Adjust saturation (1.0 = normal). Returns Meme → Meme." },
    { name: "contrast", arity: 1, description: "contrast(amount) — Adjust contrast (1.0 = normal). Returns Meme → Meme." },
    { name: "brightness", arity: 1, description: "brightness(amount) — Adjust brightness (1.0 = normal). Returns Meme → Meme." },
    { name: "jpeg", arity: 1, description: "jpeg(quality) — JPEG compression artifacts (low = more artifacts). Returns Meme → Meme." },
    { name: "invert", arity: 1, description: "invert(meme) — Invert colors. Meme → Meme. Pipeable." },
    { name: "sepia", arity: 1, description: "sepia(meme) — Sepia tone filter. Meme → Meme. Pipeable." },
    { name: "sharpen", arity: 1, description: "sharpen(meme) — Sharpen filter. Meme → Meme. Pipeable." },
    { name: "vignette", arity: 1, description: "vignette(meme) — Dark edges effect. Meme → Meme. Pipeable." },
    { name: "beside", arity: 2, description: "beside(meme1, meme2) — Side-by-side layout. Pipeable: m1 |> beside(m2)." },
    { name: "stack", arity: 2, description: "stack(meme1, meme2) — Vertical stack layout. Pipeable: m1 |> stack(m2)." },
    { name: "grid", arity: 2, description: "grid(cols, memesArray) — Grid layout." },
    { name: "pad", arity: 2, description: "pad(meme, pixels) — Add white padding. Pipeable." },
    { name: "border", arity: 2, description: "border(meme, pixels) — Add black border. Pipeable." },
    { name: "timeline", arity: 0, description: "Internal: creates a raw timeline object." },
    { name: "_timeline_keyframe", arity: 2, description: "Internal: adds a keyframe to a timeline." },
    { name: "_timeline_transition", arity: 3, description: "Internal: adds a transition to a timeline." },
    { name: "_timeline_hold", arity: 2, description: "Internal: holds the last frame on a timeline." },
    { name: "_timeline_loop", arity: 2, description: "Internal: sets the loop count on a timeline." },
    { name: "_timeline_render", arity: 2, description: "Internal: renders a timeline to an animated GIF." },
];

// ============================================================
// Known properties/methods for built-in types (hover + completion)
// ============================================================

export const KNOWN_PROPERTIES: PropertyInfo[] = [
    { name: "text", ownerType: "Meme", kind: "method", params: ["position", "str"], description: "Add text at a position. Returns a new Meme. Chainable." },
    { name: "save", ownerType: "Meme", kind: "method", params: ["format", "path"], description: "Render and save the meme to a file. Format is `PNG`, `JPG`, or `GIF`." },
    { name: "resize", ownerType: "Meme", kind: "method", params: ["size"], description: "Returns a new Meme with the given Size dimensions." },
    { name: "_top", ownerType: "Meme", kind: "field", description: "The top text string." },
    { name: "_bottom", ownerType: "Meme", kind: "field", description: "The bottom text string." },
    { name: "_template", ownerType: "Meme", kind: "field", description: "The Template used by this meme." },
    { name: "_width", ownerType: "Meme", kind: "field", description: "Output width in pixels (0 = template default)." },
    { name: "_height", ownerType: "Meme", kind: "field", description: "Output height in pixels (0 = template default)." },
    { name: "frame", ownerType: "Gif", kind: "method", params: ["meme", "duration"], description: "Add a frame to the GIF. Returns `this` for chaining." },
    { name: "save", ownerType: "Gif", kind: "method", params: ["path"], description: "Render all frames and save as an animated GIF." },
    { name: "_frames", ownerType: "Gif", kind: "field", description: "Array of frame data maps." },
    { name: "frame", ownerType: "Timeline", kind: "method", params: ["meme", "duration"], description: "Add a keyframe with hold duration. Returns `this` for chaining." },
    { name: "transition", ownerType: "Timeline", kind: "method", params: ["type", "duration"], description: "Set transition to next frame. Types: crossfade, slideLeft, slideRight, slideUp, slideDown, wipe." },
    { name: "loop", ownerType: "Timeline", kind: "method", params: ["count"], description: "Set loop count (0 = infinite). Returns `this` for chaining." },
    { name: "render", ownerType: "Timeline", kind: "method", params: ["path"], description: "Render timeline to animated GIF." },
    { name: "save", ownerType: "Timeline", kind: "method", params: ["path"], description: "Render timeline to animated GIF. Alias for render()." },
    { name: "_tl", ownerType: "Timeline", kind: "field", description: "Internal: raw C++ timeline object." },
    { name: "name", ownerType: "Template", kind: "field", description: "The template name or path as provided." },
    { name: "path", ownerType: "Template", kind: "field", description: "Resolved file path to the template image." },
    { name: "width", ownerType: "Size", kind: "field", description: "Width in pixels." },
    { name: "height", ownerType: "Size", kind: "field", description: "Height in pixels." },
    { name: "ms", ownerType: "Duration", kind: "field", description: "Duration in milliseconds." },
    { name: "meme", ownerType: "Frame", kind: "field", description: "The Meme for this frame." },
    { name: "duration", ownerType: "Frame", kind: "field", description: "The Duration for this frame." },
    { name: "name", ownerType: "Position", kind: "field", description: "Position name (\"top\", \"bottom\", or \"center\")." },
    { name: "name", ownerType: "Format", kind: "field", description: "Format name (\"png\", \"jpg\", or \"gif\")." },
    { name: "init", ownerType: "class", kind: "method", params: ["..."], description: "Constructor — called when the class is instantiated." },
    { name: "__add__", ownerType: "class", kind: "method", params: ["other"], description: "Operator overload for `+`." },
    { name: "__sub__", ownerType: "class", kind: "method", params: ["other"], description: "Operator overload for `-`." },
    { name: "__mul__", ownerType: "class", kind: "method", params: ["other"], description: "Operator overload for `*`." },
    { name: "__div__", ownerType: "class", kind: "method", params: ["other"], description: "Operator overload for `/`." },
    { name: "__mod__", ownerType: "class", kind: "method", params: ["other"], description: "Operator overload for `%`." },
    { name: "__eq__", ownerType: "class", kind: "method", params: ["other"], description: "Operator overload for `==`." },
    { name: "__ne__", ownerType: "class", kind: "method", params: ["other"], description: "Operator overload for `!=`." },
    { name: "__lt__", ownerType: "class", kind: "method", params: ["other"], description: "Operator overload for `<`." },
    { name: "__gt__", ownerType: "class", kind: "method", params: ["other"], description: "Operator overload for `>`." },
    { name: "__neg__", ownerType: "class", kind: "method", params: [], description: "Operator overload for unary `-`." },
    { name: "__not__", ownerType: "class", kind: "method", params: [], description: "Operator overload for `!`." },
];

export const PROPERTY_INDEX = new Map<string, PropertyInfo[]>();
for (const p of KNOWN_PROPERTIES) {
    const existing = PROPERTY_INDEX.get(p.name) ?? [];
    existing.push(p);
    PROPERTY_INDEX.set(p.name, existing);
}

// ============================================================
// Return type resolvers for native functions
// ============================================================

const preserveArray = (args: MacType[]) =>
    args[0]?.tag === "array" ? args[0] : { tag: "array" as const, elementType: T_UNKNOWN };

export const NATIVE_RETURN_TYPES = new Map<string, (argTypes: MacType[]) => MacType>([
    ["clock", () => T_NUMBER], ["len", () => T_NUMBER], ["substr", () => T_STRING],
    ["split", () => ({ tag: "array", elementType: T_STRING })],
    ["type", () => T_STRING], ["sqrt", () => T_NUMBER], ["abs", () => T_NUMBER],
    ["pow", () => T_NUMBER], ["floor", () => T_NUMBER], ["ceil", () => T_NUMBER],
    ["input", () => T_STRING], ["range", () => ({ tag: "array", elementType: T_NUMBER })],
    ["join", () => T_STRING], ["any", () => T_BOOL], ["all", () => T_BOOL],
    ["upper", () => T_STRING], ["lower", () => T_STRING], ["trim", () => T_STRING],
    ["replace", () => T_STRING], ["save", () => T_BOOL], ["render", () => T_BOOL],
    ["push", () => T_NIL], ["each", () => T_NIL],
    ["reduce", (args) => args.length >= 3 && args[2].tag !== "unknown" ? args[2] : T_UNKNOWN],

    // Array-preserving
    ["filter", preserveArray], ["sort", preserveArray], ["reverse", preserveArray],
    ["take", preserveArray], ["drop", preserveArray],

    // Array-transforming
    ["map", (args) => {
        if (args[0]?.tag === "array" && args[0].elementType.tag === "instance" && args[0].elementType.className === "Meme") {
            return { tag: "array" as const, elementType: args[0].elementType };
        }
        if (args[1]?.tag === "instance") return { tag: "array" as const, elementType: args[1] };
        return { tag: "array" as const, elementType: T_UNKNOWN };
    }],
    ["flatMap", (args) => {
        if (args[0]?.tag === "array" && args[0].elementType.tag === "array") {
            return { tag: "array" as const, elementType: args[0].elementType.elementType };
        }
        if (args[0]?.tag === "array" && args[0].elementType.tag === "instance" && args[0].elementType.className === "Meme") {
            return { tag: "array" as const, elementType: args[0].elementType };
        }
        return { tag: "array" as const, elementType: T_UNKNOWN };
    }],
    ["flatten", (args) => {
        if (args[0]?.tag === "array" && args[0].elementType.tag === "array") {
            return { tag: "array" as const, elementType: args[0].elementType.elementType };
        }
        return { tag: "array" as const, elementType: T_UNKNOWN };
    }],
    ["zip", (args) => {
        // zip(a, b) — use the first array's element type that isn't unknown
        const a = args[0]?.tag === "array" ? args[0].elementType : T_UNKNOWN;
        const b = args[1]?.tag === "array" ? args[1].elementType : T_UNKNOWN;
        const inner = a.tag !== "unknown" ? a : b.tag !== "unknown" ? b : T_UNKNOWN;
        return { tag: "array" as const, elementType: { tag: "array" as const, elementType: inner } };
    }],
    ["enumerate", (args) => {
        if (args[0]?.tag === "array" && args[0].elementType.tag !== "unknown") {
            return { tag: "array" as const, elementType: { tag: "array" as const, elementType: args[0].elementType } };
        }
        return { tag: "array" as const, elementType: { tag: "array" as const, elementType: T_UNKNOWN } };
    }],

    // Element-extracting
    ["find", (args) => args[0]?.tag === "array" ? args[0].elementType : T_UNKNOWN],
    ["pop", (args) => args[0]?.tag === "array" ? args[0].elementType : T_UNKNOWN],

    // Timeline
    ["Timeline", () => ({ tag: "instance", className: "Timeline" })],
    ["timeline", () => ({ tag: "instance", className: "Timeline" })],

    // Effects (parameterized → function, direct → Meme)
    ["blur", () => ({ tag: "function", paramCount: 1 })],
    ["pixelate", () => ({ tag: "function", paramCount: 1 })],
    ["noise", () => ({ tag: "function", paramCount: 1 })],
    ["saturate", () => ({ tag: "function", paramCount: 1 })],
    ["contrast", () => ({ tag: "function", paramCount: 1 })],
    ["brightness", () => ({ tag: "function", paramCount: 1 })],
    ["jpeg", () => ({ tag: "function", paramCount: 1 })],
    ["invert", () => ({ tag: "instance", className: "Meme" })],
    ["sepia", () => ({ tag: "instance", className: "Meme" })],
    ["sharpen", () => ({ tag: "instance", className: "Meme" })],
    ["vignette", () => ({ tag: "instance", className: "Meme" })],

    // Layout
    ["beside", () => ({ tag: "instance", className: "Meme" })],
    ["stack", () => ({ tag: "instance", className: "Meme" })],
    ["grid", () => ({ tag: "instance", className: "Meme" })],
    ["pad", () => ({ tag: "instance", className: "Meme" })],
    ["border", () => ({ tag: "instance", className: "Meme" })],

    // Meme bridge
    ["animate", () => ({ tag: "instance", className: "Gif" })],
    ["toGrid", () => ({ tag: "instance", className: "Meme" })],
]);

// ============================================================
// Method/field return types
// ============================================================

export const METHOD_RETURN_TYPES = new Map<string, MacType>([
    ["Meme.text", { tag: "instance", className: "Meme" }],
    ["Meme.resize", { tag: "instance", className: "Meme" }],
    ["Meme.save", T_NIL],
    ["Gif.frame", { tag: "instance", className: "Gif" }],
    ["Gif.save", T_NIL],
    ["Timeline.frame", { tag: "instance", className: "Timeline" }],
    ["Timeline.transition", { tag: "instance", className: "Timeline" }],
    ["Timeline.loop", { tag: "instance", className: "Timeline" }],
    ["Timeline.render", { tag: "instance", className: "Timeline" }],
    ["Timeline.save", { tag: "instance", className: "Timeline" }],
]);

export const FIELD_TYPES = new Map<string, MacType>([
    ["Meme._template", { tag: "instance", className: "Template" }],
    ["Meme._top", T_STRING], ["Meme._bottom", T_STRING],
    ["Meme._width", T_NUMBER], ["Meme._height", T_NUMBER],
    ["Template.name", T_STRING], ["Template.path", T_STRING],
    ["Size.width", T_NUMBER], ["Size.height", T_NUMBER],
    ["Duration.ms", T_NUMBER],
    ["Position.name", T_STRING], ["Format.name", T_STRING],
    ["Frame.meme", { tag: "instance", className: "Meme" }],
    ["Frame.duration", { tag: "instance", className: "Duration" }],
    ["Gif._frames", { tag: "array", elementType: T_UNKNOWN }],
]);

// ============================================================
// Prelude type definitions (stdlib classes + constants)
// ============================================================

export const PRELUDE_TYPES: { name: string; kind: SymbolKind; params?: string[]; description: string; type?: MacType }[] = [
    { name: "Size", kind: "class", params: ["width", "height"], description: "Pixel dimensions. Supports `+`, `*`, `==` operators.", type: { tag: "class", className: "Size" } },
    { name: "Duration", kind: "class", params: ["ms"], description: "Time in milliseconds. Supports `+`, `*`, `==` operators.", type: { tag: "class", className: "Duration" } },
    { name: "Position", kind: "class", params: ["name"], description: "Text position on a meme. Use the constants `Top`, `Bottom`, `Center`.", type: { tag: "class", className: "Position" } },
    { name: "Format", kind: "class", params: ["name"], description: "Image output format. Use the constants `PNG`, `JPG`, `GIF`.", type: { tag: "class", className: "Format" } },
    { name: "Template", kind: "class", params: ["nameOrPath"], description: "Meme template image. Pass a built-in name (`two_panel`, `three_panel`, `bottom_text`, `blank`) or a file path.", type: { tag: "class", className: "Template" } },
    { name: "Meme", kind: "class", params: ["template"], description: "Meme builder. Chain `.text(position, str)` to add text, `.save(format, path)` to export, `.resize(size)` to resize. `Meme + Duration` creates a Frame.", type: { tag: "class", className: "Meme" } },
    { name: "Frame", kind: "class", params: ["meme", "duration"], description: "A single animation frame pairing a Meme with a Duration. Created by `Meme + Duration`.", type: { tag: "class", className: "Frame" } },
    { name: "Gif", kind: "class", params: [], description: "Animated GIF builder. Chain `.frame(meme, duration)` to add frames, `.save(path)` to export. `Gif + Frame` adds a frame.", type: { tag: "class", className: "Gif" } },
    { name: "Timeline", kind: "class", params: [], description: "Animation timeline with transitions. Chain `.frame(meme, duration)`, `.transition(type, duration)`, `.loop(count)`, `.render(path)`. `Timeline + Frame` adds a frame.", type: { tag: "class", className: "Timeline" } },
    { name: "Top", kind: "variable", description: "Position constant — top of the meme.", type: { tag: "instance", className: "Position" } },
    { name: "Bottom", kind: "variable", description: "Position constant — bottom of the meme.", type: { tag: "instance", className: "Position" } },
    { name: "Center", kind: "variable", description: "Position constant — center of the meme.", type: { tag: "instance", className: "Position" } },
    { name: "PNG", kind: "variable", description: "Format constant — PNG image output.", type: { tag: "instance", className: "Format" } },
    { name: "JPG", kind: "variable", description: "Format constant — JPG image output.", type: { tag: "instance", className: "Format" } },
    { name: "GIF", kind: "variable", description: "Format constant — GIF image output.", type: { tag: "instance", className: "Format" } },
    { name: "deepfry", kind: "variable", description: "Composed effect: saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1).", type: { tag: "function", paramCount: 1 } },
    { name: "crossfade", kind: "variable", description: "Timeline transition — cross-fade between frames.", type: T_STRING },
    { name: "slideLeft", kind: "variable", description: "Timeline transition — slide left.", type: T_STRING },
    { name: "slideRight", kind: "variable", description: "Timeline transition — slide right.", type: T_STRING },
    { name: "slideUp", kind: "variable", description: "Timeline transition — slide up.", type: T_STRING },
    { name: "slideDown", kind: "variable", description: "Timeline transition — slide down.", type: T_STRING },
    { name: "wipe", kind: "variable", description: "Timeline transition — wipe reveal.", type: T_STRING },
];
