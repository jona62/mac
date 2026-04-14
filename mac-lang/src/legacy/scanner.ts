// Mac Language Scanner — TypeScript port
// Tokenizes Mac source code into a stream of tokens.

export enum TokenType {
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE, LEFT_BRACKET, RIGHT_BRACKET,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR, COLON, PERCENT,

    // One or two character tokens
    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL, GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,
    PIPE, COMPOSE, ARROW,

    // Literals
    IDENTIFIER, STRING, NUMBER,

    // Keywords
    AND, BREAK, CLASS, CONTINUE, ELSE, FALSE, FUN, FOR, IF, IN, NIL, OR,
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    // End of file
    EOF,
}

export interface Token {
    type: TokenType;
    lexeme: string;
    literal: string | number | boolean | null;
    line: number;
    column: number;
}

export interface ScanError {
    line: number;
    column: number;
    message: string;
}

const keywords: Map<string, TokenType> = new Map([
    ["and", TokenType.AND],
    ["break", TokenType.BREAK],
    ["class", TokenType.CLASS],
    ["continue", TokenType.CONTINUE],
    ["else", TokenType.ELSE],
    ["false", TokenType.FALSE],
    ["fun", TokenType.FUN],
    ["for", TokenType.FOR],
    ["if", TokenType.IF],
    ["in", TokenType.IN],
    ["nil", TokenType.NIL],
    ["or", TokenType.OR],
    ["print", TokenType.PRINT],
    ["return", TokenType.RETURN],
    ["super", TokenType.SUPER],
    ["this", TokenType.THIS],
    ["true", TokenType.TRUE],
    ["var", TokenType.VAR],
    ["while", TokenType.WHILE],
]);

export class Scanner {
    private source: string;
    private tokens: Token[] = [];
    private errors: ScanError[] = [];
    private start = 0;
    private current = 0;
    private line = 1;
    private column = 1;
    private startColumn = 1;

    constructor(source: string) {
        this.source = source;
    }

    scanTokens(): { tokens: Token[]; errors: ScanError[] } {
        while (!this.isAtEnd()) {
            this.start = this.current;
            this.startColumn = this.column;
            this.scanToken();
        }

        this.tokens.push({
            type: TokenType.EOF,
            lexeme: "",
            literal: null,
            line: this.line,
            column: this.column,
        });

        return { tokens: this.tokens, errors: this.errors };
    }

    private scanToken(): void {
        const c = this.advance();
        switch (c) {
            case "(": this.addToken(TokenType.LEFT_PAREN); break;
            case ")": this.addToken(TokenType.RIGHT_PAREN); break;
            case "{": this.addToken(TokenType.LEFT_BRACE); break;
            case "}": this.addToken(TokenType.RIGHT_BRACE); break;
            case "[": this.addToken(TokenType.LEFT_BRACKET); break;
            case "]": this.addToken(TokenType.RIGHT_BRACKET); break;
            case ",": this.addToken(TokenType.COMMA); break;
            case ".": this.addToken(TokenType.DOT); break;
            case "-": this.addToken(this.match(">") ? TokenType.ARROW : TokenType.MINUS); break;
            case "+": this.addToken(TokenType.PLUS); break;
            case ";": this.addToken(TokenType.SEMICOLON); break;
            case "*": this.addToken(TokenType.STAR); break;
            case ":": this.addToken(TokenType.COLON); break;
            case "%": this.addToken(TokenType.PERCENT); break;

            // One or two character tokens
            case "!":
                this.addToken(this.match("=") ? TokenType.BANG_EQUAL : TokenType.BANG);
                break;
            case "=":
                this.addToken(this.match("=") ? TokenType.EQUAL_EQUAL : TokenType.EQUAL);
                break;
            case ">":
                if (this.match("=")) {
                    this.addToken(TokenType.GREATER_EQUAL);
                } else if (this.match(">")) {
                    this.addToken(TokenType.COMPOSE);
                } else {
                    this.addToken(TokenType.GREATER);
                }
                break;
            case "|":
                if (this.match(">")) {
                    this.addToken(TokenType.PIPE);
                } else {
                    this.errors.push({
                        line: this.line,
                        column: this.startColumn,
                        message: `Unexpected character '${c}'.`,
                    });
                }
                break;
            case "<":
                this.addToken(this.match("=") ? TokenType.LESS_EQUAL : TokenType.LESS);
                break;

            // Slash or comment
            case "/":
                if (this.match("/")) {
                    // Comment — skip to end of line
                    while (this.peek() !== "\n" && !this.isAtEnd()) {
                        this.advance();
                    }
                } else {
                    this.addToken(TokenType.SLASH);
                }
                break;

            // Whitespace
            case " ":
            case "\r":
            case "\t":
                break;

            // Newline
            case "\n":
                this.line++;
                this.column = 1;
                break;

            // String literal
            case '"':
                this.string();
                break;

            default:
                if (this.isDigit(c)) {
                    this.number();
                } else if (this.isAlpha(c)) {
                    this.identifier();
                } else {
                    this.errors.push({
                        line: this.line,
                        column: this.startColumn,
                        message: `Unexpected character '${c}'.`,
                    });
                }
                break;
        }
    }

