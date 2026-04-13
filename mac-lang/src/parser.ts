// Mac Language Parser — TypeScript port
// Parses tokens into an AST using discriminated unions.

import { Token, TokenType } from "./scanner";

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

export interface BinaryExpr {
    kind: "binary";
    left: Expr;
    operator: Token;
    right: Expr;
}

export interface UnaryExpr {
    kind: "unary";
    operator: Token;
    right: Expr;
}

export interface LiteralExpr {
    kind: "literal";
    value: string | number | boolean | null;
    token: Token;
}

export interface VariableExpr {
    kind: "variable";
    name: Token;
}

export interface GroupingExpr {
    kind: "grouping";
    expression: Expr;
}

export interface LogicalExpr {
    kind: "logical";
    left: Expr;
    operator: Token;
    right: Expr;
}

export interface CallExpr {
    kind: "call";
    callee: Expr;
    paren: Token;
    args: Expr[];
}

export interface AssignExpr {
    kind: "assign";
    name: Token;
    value: Expr;
}

export interface GetExpr {
    kind: "get";
    object: Expr;
    name: Token;
}

export interface SetExpr {
    kind: "set";
    object: Expr;
    name: Token;
    value: Expr;
}

export interface ThisExpr {
    kind: "this";
    keyword: Token;
}

export interface SuperExpr {
    kind: "super";
    keyword: Token;
    method: Token;
}

export interface ArrayExpr {
    kind: "array";
    bracket: Token;
    elements: Expr[];
}

export interface MapExpr {
    kind: "map";
    brace: Token;
    keys: Token[];
    values: Expr[];
}

export interface IndexGetExpr {
    kind: "indexGet";
    object: Expr;
    bracket: Token;
    index: Expr;
}

export interface IndexSetExpr {
    kind: "indexSet";
    object: Expr;
    bracket: Token;
    index: Expr;
    value: Expr;
}

export interface LambdaExpr {
    kind: "lambda";
    funKeyword: Token;
    params: Token[];
    body: Stmt[];
}

export interface PipeExpr {
    kind: "pipe";
    value: Expr;
    operator: Token;
    func: Expr;
}

export interface ComposeExpr {
    kind: "compose";
    left: Expr;
    operator: Token;
    right: Expr;
}

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

export interface ExpressionStmt {
    kind: "expression";
    expression: Expr;
}

export interface PrintStmt {
    kind: "print";
    expression: Expr;
}

export interface VarStmt {
    kind: "var";
    name: Token;
    initializer: Expr | null;
}

export interface BlockStmt {
    kind: "block";
    statements: Stmt[];
}

export interface IfStmt {
    kind: "if";
    condition: Expr;
    thenBranch: Stmt;
    elseBranch: Stmt | null;
}

export interface WhileStmt {
    kind: "while";
    condition: Expr;
    body: Stmt;
}

export interface FunctionStmt {
    kind: "function";
    name: Token;
    params: Token[];
    body: Stmt[];
}

export interface ReturnStmt {
    kind: "return";
    keyword: Token;
    value: Expr | null;
}

export interface ClassStmt {
    kind: "class";
    name: Token;
    superclass: VariableExpr | null;
    methods: FunctionStmt[];
}

export interface ForInStmt {
    kind: "forIn";
    varName: Token;
    iterable: Expr;
    body: Stmt;
}

export interface BreakStmt {
    kind: "break";
    keyword: Token;
}

export interface ContinueStmt {
    kind: "continue";
    keyword: Token;
}

// ============================================================
// Parse error
// ============================================================

export interface ParseError {
    token: Token;
    message: string;
}

// Internal error class used for control flow within the parser
class ParseErrorSignal extends Error {
    token: Token;
    constructor(token: Token, message: string) {
        super(message);
        this.token = token;
    }
}

// ============================================================
// Parser
// ============================================================

export class Parser {
    private tokens: Token[];
    private current = 0;
    private errors: ParseError[] = [];

    constructor(tokens: Token[]) {
        this.tokens = tokens;
    }

