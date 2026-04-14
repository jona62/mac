// Mac Language Analyzer — scope-aware symbol table, diagnostic collector, and type inference

import { Token, TokenType } from "./scanner";
import {
    Expr,
    Stmt,
    FunctionStmt,
} from "./parser";

// ============================================================
// Types
// ============================================================

export type SymbolKind = "variable" | "function" | "class" | "parameter" | "native" | "method";

export interface Symbol {
    name: string;
    kind: SymbolKind;
    token: Token;
    params?: Token[];
    superclass?: string;
    methods?: string[];
    description?: string;
    type?: MacType;
}

export interface Scope {
    symbols: Map<string, Symbol>;
    parent: Scope | null;
}

export type DiagnosticSeverity = "error" | "warning";

export interface Diagnostic {
    line: number;
    column: number;
    endColumn: number;
    message: string;
    severity: DiagnosticSeverity;
}

export interface SymbolReference {
    token: Token;
    definition: Symbol | null;
}

export interface PropertyInfo {
    name: string;
    ownerType: string;
    kind: "method" | "field";
    params?: string[];
    description: string;
}

export interface PropertyReference {
    token: Token;
    info: PropertyInfo;
}

export interface AnalysisResult {
    diagnostics: Diagnostic[];
    symbols: Symbol[];
    references: SymbolReference[];
    propertyRefs: PropertyReference[];
    scopes: Scope[];
}

// ============================================================
// Type Inference
// ============================================================

export type MacType =
    | { tag: "number" }
    | { tag: "string" }
    | { tag: "bool" }
    | { tag: "nil" }
    | { tag: "array"; elementType: MacType }
    | { tag: "map"; valueType: MacType }
    | { tag: "instance"; className: string }
    | { tag: "function"; paramCount: number }
    | { tag: "class"; className: string }
    | { tag: "unknown" };

const T_NUMBER:  MacType = { tag: "number" };
const T_STRING:  MacType = { tag: "string" };
const T_BOOL:    MacType = { tag: "bool" };
const T_NIL:     MacType = { tag: "nil" };
const T_UNKNOWN: MacType = { tag: "unknown" };

export function formatMacType(t: MacType): string {
    switch (t.tag) {
        case "number": return "number";
        case "string": return "string";
        case "bool": return "bool";
        case "nil": return "nil";
        case "array": return `[${formatMacType(t.elementType)}]`;
        case "map": return `{${formatMacType(t.valueType)}}`;
        case "instance": return t.className;
        case "function": return `fun(${t.paramCount})`;
        case "class": return `class ${t.className}`;
        case "unknown": return "unknown";
    }
}

// ============================================================
// Analyzer
// ============================================================

// Helper to create a synthetic token for native functions
function nativeToken(name: string): Token {
    return {
        type: TokenType.IDENTIFIER,
        lexeme: name,
        literal: null,
        line: 0,
        column: 0,
    };
}

interface NativeDef {
    name: string;
    arity: number;
    description: string;
}

const NATIVE_FUNCTIONS: NativeDef[] = [
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
    // Functional toolkit — Array
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
    // Functional toolkit — String
    { name: "upper", arity: 1, description: "upper(str) — Returns uppercased string. Pipeable." },
    { name: "lower", arity: 1, description: "lower(str) — Returns lowercased string. Pipeable." },
    { name: "trim", arity: 1, description: "trim(str) — Strip leading/trailing whitespace. Pipeable." },
    { name: "replace", arity: 3, description: "replace(str, from, to) — Replace all occurrences. Pipeable." },
    // Functional toolkit — Meme bridge
    { name: "animate", arity: 2, description: "animate(memesArray, duration) — Create animated GIF from array of Memes. Pipeable." },
    { name: "toGrid", arity: 3, description: "toGrid(memesArray, cols, rows) — Arrange memes in a grid layout. Pipeable." },
    // Save
    { name: "save", arity: 2, description: "save(thing, path) — Save a Meme, Gif, Timeline, or effect result to a file." },
    // Effects
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
    // Layout
    { name: "beside", arity: 2, description: "beside(meme1, meme2) — Side-by-side layout. Pipeable: m1 |> beside(m2)." },
    { name: "stack", arity: 2, description: "stack(meme1, meme2) — Vertical stack layout. Pipeable: m1 |> stack(m2)." },
    { name: "grid", arity: 2, description: "grid(cols, memesArray) — Grid layout." },
    { name: "pad", arity: 2, description: "pad(meme, pixels) — Add white padding. Pipeable." },
    { name: "border", arity: 2, description: "border(meme, pixels) — Add black border. Pipeable." },
    // Timeline internals
    { name: "timeline", arity: 0, description: "Internal: creates a raw timeline object." },
    { name: "_timeline_keyframe", arity: 2, description: "Internal: adds a keyframe to a timeline." },
    { name: "_timeline_transition", arity: 3, description: "Internal: adds a transition to a timeline." },
    { name: "_timeline_hold", arity: 2, description: "Internal: holds the last frame on a timeline." },
    { name: "_timeline_loop", arity: 2, description: "Internal: sets the loop count on a timeline." },
    { name: "_timeline_render", arity: 2, description: "Internal: renders a timeline to an animated GIF." },
];

