import * as fs from "fs";
import * as os from "os";
import * as path from "path";
import * as cp from "child_process";

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
    DiagnosticSeverity as LspDiagnosticSeverity,
    Location,
    Position,
    Range,
    InlayHint,
    InlayHintKind,
    InlayHintParams,
    DocumentSymbol,
    SymbolKind,
    DocumentSymbolParams,
    FoldingRange,
    FoldingRangeKind,
    FoldingRangeParams,
    SemanticTokensParams,
    SemanticTokens,
    SemanticTokensLegend,
    SemanticTokensBuilder,
    SignatureHelp,
    SignatureHelpParams,
    SignatureInformation,
    ParameterInformation,
    ReferenceParams,
} from "vscode-languageserver/node";

import { TextDocument } from "vscode-languageserver-textdocument";

const connection = createConnection(ProposedFeatures.all);
const documents = new TextDocuments(TextDocument);

interface SymbolDef {
    name: string;
    kind: string;
    type: string;
    description: string;
    source: string;
    visibility: string;
    ownerType: string;
    line: number;
    col: number;
    endCol: number;
}

interface Reference {
    line: number;
    col: number;
    endCol: number;
    defLine: number;
    defCol: number;
    defEndCol: number;
    defName: string;
    source: string;
    defSource: string;
    defVisibility: string;
    defOwnerType: string;
}

interface AnalysisDiag {
    line: number;
    col: number;
    endCol: number;
    message: string;
    severity: string;
    source: string;
}

interface PropertyRef {
    line: number;
    col: number;
    endCol: number;
    defLine: number;
    defCol: number;
    defEndCol: number;
    name: string;
    ownerType: string;
    kind: string;
    type: string;
    description: string;
    source: string;
    defSource: string;
    visibility: string;
}

interface FoldRangeData {
    startLine: number;
    endLine: number;
    source: string;
}

interface SemanticTokenData {
    line: number;
    col: number;
    length: number;
    tokenType: string;
    source: string;
}

interface ParamHintData {
    line: number;
    col: number;
    name: string;
    source: string;
}

interface ChainHintData {
    line: number;
    endCol: number;
    type: string;
    source: string;
}

interface SignatureData {
    name: string;
    ownerType: string;
    kind: string;
    returnType: string;
    description: string;
    source: string;
    visibility: string;
    line: number;
    col: number;
    endCol: number;
    params: string[];
}

interface ClassMember {
    name: string;
    kind: string;
    type: string;
    returnType: string;
    description: string;
    source: string;
    visibility: string;
    line: number;
    col: number;
    endCol: number;
    params: string[];
}

interface ClassInfo {
    name: string;
    superclass: string;
    description: string;
    source: string;
    visibility: string;
    line: number;
    col: number;
    endCol: number;
    members: ClassMember[];
}

interface TemplateInfo {
    name: string;
    category: string;
    description: string;
}

interface AnalysisResult {
    symbols: SymbolDef[];
    references: Reference[];
    diagnostics: AnalysisDiag[];
    properties: PropertyRef[];
    foldingRanges: FoldRangeData[];
    semanticTokens: SemanticTokenData[];
    paramHints: ParamHintData[];
    chainHints: ChainHintData[];
    signatures: SignatureData[];
    classes: ClassInfo[];
    templates: TemplateInfo[];
}

interface CallContext {
    callee: string;
    activeParameter: number;
}

const analysisCache = new Map<string, AnalysisResult>();

