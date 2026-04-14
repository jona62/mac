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
    DocumentSymbol, SymbolKind, DocumentSymbolParams,
    FoldingRange, FoldingRangeKind, FoldingRangeParams,
    SemanticTokensParams, SemanticTokens, SemanticTokensLegend, SemanticTokensBuilder,
    SignatureHelp, SignatureHelpParams, SignatureInformation, ParameterInformation,
    ReferenceParams,
} from "vscode-languageserver/node";

import { TextDocument } from "vscode-languageserver-textdocument";

const connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments(TextDocument);

// --- Analysis types ---

interface SymbolDef { name: string; kind: string; line: number; col: number; endCol: number; type: string; description: string; }
interface Reference { line: number; col: number; endCol: number; defLine: number; defCol: number; defName: string; }
interface AnalysisDiag { line: number; col: number; endCol: number; message: string; severity: string; }
interface PropertyRef { line: number; col: number; endCol: number; name: string; ownerType: string; kind: string; description: string; }
interface FoldRangeData { startLine: number; endLine: number; }
interface SemanticTokenData { line: number; col: number; length: number; tokenType: string; }
interface ParamHintData { line: number; col: number; name: string; }
interface ChainHintData { line: number; endCol: number; type: string; }
interface SignatureData { name: string; returnType: string; description: string; params: string[]; }
interface AnalysisResult {
    symbols: SymbolDef[]; references: Reference[]; diagnostics: AnalysisDiag[];
    properties: PropertyRef[]; foldingRanges: FoldRangeData[];
    semanticTokens: SemanticTokenData[]; paramHints: ParamHintData[];
    chainHints: ChainHintData[]; signatures: SignatureData[];
}

const analysisCache = new Map<string, AnalysisResult>();

// --- Binary + prelude ---

function findPath(...candidates: string[]): string | null {
    for (const p of candidates) if (fs.existsSync(p)) return p;
    return null;
}

const macBinary = findPath(
    path.resolve(__dirname, "../../build/mac"),
    path.resolve(__dirname, "../../../build/mac"),
) ?? "mac";

const preludePath = findPath(
    path.resolve(__dirname, "../../stdlib/prelude.mac"),
    path.resolve(__dirname, "../../../stdlib/prelude.mac"),
);

const preludeLocations = new Map<string, { line: number; col: number }>();
if (preludePath) {
    const lines = fs.readFileSync(preludePath, "utf-8").split("\n");
    for (let i = 0; i < lines.length; i++) {
        const m = lines[i].match(/^(class|var|fun)\s+(\w+)/);
        if (m) preludeLocations.set(m[2], { line: i, col: lines[i].indexOf(m[2]) });
    }
}

// --- Analyzer ---

function runAnalysis(text: string): AnalysisResult | null {
    const tmpFile = path.join(os.tmpdir(), `mac-lsp-${Date.now()}.mac`);
    try {
        fs.writeFileSync(tmpFile, text);
        const out = cp.execFileSync(macBinary, ["--analyze", tmpFile], {
            timeout: 5000, encoding: "utf-8", cwd: path.dirname(macBinary),
        });
        return JSON.parse(out);
    } catch { return null; }
    finally { try { fs.unlinkSync(tmpFile); } catch { /* */ } }
}

// --- Semantic tokens legend ---

const TOKEN_TYPES = ["variable", "parameter", "function", "method", "class", "property"];
const TOKEN_MODIFIERS: string[] = [];
const tokenLegend: SemanticTokensLegend = { tokenTypes: TOKEN_TYPES, tokenModifiers: TOKEN_MODIFIERS };

// --- Initialization ---

connection.onInitialize((_params: InitializeParams): InitializeResult => ({
    capabilities: {
        textDocumentSync: TextDocumentSyncKind.Full,
        completionProvider: { triggerCharacters: ["."] },
        hoverProvider: true,
        definitionProvider: true,
        inlayHintProvider: true,
        documentSymbolProvider: true,
        foldingRangeProvider: true,
        referencesProvider: true,
        signatureHelpProvider: { triggerCharacters: ["(", ","] },
        semanticTokensProvider: { full: true, legend: tokenLegend },
    },
}));

// --- Document change ---

documents.onDidChangeContent((change) => {
    const doc = change.document;
    const result = runAnalysis(doc.getText());
    if (!result) { analysisCache.delete(doc.uri); connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] }); return; }
    analysisCache.set(doc.uri, result);
    connection.sendDiagnostics({ uri: doc.uri, diagnostics: result.diagnostics.map(d => ({
        severity: d.severity === "error" ? LspDiagnosticSeverity.Error : LspDiagnosticSeverity.Warning,
        range: Range.create(Math.max(0, d.line - 1), Math.max(0, d.col - 1), Math.max(0, d.line - 1), Math.max(0, d.endCol - 1)),
        message: d.message, source: "mac",
    }))});
});

// --- Go to definition ---

