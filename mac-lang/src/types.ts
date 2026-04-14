// Mac Type System — type definitions and formatting

import { Token, TokenType } from "./scanner";

// ============================================================
// Mac types (for analyzer inference)
// ============================================================

export type MacType =
    | { tag: "number" }
    | { tag: "string" }
    | { tag: "bool" }
    | { tag: "nil" }
    | { tag: "array"; elementType: MacType }
    | { tag: "tuple"; elementTypes: MacType[] }
    | { tag: "map"; valueType: MacType }
    | { tag: "instance"; className: string }
    | { tag: "function"; paramCount: number }
    | { tag: "class"; className: string }
    | { tag: "unknown" };

export const T_NUMBER:  MacType = { tag: "number" };
export const T_STRING:  MacType = { tag: "string" };
export const T_BOOL:    MacType = { tag: "bool" };
export const T_NIL:     MacType = { tag: "nil" };
export const T_UNKNOWN: MacType = { tag: "unknown" };

export function formatMacType(t: MacType): string {
    switch (t.tag) {
        case "number": return "number";
        case "string": return "string";
        case "bool": return "bool";
        case "nil": return "nil";
        case "array": return `[${formatMacType(t.elementType)}]`;
        case "tuple": return `(${t.elementTypes.map(formatMacType).join(", ")})`;
        case "map": return `{${formatMacType(t.valueType)}}`;
        case "instance": return t.className;
        case "function": return `fun(${t.paramCount})`;
        case "class": return `class ${t.className}`;
        case "unknown": return "unknown";
    }
}

// ============================================================
// Symbol and analysis types
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
// Helpers
// ============================================================

export function nativeToken(name: string): Token {
    return {
        type: TokenType.IDENTIFIER,
        lexeme: name,
        literal: null,
        line: 0,
        column: 0,
    };
}
