#ifndef RUNTIME_ERROR_H
#define RUNTIME_ERROR_H

#include <exception>
#include <string>
#include "Token.h"

namespace errors {

    class RuntimeError : public std::exception {
    private:
        std::string message;
    public:
        token::Token token;

        RuntimeError(const token::Token& token, const std::string& message)
            : token(token) {
            this->message = "[line " + std::to_string(token.line) + "] Runtime Error: " + message;
        }

        const char* what() const noexcept override {
            return message.c_str();
        }
    };

} // namespace errors

#endif // RUNTIME_ERROR_H
