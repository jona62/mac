// Mac Language Analyzer — scope-aware symbol table and diagnostic collector

import { Token } from "./scanner";
import { Expr, Stmt, FunctionStmt } from "./ast";
import {
    MacType, T_NUMBER, T_STRING, T_BOOL, T_NIL, T_UNKNOWN,
    Symbol, Scope, Diagnostic, SymbolReference, PropertyReference,
    AnalysisResult, SymbolKind, nativeToken, formatMacType,
} from "./types";
import {
    NATIVE_FUNCTIONS, KNOWN_PROPERTIES, PROPERTY_INDEX,
    NATIVE_RETURN_TYPES, METHOD_RETURN_TYPES, FIELD_TYPES,
    PRELUDE_TYPES,
} from "./registry";

// Re-export types for backward compatibility
export type { MacType, Symbol, Scope, Diagnostic, SymbolReference, PropertyInfo, PropertyReference, AnalysisResult } from "./types";
export { formatMacType } from "./types";

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

    // HOFs that take a callback and transform array elements
    private static readonly MAPPING_HOFS = new Set(["map", "flatMap"]);
    // HOFs that take a callback + initial value (fold)
    private static readonly FOLDING_HOFS = new Set(["reduce"]);

    constructor() {
        this.currentScope = { symbols: new Map(), parent: null };
        this.scopes.push(this.currentScope);
        this.nativeNames = new Set();

        for (const def of NATIVE_FUNCTIONS) {
            const params: Token[] = [];
            for (let i = 0; i < def.arity; i++) params.push(nativeToken(`arg${i}`));
            const sym: Symbol = {
                name: def.name, kind: "native", token: nativeToken(def.name),
                params, description: def.description,
                type: def.type ?? { tag: "function", paramCount: Math.max(0, def.arity) },
            };
            this.currentScope.symbols.set(def.name, sym);
            this.allSymbols.push(sym);
            this.nativeNames.add(def.name);
        }

        for (const def of PRELUDE_TYPES) {
            this.nativeNames.add(def.name);
            const params: Token[] = (def.params ?? []).map(p => nativeToken(p));
            const sym: Symbol = {
                name: def.name, kind: def.kind, token: nativeToken(def.name),
                params: params.length > 0 ? params : undefined,
                description: def.description, type: def.type,
            };
            this.currentScope.symbols.set(def.name, sym);
            this.allSymbols.push(sym);
        }
    }

    analyze(statements: Stmt[]): AnalysisResult {
        for (const stmt of statements) this.analyzeStmt(stmt);
        return {
            diagnostics: this.diagnostics, symbols: this.allSymbols,
            references: this.references, propertyRefs: this.propertyRefs,
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
        if (this.currentScope.parent) this.currentScope = this.currentScope.parent;
    }

    private define(name: string, sym: Symbol): void {
        this.currentScope.symbols.set(name, sym);
        this.allSymbols.push(sym);
    }

    private resolve(name: string): Symbol | null {
        let scope: Scope | null = this.currentScope;
        while (scope !== null) {
            const sym = scope.symbols.get(name);
            if (sym !== undefined) return sym;
            scope = scope.parent;
        }
        return null;
    }

    // --- Statement analysis ---

    private analyzeStmt(stmt: Stmt): void {
        switch (stmt.kind) {
            case "expression": this.analyzeExpr(stmt.expression); break;
            case "print": this.analyzeExpr(stmt.expression); break;
            case "var": this.analyzeVarStmt(stmt); break;
            case "block":
                this.beginScope();
                for (const s of stmt.statements) this.analyzeStmt(s);
                this.endScope();
                break;
            case "if":
                this.analyzeExpr(stmt.condition);
                this.analyzeStmt(stmt.thenBranch);
                if (stmt.elseBranch) this.analyzeStmt(stmt.elseBranch);
                break;
            case "while":
                this.analyzeExpr(stmt.condition);
                this.analyzeStmt(stmt.body);
                break;
            case "function": this.analyzeFunctionStmt(stmt); break;
            case "return":
                if (stmt.value) this.analyzeExpr(stmt.value);
                break;
            case "class": this.analyzeClassStmt(stmt); break;
            case "forIn": {
                this.analyzeExpr(stmt.iterable);
                this.beginScope();
                const iterType = this.analyzeExpr(stmt.iterable);
                const elemType = iterType.tag === "array" ? iterType.elementType : T_UNKNOWN;
                this.define(stmt.varName.lexeme, {
                    name: stmt.varName.lexeme, kind: "variable",
                    token: stmt.varName, type: elemType,
                });
                this.analyzeStmt(stmt.body);
                this.endScope();
                break;
            }
            case "break": case "continue": break;
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
            name: stmt.name.lexeme, kind: "variable",
            token: stmt.name, type: inferredType, description,
        });
    }

    private analyzeFunctionStmt(stmt: FunctionStmt): void {
        this.define(stmt.name.lexeme, {
            name: stmt.name.lexeme, kind: "function", token: stmt.name,
            params: stmt.params,
            type: { tag: "function", paramCount: stmt.params.length },
        });
        this.beginScope();
        for (const param of stmt.params) {
            this.define(param.lexeme, { name: param.lexeme, kind: "parameter", token: param });
        }
        for (const s of stmt.body) this.analyzeStmt(s);
        this.endScope();
    }

    private analyzeClassStmt(stmt: {
        kind: "class"; name: Token;
        superclass: { kind: "variable"; name: Token } | null;
        methods: FunctionStmt[];
    }): void {
        const methodNames = stmt.methods.map((m) => m.name.lexeme);
        this.define(stmt.name.lexeme, {
            name: stmt.name.lexeme, kind: "class", token: stmt.name,
            superclass: stmt.superclass ? stmt.superclass.name.lexeme : undefined,
            methods: methodNames,
            type: { tag: "class", className: stmt.name.lexeme },
        });

        if (stmt.superclass) {
            const def = this.resolve(stmt.superclass.name.lexeme);
            this.references.push({ token: stmt.superclass.name, definition: def });
            if (!def && !this.nativeNames.has(stmt.superclass.name.lexeme)) {
                this.diagnostics.push({
                    line: stmt.superclass.name.line, column: stmt.superclass.name.column,
                    endColumn: stmt.superclass.name.column + stmt.superclass.name.lexeme.length,
                    message: `Unknown superclass '${stmt.superclass.name.lexeme}'.`,
                    severity: "warning",
                });
            }
        }

        const savedClass = this.currentClassName;
        this.currentClassName = stmt.name.lexeme;
        this.beginScope();
        this.define("this", {
            name: "this", kind: "variable", token: nativeToken("this"),
            type: { tag: "instance", className: stmt.name.lexeme },
        });
        for (const method of stmt.methods) {
            this.define(method.name.lexeme, {
                name: method.name.lexeme, kind: "method", token: method.name, params: method.params,
            });
            this.beginScope();
            for (const param of method.params) {
                this.define(param.lexeme, { name: param.lexeme, kind: "parameter", token: param });
            }
            for (const s of method.body) this.analyzeStmt(s);
            this.endScope();
        }
        this.endScope();
        this.currentClassName = savedClass;
    }

    // --- Expression analysis ---

    private analyzeExpr(expr: Expr): MacType {
        switch (expr.kind) {
            case "literal": {
                if (typeof expr.value === "number") return T_NUMBER;
                if (typeof expr.value === "string") return T_STRING;
                if (typeof expr.value === "boolean") return T_BOOL;
                return T_NIL;
            }
            case "variable": {
                const def = this.resolve(expr.name.lexeme);
                this.references.push({ token: expr.name, definition: def });
                if (!def && !this.nativeNames.has(expr.name.lexeme)
                    && expr.name.lexeme !== "this" && expr.name.lexeme !== "super") {
                    this.diagnostics.push({
                        line: expr.name.line, column: expr.name.column,
                        endColumn: expr.name.column + expr.name.lexeme.length,
                        message: `Undefined variable '${expr.name.lexeme}'.`,
                        severity: "warning",
                    });
                }
                return def?.type ?? T_UNKNOWN;
            }
            case "assign": {
                const valType = this.analyzeExpr(expr.value);
                const def = this.resolve(expr.name.lexeme);
                this.references.push({ token: expr.name, definition: def });
                return valType;
            }
            case "binary": {
                this.analyzeExpr(expr.left);
                this.analyzeExpr(expr.right);
                const op = expr.operator.lexeme;
                if (op === "==" || op === "!=" || op === "<" || op === ">"
                    || op === "<=" || op === ">=") return T_BOOL;
                return T_UNKNOWN;
            }
            case "logical": {
                this.analyzeExpr(expr.left);
                this.analyzeExpr(expr.right);
                return T_BOOL;
            }
            case "unary": {
                this.analyzeExpr(expr.right);
                if (expr.operator.lexeme === "!") return T_BOOL;
                return T_NUMBER;
            }
            case "grouping": return this.analyzeExpr(expr.expression);
            case "call": return this.inferCallType(expr, this.analyzeExpr(expr.callee),
                expr.args.map(a => this.analyzeExpr(a)));
            case "get": {
                const objType = this.analyzeExpr(expr.object);
                const propName = expr.name.lexeme;
                const candidates = PROPERTY_INDEX.get(propName);
                if (candidates) {
                    const match = candidates.find(p =>
                        objType.tag === "instance" && p.ownerType === objType.className)
                        ?? candidates.find(p => p.ownerType === "class");
                    if (match) {
                        this.propertyRefs.push({ token: expr.name, info: match });
                        if (objType.tag === "instance") {
                            const key = `${objType.className}.${propName}`;
                            return FIELD_TYPES.get(key) ?? T_UNKNOWN;
                        }
                    }
                }
                return T_UNKNOWN;
            }
            case "set": {
                this.analyzeExpr(expr.object);
                return this.analyzeExpr(expr.value);
            }
            case "this": return this.currentClassName
                ? { tag: "instance", className: this.currentClassName } : T_UNKNOWN;
            case "super": return T_UNKNOWN;
            case "array": {
                if (expr.elements.length > 0) {
                    const firstType = this.analyzeExpr(expr.elements[0]);
                    for (let i = 1; i < expr.elements.length; i++) this.analyzeExpr(expr.elements[i]);
                    return { tag: "array", elementType: firstType };
                }
                return { tag: "array", elementType: T_UNKNOWN };
            }
            case "map": {
                if (expr.values.length > 0) {
                    const firstValType = this.analyzeExpr(expr.values[0]);
                    for (let i = 1; i < expr.values.length; i++) this.analyzeExpr(expr.values[i]);
                    return { tag: "map", valueType: firstValType };
                }
                return { tag: "map", valueType: T_UNKNOWN };
            }
            case "indexGet": {
                const objType = this.analyzeExpr(expr.object);
                this.analyzeExpr(expr.index);
                if (objType.tag === "tuple" && expr.index.kind === "literal" && typeof expr.index.value === "number") {
                    const i = expr.index.value;
                    if (i >= 0 && i < objType.elementTypes.length) return objType.elementTypes[i];
                }
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
                    this.define(param.lexeme, { name: param.lexeme, kind: "parameter", token: param });
                }
                for (const s of expr.body) this.analyzeStmt(s);
                this.endScope();
                return { tag: "function", paramCount: expr.params.length };
            }
            case "pipe": {
                const inputType = this.analyzeExpr(expr.value);
                this.analyzeExpr(expr.func);
                return this.inferPipeType(expr.func, inputType);
            }
            case "compose": {
                const leftType = this.analyzeExpr(expr.left);
                const rightType = this.analyzeExpr(expr.right);
                // If either side has a known returnType, propagate it
                const retType = (rightType.tag === "function" && rightType.returnType)
                    ? rightType.returnType
                    : (leftType.tag === "function" && leftType.returnType)
                    ? leftType.returnType : undefined;
                return { tag: "function", paramCount: 1, returnType: retType };
            }
        }
    }

    // --- Type inference ---

    private inferCallType(expr: { callee: Expr; args: Expr[] }, calleeType: MacType, argTypes: MacType[]): MacType {
        if (expr.callee.kind === "variable") {
            const name = expr.callee.name.lexeme;
            const def = this.resolve(name);
            if (def?.kind === "class") return { tag: "instance", className: name };
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) return resolver(argTypes);
        }
        if (expr.callee.kind === "get") {
            const methodName = expr.callee.name.lexeme;
            let objectType: MacType = T_UNKNOWN;
            if (expr.callee.object.kind === "variable") {
                const objDef = this.resolve(expr.callee.object.name.lexeme);
                if (objDef?.type) objectType = objDef.type;
            } else if (expr.callee.object.kind === "call") {
                if (expr.callee.object.callee.kind === "variable") {
                    const innerDef = this.resolve(expr.callee.object.callee.name.lexeme);
                    if (innerDef?.kind === "class") objectType = { tag: "instance", className: innerDef.name };
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

    private inferPipeType(func: Expr, inputType: MacType): MacType {
        if (func.kind === "variable") {
            const name = func.name.lexeme;
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) return resolver([inputType]);
            const def = this.resolve(name);
            if (def?.kind === "class") return { tag: "instance", className: def.name };
            if (inputType.tag === "instance" && inputType.className === "Meme") return inputType;
            return T_UNKNOWN;
        }
        if (func.kind === "call" && func.callee.kind === "variable") {
            const name = func.callee.name.lexeme;
            if (Analyzer.FOLDING_HOFS.has(name) && func.args.length >= 2) {
                const initType = this.analyzeExpr(func.args[1]);
                if (initType.tag !== "unknown") return initType;
            }
            if (Analyzer.MAPPING_HOFS.has(name) && func.args.length >= 1) {
                const cbReturnType = this.inferCallbackReturnType(func.args[0], inputType);
                if (cbReturnType.tag !== "unknown") {
                    const elemType = name === "flatMap" && cbReturnType.tag === "array"
                        ? cbReturnType.elementType : cbReturnType;
                    return { tag: "array", elementType: elemType };
                }
            }
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) return resolver([inputType]);
            if (inputType.tag === "instance" && inputType.className === "Meme") return inputType;
            return T_UNKNOWN;
        }
        if (inputType.tag === "instance" && inputType.className === "Meme") return inputType;
        return T_UNKNOWN;
    }

    private inferCallbackReturnType(callback: Expr, inputType: MacType): MacType {
        if (callback.kind === "lambda") {
            // Temporarily define lambda params with the input element type
            const elemType = inputType.tag === "array" ? inputType.elementType : inputType;
            this.beginScope();
            for (const param of callback.params) {
                this.define(param.lexeme, {
                    name: param.lexeme, kind: "parameter", token: param, type: elemType,
                });
            }
            const lastStmt = callback.body[callback.body.length - 1];
            let bodyType: MacType = T_UNKNOWN;
            if (lastStmt) bodyType = this.inferLambdaReturnType(lastStmt);
            this.endScope();
            if (bodyType.tag !== "unknown") return bodyType;
        }
        if (callback.kind === "variable") {
            const name = callback.name.lexeme;
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) {
                const elemType = inputType.tag === "array" ? inputType.elementType : inputType;
                return resolver([elemType]);
            }
            if (inputType.tag === "array" && inputType.elementType.tag === "instance"
                && inputType.elementType.className === "Meme") return inputType.elementType;
        }
        if (callback.kind === "call" && callback.callee.kind === "variable") {
            const name = callback.callee.name.lexeme;
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) {
                const resultType = resolver([]);
                if (resultType.tag === "function" && inputType.tag === "array"
                    && inputType.elementType.tag === "instance"
                    && inputType.elementType.className === "Meme") return inputType.elementType;
                return resultType;
            }
        }
        return T_UNKNOWN;
    }

    private inferLambdaReturnType(stmt: Stmt): MacType {
        if (stmt.kind === "return" && stmt.value) return this.inferExprType(stmt.value);
        if (stmt.kind === "expression") return this.inferExprType(stmt.expression);
        return T_UNKNOWN;
    }

    private inferExprType(expr: Expr): MacType {
        if (expr.kind === "call" && expr.callee.kind === "variable") {
            const name = expr.callee.name.lexeme;
            const def = this.resolve(name);
            if (def?.kind === "class") return { tag: "instance", className: name };
            const resolver = NATIVE_RETURN_TYPES.get(name);
            if (resolver) return resolver([]);
        }
        if (expr.kind === "call" && expr.callee.kind === "get") {
            const objectType = this.inferExprType(expr.callee.object);
            if (objectType.tag === "instance") {
                const key = `${objectType.className}.${expr.callee.name.lexeme}`;
                const retType = METHOD_RETURN_TYPES.get(key);
                if (retType) return retType;
            }
            return objectType;
        }
        if (expr.kind === "pipe") {
            const leftType = this.inferExprType(expr.value);
            if (leftType.tag === "instance" && leftType.className === "Meme") return leftType;
            return leftType;
        }
        if (expr.kind === "indexGet") {
            const objType = this.inferExprType(expr.object);
            if (objType.tag === "tuple" && expr.index.kind === "literal" && typeof expr.index.value === "number") {
                const i = expr.index.value;
                if (i >= 0 && i < objType.elementTypes.length) return objType.elementTypes[i];
            }
            if (objType.tag === "array") return objType.elementType;
            if (objType.tag === "map") return objType.valueType;
            return T_UNKNOWN;
        }
        if (expr.kind === "variable") {
            const def = this.resolve(expr.name.lexeme);
            if (def?.type) return def.type;
        }
        return T_UNKNOWN;
    }

    // --- Compose chain description ---

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
}