    parse(): { statements: Stmt[]; errors: ParseError[] } {
        const statements: Stmt[] = [];
        while (!this.isAtEnd()) {
            const decl = this.declaration();
            if (decl !== null) {
                statements.push(decl);
            }
        }
        return { statements, errors: this.errors };
    }

    // --- Statement parsing ---

    private declaration(): Stmt | null {
        try {
            if (this.match(TokenType.CLASS)) return this.classDeclaration();
            // 2-token lookahead: only treat as function declaration if FUN followed by IDENTIFIER
            if (
                this.peek().type === TokenType.FUN &&
                this.current + 1 < this.tokens.length &&
                this.tokens[this.current + 1].type === TokenType.IDENTIFIER
            ) {
                this.advance(); // consume FUN
                return this.functionDeclaration("function");
            }
            if (this.match(TokenType.VAR)) return this.varDeclaration();
            return this.statement();
        } catch (error) {
            if (error instanceof ParseErrorSignal) {
                this.errors.push({ token: error.token, message: error.message });
                this.synchronize();
                return null;
            }
            throw error;
        }
    }

    private varDeclaration(): Stmt {
        this.consume(TokenType.IDENTIFIER, "Expected variable name.");
        const name = this.previous();

        let initializer: Expr | null = null;
        if (this.match(TokenType.EQUAL)) {
            initializer = this.expression();
        }

        this.consume(TokenType.SEMICOLON, "Expected ';' after variable declaration.");
        return { kind: "var", name, initializer };
    }

    private functionDeclaration(fnKind: string): FunctionStmt {
        this.consume(TokenType.IDENTIFIER, `Expected ${fnKind} name.`);
        const name = this.previous();
        this.consume(TokenType.LEFT_PAREN, `Expected '(' after ${fnKind} name.`);

        const params: Token[] = [];
        if (this.peek().type !== TokenType.RIGHT_PAREN) {
            do {
                if (params.length >= 255) {
                    this.error(this.peek(), "Can't have more than 255 parameters.");
                }
                this.consume(TokenType.IDENTIFIER, "Expected parameter name.");
                params.push(this.previous());
            } while (this.match(TokenType.COMMA));
        }
        this.consume(TokenType.RIGHT_PAREN, "Expected ')' after parameters.");

        this.consume(TokenType.LEFT_BRACE, `Expected '{' before ${fnKind} body.`);
        const body = this.block();
        return { kind: "function", name, params, body };
    }

    private classDeclaration(): Stmt {
        this.consume(TokenType.IDENTIFIER, "Expected class name.");
        const name = this.previous();

        let superclass: VariableExpr | null = null;
        if (this.match(TokenType.LESS)) {
            this.consume(TokenType.IDENTIFIER, "Expected superclass name.");
            superclass = { kind: "variable", name: this.previous() };
        }

        this.consume(TokenType.LEFT_BRACE, "Expected '{' before class body.");

        const methods: FunctionStmt[] = [];
        while (!this.isAtEnd() && this.peek().type !== TokenType.RIGHT_BRACE) {
            methods.push(this.functionDeclaration("method"));
        }

        this.consume(TokenType.RIGHT_BRACE, "Expected '}' after class body.");
        return { kind: "class", name, superclass, methods };
    }

    private statement(): Stmt {
        if (this.match(TokenType.IF)) return this.ifStatement();
        if (this.match(TokenType.PRINT)) return this.printStatement();
        if (this.match(TokenType.RETURN)) return this.returnStatement();
        if (this.match(TokenType.BREAK)) return this.breakStatement();
        if (this.match(TokenType.CONTINUE)) return this.continueStatement();
        if (this.match(TokenType.WHILE)) return this.whileStatement();
        if (this.match(TokenType.FOR)) return this.forStatement();
        if (this.match(TokenType.LEFT_BRACE)) return { kind: "block", statements: this.block() };
        return this.expressionStatement();
    }

    private printStatement(): Stmt {
        const expression = this.expression();
        this.consume(TokenType.SEMICOLON, "Expected ';' after value.");
        return { kind: "print", expression };
    }

