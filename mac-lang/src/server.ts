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
} from "vscode-languageserver/node";

import { TextDocument } from "vscode-languageserver-textdocument";

import { Scanner, ScanError } from "./scanner";
import { Parser, ParseError } from "./parser";
import { Analyzer, AnalysisResult, Symbol, SymbolReference } from "./analyzer";

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

    return null;
});

function formatSymbolHover(sym: Symbol): string {
    switch (sym.kind) {
        case "native": {
            const args = sym.params
                ? sym.params.map((p) => p.lexeme).join(", ")
                : "";
            const desc = sym.description ?? "";
            return `**native** ${sym.name}(${args}) \u2014 ${desc}`;
        }
        case "function":
        case "method": {
            const params = sym.params
                ? sym.params.map((p) => p.lexeme).join(", ")
                : "";
            return `**fun** ${sym.name}(${params})`;
        }
        case "class": {
            return `**class** ${sym.name}`;
        }
        case "variable":
        case "parameter":
        default:
            return `**var** ${sym.name}`;
    }
}

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
        const memeProps = ["text", "save", "resize", "_top", "_bottom", "_template", "_width", "_height"];
        const gifProps = ["frame", "save"];
        const templateProps = ["name", "path"];
        const sizeProps = ["width", "height"];
        const durationProps = ["ms"];
        const allDotProps = [
            ...memeProps.map(p => ({ label: p, detail: "Meme" })),
            ...gifProps.map(p => ({ label: p, detail: "Gif" })),
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
// Helpers
// ============================================================

function findReferenceAtPosition(
    refs: SymbolReference[],
    pos: Position
): SymbolReference | null {
    // Position is 0-based; token line/column are 1-based
    const line = pos.line + 1;
    const col = pos.character + 1;

    for (const ref of refs) {
        if (ref.token.line !== line) continue;
        const startCol = ref.token.column;
        const endCol = startCol + ref.token.lexeme.length;
        if (col >= startCol && col < endCol) {
            return ref;
        }
    }
    return null;
}

function findSymbolAtPosition(
    symbols: Symbol[],
    pos: Position
): Symbol | null {
    const line = pos.line + 1;
    const col = pos.character + 1;

    for (const sym of symbols) {
        if (sym.token.line !== line) continue;
        const startCol = sym.token.column;
        const endCol = startCol + sym.name.length;
        if (col >= startCol && col < endCol) {
            return sym;
        }
    }
    return null;
}

// ============================================================
// Start
// ============================================================

documents.listen(connection);
connection.listen();
