// Mac AST — Expression and Statement node types

import { Token } from "./scanner";

// ============================================================
// Expression types (discriminated union)
// ============================================================

export type Expr =
    | BinaryExpr
    | UnaryExpr
    | LiteralExpr
    | VariableExpr
    | GroupingExpr
    | LogicalExpr
    | CallExpr
    | AssignExpr
    | GetExpr
    | SetExpr
    | ThisExpr
    | SuperExpr
    | ArrayExpr
    | MapExpr
    | IndexGetExpr
    | IndexSetExpr
    | LambdaExpr
    | PipeExpr
    | ComposeExpr;

export interface BinaryExpr { kind: "binary"; left: Expr; operator: Token; right: Expr; }
export interface UnaryExpr { kind: "unary"; operator: Token; right: Expr; }
export interface LiteralExpr { kind: "literal"; value: string | number | boolean | null; token: Token; }
export interface VariableExpr { kind: "variable"; name: Token; }
export interface GroupingExpr { kind: "grouping"; expression: Expr; }
export interface LogicalExpr { kind: "logical"; left: Expr; operator: Token; right: Expr; }
export interface CallExpr { kind: "call"; callee: Expr; paren: Token; args: Expr[]; }
export interface AssignExpr { kind: "assign"; name: Token; value: Expr; }
export interface GetExpr { kind: "get"; object: Expr; name: Token; }
export interface SetExpr { kind: "set"; object: Expr; name: Token; value: Expr; }
export interface ThisExpr { kind: "this"; keyword: Token; }
export interface SuperExpr { kind: "super"; keyword: Token; method: Token; }
export interface ArrayExpr { kind: "array"; bracket: Token; elements: Expr[]; }
export interface MapExpr { kind: "map"; brace: Token; keys: Token[]; values: Expr[]; }
export interface IndexGetExpr { kind: "indexGet"; object: Expr; bracket: Token; index: Expr; }
export interface IndexSetExpr { kind: "indexSet"; object: Expr; bracket: Token; index: Expr; value: Expr; }
export interface LambdaExpr { kind: "lambda"; funKeyword: Token; params: Token[]; body: Stmt[]; }
export interface PipeExpr { kind: "pipe"; value: Expr; operator: Token; func: Expr; }
export interface ComposeExpr { kind: "compose"; left: Expr; operator: Token; right: Expr; }

// ============================================================
// Statement types (discriminated union)
// ============================================================

export type Stmt =
    | ExpressionStmt
    | PrintStmt
    | VarStmt
    | BlockStmt
    | IfStmt
    | WhileStmt
    | FunctionStmt
    | ReturnStmt
    | ClassStmt
    | ForInStmt
    | BreakStmt
    | ContinueStmt;

export interface ExpressionStmt { kind: "expression"; expression: Expr; }
export interface PrintStmt { kind: "print"; expression: Expr; }
export interface VarStmt { kind: "var"; name: Token; initializer: Expr | null; }
export interface BlockStmt { kind: "block"; statements: Stmt[]; }
export interface IfStmt { kind: "if"; condition: Expr; thenBranch: Stmt; elseBranch: Stmt | null; }
export interface WhileStmt { kind: "while"; condition: Expr; body: Stmt; }
export interface FunctionStmt { kind: "function"; name: Token; params: Token[]; body: Stmt[]; }
export interface ReturnStmt { kind: "return"; keyword: Token; value: Expr | null; }
export interface ClassStmt { kind: "class"; name: Token; superclass: VariableExpr | null; methods: FunctionStmt[]; }
export interface ForInStmt { kind: "forIn"; varName: Token; iterable: Expr; body: Stmt; }
export interface BreakStmt { kind: "break"; keyword: Token; }
export interface ContinueStmt { kind: "continue"; keyword: Token; }

// ============================================================
// Parse error
// ============================================================

export interface ParseError {
    token: Token;
    message: string;
}
