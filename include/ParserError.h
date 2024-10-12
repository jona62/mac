#ifndef PARSER_ERROR_H
#define PARSER_ERROR_H

#include <exception>
#include <string>
#include "Token.h"

using token::Token;

namespace errors {

    class ParseError : public std::exception {
    private:
        std::string message;
    public:
        ParseError(int line, const std::string& where, const std::string& message) {
            this->message = "[line " + std::to_string(line) + "] Error" + where + ": " + message;
        }

        ParseError(const token::Token& token, const std::string& message) {
            if (token.type == token::TokenType::END_OF_FILE) {
                this->message = "[line " + std::to_string(token.line) + "] Error at end: " + message;
            } else {
                this->message = "[line " + std::to_string(token.line) + "] Error at '" + token.tokenAsString() + "': " + message;
            }
        }

        const char* what() const noexcept override {
            return message.c_str();
        }
    };
} // namespace errors

#endif // PARSER_ERROR_H