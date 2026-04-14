// Mac Language LSP Server

import {
    createConnection,
    ProposedFeatures,
    TextDocuments,
    TextDocumentSyncKind,
    InitializeParams,
    InitializeResult,
    CompletionItem,
    CompletionItemKind,
    Hover,
    DefinitionParams,
    HoverParams,
    CompletionParams,
    Diagnostic as LspDiagnostic,
    DiagnosticSeverity as LspDiagnosticSeverity,
    Location,
    Position,
    Range,
    InlayHint,
    InlayHintKind,
    InlayHintParams,
} from "vscode-languageserver/node";

import { TextDocument } from "vscode-languageserver-textdocument";

import { Scanner, ScanError } from "./scanner";
import { Parser } from "./parser";
import { ParseError } from "./ast";
import { Analyzer } from "./analyzer";
import { AnalysisResult, formatMacType } from "./types";
import {
    findReferenceAtPosition, findSymbolAtPosition, findPropertyAtPosition,
    formatSymbolHover, formatPropertyHover,
} from "./lsp-helpers";

// ============================================================
// Setup
// ============================================================

const connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments(TextDocument);

// Cache analysis results by document URI
const analysisCache = new Map<string, AnalysisResult>();

// ============================================================
// Initialization
// ============================================================

connection.onInitialize((_params: InitializeParams): InitializeResult => {
    return {
        capabilities: {
            textDocumentSync: TextDocumentSyncKind.Full,
            completionProvider: {
                triggerCharacters: ["."],
            },
            hoverProvider: true,
            definitionProvider: true,
            inlayHintProvider: true,
        },
    };
});

// ============================================================
// Document change — run pipeline and publish diagnostics
// ============================================================

documents.onDidChangeContent((change) => {
    const document = change.document;
    const text = document.getText();
    const uri = document.uri;

    // Run Scanner
    const scanner = new Scanner(text);
    const { tokens, errors: scanErrors } = scanner.scanTokens();

    // Run Parser
    const parser = new Parser(tokens);
    const { statements, errors: parseErrors } = parser.parse();

    // Run Analyzer
    const analyzer = new Analyzer();
    const result = analyzer.analyze(statements);

    // Cache result
    analysisCache.set(uri, result);

    // Convert all diagnostics to LSP format
    const lspDiagnostics: LspDiagnostic[] = [];

    // Scan errors
    for (const err of scanErrors) {
        lspDiagnostics.push(scanErrorToDiagnostic(err));
    }

    // Parse errors
    for (const err of parseErrors) {
        lspDiagnostics.push(parseErrorToDiagnostic(err));
    }

    // Analyzer diagnostics
    for (const diag of result.diagnostics) {
        lspDiagnostics.push(analyzerDiagnosticToLsp(diag));
    }

    connection.sendDiagnostics({ uri, diagnostics: lspDiagnostics });
});

function scanErrorToDiagnostic(err: ScanError): LspDiagnostic {
    const line = Math.max(0, err.line - 1);
    const col = Math.max(0, err.column - 1);
    return {
        severity: LspDiagnosticSeverity.Error,
        range: Range.create(line, col, line, col + 1),
        message: err.message,
        source: "mac",
    };
}

function parseErrorToDiagnostic(err: ParseError): LspDiagnostic {
    const line = Math.max(0, err.token.line - 1);
    const col = Math.max(0, err.token.column - 1);
    const endCol = col + Math.max(1, err.token.lexeme.length);
    return {
        severity: LspDiagnosticSeverity.Error,
        range: Range.create(line, col, line, endCol),
        message: err.message,
        source: "mac",
    };
}

function analyzerDiagnosticToLsp(
    diag: import("./analyzer").Diagnostic
): LspDiagnostic {
    const line = Math.max(0, diag.line - 1);
    const col = Math.max(0, diag.column - 1);
    const endCol = Math.max(0, diag.endColumn - 1);
    return {
        severity:
            diag.severity === "error"
                ? LspDiagnosticSeverity.Error
                : LspDiagnosticSeverity.Warning,
        range: Range.create(line, col, line, endCol),
        message: diag.message,
        source: "mac",
    };
}

// ============================================================
// Go to definition
// ============================================================

connection.onDefinition((params: DefinitionParams): Location | null => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return null;

    const ref = findReferenceAtPosition(result.references, params.position);
    if (!ref || !ref.definition) return null;

    // Skip native functions (line 0 means synthetic / built-in)
    if (ref.definition.token.line <= 0) return null;

    const defLine = ref.definition.token.line - 1;
    const defCol = ref.definition.token.column - 1;
    const defEndCol = defCol + ref.definition.name.length;

    return Location.create(
        params.textDocument.uri,
        Range.create(defLine, defCol, defLine, defEndCol)
    );
});