connection.onDefinition((params: DefinitionParams): Location | null => {
    const r = analysisCache.get(params.textDocument.uri);
    if (!r) return null;
    const ref = findAt(r.references, params.position);
    if (!ref) return null;
    if (ref.defLine <= 0) {
        const loc = preludeLocations.get(ref.defName);
        if (loc && preludePath) return Location.create("file://" + preludePath, Range.create(loc.line, loc.col, loc.line, loc.col + ref.defName.length));
        return null;
    }
    return Location.create(params.textDocument.uri, Range.create(ref.defLine - 1, Math.max(0, ref.defCol - 1), ref.defLine - 1, Math.max(0, ref.defCol - 1) + ref.defName.length));
});

// --- Hover ---

connection.onHover((params: HoverParams): Hover | null => {
    const r = analysisCache.get(params.textDocument.uri);
    if (!r) return null;
    const ref = findAt(r.references, params.position);
    if (ref) {
        const def = r.symbols.find(s => s.name === ref.defName && s.line === ref.defLine)
            ?? r.symbols.find(s => s.name === ref.defName && s.line <= 0);
        if (def) return { contents: { kind: "markdown", value: fmtHover(def) } };
    }
    const sym = findSym(r.symbols, params.position);
    if (sym) return { contents: { kind: "markdown", value: fmtHover(sym) } };
    const prop = findAt(r.properties, params.position);
    if (prop) return { contents: { kind: "markdown", value: `\`\`\`mac\n.${prop.name}\n\`\`\`\n\n*${prop.ownerType}* — ${prop.description}` } };
    return null;
});

// --- Inlay hints (type + param + chain) ---

connection.languages.inlayHint.on((params: InlayHintParams): InlayHint[] => {
    const r = analysisCache.get(params.textDocument.uri);
    if (!r) return [];
    const hints: InlayHint[] = [];
    const [sL, eL] = [params.range.start.line + 1, params.range.end.line + 1];

    // Type hints for variables
    for (const sym of r.symbols) {
        if (sym.kind !== "variable" || sym.line < sL || sym.line > eL || sym.line <= 0) continue;
        if (!sym.type || sym.type === "unknown") continue;
        const label = sym.type.includes("->") && sym.description ? `: ${sym.description}` : `: ${sym.type}`;
        hints.push({ position: Position.create(sym.line - 1, sym.endCol - 1), label, kind: InlayHintKind.Type, paddingLeft: false, paddingRight: true });
    }

    // Parameter name hints
    for (const h of r.paramHints) {
        if (h.line < sL || h.line > eL) continue;
        hints.push({ position: Position.create(h.line - 1, h.col - 1), label: `${h.name}:`, kind: InlayHintKind.Parameter, paddingLeft: false, paddingRight: true });
    }

    // Chain type hints
    for (const h of r.chainHints) {
        if (h.line < sL || h.line > eL) continue;
        hints.push({ position: Position.create(h.line - 1, h.endCol - 1), label: `: ${h.type}`, kind: InlayHintKind.Type, paddingLeft: true, paddingRight: false });
    }

    return hints;
});

// --- Document symbols (outline) ---

connection.onDocumentSymbol((params: DocumentSymbolParams): DocumentSymbol[] => {
    const r = analysisCache.get(params.textDocument.uri);
    if (!r) return [];
    const symbols: DocumentSymbol[] = [];
    for (const sym of r.symbols) {
        if (sym.line <= 0) continue;
        if (sym.kind === "parameter" || sym.kind === "method") continue;
        const kind = sym.kind === "class" ? SymbolKind.Class
            : sym.kind === "function" ? SymbolKind.Function : SymbolKind.Variable;
        const range = Range.create(sym.line - 1, Math.max(0, sym.col - 1), sym.line - 1, sym.endCol - 1);
        symbols.push({ name: sym.name, kind, range, selectionRange: range, detail: sym.type !== "unknown" ? sym.type : undefined });
    }
    return symbols;
});

// --- Folding ranges ---

connection.onFoldingRanges((params: FoldingRangeParams): FoldingRange[] => {
    const r = analysisCache.get(params.textDocument.uri);
    if (!r) return [];
    return (r.foldingRanges || []).map(f => ({ startLine: f.startLine - 1, endLine: f.endLine - 1, kind: FoldingRangeKind.Region }));
});

// --- Find all references ---

connection.onReferences((params: ReferenceParams): Location[] => {
    const r = analysisCache.get(params.textDocument.uri);
    if (!r) return [];
    // Find which symbol the cursor is on
    const ref = findAt(r.references, params.position);
    const sym = ref ? ref.defName : findSym(r.symbols, params.position)?.name;
    if (!sym) return [];
    const defLine = ref?.defLine ?? findSym(r.symbols, params.position)?.line ?? 0;
    return r.references.filter(rv => rv.defName === sym && rv.defLine === defLine)
        .map(rv => Location.create(params.textDocument.uri, Range.create(rv.line - 1, Math.max(0, rv.col - 1), rv.line - 1, rv.endCol - 1)));
});