function findPath(...candidates: string[]): string | null {
    for (const candidate of candidates) {
        if (fs.existsSync(candidate)) return candidate;
    }
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

const preludeUri = preludePath ? `file://${preludePath}` : null;

function runAnalysis(text: string): AnalysisResult | null {
    const tmpFile = path.join(os.tmpdir(), `mac-lsp-${Date.now()}.mac`);
    try {
        fs.writeFileSync(tmpFile, text);
        const out = cp.execFileSync(macBinary, ["--analyze", tmpFile], {
            timeout: 5000,
            encoding: "utf-8",
            // Run from project root so analyzer reads source stdlib/prelude.mac
            // (not the build copy which may be stale)
            cwd: path.resolve(path.dirname(macBinary), ".."),
        });
        return JSON.parse(out) as AnalysisResult;
    } catch {
        return null;
    } finally {
        try { fs.unlinkSync(tmpFile); } catch { /* noop */ }
    }
}

const TOKEN_TYPES = ["variable", "parameter", "function", "method", "class", "property", "keyword"];
const TOKEN_MODIFIERS: string[] = [];
const tokenLegend: SemanticTokensLegend = { tokenTypes: TOKEN_TYPES, tokenModifiers: TOKEN_MODIFIERS };

connection.onInitialize((_params: InitializeParams): InitializeResult => ({
    capabilities: {
        textDocumentSync: TextDocumentSyncKind.Full,
        completionProvider: { triggerCharacters: [".", "@"] },
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

documents.onDidChangeContent((change) => {
    refreshAnalysis(change.document);
});

documents.onDidOpen((change) => {
    refreshAnalysis(change.document);
});

documents.onDidClose((change) => {
    analysisCache.delete(change.document.uri);
    connection.sendDiagnostics({ uri: change.document.uri, diagnostics: [] });
});

connection.onDefinition((params: DefinitionParams): Location | null => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return null;

    const prop = findAt(result.properties.filter((item) => item.source === "user"), params.position);
    if (prop) {
        return locationForDefinition(params.textDocument.uri, prop.defSource, prop.defLine, prop.defCol, prop.defEndCol);
    }

    const ref = findAt(result.references.filter((item) => item.source === "user"), params.position);
    if (!ref) return null;
    return locationForDefinition(params.textDocument.uri, ref.defSource, ref.defLine, ref.defCol, ref.defEndCol);
});

connection.onHover((params: HoverParams): Hover | null => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return null;

    const prop = findAt(result.properties.filter((item) => item.source === "user"), params.position);
    if (prop && isVisibleToUser(prop.defSource, prop.visibility)) {
        return { contents: { kind: "markdown", value: formatPropertyHover(result, prop) } };
    }

    const ref = findAt(result.references.filter((item) => item.source === "user"), params.position);
    if (ref) {
        const def = findDefinitionSymbol(result, ref.defName, ref.defLine, ref.defSource, ref.defOwnerType);
        if (def && isVisibleToUser(def.source, def.visibility)) {
            return { contents: { kind: "markdown", value: formatSymbolHover(result, def) } };
        }
    }

    const sym = findAt(result.symbols.filter((item) => item.source === "user"), params.position);
    if (!sym) return null;
    return { contents: { kind: "markdown", value: formatSymbolHover(result, sym) } };
});

connection.languages.inlayHint.on((params: InlayHintParams): InlayHint[] => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return [];

    const hints: InlayHint[] = [];
    const startLine = params.range.start.line + 1;
    const endLine = params.range.end.line + 1;

    for (const sym of result.symbols) {
        if (sym.source !== "user") continue;
        if (sym.kind !== "variable" && sym.kind !== "parameter") continue;
        if (sym.line < startLine || sym.line > endLine || sym.line <= 0) continue;
        if (!sym.type || sym.type === "unknown") continue;
        const label = sym.type.includes("->") && sym.description ? `: ${sym.description}` : `: ${sym.type}`;
        hints.push({
            position: Position.create(sym.line - 1, sym.endCol - 1),
            label,
            kind: InlayHintKind.Type,
            paddingLeft: false,
            paddingRight: true,
        });
    }

    for (const hint of result.paramHints) {
        if (hint.source !== "user") continue;
        if (hint.line < startLine || hint.line > endLine) continue;
        hints.push({
            position: Position.create(hint.line - 1, hint.col - 1),
            label: `${hint.name}:`,
            kind: InlayHintKind.Parameter,
            paddingLeft: false,
            paddingRight: true,
        });
    }

    for (const hint of result.chainHints) {
        if (hint.source !== "user") continue;
        if (hint.line < startLine || hint.line > endLine) continue;
        hints.push({
            position: Position.create(hint.line - 1, hint.endCol - 1),
            label: `: ${hint.type}`,
            kind: InlayHintKind.Type,
            paddingLeft: true,
            paddingRight: false,
        });
    }

    return hints;
});

connection.onDocumentSymbol((params: DocumentSymbolParams): DocumentSymbol[] => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return [];

    const symbols: DocumentSymbol[] = [];
    const classNames = new Set<string>();

    for (const cls of result.classes.filter((item) => item.source === "user")) {
        classNames.add(cls.name);
        const children = cls.members
            .filter((member) => member.source === "user")
            .map((member) => ({
                name: member.name,
                kind: member.kind === "field" ? SymbolKind.Property : SymbolKind.Method,
                range: toRange(member.line, member.col, member.endCol),
                selectionRange: toRange(member.line, member.col, member.endCol),
                detail: member.kind === "field" ? member.type : signatureLabel(member.name, member.params, member.returnType),
            }));

        symbols.push({
            name: cls.name,
            kind: SymbolKind.Class,
            range: toRange(cls.line, cls.col, cls.endCol),
            selectionRange: toRange(cls.line, cls.col, cls.endCol),
            children,
        });
    }

    for (const sym of result.symbols) {
        if (sym.source !== "user" || sym.line <= 0) continue;
        if (sym.kind === "parameter" || sym.kind === "method" || sym.kind === "field") continue;
        if (sym.kind === "class" && classNames.has(sym.name)) continue;

        const kind = sym.kind === "function" ? SymbolKind.Function : SymbolKind.Variable;
        symbols.push({
            name: sym.name,
            kind,
            range: toRange(sym.line, sym.col, sym.endCol),
            selectionRange: toRange(sym.line, sym.col, sym.endCol),
            detail: sym.type !== "unknown" ? sym.type : undefined,
        });
    }

    return symbols;
});

connection.onFoldingRanges((params: FoldingRangeParams): FoldingRange[] => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return [];
    return result.foldingRanges
        .filter((range) => range.source === "user")
        .map((range) => ({
            startLine: range.startLine - 1,
            endLine: range.endLine - 1,
            kind: FoldingRangeKind.Region,
        }));
});

