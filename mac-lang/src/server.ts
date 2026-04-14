// Mac LSP Server — thin adapter over `mac --analyze`

import * as fs from "fs";
import * as path from "path";
import * as cp from "child_process";
import * as os from "os";

import {
    createConnection, ProposedFeatures, TextDocuments, TextDocumentSyncKind,
    InitializeParams, InitializeResult,
    CompletionItem, CompletionItemKind,
    Hover, DefinitionParams, HoverParams, CompletionParams,
    Diagnostic as LspDiagnostic, DiagnosticSeverity as LspDiagnosticSeverity,
    Location, Position, Range,
    InlayHint, InlayHintKind, InlayHintParams,
} from "vscode-languageserver/node";

import { TextDocument } from "vscode-languageserver-textdocument";

const connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments(TextDocument);

// --- Analysis result types (from mac --analyze JSON) ---

interface SymbolDef {
    name: string; kind: string; line: number; col: number; endCol: number;
    type: string; description: string;
}
interface Reference {
    line: number; col: number; endCol: number;
    defLine: number; defCol: number; defName: string;
}
interface AnalysisDiag {
    line: number; col: number; endCol: number; message: string; severity: string;
}
interface PropertyRef {
    line: number; col: number; endCol: number;
    name: string; ownerType: string; kind: string; description: string;
}
interface AnalysisResult {
    symbols: SymbolDef[]; references: Reference[]; diagnostics: AnalysisDiag[];
    properties: PropertyRef[];
}

const analysisCache = new Map<string, AnalysisResult>();

// --- Find the mac binary ---

function findMacBinary(): string {
    const candidates = [
        path.resolve(__dirname, "../../build/mac"),
        path.resolve(__dirname, "../../../build/mac"),
    ];
    for (const p of candidates) {
        if (fs.existsSync(p)) return p;
    }
    return "mac"; // fallback to PATH
}

const macBinary = findMacBinary();

// --- Find prelude for go-to-definition ---

function findPrelude(): string | null {
    const candidates = [
        path.resolve(__dirname, "../../stdlib/prelude.mac"),
        path.resolve(__dirname, "../../../stdlib/prelude.mac"),
    ];
    for (const p of candidates) {
        if (fs.existsSync(p)) return p;
    }
    return null;
}

const preludePath = findPrelude();
const preludeLocations = new Map<string, { line: number; col: number }>();

if (preludePath) {
    const lines = fs.readFileSync(preludePath, "utf-8").split("\n");
    for (let i = 0; i < lines.length; i++) {
        const m = lines[i].match(/^(class|var|fun)\s+(\w+)/);
        if (m) preludeLocations.set(m[2], { line: i, col: lines[i].indexOf(m[2]) });
    }
}

// --- Run analyzer ---

function runAnalysis(text: string): AnalysisResult | null {
    const tmpFile = path.join(os.tmpdir(), `mac-lsp-${Date.now()}.mac`);
    try {
        fs.writeFileSync(tmpFile, text);
        const result = cp.execFileSync(macBinary, ["--analyze", tmpFile], {
            timeout: 5000, encoding: "utf-8", cwd: path.dirname(macBinary),
        });
        return JSON.parse(result);
    } catch {
        return null;
    } finally {
        try { fs.unlinkSync(tmpFile); } catch { /* ignore */ }
    }
}

// --- Initialization ---

connection.onInitialize((_params: InitializeParams): InitializeResult => ({
    capabilities: {
        textDocumentSync: TextDocumentSyncKind.Full,
        completionProvider: { triggerCharacters: ["."] },
        hoverProvider: true,
        definitionProvider: true,
        inlayHintProvider: true,
    },
}));

// --- Document change ---

documents.onDidChangeContent((change) => {
    const doc = change.document;
    const result = runAnalysis(doc.getText());
    if (!result) {
        analysisCache.delete(doc.uri);
        connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
        return;
    }

    analysisCache.set(doc.uri, result);

    const diagnostics: LspDiagnostic[] = result.diagnostics.map(d => ({
        severity: d.severity === "error" ? LspDiagnosticSeverity.Error : LspDiagnosticSeverity.Warning,
        range: Range.create(Math.max(0, d.line - 1), Math.max(0, d.col - 1),
                            Math.max(0, d.line - 1), Math.max(0, d.endCol - 1)),
        message: d.message,
        source: "mac",
    }));
    connection.sendDiagnostics({ uri: doc.uri, diagnostics });
});

// --- Go to definition ---

connection.onDefinition((params: DefinitionParams): Location | null => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return null;

    const ref = findRefAt(result.references, params.position);
    if (!ref) return null;

    // Prelude symbol (line 0 = native/prelude)
    if (ref.defLine <= 0) {
        const loc = preludeLocations.get(ref.defName);
        if (loc && preludePath) {
            return Location.create("file://" + preludePath,
                Range.create(loc.line, loc.col, loc.line, loc.col + ref.defName.length));
        }
        return null;
    }

    return Location.create(params.textDocument.uri,
        Range.create(ref.defLine - 1, Math.max(0, ref.defCol - 1),
                     ref.defLine - 1, Math.max(0, ref.defCol - 1) + ref.defName.length));
});

// --- Hover ---

