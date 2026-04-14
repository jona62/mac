// LSP helpers — position lookup, hover formatting

import { Position } from "vscode-languageserver/node";
import { Symbol, SymbolReference, PropertyReference, PropertyInfo, formatMacType } from "./types";

export function findReferenceAtPosition(
    refs: SymbolReference[],
    pos: Position
): SymbolReference | null {
    const line = pos.line + 1;
    const col = pos.character + 1;
    for (const ref of refs) {
        if (ref.token.line !== line) continue;
        const startCol = ref.token.column;
        const endCol = startCol + ref.token.lexeme.length;
        if (col >= startCol && col < endCol) return ref;
    }
    return null;
}

export function findSymbolAtPosition(
    symbols: Symbol[],
    pos: Position
): Symbol | null {
    const line = pos.line + 1;
    const col = pos.character + 1;
    for (const sym of symbols) {
        if (sym.token.line !== line) continue;
        const startCol = sym.token.column;
        const endCol = startCol + sym.name.length;
        if (col >= startCol && col < endCol) return sym;
    }
    return null;
}

export function findPropertyAtPosition(
    propRefs: PropertyReference[],
    pos: Position
): PropertyReference | null {
    const line = pos.line + 1;
    const col = pos.character + 1;
    for (const pr of propRefs) {
        if (pr.token.line !== line) continue;
        const startCol = pr.token.column;
        const endCol = startCol + pr.token.lexeme.length;
        if (col >= startCol && col < endCol) return pr;
    }
    return null;
}

export function formatSymbolHover(sym: Symbol): string {
    const desc = sym.description ? `\n\n${sym.description}` : "";
    switch (sym.kind) {
        case "native": {
            const args = sym.params ? sym.params.map((p) => p.lexeme).join(", ") : "";
            return `\`\`\`mac\n${sym.name}(${args})\n\`\`\`${desc}`;
        }
        case "function":
        case "method": {
            const params = sym.params ? sym.params.map((p) => p.lexeme).join(", ") : "";
            return `\`\`\`mac\nfun ${sym.name}(${params})\n\`\`\`${desc}`;
        }
        case "class": {
            const params = sym.params ? sym.params.map((p) => p.lexeme).join(", ") : "";
            const sig = params ? `${sym.name}(${params})` : sym.name;
            return `\`\`\`mac\nclass ${sig}\n\`\`\`${desc}`;
        }
        case "variable":
        case "parameter":
        default: {
            const typeStr = sym.type && sym.type.tag !== "unknown"
                ? `: ${formatMacType(sym.type)}` : "";
            const prefix = sym.kind === "parameter" ? "param" : "var";
            return `\`\`\`mac\n${prefix} ${sym.name}${typeStr}\n\`\`\`${desc}`;
        }
    }
}

export function formatPropertyHover(info: PropertyInfo): string {
    if (info.kind === "method") {
        const params = info.params?.join(", ") ?? "";
        return `\`\`\`mac\n.${info.name}(${params})\n\`\`\`\n\n*${info.ownerType}* — ${info.description}`;
    }
    return `\`\`\`mac\n.${info.name}\n\`\`\`\n\n*${info.ownerType}* — ${info.description}`;
}