connection.onReferences((params: ReferenceParams): Location[] => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return [];

    const prop = findAt(result.properties.filter((item) => item.source === "user"), params.position);
    if (prop) {
        return result.properties
            .filter((item) =>
                item.source === "user" &&
                item.name === prop.name &&
                item.ownerType === prop.ownerType &&
                item.defSource === prop.defSource &&
                item.defLine === prop.defLine &&
                item.defCol === prop.defCol)
            .map((item) => Location.create(params.textDocument.uri, toRange(item.line, item.col, item.endCol)));
    }

    const ref = findAt(result.references.filter((item) => item.source === "user"), params.position);
    const def = ref ? {
        name: ref.defName,
        line: ref.defLine,
        source: ref.defSource,
        ownerType: ref.defOwnerType,
    } : (() => {
        const sym = findAt(result.symbols.filter((item) => item.source === "user"), params.position);
        if (!sym) return null;
        return { name: sym.name, line: sym.line, source: sym.source, ownerType: sym.ownerType };
    })();

    if (!def) return [];
    return result.references
        .filter((item) =>
            item.source === "user" &&
            item.defName === def.name &&
            item.defLine === def.line &&
            item.defSource === def.source &&
            item.defOwnerType === def.ownerType)
        .map((item) => Location.create(params.textDocument.uri, toRange(item.line, item.col, item.endCol)));
});

connection.languages.semanticTokens.on((params: SemanticTokensParams): SemanticTokens => {
    const result = analysisCache.get(params.textDocument.uri);
    if (!result) return { data: [] };

    const builder = new SemanticTokensBuilder();
    const sorted = [...result.semanticTokens]
        .filter((token) => token.source === "user")
        .sort((a, b) => a.line - b.line || a.col - b.col);

    for (const token of sorted) {
        const tokenIndex = TOKEN_TYPES.indexOf(token.tokenType);
        if (tokenIndex < 0) continue;
        builder.push(token.line - 1, token.col - 1, token.length, tokenIndex, 0);
    }

    return builder.build();
});

connection.onSignatureHelp((params: SignatureHelpParams): SignatureHelp | null => {
    const result = analysisCache.get(params.textDocument.uri);
    const doc = documents.get(params.textDocument.uri);
    if (!result || !doc) return null;

    const context = getCallContext(doc, params.position);
    if (!context) return null;

    const signatures = resolveSignaturesForCallee(context.callee, result)
        .filter((sig) => isVisibleToUser(sig.source, sig.visibility));

    if (!signatures.length) return null;

    const items: SignatureInformation[] = signatures.map((sig) => ({
        label: signatureLabel(displaySignatureName(sig), sig.params, sig.returnType),
        parameters: sig.params.map((param) => ParameterInformation.create(param)),
        documentation: sig.description || undefined,
    }));

    let activeSignature = signatures.findIndex((sig) => context.activeParameter < sig.params.length);
    if (activeSignature < 0) activeSignature = 0;

    return {
        signatures: items,
        activeSignature,
        activeParameter: Math.max(0, context.activeParameter),
    };
});

