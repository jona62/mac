#ifndef TOKEN_H
#define TOKEN_H

#include <cstddef>
#include <iostream>
#include <iterator> // for std::forward_iterator_tag
#include <string>
#include <memory> // for std::shared_ptr
#include <stdexcept> // for std::runtime_error
#include <variant> // for std::get, std::variant, std::holds_alternative and std::monostate
#include <unordered_map>

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
        COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

        // One or two character tokens.
        BANG, BANG_EQUAL,
        EQUAL, EQUAL_EQUAL,
        GREATER, GREATER_EQUAL,
        LESS, LESS_EQUAL,

        // Literals.
        IDENTIFIER, STRING, NUMBER,

        // Keywords.
        AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
        PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

        // End of file.
        END_OF_FILE,
    };

    // The order of this matters
    const char* const TokenTypeNames[] = {
        "NONE",           // index 0
        "LEFT_PAREN",     // index 1
        "RIGHT_PAREN",    // index 2
        "LEFT_BRACE",     // index 3
        "RIGHT_BRACE",    // index 4
        "COMMA",          // index 5
        "DOT",            // index 6
        "MINUS",          // index 7
        "PLUS",           // index 8
        "SEMICOLON",      // index 9
        "SLASH",          // index 10
        "STAR",           // index 11
        "BANG",           // index 12
        "BANG_EQUAL",     // index 13
        "EQUAL",          // index 14
        "EQUAL_EQUAL",    // index 15
        "GREATER",        // index 16
        "GREATER_EQUAL",  // index 17
        "LESS",           // index 18
        "LESS_EQUAL",     // index 19
        "IDENTIFIER",     // index 20
        "STRING",         // index 21
        "NUMBER",         // index 22
        "AND",            // index 23
        "CLASS",          // index 24
        "ELSE",           // index 25
        "FALSE",          // index 26
        "FUN",            // index 27
        "FOR",            // index 28
        "IF",             // index 29
        "NIL",            // index 30
        "OR",             // index 31
        "PRINT",          // index 32
        "RETURN",         // index 33
        "SUPER",          // index 34
        "THIS",           // index 35
        "TRUE",           // index 36
        "VAR",            // index 37
        "WHILE",          // index 38
        "END_OF_FILE"     // index 39
    };

    struct Token {
        Token() : type(TokenType::NONE), lexeme(""), line(0) {}
        Token(TokenType type, TokenValue lexeme, int line)
            : type(type), lexeme(lexeme), line(line) {}

        void print() const {
            cout << "Token type: " << TokenTypeNames[static_cast<int>(type)];
            if (type == TokenType::STRING) {
                cout << ", Literal: " << get<string>(lexeme);
            } else if (type == TokenType::NUMBER) {
                cout << ", Literal: " << get<double>(lexeme);
            } else {
                cout << ", Lexeme: " << get<string>(lexeme);
            }
            cout << ", Line: " << line << endl;
        }

        TokenType type;
        TokenValue lexeme;
        int line;
    };

} // Token

#endif // TOKEN_H