// --- Semantic tokens ---

connection.languages.semanticTokens.on((params: SemanticTokensParams): SemanticTokens => {
    const r = analysisCache.get(params.textDocument.uri);
    if (!r) return { data: [] };
    const builder = new SemanticTokensBuilder();
    const sorted = [...(r.semanticTokens || [])].sort((a, b) => a.line - b.line || a.col - b.col);
    for (const t of sorted) {
        const idx = TOKEN_TYPES.indexOf(t.tokenType);
        if (idx < 0) continue;
        builder.push(t.line - 1, t.col - 1, t.length, idx, 0);
    }
    return builder.build();
});

// --- Signature help ---

connection.onSignatureHelp((params: SignatureHelpParams): SignatureHelp | null => {
    const r = analysisCache.get(params.textDocument.uri);
    if (!r) return null;
    const doc = documents.get(params.textDocument.uri);
    if (!doc) return null;
    // Find the function name before the cursor by scanning backward
    const text = doc.getText(Range.create(Position.create(params.position.line, 0), params.position));
    const match = text.match(/(\w+)\s*\([^)]*$/);
    if (!match) return null;
    const fnName = match[1];
    const sig = r.signatures.find(s => s.name === fnName);
    if (!sig) return null;
    const paramInfo: ParameterInformation[] = sig.params.map(p => ({ label: p }));
    const label = `${sig.name}(${sig.params.join(", ")})` + (sig.returnType ? ` → ${sig.returnType}` : "");
    const sigInfo: SignatureInformation = { label, parameters: paramInfo, documentation: sig.description || undefined };
    // Count commas to determine active parameter
    const afterParen = text.slice(text.lastIndexOf("(") + 1);
    let activeParam = 0;
    let depth = 0;
    for (const ch of afterParen) {
        if (ch === "(" || ch === "[") depth++;
        else if (ch === ")" || ch === "]") depth--;
        else if (ch === "," && depth === 0) activeParam++;
    }
    return { signatures: [sigInfo], activeSignature: 0, activeParameter: activeParam };
});

// --- Completion ---

connection.onCompletion((_params: CompletionParams): CompletionItem[] => {
    const items: CompletionItem[] = [];
    for (const kw of ["and","break","class","continue","else","false","fun","for","if","in","nil","or","print","return","super","this","true","var","while"]) {
        items.push({ label: kw, kind: CompletionItemKind.Keyword, detail: "keyword" });
    }
    const r = analysisCache.get(_params.textDocument.uri);
    if (r) {
        for (const sym of r.symbols) {
            if (sym.name.startsWith("_")) continue;
            const kind = sym.kind === "function" || sym.kind === "method" || sym.kind === "native" ? CompletionItemKind.Function
                : sym.kind === "class" ? CompletionItemKind.Class : CompletionItemKind.Variable;
            items.push({ label: sym.name, kind, detail: sym.kind, documentation: sym.description || undefined });
        }
    }
    if (_params.context?.triggerCharacter === ".") {
        for (const [l, d] of [["text","Meme"],["save","Meme/Gif/Timeline"],["resize","Meme"],["frame","Gif/Timeline"],["transition","Timeline"],["loop","Timeline"],["render","Timeline"],["name","Template"],["path","Template"],["width","Size"],["height","Size"],["ms","Duration"]]) {
            items.push({ label: l, kind: CompletionItemKind.Property, detail: d });
        }
    }
    return items;
});

// --- Helpers ---

function findAt<T extends { line: number; col: number; endCol: number }>(items: T[], pos: Position): T | null {
    const line = pos.line + 1, col = pos.character + 1;
    for (const item of items) { if (item.line === line && col >= item.col && col < item.endCol) return item; }
    return null;
}

function findSym(syms: SymbolDef[], pos: Position): SymbolDef | null {
    const line = pos.line + 1, col = pos.character + 1;
    for (const s of syms) { if (s.line === line && col >= s.col && col < s.endCol) return s; }
    return null;
}

function fmtHover(sym: SymbolDef): string {
    const desc = sym.description ? `\n\n${sym.description}` : "";
    if (sym.kind === "native") return `\`\`\`mac\n${sym.name}(...)\n\`\`\`${desc}`;
    if (sym.kind === "function" || sym.kind === "method") return `\`\`\`mac\nfun ${sym.name}(...)\n\`\`\`${desc}`;
    if (sym.kind === "class") return `\`\`\`mac\nclass ${sym.name}\n\`\`\`${desc}`;
    const t = sym.type && sym.type !== "unknown" ? `: ${sym.type}` : "";
    return `\`\`\`mac\nvar ${sym.name}${t}\n\`\`\`${desc}`;
}

// --- Start ---

documents.listen(connection);
connection.listen();