    private returnStatement(): Stmt {
        const keyword = this.previous();
        let value: Expr | null = null;
        if (this.peek().type !== TokenType.SEMICOLON) {
            value = this.expression();
        }
        this.consume(TokenType.SEMICOLON, "Expected ';' after return value.");
        return { kind: "return", keyword, value };
    }

    private breakStatement(): Stmt {
        const keyword = this.previous();
        this.consume(TokenType.SEMICOLON, "Expected ';' after 'break'.");
        return { kind: "break", keyword };
    }

    private continueStatement(): Stmt {
        const keyword = this.previous();
        this.consume(TokenType.SEMICOLON, "Expected ';' after 'continue'.");
        return { kind: "continue", keyword };
    }

    private ifStatement(): Stmt {
        this.consume(TokenType.LEFT_PAREN, "Expected '(' after 'if'.");
        const condition = this.expression();
        this.consume(TokenType.RIGHT_PAREN, "Expected ')' after if condition.");

        const thenBranch = this.statement();
        let elseBranch: Stmt | null = null;
        if (this.match(TokenType.ELSE)) {
            elseBranch = this.statement();
        }

        return { kind: "if", condition, thenBranch, elseBranch };
    }

    private whileStatement(): Stmt {
        this.consume(TokenType.LEFT_PAREN, "Expected '(' after 'while'.");
        const condition = this.expression();
        this.consume(TokenType.RIGHT_PAREN, "Expected ')' after while condition.");

        const body = this.statement();
        return { kind: "while", condition, body };
    }

    private forStatement(): Stmt {
        this.consume(TokenType.LEFT_PAREN, "Expected '(' after 'for'.");

        // Check for for-in: for (var x in collection)
        if (this.match(TokenType.VAR)) {
            this.consume(TokenType.IDENTIFIER, "Expected variable name.");
            const varName = this.previous();

            if (this.match(TokenType.IN)) {
                // for-in loop
                const iterable = this.expression();
                this.consume(TokenType.RIGHT_PAREN, "Expected ')' after for-in clause.");
                const body = this.statement();
                return { kind: "forIn", varName, iterable, body };
            }

            // C-style for with var initializer — we already consumed VAR and IDENTIFIER
            let init: Expr | null = null;
            if (this.match(TokenType.EQUAL)) {
                init = this.expression();
            }
            this.consume(TokenType.SEMICOLON, "Expected ';' after variable declaration.");
            const initializer: VarStmt = { kind: "var", name: varName, initializer: init };

            // Condition
            let condition: Expr | null = null;
            if (this.peek().type !== TokenType.SEMICOLON) {
                condition = this.expression();
            }
            this.consume(TokenType.SEMICOLON, "Expected ';' after loop condition.");

            // Increment
            let increment: Expr | null = null;
            if (this.peek().type !== TokenType.RIGHT_PAREN) {
                increment = this.expression();
            }
            this.consume(TokenType.RIGHT_PAREN, "Expected ')' after for clauses.");

            let body: Stmt = this.statement();

            // Desugar
            if (increment !== null) {
                body = {
                    kind: "block",
                    statements: [body, { kind: "expression", expression: increment }],
                };
            }
            if (condition === null) {
                condition = { kind: "literal", value: true, token: varName };
            }
            body = { kind: "while", condition, body };

            return {
                kind: "block",
                statements: [initializer, body],
            };
        }

        // C-style for without var
        let forInitializer: Stmt | null;
        if (this.match(TokenType.SEMICOLON)) {
            forInitializer = null;
        } else {
            forInitializer = this.expressionStatement();
        }

        let condition: Expr | null = null;
        if (this.peek().type !== TokenType.SEMICOLON) {
            condition = this.expression();
        }
        this.consume(TokenType.SEMICOLON, "Expected ';' after loop condition.");

        let increment: Expr | null = null;
        if (this.peek().type !== TokenType.RIGHT_PAREN) {
            increment = this.expression();
        }
        this.consume(TokenType.RIGHT_PAREN, "Expected ')' after for clauses.");

        let body: Stmt = this.statement();

        if (increment !== null) {
            body = {
                kind: "block",
                statements: [body, { kind: "expression", expression: increment }],
            };
        }
        if (condition === null) {
            condition = { kind: "literal", value: true, token: this.previous() };
        }
        body = { kind: "while", condition, body };
        if (forInitializer !== null) {
            body = { kind: "block", statements: [forInitializer, body] };
        }

        return body;
    }