    private string(): void {
        while (this.peek() !== '"' && !this.isAtEnd()) {
            if (this.peek() === "\n") {
                this.line++;
                this.column = 1;
            }
            this.advance();
        }

        if (this.isAtEnd()) {
            this.errors.push({
                line: this.line,
                column: this.startColumn,
                message: "Unterminated string.",
            });
            return;
        }

        // Consume closing "
        this.advance();

        // Extract the string value (without quotes)
        const value = this.source.substring(this.start + 1, this.current - 1);
        this.addToken(TokenType.STRING, value);
    }

    private number(): void {
        while (this.isDigit(this.peek())) {
            this.advance();
        }

        // Look for fractional part
        if (this.peek() === "." && this.isDigit(this.peekNext())) {
            // Consume the "."
            this.advance();
            while (this.isDigit(this.peek())) {
                this.advance();
            }
        }

        const value = parseFloat(this.source.substring(this.start, this.current));
        this.addToken(TokenType.NUMBER, value);
    }

    private identifier(): void {
        while (this.isAlphaNumeric(this.peek())) {
            this.advance();
        }

        const text = this.source.substring(this.start, this.current);
        const type = keywords.get(text);
        if (type !== undefined) {
            // For boolean and nil keywords, set appropriate literal
            if (type === TokenType.TRUE) {
                this.addToken(type, true);
            } else if (type === TokenType.FALSE) {
                this.addToken(type, false);
            } else if (type === TokenType.NIL) {
                this.addToken(type, null);
            } else {
                this.addToken(type);
            }
        } else {
            this.addToken(TokenType.IDENTIFIER);
        }
    }

    // --- Helpers ---

    private isAtEnd(): boolean {
        return this.current >= this.source.length;
    }

    private advance(): string {
        const c = this.source.charAt(this.current);
        this.current++;
        this.column++;
        return c;
    }

    private match(expected: string): boolean {
        if (this.isAtEnd()) return false;
        if (this.source.charAt(this.current) !== expected) return false;
        this.current++;
        this.column++;
        return true;
    }

    private peek(): string {
        if (this.isAtEnd()) return "\0";
        return this.source.charAt(this.current);
    }

    private peekNext(): string {
        if (this.current + 1 >= this.source.length) return "\0";
        return this.source.charAt(this.current + 1);
    }

    private isDigit(c: string): boolean {
        return c >= "0" && c <= "9";
    }

    private isAlpha(c: string): boolean {
        return (c >= "a" && c <= "z") ||
               (c >= "A" && c <= "Z") ||
               c === "_";
    }

    private isAlphaNumeric(c: string): boolean {
        return this.isAlpha(c) || this.isDigit(c);
    }

    private addToken(type: TokenType, literal: string | number | boolean | null = null): void {
        const lexeme = this.source.substring(this.start, this.current);
        this.tokens.push({
            type,
            lexeme,
            literal,
            line: this.line,
            column: this.startColumn,
        });
    }
}