connection.onCompletion((params: CompletionParams): CompletionItem[] => {
    const items: CompletionItem[] = [];
    const seen = new Set<string>();

    for (const keyword of ["and", "break", "class", "continue", "else", "false", "fun", "for", "if", "in", "nil", "or", "print", "return", "super", "this", "true", "var", "while"]) {
        pushCompletion(items, seen, {
            label: keyword,
            kind: CompletionItemKind.Keyword,
            detail: "keyword",
        });
    }

    const result = analysisCache.get(params.textDocument.uri);
    const doc = documents.get(params.textDocument.uri);
    if (!result || !doc) return items;

    // Template completions after @ or @category.
    const templates = result.templates || [];
    if (templates.length > 0) {
        const lineStart = Position.create(params.position.line, 0);
        const prefix = doc.getText(Range.create(lineStart, params.position));

        if (params.context?.triggerCharacter === "@") {
            // After @: show built-in templates + category names
            const categories = new Set<string>();
            for (const t of templates) {
                if (t.category) {
                    categories.add(t.category);
                } else {
                    pushCompletion(items, seen, {
                        label: t.name,
                        kind: CompletionItemKind.Constant,
                        detail: t.description,
                    });
                }
            }
            for (const cat of categories) {
                pushCompletion(items, seen, {
                    label: cat,
                    kind: CompletionItemKind.Module,
                    detail: `Template category`,
                });
            }
            return items;
        }

        // After @category. : show templates in that category
        const catMatch = prefix.match(/@(\w+)\.$/);
        if (catMatch && params.context?.triggerCharacter === ".") {
            const category = catMatch[1];
            for (const t of templates) {
                if (t.category !== category) continue;
                const shortName = t.name.includes(".") ? t.name.split(".").pop()! : t.name;
                pushCompletion(items, seen, {
                    label: shortName,
                    kind: CompletionItemKind.Constant,
                    detail: t.description,
                });
            }
            return items;
        }
    }

    if (params.context?.triggerCharacter === ".") {
        const ownerType = resolveCompletionOwnerType(doc, params.position, result);
        if (!ownerType) return items;

        for (const member of collectMembers(result, ownerType)) {
            if (!isVisibleToUser(member.source, member.visibility)) continue;
            const detail = member.kind === "field"
                ? member.type || "property"
                : signatureLabel(member.name, member.params, member.returnType);
            pushCompletion(items, seen, {
                label: member.name,
                kind: member.kind === "field" ? CompletionItemKind.Property : CompletionItemKind.Method,
                detail,
                documentation: member.description || undefined,
            });
        }
        return items;
    }

    for (const sym of result.symbols) {
        if (sym.ownerType) continue;
        if (sym.kind === "parameter" || sym.kind === "method" || sym.kind === "field") continue;
        if (!isVisibleToUser(sym.source, sym.visibility)) continue;
        const kind = sym.kind === "class"
            ? CompletionItemKind.Class
            : sym.kind === "function" || sym.kind === "native"
                ? CompletionItemKind.Function
                : CompletionItemKind.Variable;
        pushCompletion(items, seen, {
            label: sym.name,
            kind,
            detail: sym.kind,
            documentation: sym.description || undefined,
        });
    }

    return items;
});

function locationForDefinition(docUri: string, source: string, line: number, col: number, endCol: number): Location | null {
    if (source === "user" && line > 0) return Location.create(docUri, toRange(line, col, endCol));
    if (source === "prelude" && preludeUri && line > 0) return Location.create(preludeUri, toRange(line, col, endCol));
    return null;
}

function refreshAnalysis(doc: TextDocument): void {
    const result = runAnalysis(doc.getText());
    if (!result) {
        connection.sendDiagnostics({ uri: doc.uri, diagnostics: [] });
        return;
    }

    analysisCache.set(doc.uri, result);
    const diagnostics = result.diagnostics
        .filter((diag) => diag.source === "user")
        .map((diag) => ({
            severity: diag.severity === "error" ? LspDiagnosticSeverity.Error : LspDiagnosticSeverity.Warning,
            range: toRange(diag.line, diag.col, diag.endCol),
            message: diag.message,
            source: "mac",
        }));

    connection.sendDiagnostics({ uri: doc.uri, diagnostics });
}