    private expressionStatement(): Stmt {
        const expression = this.expression();
        this.consume(TokenType.SEMICOLON, "Expected ';' after expression.");
        return { kind: "expression", expression };
    }

    private block(): Stmt[] {
        const statements: Stmt[] = [];

        while (!this.isAtEnd() && this.peek().type !== TokenType.RIGHT_BRACE) {
            const decl = this.declaration();
            if (decl !== null) {
                statements.push(decl);
            }
        }

        this.consume(TokenType.RIGHT_BRACE, "Expected '}' after block.");
        return statements;
    }

    // --- Expression parsing ---

    private expression(): Expr {
        return this.pipe();
    }

    private pipe(): Expr {
        let expr = this.compose();

        while (this.match(TokenType.PIPE)) {
            const operator = this.previous();
            const right = this.compose();
            expr = { kind: "pipe", value: expr, operator, func: right };
        }

        return expr;
    }

    private compose(): Expr {
        let expr = this.assignment();

        while (this.match(TokenType.COMPOSE)) {
            const operator = this.previous();
            const right = this.assignment();
            expr = { kind: "compose", left: expr, operator, right };
        }

        return expr;
    }

    private assignment(): Expr {
        const expr = this.logicalOr();

        if (this.match(TokenType.EQUAL)) {
            const equals = this.previous();
            const value = this.assignment();

            if (expr.kind === "variable") {
                return { kind: "assign", name: expr.name, value };
            }

            if (expr.kind === "get") {
                return { kind: "set", object: expr.object, name: expr.name, value };
            }

            if (expr.kind === "indexGet") {
                return {
                    kind: "indexSet",
                    object: expr.object,
                    bracket: expr.bracket,
                    index: expr.index,
                    value,
                };
            }

            this.error(equals, "Invalid assignment target.");
        }

        return expr;
    }

    private logicalOr(): Expr {
        let expr = this.logicalAnd();
        while (this.match(TokenType.OR)) {
            const operator = this.previous();
            const right = this.logicalAnd();
            expr = { kind: "logical", left: expr, operator, right };
        }
        return expr;
    }

    private logicalAnd(): Expr {
        let expr = this.equality();
        while (this.match(TokenType.AND)) {
            const operator = this.previous();
            const right = this.equality();
            expr = { kind: "logical", left: expr, operator, right };
        }
        return expr;
    }

    private equality(): Expr {
        let expr = this.comparison();
        while (this.match(TokenType.BANG_EQUAL) || this.match(TokenType.EQUAL_EQUAL)) {
            const operator = this.previous();
            const right = this.comparison();
            expr = { kind: "binary", left: expr, operator, right };
        }
        return expr;
    }

    private comparison(): Expr {
        let expr = this.term();
        while (
            this.match(TokenType.GREATER) ||
            this.match(TokenType.GREATER_EQUAL) ||
            this.match(TokenType.LESS) ||
            this.match(TokenType.LESS_EQUAL)
        ) {
            const operator = this.previous();
            const right = this.term();
            expr = { kind: "binary", left: expr, operator, right };
        }
        return expr;
    }

    private term(): Expr {
        let expr = this.factor();
        while (this.match(TokenType.MINUS) || this.match(TokenType.PLUS)) {
            const operator = this.previous();
            const right = this.factor();
            expr = { kind: "binary", left: expr, operator, right };
        }
        return expr;
    }

    private factor(): Expr {
        let expr = this.unary();
        while (
            this.match(TokenType.SLASH) ||
            this.match(TokenType.STAR) ||
            this.match(TokenType.PERCENT)
        ) {
            const operator = this.previous();
            const right = this.unary();
            expr = { kind: "binary", left: expr, operator, right };
        }
        return expr;
    }

