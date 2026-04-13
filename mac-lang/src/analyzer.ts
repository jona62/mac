// Mac Language Analyzer — scope-aware symbol table and diagnostic collector

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
    { name: "grid", arity: 3, description: "grid(cols, rows, memesArray) — Grid layout." },
    { name: "pad", arity: 2, description: "pad(meme, pixels) — Add white padding. Pipeable." },
    { name: "border", arity: 2, description: "border(meme, pixels) — Add black border. Pipeable." },
    // Timeline
    { name: "Timeline", arity: 0, description: "Timeline() — Create an empty animation timeline." },
    { name: "at", arity: 3, description: "at(timeline, timeMs, meme) — Add keyframe. Returns timeline. Pipeable." },
    { name: "transition", arity: 3, description: "transition(timeline, durationMs, type) — Add transition. Types: crossfade, slideLeft, slideRight, slideUp, slideDown, wipe." },
    { name: "hold", arity: 2, description: "hold(timeline, durationMs) — Hold last frame. Returns timeline. Pipeable." },
    { name: "render", arity: 2, description: "render(timeline, path) — Render timeline to animated GIF." },
    { name: "loop", arity: 2, description: "loop(timeline, count) — Set loop count. Returns timeline. Pipeable." },
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

export class Analyzer {
    private diagnostics: Diagnostic[] = [];
    private allSymbols: Symbol[] = [];
    private references: SymbolReference[] = [];
    private propertyRefs: PropertyReference[] = [];
    private scopes: Scope[] = [];
    private currentScope: Scope;
    private nativeNames: Set<string>;

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
            };
            this.currentScope.symbols.set(def.name, sym);
            this.allSymbols.push(sym);
            this.nativeNames.add(def.name);
        }

        // Stdlib prelude types (defined in Mac, loaded at startup)
        const preludeTypes: { name: string; kind: SymbolKind; params?: string[]; description: string }[] = [
            {
                name: "Size", kind: "class",
                params: ["width", "height"],
                description: "Pixel dimensions. Supports `+`, `*`, `==` operators.",
            },
            {
                name: "Duration", kind: "class",
                params: ["ms"],
                description: "Time in milliseconds. Supports `+`, `*`, `==` operators.",
            },
            {
                name: "Position", kind: "class",
                params: ["name"],
                description: "Text position on a meme. Use the constants `Top`, `Bottom`, `Center`.",
            },
            {
                name: "Format", kind: "class",
                params: ["name"],
                description: "Image output format. Use the constants `PNG`, `JPG`, `GIF`.",
            },
            {
                name: "Template", kind: "class",
                params: ["nameOrPath"],
                description: "Meme template image. Pass a built-in name (`two_panel`, `three_panel`, `bottom_text`, `blank`) or a file path.",
            },
            {
                name: "Meme", kind: "class",
                params: ["template"],
                description: "Meme builder. Chain `.text(position, str)` to add text, `.save(format, path)` to export, `.resize(size)` to resize. `Meme + Duration` creates a Frame.",
            },
            {
                name: "Frame", kind: "class",
                params: ["meme", "duration"],
                description: "A single animation frame pairing a Meme with a Duration. Created by `Meme + Duration`.",
            },
            {
                name: "Gif", kind: "class",
                params: [],
                description: "Animated GIF builder. Chain `.frame(meme, duration)` to add frames, `.save(path)` to export. `Gif + Frame` adds a frame.",
            },
            { name: "Top", kind: "variable", description: "Position constant — top of the meme." },
            { name: "Bottom", kind: "variable", description: "Position constant — bottom of the meme." },
            { name: "Center", kind: "variable", description: "Position constant — center of the meme." },
            { name: "PNG", kind: "variable", description: "Format constant — PNG image output." },
            { name: "JPG", kind: "variable", description: "Format constant — JPG image output." },
            { name: "GIF", kind: "variable", description: "Format constant — GIF image output." },
            { name: "deepfry", kind: "variable", description: "Composed effect: saturate(3.0) >> contrast(2.0) >> jpeg(10) >> noise(0.1)." },
            { name: "crossfade", kind: "variable", description: "Timeline transition — cross-fade between frames." },
            { name: "slideLeft", kind: "variable", description: "Timeline transition — slide left." },
            { name: "slideRight", kind: "variable", description: "Timeline transition — slide right." },
            { name: "slideUp", kind: "variable", description: "Timeline transition — slide up." },
            { name: "slideDown", kind: "variable", description: "Timeline transition — slide down." },
            { name: "wipe", kind: "variable", description: "Timeline transition — wipe reveal." },
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
            case "forIn":
                this.analyzeExpr(stmt.iterable);
                this.beginScope();
                this.define(stmt.varName.lexeme, {
                    name: stmt.varName.lexeme,
                    kind: "variable",
                    token: stmt.varName,
                });
                this.analyzeStmt(stmt.body);
                this.endScope();
                break;
            case "break":
            case "continue":
                // Nothing to analyze
                break;
        }
    }

    private analyzeVarStmt(stmt: { kind: "var"; name: Token; initializer: Expr | null }): void {
        if (stmt.initializer) {
            this.analyzeExpr(stmt.initializer);
        }
        this.define(stmt.name.lexeme, {
            name: stmt.name.lexeme,
            kind: "variable",
            token: stmt.name,
        });
    }

    private analyzeFunctionStmt(stmt: FunctionStmt): void {
        const params = stmt.params;
        this.define(stmt.name.lexeme, {
            name: stmt.name.lexeme,
            kind: "function",
            token: stmt.name,
            params,
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
        });

        if (stmt.superclass) {
            // Resolve the superclass reference
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

        this.beginScope();
        // Define "this" inside the class scope
        this.define("this", {
            name: "this",
            kind: "variable",
            token: stmt.name,
        });

        for (const method of stmt.methods) {
            // Define the method as a method-kind symbol in class scope
            this.define(method.name.lexeme, {
                name: method.name.lexeme,
                kind: "method",
                token: method.name,
                params: method.params,
            });

            // Analyze the method body like a function
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
    }

    // --- Expression analysis ---

    private analyzeExpr(expr: Expr): void {
        switch (expr.kind) {
            case "binary":
                this.analyzeExpr(expr.left);
                this.analyzeExpr(expr.right);
                break;
            case "unary":
                this.analyzeExpr(expr.right);
                break;
            case "literal":
                // Nothing to analyze
                break;
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
                break;
            }
            case "grouping":
                this.analyzeExpr(expr.expression);
                break;
            case "logical":
                this.analyzeExpr(expr.left);
                this.analyzeExpr(expr.right);
                break;
            case "call":
                this.analyzeExpr(expr.callee);
                for (const arg of expr.args) {
                    this.analyzeExpr(arg);
                }
                break;
            case "assign": {
                const assignDef = this.resolve(expr.name.lexeme);
                this.references.push({ token: expr.name, definition: assignDef });
                this.analyzeExpr(expr.value);
                break;
            }
            case "get": {
                this.analyzeExpr(expr.object);
                const propName = expr.name.lexeme;
                const infos = PROPERTY_INDEX.get(propName);
                if (infos && infos.length > 0) {
                    // Pick the most specific match — prefer typed over generic "class"
                    const specific = infos.find(i => i.ownerType !== "class") ?? infos[0];
                    this.propertyRefs.push({ token: expr.name, info: specific });
                }
                break;
            }
            case "set":
                this.analyzeExpr(expr.object);
                this.analyzeExpr(expr.value);
                break;
            case "this":
                // Nothing extra to analyze — "this" is resolved in scope
                break;
            case "super":
                // Nothing extra to analyze
                break;
            case "array":
                for (const elem of expr.elements) {
                    this.analyzeExpr(elem);
                }
                break;
            case "map":
                for (const val of expr.values) {
                    this.analyzeExpr(val);
                }
                break;
            case "indexGet":
                this.analyzeExpr(expr.object);
                this.analyzeExpr(expr.index);
                break;
            case "indexSet":
                this.analyzeExpr(expr.object);
                this.analyzeExpr(expr.index);
                this.analyzeExpr(expr.value);
                break;
            case "lambda":
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
                break;
            case "pipe":
                this.analyzeExpr(expr.value);
                this.analyzeExpr(expr.func);
                break;
            case "compose":
                this.analyzeExpr(expr.left);
                this.analyzeExpr(expr.right);
                break;
        }
    }
}