function toRange(line: number, col: number, endCol: number): Range {
    return Range.create(Math.max(0, line - 1), Math.max(0, col - 1), Math.max(0, line - 1), Math.max(0, endCol - 1));
}

function findAt<T extends { line: number; col: number; endCol: number }>(items: T[], position: Position): T | null {
    const line = position.line + 1;
    const col = position.character + 1;
    for (const item of items) {
        if (item.line === line && col >= item.col && col < item.endCol) return item;
    }
    return null;
}

function isVisibleToUser(source: string, visibility: string): boolean {
    return source === "user" || visibility === "public";
}

function findDefinitionSymbol(result: AnalysisResult, name: string, line: number, source: string, ownerType: string): SymbolDef | null {
    return result.symbols.find((sym) =>
        sym.name === name &&
        sym.line === line &&
        sym.source === source &&
        sym.ownerType === ownerType) ?? result.symbols.find((sym) =>
        sym.name === name &&
        sym.line === line &&
        sym.source === source) ?? null;
}

function findClass(result: AnalysisResult, name: string): ClassInfo | null {
    return result.classes.find((cls) => cls.name === name) ?? null;
}

function findMember(result: AnalysisResult, ownerType: string, name: string): ClassMember | null {
    const seen = new Set<string>();
    let current: string | null = ownerType;
    while (current && !seen.has(current)) {
        seen.add(current);
        const cls = findClass(result, current);
        if (!cls) return null;
        const member = cls.members.find((item) => item.name === name);
        if (member) return member;
        current = cls.superclass || null;
    }
    return null;
}

function collectMembers(result: AnalysisResult, ownerType: string): ClassMember[] {
    const members: ClassMember[] = [];
    const seenNames = new Set<string>();
    const seenTypes = new Set<string>();
    let current: string | null = ownerType;

    while (current && !seenTypes.has(current)) {
        seenTypes.add(current);
        const cls = findClass(result, current);
        if (!cls) break;
        for (const member of cls.members) {
            if (seenNames.has(member.name)) continue;
            seenNames.add(member.name);
            members.push(member);
        }
        current = cls.superclass || null;
    }

    return members;
}

function resolveTopLevelType(name: string, result: AnalysisResult): string | null {
    const sym = result.symbols.find((item) =>
        item.name === name &&
        !item.ownerType &&
        item.kind !== "parameter" &&
        item.kind !== "method" &&
        item.kind !== "field");

    if (sym) {
        if (sym.kind === "class") return sym.name;
        if (sym.type && sym.type !== "unknown" && !sym.type.startsWith("fun(")) return sym.type;
    }

    return findClass(result, name)?.name ?? null;
}

function resolveMemberType(result: AnalysisResult, ownerType: string, name: string): string | null {
    const member = findMember(result, ownerType, name);
    if (!member) return null;
    if (member.kind === "field") return member.type && member.type !== "unknown" ? member.type : null;
    if (member.kind === "constructor") return ownerType;
    if (member.returnType && member.returnType !== "unknown") return member.returnType;
    if (member.type && member.type !== "unknown") return member.type;
    return null;
}

function findMatchingSignatures(result: AnalysisResult, name: string, ownerType: string, argCount?: number): SignatureData[] {
    const exact = collectSignatures(result, name, ownerType)
        .filter((sig) => argCount === undefined || sig.params.length === argCount);
    if (exact.length) return exact;
    return collectSignatures(result, name, ownerType);
}

function collectSignatures(result: AnalysisResult, name: string, ownerType: string): SignatureData[] {
    const signatures: SignatureData[] = [];
    const seenTypes = new Set<string>();
    let current: string | null = ownerType;

    while (current && !seenTypes.has(current)) {
        seenTypes.add(current);
        signatures.push(...result.signatures.filter((sig) => sig.name === name && sig.ownerType === current));
        const cls = findClass(result, current);
        current = cls?.superclass || null;
    }

    return signatures;
}