    private unary(): Expr {
        if (this.match(TokenType.BANG) || this.match(TokenType.MINUS)) {
            const operator = this.previous();
            const right = this.unary();
            return { kind: "unary", operator, right };
        }
        return this.call();
    }

    private call(): Expr {
        let expr = this.primary();

        while (true) {
            if (this.match(TokenType.LEFT_PAREN)) {
                expr = this.finishCall(expr);
            } else if (this.match(TokenType.DOT)) {
                this.consume(TokenType.IDENTIFIER, "Expected property name after '.'.");
                const name = this.previous();
                expr = { kind: "get", object: expr, name };
            } else if (this.match(TokenType.LEFT_BRACKET)) {
                const index = this.expression();
                this.consume(TokenType.RIGHT_BRACKET, "Expected ']' after index.");
                const bracket = this.previous();
                expr = { kind: "indexGet", object: expr, bracket, index };
            } else {
                break;
            }
        }

        return expr;
    }

    private finishCall(callee: Expr): Expr {
        const args: Expr[] = [];
        if (this.peek().type !== TokenType.RIGHT_PAREN) {
            do {
                if (args.length >= 255) {
                    this.error(this.peek(), "Can't have more than 255 arguments.");
                }
                args.push(this.expression());
            } while (this.match(TokenType.COMMA));
        }

        this.consume(TokenType.RIGHT_PAREN, "Expected ')' after arguments.");
        const paren = this.previous();
        return { kind: "call", callee, paren, args };
    }

    private primary(): Expr {
        if (this.match(TokenType.FALSE)) {
            return { kind: "literal", value: false, token: this.previous() };
        }
        if (this.match(TokenType.TRUE)) {
            return { kind: "literal", value: true, token: this.previous() };
        }
        if (this.match(TokenType.NIL)) {
            return { kind: "literal", value: null, token: this.previous() };
        }

        if (this.match(TokenType.NUMBER)) {
            return { kind: "literal", value: this.previous().literal as number, token: this.previous() };
        }
        if (this.match(TokenType.STRING)) {
            return { kind: "literal", value: this.previous().literal as string, token: this.previous() };
        }

        if (this.match(TokenType.SUPER)) {
            const keyword = this.previous();
            this.consume(TokenType.DOT, "Expected '.' after 'super'.");
            this.consume(TokenType.IDENTIFIER, "Expected superclass method name.");
            const method = this.previous();
            return { kind: "super", keyword, method };
        }

        if (this.match(TokenType.THIS)) {
            return { kind: "this", keyword: this.previous() };
        }

        if (this.match(TokenType.FUN)) {
            // Lambda: fun(params) { body }
            const funKeyword = this.previous();
            this.consume(TokenType.LEFT_PAREN, "Expected '(' for lambda.");
            const params: Token[] = [];
            if (this.peek().type !== TokenType.RIGHT_PAREN) {
                do {
                    this.consume(TokenType.IDENTIFIER, "Expected parameter name.");
                    params.push(this.previous());
                } while (this.match(TokenType.COMMA));
            }
            this.consume(TokenType.RIGHT_PAREN, "Expected ')' after lambda parameters.");
            this.consume(TokenType.LEFT_BRACE, "Expected '{' before lambda body.");
            const body = this.block();
            return { kind: "lambda", funKeyword, params, body };
        }

        // Arrow function: x -> expr
        if (this.peek().type === TokenType.IDENTIFIER && this.current + 1 < this.tokens.length
            && this.tokens[this.current + 1].type === TokenType.ARROW) {
            this.advance();
            const param = this.previous();
            this.advance(); // consume ->
            const body = this.expression();
            const ret: Stmt = { kind: "return", keyword: param, value: body };
            return { kind: "lambda", funKeyword: param, params: [param], body: [ret] };
        }

        if (this.match(TokenType.IDENTIFIER)) {
            return { kind: "variable", name: this.previous() };
        }

        if (this.match(TokenType.LEFT_PAREN)) {
            // Try arrow: (params) -> expr
            const savedPos = this.current;
            const arrowParams: Token[] = [];
            let isArrow = false;

            if (this.peek().type === TokenType.IDENTIFIER || this.peek().type === TokenType.RIGHT_PAREN) {
                if (this.peek().type !== TokenType.RIGHT_PAREN) {
                    let valid = true;
                    do {
                        if (this.peek().type !== TokenType.IDENTIFIER) { valid = false; break; }
                        this.advance();
                        arrowParams.push(this.previous());
                    } while (this.match(TokenType.COMMA));
                    if (!valid) arrowParams.length = 0;
                }
                if ((arrowParams.length > 0 || this.peek().type === TokenType.RIGHT_PAREN)
                    && this.peek().type === TokenType.RIGHT_PAREN) {
                    this.advance(); // )
                    if (this.peek().type === TokenType.ARROW) {
                        isArrow = true;
                    }
                }
            }

            if (isArrow) {
                this.advance(); // consume ->
                const arrowToken = this.previous();
                const body = this.expression();
                const ret: Stmt = { kind: "return", keyword: arrowToken, value: body };
                return { kind: "lambda", funKeyword: arrowToken, params: arrowParams, body: [ret] };
            }

            // Restore and parse as grouping
            this.current = savedPos;
            const expr = this.expression();
            this.consume(TokenType.RIGHT_PAREN, "Expected ')' after expression.");
            return { kind: "grouping", expression: expr };
        }

        if (this.match(TokenType.LEFT_BRACKET)) {
            return this.arrayLiteral();
        }

        if (this.match(TokenType.LEFT_BRACE)) {
            return this.mapLiteral();
        }

        throw this.error(this.peek(), "Expected expression.");
    }