connection.onHover((params: HoverParams): Hover | null => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return null;

    // Check references first (usage → definition)
    const ref = findRefAt(result.references, params.position);
    if (ref) {
        const def = result.symbols.find(s => s.name === ref.defName && s.line === ref.defLine);
        if (def) return { contents: { kind: "markdown", value: formatHover(def) } };
        // Try native/prelude (line 0)
        const native = result.symbols.find(s => s.name === ref.defName && s.line <= 0);
        if (native) return { contents: { kind: "markdown", value: formatHover(native) } };
    }

    // Check symbol definitions
    const sym = findSymAt(result.symbols, params.position);
    if (sym) return { contents: { kind: "markdown", value: formatHover(sym) } };

    // Check property accesses (.text, .frame, .transition, etc.)
    const prop = findPropAt(result.properties, params.position);
    if (prop) return { contents: { kind: "markdown", value: formatPropHover(prop) } };

    return null;
});

// --- Inlay hints ---

connection.languages.inlayHint.on((params: InlayHintParams): InlayHint[] => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return [];

    const hints: InlayHint[] = [];
    const startLine = params.range.start.line + 1;
    const endLine = params.range.end.line + 1;

    for (const sym of result.symbols) {
        if (sym.kind !== "variable") continue;
        if (sym.line < startLine || sym.line > endLine) continue;
        if (sym.line <= 0) continue;
        if (!sym.type || sym.type === "unknown") continue;

        const label = (sym.type.includes("->") && sym.description)
            ? `: ${sym.description}` : `: ${sym.type}`;

        hints.push({
            position: Position.create(sym.line - 1, sym.endCol - 1),
            label,
            kind: InlayHintKind.Type,
            paddingLeft: false,
            paddingRight: true,
        });
    }
    return hints;
});

// --- Completion ---

connection.onCompletion((_params: CompletionParams): CompletionItem[] => {
    const items: CompletionItem[] = [];

    const keywords = [
        "and", "break", "class", "continue", "else", "false", "fun",
        "for", "if", "in", "nil", "or", "print", "return", "super",
        "this", "true", "var", "while",
    ];
    for (const kw of keywords) {
        items.push({ label: kw, kind: CompletionItemKind.Keyword, detail: "keyword" });
    }

    const result = analysisCache.get(_params.textDocument.uri);
    if (result) {
        for (const sym of result.symbols) {
            if (sym.name.startsWith("_")) continue;
            let kind: CompletionItemKind;
            switch (sym.kind) {
                case "function": case "method": case "native":
                    kind = CompletionItemKind.Function; break;
                case "class":
                    kind = CompletionItemKind.Class; break;
                default:
                    kind = CompletionItemKind.Variable; break;
            }
            items.push({ label: sym.name, kind, detail: sym.kind, documentation: sym.description || undefined });
        }
    }

    if (_params.context?.triggerCharacter === ".") {
        const props = [
            { l: "text", d: "Meme" }, { l: "save", d: "Meme/Gif/Timeline" }, { l: "resize", d: "Meme" },
            { l: "frame", d: "Gif/Timeline" }, { l: "transition", d: "Timeline" },
            { l: "loop", d: "Timeline" }, { l: "render", d: "Timeline" },
            { l: "name", d: "Template" }, { l: "path", d: "Template" },
            { l: "width", d: "Size" }, { l: "height", d: "Size" }, { l: "ms", d: "Duration" },
        ];
        for (const p of props) {
            items.push({ label: p.l, kind: CompletionItemKind.Property, detail: p.d });
        }
    }

    return items;
});

// --- Helpers ---

function findRefAt(refs: Reference[], pos: Position): Reference | null {
    const line = pos.line + 1, col = pos.character + 1;
    for (const r of refs) {
        if (r.line !== line) continue;
        if (col >= r.col && col < r.endCol) return r;
    }
    return null;
}

function findSymAt(syms: SymbolDef[], pos: Position): SymbolDef | null {
    const line = pos.line + 1, col = pos.character + 1;
    for (const s of syms) {
        if (s.line !== line) continue;
        if (col >= s.col && col < s.endCol) return s;
    }
    return null;
}

function findPropAt(props: PropertyRef[], pos: Position): PropertyRef | null {
    const line = pos.line + 1, col = pos.character + 1;
    for (const p of props) {
        if (p.line !== line) continue;
        if (col >= p.col && col < p.endCol) return p;
    }
    return null;
}

function formatPropHover(prop: PropertyRef): string {
    return `\`\`\`mac\n.${prop.name}\n\`\`\`\n\n*${prop.ownerType}* — ${prop.description}`;
}

function formatHover(sym: SymbolDef): string {
    const desc = sym.description ? `\n\n${sym.description}` : "";
    switch (sym.kind) {
        case "native": return `\`\`\`mac\n${sym.name}(...)\n\`\`\`${desc}`;
        case "function": case "method":
            return `\`\`\`mac\nfun ${sym.name}(...)\n\`\`\`${desc}`;
        case "class":
            return `\`\`\`mac\nclass ${sym.name}\n\`\`\`${desc}`;
        default: {
            const t = sym.type && sym.type !== "unknown" ? `: ${sym.type}` : "";
            return `\`\`\`mac\nvar ${sym.name}${t}\n\`\`\`${desc}`;
        }
    }
}

// --- Start ---

documents.listen(connection);
connection.listen();