function resolveInvocationType(segment: string, result: AnalysisResult): string | null {
    const invocation = parseInvocation(segment);
    if (invocation) {
        const classInfo = findClass(result, invocation.name);
        if (classInfo && isVisibleToUser(classInfo.source, classInfo.visibility)) return classInfo.name;

        const signatures = result.signatures.filter((sig) =>
            sig.name === invocation.name &&
            !sig.ownerType &&
            sig.params.length === invocation.argCount &&
            isVisibleToUser(sig.source, sig.visibility));
        if (signatures.length) return signatures[0].returnType || null;
        return null;
    }

    return resolveTopLevelType(segment.trim(), result);
}

function resolveExpressionType(expr: string, result: AnalysisResult): string | null {
    const segments = splitChain(expr.trim());
    if (!segments.length) return null;

    let currentType = resolveInvocationType(segments[0], result);
    if (!currentType) return null;

    for (let index = 1; index < segments.length; index++) {
        const segment = segments[index].trim();
        if (!currentType) return null;
        const ownerType = currentType;
        const invocation = parseInvocation(segment);
        if (invocation) {
            const signatures: SignatureData[] = findMatchingSignatures(result, invocation.name, ownerType, invocation.argCount)
                .filter((sig) => isVisibleToUser(sig.source, sig.visibility));
            if (signatures.length) {
                currentType = signatures[0].returnType || null;
                continue;
            }
            currentType = resolveMemberType(result, ownerType, invocation.name);
            if (!currentType) return null;
            continue;
        }

        currentType = resolveMemberType(result, ownerType, segment);
        if (!currentType) return null;
    }

    return currentType;
}

function splitChain(expr: string): string[] {
    const parts: string[] = [];
    let current = "";
    let parenDepth = 0;
    let bracketDepth = 0;
    let braceDepth = 0;

    for (const char of expr) {
        if (char === "." && parenDepth === 0 && bracketDepth === 0 && braceDepth === 0) {
            if (current.trim()) parts.push(current.trim());
            current = "";
            continue;
        }

        current += char;
        if (char === "(") parenDepth++;
        else if (char === ")") parenDepth = Math.max(0, parenDepth - 1);
        else if (char === "[") bracketDepth++;
        else if (char === "]") bracketDepth = Math.max(0, bracketDepth - 1);
        else if (char === "{") braceDepth++;
        else if (char === "}") braceDepth = Math.max(0, braceDepth - 1);
    }

    if (current.trim()) parts.push(current.trim());
    return parts;
}

function parseInvocation(segment: string): { name: string; argCount: number } | null {
    const match = segment.trim().match(/^([A-Za-z_]\w*)\s*\((.*)\)$/s);
    if (!match) return null;
    return {
        name: match[1],
        argCount: countArguments(match[2]),
    };
}

function countArguments(argText: string): number {
    if (!argText.trim()) return 0;
    let count = 1;
    let parenDepth = 0;
    let bracketDepth = 0;
    let braceDepth = 0;

    for (const char of argText) {
        if (char === "(") parenDepth++;
        else if (char === ")") parenDepth = Math.max(0, parenDepth - 1);
        else if (char === "[") bracketDepth++;
        else if (char === "]") bracketDepth = Math.max(0, bracketDepth - 1);
        else if (char === "{") braceDepth++;
        else if (char === "}") braceDepth = Math.max(0, braceDepth - 1);
        else if (char === "," && parenDepth === 0 && bracketDepth === 0 && braceDepth === 0) count++;
    }

    return count;
}

function getCallContext(doc: TextDocument, position: Position): CallContext | null {
    const offset = doc.offsetAt(position);
    const text = doc.getText().slice(Math.max(0, offset - 4000), offset);

    let parenDepth = 0;
    let bracketDepth = 0;
    let braceDepth = 0;
    let openIndex = -1;

    for (let index = text.length - 1; index >= 0; index--) {
        const char = text[index];
        if (char === ")") parenDepth++;
        else if (char === "(") {
            if (parenDepth === 0 && bracketDepth === 0 && braceDepth === 0) {
                openIndex = index;
                break;
            }
            parenDepth = Math.max(0, parenDepth - 1);
        } else if (char === "]") bracketDepth++;
        else if (char === "[") bracketDepth = Math.max(0, bracketDepth - 1);
        else if (char === "}") braceDepth++;
        else if (char === "{") braceDepth = Math.max(0, braceDepth - 1);
    }

    if (openIndex < 0) return null;

    const before = text.slice(0, openIndex).trimEnd();
    const calleeMatch = before.match(/([A-Za-z_]\w*(?:\([^()]*\))?(?:\.[A-Za-z_]\w*(?:\([^()]*\))?)*)$/);
    if (!calleeMatch) return null;

    return {
        callee: calleeMatch[1],
        activeParameter: countArguments(text.slice(openIndex + 1)) - 1,
    };
}