// Known properties/methods for built-in types
const KNOWN_PROPERTIES: PropertyInfo[] = [
    // Meme
    { name: "text", ownerType: "Meme", kind: "method", params: ["position", "str"], description: "Add text at a position. Returns a new Meme. Chainable." },
    { name: "save", ownerType: "Meme", kind: "method", params: ["format", "path"], description: "Render and save the meme to a file. Format is `PNG`, `JPG`, or `GIF`." },
    { name: "resize", ownerType: "Meme", kind: "method", params: ["size"], description: "Returns a new Meme with the given Size dimensions." },
    { name: "_top", ownerType: "Meme", kind: "field", description: "The top text string." },
    { name: "_bottom", ownerType: "Meme", kind: "field", description: "The bottom text string." },
    { name: "_template", ownerType: "Meme", kind: "field", description: "The Template used by this meme." },
    { name: "_width", ownerType: "Meme", kind: "field", description: "Output width in pixels (0 = template default)." },
    { name: "_height", ownerType: "Meme", kind: "field", description: "Output height in pixels (0 = template default)." },
    // Gif
    { name: "frame", ownerType: "Gif", kind: "method", params: ["meme", "duration"], description: "Add a frame to the GIF. Returns `this` for chaining." },
    { name: "save", ownerType: "Gif", kind: "method", params: ["path"], description: "Render all frames and save as an animated GIF." },
    { name: "_frames", ownerType: "Gif", kind: "field", description: "Array of frame data maps." },
    // Timeline
    { name: "frame", ownerType: "Timeline", kind: "method", params: ["meme", "duration"], description: "Add a keyframe with hold duration. Returns `this` for chaining." },
    { name: "transition", ownerType: "Timeline", kind: "method", params: ["type", "duration"], description: "Set transition to next frame. Types: crossfade, slideLeft, slideRight, slideUp, slideDown, wipe." },
    { name: "loop", ownerType: "Timeline", kind: "method", params: ["count"], description: "Set loop count (0 = infinite). Returns `this` for chaining." },
    { name: "render", ownerType: "Timeline", kind: "method", params: ["path"], description: "Render timeline to animated GIF." },
    { name: "save", ownerType: "Timeline", kind: "method", params: ["path"], description: "Render timeline to animated GIF. Alias for render()." },
    { name: "_tl", ownerType: "Timeline", kind: "field", description: "Internal: raw C++ timeline object." },
    // Template
    { name: "name", ownerType: "Template", kind: "field", description: "The template name or path as provided." },
    { name: "path", ownerType: "Template", kind: "field", description: "Resolved file path to the template image." },
    // Size
    { name: "width", ownerType: "Size", kind: "field", description: "Width in pixels." },
    { name: "height", ownerType: "Size", kind: "field", description: "Height in pixels." },
    // Duration
    { name: "ms", ownerType: "Duration", kind: "field", description: "Duration in milliseconds." },
    // Frame
    { name: "meme", ownerType: "Frame", kind: "field", description: "The Meme for this frame." },
    { name: "duration", ownerType: "Frame", kind: "field", description: "The Duration for this frame." },
    // Position / Format
    { name: "name", ownerType: "Position", kind: "field", description: "Position name (\"top\", \"bottom\", or \"center\")." },
    { name: "name", ownerType: "Format", kind: "field", description: "Format name (\"png\", \"jpg\", or \"gif\")." },
    // Class instances
    { name: "init", ownerType: "class", kind: "method", params: ["..."], description: "Constructor — called when the class is instantiated." },
    // Dunder methods
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

// Index for fast lookup by property name
const PROPERTY_INDEX = new Map<string, PropertyInfo[]>();
for (const p of KNOWN_PROPERTIES) {
    const existing = PROPERTY_INDEX.get(p.name) ?? [];
    existing.push(p);
    PROPERTY_INDEX.set(p.name, existing);
}

// ============================================================
// Native function return type resolution
// ============================================================

const NATIVE_RETURN_TYPES = new Map<string, (argTypes: MacType[]) => MacType>([
    // Primitives
    ["clock",     () => T_NUMBER],
    ["len",       () => T_NUMBER],
    ["substr",    () => T_STRING],
    ["split",     () => ({ tag: "array", elementType: T_STRING })],
    ["type",      () => T_STRING],
    ["sqrt",      () => T_NUMBER],
    ["abs",       () => T_NUMBER],
    ["pow",       () => T_NUMBER],
    ["floor",     () => T_NUMBER],
    ["ceil",      () => T_NUMBER],
    ["input",     () => T_STRING],
    ["range",     () => ({ tag: "array", elementType: T_NUMBER })],
    ["join",      () => T_STRING],
    ["any",       () => T_BOOL],
    ["all",       () => T_BOOL],
    ["upper",     () => T_STRING],
    ["lower",     () => T_STRING],
    ["trim",      () => T_STRING],
    ["replace",   () => T_STRING],
    ["save",      () => T_BOOL],
    ["render",    () => T_BOOL],
    ["push",      () => T_NIL],
    ["each",      () => T_NIL],
    ["reduce",    (args) => {
        // reduce(arr, fn, initial) — return type matches the initial value
        // When piped: arr |> reduce(fn, initial) — initial is args[2]
        // Direct call: reduce(arr, fn, initial) — initial is args[2]
        if (args.length >= 3 && args[2].tag !== "unknown") return args[2];
        return T_UNKNOWN;
    }],

    // Array-preserving (return type matches first arg)
    ["filter",  (args) => args[0]?.tag === "array" ? args[0] : { tag: "array", elementType: T_UNKNOWN }],
    ["sort",    (args) => args[0]?.tag === "array" ? args[0] : { tag: "array", elementType: T_UNKNOWN }],
    ["reverse", (args) => args[0]?.tag === "array" ? args[0] : { tag: "array", elementType: T_UNKNOWN }],
    ["take",    (args) => args[0]?.tag === "array" ? args[0] : { tag: "array", elementType: T_UNKNOWN }],
    ["drop",    (args) => args[0]?.tag === "array" ? args[0] : { tag: "array", elementType: T_UNKNOWN }],

    // Array-transforming — element type depends on callback or structure
    ["map",       (args) => {
        // map(arr, fn) — if input is [Meme] and fn is an effect, preserve Meme
        if (args[0]?.tag === "array" && args[0].elementType.tag === "instance" && args[0].elementType.className === "Meme") {
            return { tag: "array" as const, elementType: args[0].elementType };
        }
        if (args[1]?.tag === "instance") return { tag: "array" as const, elementType: args[1] };
        return { tag: "array" as const, elementType: T_UNKNOWN };
    }],
    ["flatMap",   (args) => {
        // flatMap(arr, fn) — map then flatten. If input is [[A]], result is [A]
        if (args[0]?.tag === "array" && args[0].elementType.tag === "array") {
            return { tag: "array" as const, elementType: args[0].elementType.elementType };
        }
        // If input is [Meme] and fn is an effect, preserve
        if (args[0]?.tag === "array" && args[0].elementType.tag === "instance" && args[0].elementType.className === "Meme") {
            return { tag: "array" as const, elementType: args[0].elementType };
        }
        return { tag: "array" as const, elementType: T_UNKNOWN };
    }],
    ["flatten",   (args) => {
        if (args[0]?.tag === "array" && args[0].elementType.tag === "array") {
            return { tag: "array" as const, elementType: args[0].elementType.elementType };
        }
        return { tag: "array" as const, elementType: T_UNKNOWN };
    }],
    ["zip",       (args) => {
        // zip(a, b) — if both arrays have the same element type, preserve it
        if (args[0]?.tag === "array" && args[1]?.tag === "array") {
            const a = args[0].elementType, b = args[1].elementType;
            if (a.tag === b.tag && a.tag === "instance" && b.tag === "instance" && a.className === b.className) {
                return { tag: "array" as const, elementType: { tag: "array" as const, elementType: a } };
            }
        }
        return { tag: "array" as const, elementType: { tag: "array" as const, elementType: T_UNKNOWN } };
    }],
    ["enumerate", (args) => {
        // enumerate(arr) — inner arrays are [number, element] but we can't express tuples
        // Best effort: if input element is known, propagate it as inner array element
        if (args[0]?.tag === "array" && args[0].elementType.tag !== "unknown") {
            return { tag: "array" as const, elementType: { tag: "array" as const, elementType: args[0].elementType } };
        }
        return { tag: "array" as const, elementType: { tag: "array" as const, elementType: T_UNKNOWN } };
    }],

    // Element-extracting
    ["find", (args) => args[0]?.tag === "array" ? args[0].elementType : T_UNKNOWN],
    ["pop",  (args) => args[0]?.tag === "array" ? args[0].elementType : T_UNKNOWN],

    // Timeline
    ["Timeline",  () => ({ tag: "instance", className: "Timeline" })],
    ["timeline",  () => ({ tag: "instance", className: "Timeline" })],

    // Effects (parameterized) — return a Meme→Meme function
    ["blur",       () => ({ tag: "function", paramCount: 1 })],
    ["pixelate",   () => ({ tag: "function", paramCount: 1 })],
    ["noise",      () => ({ tag: "function", paramCount: 1 })],
    ["saturate",   () => ({ tag: "function", paramCount: 1 })],
    ["contrast",   () => ({ tag: "function", paramCount: 1 })],
    ["brightness", () => ({ tag: "function", paramCount: 1 })],
    ["jpeg",       () => ({ tag: "function", paramCount: 1 })],

    // Effects (direct) — Meme → Meme
    ["invert",   () => ({ tag: "instance", className: "Meme" })],
    ["sepia",    () => ({ tag: "instance", className: "Meme" })],
    ["sharpen",  () => ({ tag: "instance", className: "Meme" })],
    ["vignette", () => ({ tag: "instance", className: "Meme" })],

    // Layout — returns rendered Meme
    ["beside",  () => ({ tag: "instance", className: "Meme" })],
    ["stack",   () => ({ tag: "instance", className: "Meme" })],
    ["grid",    () => ({ tag: "instance", className: "Meme" })],
    ["pad",     () => ({ tag: "instance", className: "Meme" })],
    ["border",  () => ({ tag: "instance", className: "Meme" })],

    // Meme bridge
    ["animate", () => ({ tag: "instance", className: "Gif" })],
    ["toGrid",  () => ({ tag: "instance", className: "Meme" })],
]);

// Method return types keyed by "ClassName.method"
const METHOD_RETURN_TYPES = new Map<string, MacType>([
    ["Meme.text",   { tag: "instance", className: "Meme" }],
    ["Meme.resize", { tag: "instance", className: "Meme" }],
    ["Meme.save",   T_NIL],
    ["Gif.frame",   { tag: "instance", className: "Gif" }],
    ["Gif.save",    T_NIL],
    ["Timeline.frame",      { tag: "instance", className: "Timeline" }],
    ["Timeline.transition",  { tag: "instance", className: "Timeline" }],
    ["Timeline.loop",       { tag: "instance", className: "Timeline" }],
    ["Timeline.render",     { tag: "instance", className: "Timeline" }],
    ["Timeline.save",       { tag: "instance", className: "Timeline" }],
]);

// Field types keyed by "ClassName.field"
const FIELD_TYPES = new Map<string, MacType>([
    ["Meme._template",  { tag: "instance", className: "Template" }],
    ["Meme._top",       T_STRING],
    ["Meme._bottom",    T_STRING],
    ["Meme._width",     T_NUMBER],
    ["Meme._height",    T_NUMBER],
    ["Template.name",   T_STRING],
    ["Template.path",   T_STRING],
    ["Size.width",      T_NUMBER],
    ["Size.height",     T_NUMBER],
    ["Duration.ms",     T_NUMBER],
    ["Position.name",   T_STRING],
    ["Format.name",     T_STRING],
    ["Frame.meme",      { tag: "instance", className: "Meme" }],
    ["Frame.duration",  { tag: "instance", className: "Duration" }],
    ["Gif._frames",     { tag: "array", elementType: T_UNKNOWN }],
]);

// ============================================================
// Analyzer class
// ============================================================

export class Analyzer {
    private diagnostics: Diagnostic[] = [];
    private allSymbols: Symbol[] = [];
    private references: SymbolReference[] = [];
    private propertyRefs: PropertyReference[] = [];
    private scopes: Scope[] = [];
    private currentScope: Scope;
    private nativeNames: Set<string>;
    private currentClassName: string | null = null;

    constructor() {
        // Create global scope with native functions
        this.currentScope = { symbols: new Map(), parent: null };
        this.scopes.push(this.currentScope);
        this.nativeNames = new Set();

        for (const def of NATIVE_FUNCTIONS) {
            const params: Token[] = [];
            for (let i = 0; i < def.arity; i++) {
                params.push(nativeToken(`arg${i}`));
            }
            const sym: Symbol = {
                name: def.name,
                kind: "native",
                token: nativeToken(def.name),
                params,
                description: def.description,
                type: { tag: "function", paramCount: Math.max(0, def.arity) },
            };
            this.currentScope.symbols.set(def.name, sym);
            this.allSymbols.push(sym);
            this.nativeNames.add(def.name);
        }

        // Stdlib prelude types (defined in Mac, loaded at startup)
        const preludeTypes: { name: string; kind: SymbolKind; params?: string[]; description: string; type?: MacType }[] = [
            {
                name: "Size", kind: "class",
                params: ["width", "height"],
                description: "Pixel dimensions. Supports `+`, `*`, `==` operators.",
                type: { tag: "class", className: "Size" },
            },
            {
                name: "Duration", kind: "class",
                params: ["ms"],
                description: "Time in milliseconds. Supports `+`, `*`, `==` operators.",
                type: { tag: "class", className: "Duration" },
            },
            {
                name: "Position", kind: "class",
                params: ["name"],
                description: "Text position on a meme. Use the constants `Top`, `Bottom`, `Center`.",
                type: { tag: "class", className: "Position" },
            },
            {
                name: "Format", kind: "class",
                params: ["name"],
                description: "Image output format. Use the constants `PNG`, `JPG`, `GIF`.",
                type: { tag: "class", className: "Format" },
            },
            {
                name: "Template", kind: "class",
                params: ["nameOrPath"],
                description: "Meme template image. Pass a built-in name (`two_panel`, `three_panel`, `bottom_text`, `blank`) or a file path.",
                type: { tag: "class", className: "Template" },
            },
            {
                name: "Meme", kind: "class",
                params: ["template"],
                description: "Meme builder. Chain `.text(position, str)` to add text, `.save(format, path)` to export, `.resize(size)` to resize. `Meme + Duration` creates a Frame.",
                type: { tag: "class", className: "Meme" },
            },
            {
                name: "Frame", kind: "class",
                params: ["meme", "duration"],
                description: "A single animation frame pairing a Meme with a Duration. Created by `Meme + Duration`.",
                type: { tag: "class", className: "Frame" },
            },
            {
                name: "Gif", kind: "class",
                params: [],
                description: "Animated GIF builder. Chain `.frame(meme, duration)` to add frames, `.save(path)` to export. `Gif + Frame` adds a frame.",
                type: { tag: "class", className: "Gif" },
            },
            {
                name: "Timeline", kind: "class",
                params: [],
                description: "Animation timeline with transitions. Chain `.frame(meme, duration)`, `.transition(type, duration)`, `.loop(count)`, `.render(path)`. `Timeline + Frame` adds a frame.",
                type: { tag: "class", className: "Timeline" },
            },
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
        for (const def of preludeTypes) {
            this.nativeNames.add(def.name);
            const params: Token[] = (def.params ?? []).map(p => nativeToken(p));
            const sym: Symbol = {
                name: def.name,
                kind: def.kind,
                token: nativeToken(def.name),
                params: params.length > 0 ? params : undefined,
                description: def.description,
                type: def.type,
            };
            this.currentScope.symbols.set(def.name, sym);
            this.allSymbols.push(sym);
        }
    }

    analyze(statements: Stmt[]): AnalysisResult {
        for (const stmt of statements) {
            this.analyzeStmt(stmt);
        }
        return {
            diagnostics: this.diagnostics,
            symbols: this.allSymbols,
            references: this.references,
            propertyRefs: this.propertyRefs,
            scopes: this.scopes,
        };
    }

    // --- Scope management ---

    private beginScope(): void {
        const scope: Scope = { symbols: new Map(), parent: this.currentScope };
        this.scopes.push(scope);
        this.currentScope = scope;
    }

    private endScope(): void {
        if (this.currentScope.parent) {
            this.currentScope = this.currentScope.parent;
        }
    }

    private define(name: string, sym: Symbol): void {
        this.currentScope.symbols.set(name, sym);
        this.allSymbols.push(sym);
    }

    private resolve(name: string): Symbol | null {
        let scope: Scope | null = this.currentScope;
        while (scope !== null) {
            const sym = scope.symbols.get(name);
            if (sym !== undefined) {
                return sym;
            }
            scope = scope.parent;
        }
        return null;
    }

    // --- Statement analysis ---

    private analyzeStmt(stmt: Stmt): void {
        switch (stmt.kind) {
            case "expression":
                this.analyzeExpr(stmt.expression);
                break;
            case "print":
                this.analyzeExpr(stmt.expression);
                break;
            case "var":
                this.analyzeVarStmt(stmt);
                break;
            case "block":
                this.beginScope();
                for (const s of stmt.statements) {
                    this.analyzeStmt(s);
                }
                this.endScope();
                break;
            case "if":
                this.analyzeExpr(stmt.condition);
                this.analyzeStmt(stmt.thenBranch);
                if (stmt.elseBranch) {
                    this.analyzeStmt(stmt.elseBranch);
                }
                break;
            case "while":
                this.analyzeExpr(stmt.condition);
                this.analyzeStmt(stmt.body);
                break;
            case "function":
                this.analyzeFunctionStmt(stmt);
                break;
            case "return":
                if (stmt.value) {
                    this.analyzeExpr(stmt.value);
                }
                break;
            case "class":
                this.analyzeClassStmt(stmt);
                break;
            case "forIn": {
                const iterableType = this.analyzeExpr(stmt.iterable);
                this.beginScope();
                let elemType: MacType = T_UNKNOWN;
                if (iterableType.tag === "array") {
                    elemType = iterableType.elementType;
                } else if (iterableType.tag === "map") {
                    elemType = T_STRING; // for-in over map yields string keys
                }
                this.define(stmt.varName.lexeme, {
                    name: stmt.varName.lexeme,
                    kind: "variable",
                    token: stmt.varName,
                    type: elemType,
                });
                this.analyzeStmt(stmt.body);
                this.endScope();
                break;
            }
            case "break":
            case "continue":
                break;
        }
    }

    private analyzeVarStmt(stmt: { kind: "var"; name: Token; initializer: Expr | null }): void {
        let inferredType: MacType = T_UNKNOWN;
        let description: string | undefined;
        if (stmt.initializer) {
            inferredType = this.analyzeExpr(stmt.initializer);
            if (stmt.initializer.kind === "compose") {
                description = this.describeComposeChain(stmt.initializer);
            }
        }
        this.define(stmt.name.lexeme, {
            name: stmt.name.lexeme,
            kind: "variable",
            token: stmt.name,
            type: inferredType,
            description,
        });
    }

    private analyzeFunctionStmt(stmt: FunctionStmt): void {
        const params = stmt.params;
        this.define(stmt.name.lexeme, {
            name: stmt.name.lexeme,
            kind: "function",
            token: stmt.name,
            params,
            type: { tag: "function", paramCount: params.length },
        });

        this.beginScope();
        for (const param of params) {
            this.define(param.lexeme, {
                name: param.lexeme,
                kind: "parameter",
                token: param,
            });
        }
        for (const s of stmt.body) {
            this.analyzeStmt(s);
        }
        this.endScope();
    }

    private analyzeClassStmt(stmt: {
        kind: "class";
        name: Token;
        superclass: { kind: "variable"; name: Token } | null;
        methods: FunctionStmt[];
    }): void {
        const methodNames = stmt.methods.map((m) => m.name.lexeme);
        this.define(stmt.name.lexeme, {
            name: stmt.name.lexeme,
            kind: "class",
            token: stmt.name,
            superclass: stmt.superclass ? stmt.superclass.name.lexeme : undefined,
            methods: methodNames,
            type: { tag: "class", className: stmt.name.lexeme },
        });

        if (stmt.superclass) {
            const def = this.resolve(stmt.superclass.name.lexeme);
            this.references.push({ token: stmt.superclass.name, definition: def });
            if (!def && !this.nativeNames.has(stmt.superclass.name.lexeme)) {
                this.diagnostics.push({
                    line: stmt.superclass.name.line,
                    column: stmt.superclass.name.column,
                    endColumn: stmt.superclass.name.column + stmt.superclass.name.lexeme.length,
                    message: `Undefined variable '${stmt.superclass.name.lexeme}'.`,
                    severity: "warning",
                });
            }
        }

        const prevClassName = this.currentClassName;
        this.currentClassName = stmt.name.lexeme;

        this.beginScope();
        this.define("this", {
            name: "this",
            kind: "variable",
            token: stmt.name,
            type: { tag: "instance", className: stmt.name.lexeme },
        });

        for (const method of stmt.methods) {
            this.define(method.name.lexeme, {
                name: method.name.lexeme,
                kind: "method",
                token: method.name,
                params: method.params,
            });

            this.beginScope();
            for (const param of method.params) {
                this.define(param.lexeme, {
                    name: param.lexeme,
                    kind: "parameter",
                    token: param,
                });
            }
            for (const s of method.body) {
                this.analyzeStmt(s);
            }
            this.endScope();
        }

        this.endScope();
        this.currentClassName = prevClassName;
    }

    // --- Expression analysis with type inference ---

    private analyzeExpr(expr: Expr): MacType {
        switch (expr.kind) {
            case "literal": {
                if (expr.value === null) return T_NIL;
                switch (typeof expr.value) {
                    case "number": return T_NUMBER;
                    case "string": return T_STRING;
                    case "boolean": return T_BOOL;
                    default: return T_NIL;
                }
            }

            case "variable": {
                const def = this.resolve(expr.name.lexeme);
                this.references.push({ token: expr.name, definition: def });
                if (!def && !this.nativeNames.has(expr.name.lexeme)) {
                    this.diagnostics.push({
                        line: expr.name.line,
                        column: expr.name.column,
                        endColumn: expr.name.column + expr.name.lexeme.length,
                        message: `Undefined variable '${expr.name.lexeme}'.`,
                        severity: "warning",
                    });
                }
                if (def?.type) return def.type;
                if (def?.kind === "class") return { tag: "class", className: def.name };
                if (def?.kind === "function" || def?.kind === "native") {
                    return { tag: "function", paramCount: def.params?.length ?? 0 };
                }
                return T_UNKNOWN;
            }

            case "grouping":
                return this.analyzeExpr(expr.expression);

            case "binary": {
                const leftType = this.analyzeExpr(expr.left);
                const rightType = this.analyzeExpr(expr.right);
                switch (expr.operator.type) {
                    case TokenType.PLUS:
                        if (leftType.tag === "string" || rightType.tag === "string") return T_STRING;
                        if (leftType.tag === "number" && rightType.tag === "number") return T_NUMBER;
                        return T_UNKNOWN;
                    case TokenType.MINUS:
                    case TokenType.STAR:
                    case TokenType.SLASH:
                    case TokenType.PERCENT:
                        return T_NUMBER;
                    case TokenType.EQUAL_EQUAL:
                    case TokenType.BANG_EQUAL:
                    case TokenType.GREATER:
                    case TokenType.GREATER_EQUAL:
                    case TokenType.LESS:
                    case TokenType.LESS_EQUAL:
                        return T_BOOL;
                    default:
                        return T_UNKNOWN;
                }
            }

            case "unary": {
                this.analyzeExpr(expr.right);
                if (expr.operator.type === TokenType.MINUS) return T_NUMBER;
                if (expr.operator.type === TokenType.BANG) return T_BOOL;
                return T_UNKNOWN;
            }

            case "logical": {
                this.analyzeExpr(expr.left);
                this.analyzeExpr(expr.right);
                return T_BOOL;
            }

            case "call": {
                const calleeType = this.analyzeExpr(expr.callee);
                const argTypes: MacType[] = [];
                for (const arg of expr.args) {
                    argTypes.push(this.analyzeExpr(arg));
                }
                return this.inferCallType(expr, calleeType, argTypes);
            }

            case "assign": {
                const assignDef = this.resolve(expr.name.lexeme);
                this.references.push({ token: expr.name, definition: assignDef });
                return this.analyzeExpr(expr.value);
            }

            case "get": {
                const objectType = this.analyzeExpr(expr.object);
                const propName = expr.name.lexeme;
                const infos = PROPERTY_INDEX.get(propName);
                if (infos && infos.length > 0) {
                    const specific = infos.find(i => i.ownerType !== "class") ?? infos[0];
                    this.propertyRefs.push({ token: expr.name, info: specific });
                }
                // Infer field type from known types
                if (objectType.tag === "instance") {
                    const key = `${objectType.className}.${propName}`;
                    const fieldType = FIELD_TYPES.get(key);
                    if (fieldType) return fieldType;
                }
                return T_UNKNOWN;
            }

            case "set": {
                this.analyzeExpr(expr.object);
                return this.analyzeExpr(expr.value);
            }

            case "this": {
                if (this.currentClassName) {
                    return { tag: "instance", className: this.currentClassName };
                }
                return T_UNKNOWN;
            }

            case "super":
                return T_UNKNOWN;

            case "array": {
                if (expr.elements.length > 0) {
                    const firstType = this.analyzeExpr(expr.elements[0]);
                    for (let i = 1; i < expr.elements.length; i++) {
                        this.analyzeExpr(expr.elements[i]);
                    }
                    return { tag: "array", elementType: firstType };
                }
                return { tag: "array", elementType: T_UNKNOWN };
            }

            case "map": {
                if (expr.values.length > 0) {
                    const firstValType = this.analyzeExpr(expr.values[0]);
                    for (let i = 1; i < expr.values.length; i++) {
                        this.analyzeExpr(expr.values[i]);
                    }
                    return { tag: "map", valueType: firstValType };
                }
                return { tag: "map", valueType: T_UNKNOWN };
            }

            case "indexGet": {
                const objType = this.analyzeExpr(expr.object);
                this.analyzeExpr(expr.index);
                if (objType.tag === "array") return objType.elementType;
                if (objType.tag === "map") return objType.valueType;
                return T_UNKNOWN;
            }

            case "indexSet": {
                this.analyzeExpr(expr.object);
                this.analyzeExpr(expr.index);
                return this.analyzeExpr(expr.value);
            }

            case "lambda": {
                this.beginScope();
                for (const param of expr.params) {
                    this.define(param.lexeme, {
                        name: param.lexeme,
                        kind: "parameter",
                        token: param,
                    });
                }
                for (const s of expr.body) {
                    this.analyzeStmt(s);
                }
                this.endScope();
                return { tag: "function", paramCount: expr.params.length };
            }

            case "pipe": {
                const inputType = this.analyzeExpr(expr.value);
                this.analyzeExpr(expr.func);
                return this.inferPipeType(expr.func, inputType);
            }

            case "compose": {
                this.analyzeExpr(expr.left);
                this.analyzeExpr(expr.right);
                return { tag: "function", paramCount: 1 };
            }
        }
    }

    // --- Type inference helpers ---

    private inferCallType(expr: { callee: Expr; args: Expr[] }, calleeType: MacType, argTypes: MacType[]): MacType {
        // Direct function/class call: foo(...)
        if (expr.callee.kind === "variable") {
            const name = expr.callee.name.lexeme;

            // Class constructor → instance
            const def = this.resolve(name);
            if (def?.kind === "class") {
                return { tag: "instance", className: name };
            }

            // Native function return type
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) return resolver(argTypes);
        }

        // Method call: obj.method(...)
        if (expr.callee.kind === "get") {
            const methodName = expr.callee.name.lexeme;
            // Infer object type from the get's object
            // We already analyzed it, but we need the type. Re-derive from AST.
            const objExpr = expr.callee.object;
            let objectType: MacType = T_UNKNOWN;
            if (objExpr.kind === "variable") {
                const objDef = this.resolve(objExpr.name.lexeme);
                if (objDef?.type) objectType = objDef.type;
            } else if (objExpr.kind === "call") {
                // Chained call: Meme(t).text(...)
                // The callee type is the return type of the inner call
                // We can infer via the callee type that was already computed
                objectType = calleeType; // This is the type of the `get` expr which we returned T_UNKNOWN for methods
                // Better: check the inner call's callee
                if (objExpr.callee.kind === "variable") {
                    const innerDef = this.resolve(objExpr.callee.name.lexeme);
                    if (innerDef?.kind === "class") {
                        objectType = { tag: "instance", className: innerDef.name };
                    }
                }
            }

            if (objectType.tag === "instance") {
                const key = `${objectType.className}.${methodName}`;
                const retType = METHOD_RETURN_TYPES.get(key);
                if (retType) return retType;
            }
        }

        return T_UNKNOWN;
    }

    // HOFs that take a callback and transform array elements
    private static readonly MAPPING_HOFS = new Set(["map", "flatMap"]);
    // HOFs that take a callback + initial value (fold)
    private static readonly FOLDING_HOFS = new Set(["reduce"]);

    private inferPipeType(func: Expr, inputType: MacType): MacType {
        // x |> name — bare function (no args)
        if (func.kind === "variable") {
            const name = func.name.lexeme;
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) return resolver([inputType]);
            const def = this.resolve(name);
            if (def?.kind === "class") return { tag: "instance", className: def.name };
            // Meme piped through any callable → Meme
            if (inputType.tag === "instance" && inputType.className === "Meme") return inputType;
            return T_UNKNOWN;
        }

        // x |> name(args) — function call with pipe prepending x
        if (func.kind === "call" && func.callee.kind === "variable") {
            const name = func.callee.name.lexeme;

            // Folding HOFs: reduce(fn, initial) — return type = initial value type
            if (Analyzer.FOLDING_HOFS.has(name) && func.args.length >= 2) {
                const initType = this.analyzeExpr(func.args[1]);
                if (initType.tag !== "unknown") return initType;
            }

            // Mapping HOFs: map(fn), flatMap(fn) — infer element type from callback
            if (Analyzer.MAPPING_HOFS.has(name) && func.args.length >= 1) {
                const cbReturnType = this.inferCallbackReturnType(func.args[0], inputType);
                if (cbReturnType.tag !== "unknown") {
                    const elemType = name === "flatMap" && cbReturnType.tag === "array"
                        ? cbReturnType.elementType  // flatMap flattens one level
                        : cbReturnType;
                    return { tag: "array", elementType: elemType };
                }
            }

            // Fall through to native resolver (handles filter, sort, find, etc.)
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) return resolver([inputType]);
            // Meme piped through any callable → Meme
            if (inputType.tag === "instance" && inputType.className === "Meme") return inputType;
            return T_UNKNOWN;
        }

        // Meme piped through anything → Meme
        if (inputType.tag === "instance" && inputType.className === "Meme") {
            return inputType;
        }

        return T_UNKNOWN;
    }

    /** Infer what a callback function returns when called with elements of inputType */
    private inferCallbackReturnType(callback: Expr, inputType: MacType): MacType {
        // Lambda/arrow: trace the body
        if (callback.kind === "lambda") {
            const lastStmt = callback.body[callback.body.length - 1];
            if (lastStmt) {
                const bodyType = this.inferLambdaReturnType(lastStmt);
                if (bodyType.tag !== "unknown") return bodyType;
            }
        }

        // Variable callback: check if it's a known native or effect
        if (callback.kind === "variable") {
            const name = callback.name.lexeme;
            // Native function used as callback: map(arr, upper) → string
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) {
                const elemType = inputType.tag === "array" ? inputType.elementType : inputType;
                return resolver([elemType]);
            }
            // User-defined variable — if input elements are Memes, effects preserve them
            if (inputType.tag === "array" && inputType.elementType.tag === "instance"
                && inputType.elementType.className === "Meme") {
                return inputType.elementType;
            }
        }

        // Call expression as callback: map(arr, blur(5)) — parameterized effect
        if (callback.kind === "call" && callback.callee.kind === "variable") {
            const name = callback.callee.name.lexeme;
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) {
                const resultType = resolver([]);
                // Parameterized effects return a function; the function returns Meme
                if (resultType.tag === "function") {
                    if (inputType.tag === "array" && inputType.elementType.tag === "instance"
                        && inputType.elementType.className === "Meme") {
                        return inputType.elementType;
                    }
                }
                return resultType;
            }
        }

        return T_UNKNOWN;
    }

    private inferLambdaReturnType(stmt: Stmt): MacType {
        // return expr;
        if (stmt.kind === "return" && stmt.value) {
            return this.inferExprType(stmt.value);
        }
        // expression statement (implicit return in arrow functions)
        if (stmt.kind === "expression") {
            return this.inferExprType(stmt.expression);
        }
        return T_UNKNOWN;
    }

    private describeComposeChain(expr: Expr): string {
        const parts: string[] = [];
        this.collectComposeParts(expr, parts);
        return parts.join(" >> ");
    }

    private collectComposeParts(expr: Expr, parts: string[]): void {
        if (expr.kind === "compose") {
            this.collectComposeParts(expr.left, parts);
            this.collectComposeParts(expr.right, parts);
        } else {
            parts.push(this.exprToString(expr));
        }
    }

    private exprToString(expr: Expr): string {
        switch (expr.kind) {
            case "variable": return expr.name.lexeme;
            case "call": {
                const callee = this.exprToString(expr.callee);
                const args = expr.args.map(a => this.exprToString(a)).join(", ");
                return args ? `${callee}(${args})` : `${callee}()`;
            }
            case "get": return `${this.exprToString(expr.object)}.${expr.name.lexeme}`;
            case "binary": return `${this.exprToString(expr.left)} ${expr.operator.lexeme} ${this.exprToString(expr.right)}`;
            case "unary": return `${expr.operator.lexeme}${this.exprToString(expr.right)}`;
            case "grouping": return `(${this.exprToString(expr.expression)})`;
            case "indexGet": return `${this.exprToString(expr.object)}[${this.exprToString(expr.index)}]`;
            case "literal": {
                if (typeof expr.value === "string") return `"${expr.value}"`;
                if (typeof expr.value === "number") return String(expr.value);
                if (typeof expr.value === "boolean") return String(expr.value);
                return "nil";
            }
            case "lambda": {
                const params = expr.params.map(p => p.lexeme).join(", ");
                const body = expr.body.length === 1 && expr.body[0].kind === "return" && expr.body[0].value
                    ? this.exprToString(expr.body[0].value) : "...";
                return params.includes(",") ? `(${params}) -> ${body}` : `${params} -> ${body}`;
            }
            default: return "...";
        }
    }

    private inferExprType(expr: Expr): MacType {
        // Constructor call: Meme(...), Gif(...), Timeline(...)
        if (expr.kind === "call" && expr.callee.kind === "variable") {
            const name = expr.callee.name.lexeme;
            const def = this.resolve(name);
            if (def?.kind === "class") return { tag: "instance", className: name };
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) return resolver([]);
        }
        // Method chain: Meme(...).text(...).text(...)
        if (expr.kind === "call" && expr.callee.kind === "get") {
            const objectType = this.inferExprType(expr.callee.object);
            if (objectType.tag === "instance") {
                const key = `${objectType.className}.${expr.callee.name.lexeme}`;
                const retType = METHOD_RETURN_TYPES.get(key);
                if (retType) return retType;
            }
            return objectType; // method chains often return this
        }
        // Pipe: expr |> effect → preserves Meme type
        if (expr.kind === "pipe") {
            const leftType = this.inferExprType(expr.value);
            if (leftType.tag === "instance" && leftType.className === "Meme") return leftType;
            return leftType;
        }
        // Variable reference
        if (expr.kind === "variable") {
            const def = this.resolve(expr.name.lexeme);
            if (def?.type) return def.type;
        }
        return T_UNKNOWN;
    }
}