    private arrayLiteral(): Expr {
        const bracket = this.previous();
        const elements: Expr[] = [];
        if (this.peek().type !== TokenType.RIGHT_BRACKET) {
            do {
                elements.push(this.expression());
            } while (this.match(TokenType.COMMA));
        }
        this.consume(TokenType.RIGHT_BRACKET, "Expected ']' after array elements.");
        return { kind: "array", bracket, elements };
    }

    private mapLiteral(): Expr {
        const brace = this.previous();
        const keys: Token[] = [];
        const values: Expr[] = [];

        if (this.peek().type !== TokenType.RIGHT_BRACE) {
            do {
                if (this.match(TokenType.IDENTIFIER) || this.match(TokenType.STRING)) {
                    keys.push(this.previous());
                } else {
                    throw this.error(this.peek(), "Expected map key (identifier or string).");
                }
                this.consume(TokenType.COLON, "Expected ':' after map key.");
                values.push(this.expression());
            } while (this.match(TokenType.COMMA));
        }
        this.consume(TokenType.RIGHT_BRACE, "Expected '}' after map entries.");
        return { kind: "map", brace, keys, values };
    }

    // --- Utility ---

    private isAtEnd(): boolean {
        return this.peek().type === TokenType.EOF;
    }

    private advance(): Token {
        if (!this.isAtEnd()) this.current++;
        return this.previous();
    }

    private peek(): Token {
        return this.tokens[this.current];
    }

    private previous(): Token {
        return this.tokens[this.current - 1];
    }

    private match(...types: TokenType[]): boolean {
        for (const type of types) {
            if (this.check(type)) {
                this.advance();
                return true;
            }
        }
        return false;
    }

    private check(type: TokenType): boolean {
        if (this.isAtEnd()) return false;
        return this.peek().type === type;
    }

    private consume(type: TokenType, message: string): Token {
        if (this.check(type)) return this.advance();
        throw this.error(this.peek(), message);
    }

    private error(token: Token, message: string): ParseErrorSignal {
        return new ParseErrorSignal(token, message);
    }

    private synchronize(): void {
        this.advance();
        while (!this.isAtEnd()) {
            if (this.previous().type === TokenType.SEMICOLON) return;
            switch (this.peek().type) {
                case TokenType.CLASS:
                case TokenType.FUN:
                case TokenType.VAR:
                case TokenType.FOR:
                case TokenType.IF:
                case TokenType.WHILE:
                case TokenType.PRINT:
                case TokenType.RETURN:
                    return;
            }
            this.advance();
        }
    }
}
