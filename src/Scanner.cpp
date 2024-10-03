#include "Scanner.h"

#include <memory>

using std::monostate;
using token::Token;
using token::TokenValue;
using token::StringLiteral;
using token::NumberLiteral;

namespace scanner {

    Token Scanner::scanToken() {
        while(!isAtEnd()) {
            char c = advance();
            if (isWhitespace(c)) continue;
            if (c == '\n') {
                line++;
                continue;
            }
            start = current - 1;
            TokenType type = TokenType::NONE;
            switch (c) {
                case '(': type = TokenType::LEFT_PAREN; break;
                case ')': type = TokenType::RIGHT_PAREN; break;
                case '{': type = TokenType::LEFT_BRACE; break;
                case '}': type = TokenType::RIGHT_BRACE; break;
                case ',': type = TokenType::COMMA; break;
                case '.': type = TokenType::DOT; break;
                case '-': type = TokenType::MINUS; break;
                case '+': type = TokenType::PLUS; break;
                case ';': type = TokenType::SEMICOLON; break;
                case '*': type = TokenType::STAR; break;
                default:
                    break;
            }
            if (type != TokenType::NONE) {
                string lexeme = string(1, c);
                return Token(type, TokenValue(lexeme), line);
            }

            // two character tokens
            type = TokenType::NONE; // Reset type to NONE
            switch(c) {
                case '!':
                    if (match('=')) {
                        type = TokenType::BANG_EQUAL;
                    } else {
                        type = TokenType::BANG;
                    }
                    break;
                case '=':
                    if (match('=')) {
                        type = TokenType::EQUAL_EQUAL;
                    } else {
                        type = TokenType::EQUAL;
                    }
                    break;
                case '>':
                    if (match('=')) {
                        type = TokenType::GREATER_EQUAL;
                    } else {
                        type = TokenType::GREATER;
                    }
                    break;
                case '<':
                    if (match('=')) {
                        type = TokenType::LESS_EQUAL;
                    } else {
                        type = TokenType::LESS;
                    }
                    break;
                default:
                    break;
                }

            if (type != TokenType::NONE) {
                string lexeme = source.substr(start, current - start);
                return Token(type, TokenValue(lexeme), line);
            }

            // Longer lexemes
            type = TokenType::NONE; // Reset type to NONE
            switch (c) {
                case '/':
                    if (match('/')) {
                        while (peek() != '\n' && !isAtEnd()) advance();
                    } else {
                        type = TokenType::SLASH;
                    }
                    break;
                case '"':
                    while (peek() != '"' && !isAtEnd()) {
                        if (peek() == '\n') line++;
                        advance();
                    }
                    if (isAtEnd()) {
                        cout << "Unterminated string on line " << line << std::endl;
                        return Token(TokenType::NONE, TokenValue(), line);
                    } else {
                        advance(); // closing "
                        // TokenValue literal = source.substr(start + 1, current - start - 2);
                        string lexeme = source.substr(start, current - start);
                        return Token(TokenType::STRING, StringLiteral(lexeme), line);
                    }
                default:
                    if(isdigit(c)) {
                        while (isDigit(peek())) advance();
                        // Look for a fractional part.
                        if (peek() == '.' && isDigit(peekNext())) {
                            // Consume the "."
                            advance();

                            while (isDigit(peek())) advance();
                        }
                        string lexeme = source.substr(start, current - start);
                        return Token(TokenType::NUMBER, NumberLiteral(lexeme), line);
                    } else if (isAlpha(c)) {
                        // Consume each alphanumeric character up to the maximum length
                        while (isAlphaNumeric(peek())) advance();
                        TokenType tokenType = TokenType::IDENTIFIER;
                        string identifier = source.substr(start, current - start);
                        // If the matched identifier is a keyword, set the type accordingly
                        if (keywords.find(identifier) != keywords.end()) {
                            tokenType = keywords[identifier]; // Override type here
                        }
                        return Token(tokenType, TokenValue(identifier), line);
                    } else {
                        cout << "Unexpected character on line " << line << std::endl;
                        return Token(TokenType::NONE, TokenValue(), line);
                    }
            }
            // This handles the case where the token is a single character like '/'
            type = TokenType::NONE;
            if (type != TokenType::NONE) {
                string lexeme = source.substr(start, current - start);
                return Token(type, TokenValue(lexeme), line);
            }

        }
        return Token(TokenType::END_OF_FILE, TokenValue(), line);
    }

} // namespace scanner