function resolveSignaturesForCallee(callee: string, result: AnalysisResult): SignatureData[] {
    const parts = splitChain(callee);
    if (!parts.length) return [];

    if (parts.length === 1) {
        const name = parseInvocation(parts[0])?.name ?? parts[0].trim();
        return result.signatures
            .filter((sig) => sig.name === name && !sig.ownerType)
            .sort((left, right) => left.params.length - right.params.length);
    }

    const memberPart = parts[parts.length - 1];
    const ownerExpr = parts.slice(0, -1).join(".");
    const ownerType = resolveExpressionType(ownerExpr, result);
    if (!ownerType) return [];

    const name = parseInvocation(memberPart)?.name ?? memberPart.trim();
    return findMatchingSignatures(result, name, ownerType)
        .sort((left, right) => left.params.length - right.params.length);
}

function resolveCompletionOwnerType(doc: TextDocument, position: Position, result: AnalysisResult): string | null {
    const lineStart = Position.create(position.line, 0);
    const prefix = doc.getText(Range.create(lineStart, position));
    const match = prefix.match(/(.+)\.$/);
    if (!match) return null;
    return resolveExpressionType(match[1].trim(), result);
}

function displaySignatureName(sig: SignatureData): string {
    if (sig.kind === "constructor") return sig.ownerType || sig.name;
    return sig.ownerType ? `${sig.ownerType}.${sig.name}` : sig.name;
}

function signatureLabel(name: string, params: string[], returnType: string): string {
    return `${name}(${params.join(", ")})${returnType ? ` -> ${returnType}` : ""}`;
}

function formatSignatureBlocks(signatures: SignatureData[]): string {
    return signatures.map((sig) => {
        const desc = sig.description ? `\n\n${sig.description}` : "";
        return `\`\`\`mac\n${signatureLabel(displaySignatureName(sig), sig.params, sig.returnType)}\n\`\`\`${desc}`;
    }).join("\n\n");
}

function formatSymbolHover(result: AnalysisResult, sym: SymbolDef): string {
    if (sym.kind === "class") {
        return `\`\`\`mac\nclass ${sym.name}\n\`\`\`${sym.description ? `\n\n${sym.description}` : ""}`;
    }

    if (sym.kind === "keyword") {
        const type = sym.type && sym.type !== "unknown" ? ` → ${sym.type}` : "";
        return `\`\`\`mac\n${sym.name}${type}\n\`\`\`${sym.description ? `\n\n${sym.description}` : ""}`;
    }

    if (sym.kind === "function" || sym.kind === "native" || sym.kind === "method") {
        const signatures = result.signatures.filter((sig) =>
            sig.name === sym.name &&
            sig.ownerType === sym.ownerType &&
            (sig.source === sym.source || sig.line === sym.line));
        if (signatures.length) return formatSignatureBlocks(signatures.filter((sig) => isVisibleToUser(sig.source, sig.visibility)));
    }

    const type = sym.type && sym.type !== "unknown" ? `: ${sym.type}` : "";
    return `\`\`\`mac\nvar ${sym.name}${type}\n\`\`\`${sym.description ? `\n\n${sym.description}` : ""}`;
}

function formatPropertyHover(result: AnalysisResult, prop: PropertyRef): string {
    const member = findMember(result, prop.ownerType, prop.name);
    if (member && (member.kind === "method" || member.kind === "constructor")) {
        const signatures = findMatchingSignatures(result, member.name, prop.ownerType)
            .filter((sig) => isVisibleToUser(sig.source, sig.visibility));
        if (signatures.length) return formatSignatureBlocks(signatures);
    }

    const type = prop.type && prop.type !== "unknown" ? `: ${prop.type}` : "";
    const desc = prop.description ? `\n\n${prop.description}` : "";
    return `\`\`\`mac\n${prop.ownerType}.${prop.name}${type}\n\`\`\`${desc}`;
}

function pushCompletion(items: CompletionItem[], seen: Set<string>, item: CompletionItem): void {
    if (seen.has(item.label)) return;
    seen.add(item.label);
    items.push(item);
}

documents.listen(connection);
connection.listen();
