#ifndef TOKEN_H
#define TOKEN_H

#include <cstddef>              // size_t
#include <iostream>             // cout, endl
#include <sstream>              // ostringstream
#include <iterator>             // forward_iterator_tag
#include <string>               // string
#include <memory>               // shared_ptr
#include <stdexcept>            // runtime_error
#include <variant>              // variant, monostate, get, holds_alternative
#include <unordered_map>        // unordered_map (keyword lookup)

using std::cout, std::endl;
using std::monostate;
using std::string;
using std::variant;
using std::shared_ptr;
using std::make_shared;

namespace token {

    // exporting this type through the namespace
    using TokenValue = variant<string, double, bool, monostate>;

    // The order of this matters for the translating the enums to their corresponding string representations
    enum class TokenType {
        NONE = 0,

        // Single-character tokens.
        LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
        LEFT_BRACKET, RIGHT_BRACKET,
        COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,
        COLON, PERCENT, AT,
        PIPE,    // |>
        COMPOSE, // >>
        ARROW,   // ->
        FAT_ARROW,   // =>
        TRIPLE_DASH, // ---

        // One or two character tokens.
        BANG, BANG_EQUAL,
        EQUAL, EQUAL_EQUAL,
        GREATER, GREATER_EQUAL,
        LESS, LESS_EQUAL,

        // Literals.
        IDENTIFIER, STRING, INTERP_STRING, NUMBER,

        // Keywords.
        AND, BREAK, CLASS, CONTINUE, EFFECT, ELSE, FALSE, FUN, FOR,
        IF, IN, MATCH, NIL, OR,
        PRINT, RETURN, STYLE, SUPER, THIS, TRUE, VAR, WHILE,

        // End of file.
        END_OF_FILE,
    };

    // The order of this matters
    const char* const TokenTypeNames[] = {
        "NONE",
        "LEFT_PAREN", "RIGHT_PAREN", "LEFT_BRACE", "RIGHT_BRACE",
        "LEFT_BRACKET", "RIGHT_BRACKET",
        "COMMA", "DOT", "MINUS", "PLUS", "SEMICOLON", "SLASH", "STAR",
        "COLON", "PERCENT", "AT",
        "PIPE", "COMPOSE", "ARROW", "FAT_ARROW", "TRIPLE_DASH",
        "BANG", "BANG_EQUAL",
        "EQUAL", "EQUAL_EQUAL",
        "GREATER", "GREATER_EQUAL",
        "LESS", "LESS_EQUAL",
        "IDENTIFIER", "STRING", "INTERP_STRING", "NUMBER",
        "AND", "BREAK", "CLASS", "CONTINUE", "EFFECT", "ELSE", "FALSE", "FUN", "FOR",
        "IF", "IN", "MATCH", "NIL", "OR",
        "PRINT", "RETURN", "STYLE", "SUPER", "THIS", "TRUE", "VAR", "WHILE",
        "END_OF_FILE"
    };

    struct Token {
        Token() : type(TokenType::NONE), lexeme(""), line(0), column(0) {}
        Token(TokenType type, TokenValue lexeme, int line)
            : type(type), lexeme(lexeme), line(line), column(0) {}
        Token(TokenType type, TokenValue lexeme, int line, int column)
            : type(type), lexeme(lexeme), line(line), column(column) {}

        string toString() const {
            std::ostringstream ss;
            ss << "Token type: " << TokenTypeNames[static_cast<int>(type)];
            if (type == TokenType::STRING || type == TokenType::INTERP_STRING) {
                ss << ", Literal: " << get<string>(lexeme);
            } else if (type == TokenType::NUMBER) {
                ss << ", Literal: " << get<double>(lexeme);
            } else {
                ss << ", Lexeme: " << get<string>(lexeme);
            }
            ss << ", Line: " << line << endl;
            return ss.str();
        }

        string tokenAsString() const {
            std::ostringstream ss;
            if (holds_alternative<string>(lexeme)) {
                ss << get<string>(lexeme);
            } else if (holds_alternative<double>(lexeme)) {
                ss << get<double>(lexeme);
            } else if (holds_alternative<bool>(lexeme)) {
                ss << (get<bool>(lexeme) ? "true" : "false");
            } else {
                ss << "nil";
            }
            return ss.str();
        }

        TokenType type;
        TokenValue lexeme;
        int line;
        int column;
    };

} // Token

#endif // TOKEN_H