// ============================================================
// Hover
// ============================================================

connection.onHover((params: HoverParams): Hover | null => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return null;

    // Try to find a reference at the cursor position
    const ref = findReferenceAtPosition(result.references, params.position);
    if (ref && ref.definition) {
        return {
            contents: {
                kind: "markdown",
                value: formatSymbolHover(ref.definition),
            },
        };
    }

    // Try to find a symbol definition at the cursor position
    const sym = findSymbolAtPosition(result.symbols, params.position);
    if (sym) {
        return {
            contents: {
                kind: "markdown",
                value: formatSymbolHover(sym),
            },
        };
    }

    // Try to find a property reference at the cursor position
    const propRef = findPropertyAtPosition(result.propertyRefs, params.position);
    if (propRef) {
        return {
            contents: {
                kind: "markdown",
                value: formatPropertyHover(propRef.info),
            },
        };
    }

    return null;
});

// ============================================================
// Inlay Hints (Rust-style inline type annotations)
// ============================================================

connection.languages.inlayHint.on((params: InlayHintParams): InlayHint[] => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return [];

    const hints: InlayHint[] = [];
    const startLine = params.range.start.line + 1; // tokens are 1-based
    const endLine = params.range.end.line + 1;

    for (const sym of result.symbols) {
        // Only show hints for user-defined variables (not params, classes, methods)
        if (sym.kind !== "variable") continue;
        if (sym.token.line < startLine || sym.token.line > endLine) continue;
        if (sym.token.line <= 0) continue; // skip native/prelude symbols
        if (!sym.type || sym.type.tag === "unknown") continue;

        // For composed functions, show the pipeline instead of fun(1)
        const typeStr = (sym.type.tag === "function" && sym.description)
            ? sym.description
            : formatMacType(sym.type);
        const position = Position.create(
            sym.token.line - 1,
            sym.token.column - 1 + sym.name.length
        );

        hints.push({
            position,
            label: `: ${typeStr}`,
            kind: InlayHintKind.Type,
            paddingLeft: false,
            paddingRight: true,
        });
    }

    return hints;
});

// ============================================================
// Completion
// ============================================================

connection.onCompletion((params: CompletionParams): CompletionItem[] => {
    const items: CompletionItem[] = [];

    // Keywords
    const keywords = [
        "and", "break", "class", "continue", "else", "false", "fun",
        "for", "if", "in", "nil", "or", "print", "return", "super",
        "this", "true", "var", "while",
    ];
    for (const kw of keywords) {
        items.push({
            label: kw,
            kind: CompletionItemKind.Keyword,
            detail: "keyword",
        });
    }

    // Symbols from analysis
    const result = analysisCache.get(params.textDocument.uri);
    if (result) {
        for (const sym of result.symbols) {
            // Hide internal/private symbols from completions
            if (sym.name.startsWith("_")) continue;

            let kind: CompletionItemKind;
            let detail: string;
            switch (sym.kind) {
                case "function":
                case "method":
                case "native":
                    kind = CompletionItemKind.Function;
                    detail = sym.kind === "native"
                        ? `native function`
                        : `function`;
                    break;
                case "class":
                    kind = CompletionItemKind.Class;
                    detail = "class";
                    break;
                case "variable":
                case "parameter":
                default:
                    kind = CompletionItemKind.Variable;
                    detail = sym.kind;
                    break;
            }
            items.push({
                label: sym.name,
                kind,
                detail,
            });
        }
    }

    // Dot-completion hints for known types
    const doc = documents.get(params.textDocument.uri);
    if (doc && params.context?.triggerCharacter === ".") {
        const memeProps = ["text", "save", "resize"];
        const gifProps = ["frame", "save"];
        const timelineProps = ["frame", "transition", "loop", "render", "save"];
        const templateProps = ["name", "path"];
        const sizeProps = ["width", "height"];
        const durationProps = ["ms"];
        const allDotProps = [
            ...memeProps.map(p => ({ label: p, detail: "Meme" })),
            ...gifProps.map(p => ({ label: p, detail: "Gif" })),
            ...timelineProps.map(p => ({ label: p, detail: "Timeline" })),
            ...templateProps.map(p => ({ label: p, detail: "Template" })),
            ...sizeProps.map(p => ({ label: p, detail: "Size" })),
            ...durationProps.map(p => ({ label: p, detail: "Duration" })),
            { label: "name", detail: "Position / Format" },
        ];
        for (const prop of allDotProps) {
            items.push({
                label: prop.label,
                kind: CompletionItemKind.Property,
                detail: prop.detail,
            });
        }
    }

    return items;
});

// ============================================================
// Start
// ============================================================

documents.listen(connection);
connection.listen();
