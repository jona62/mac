#include "Parser.h"
#include <iostream>

namespace parser {

Parser::Parser(const std::vector<token::Token>& tokens) : tokens(tokens), current(0) {}

Parser::~Parser() {}

void Parser::parse() {
    while (!isAtEnd()) {
        // Implement your parsing logic here
        // Example: parseVariableDeclaration(), parseExpression(), etc.
        // After parsing a statement, call advance() to move to the next token
        advance();
    }
}

bool Parser::isAtEnd() {
    return current >= tokens.size() || peek().type == token::TokenType::END_OF_FILE;
}

const token::Token& Parser::advance() {
    if (!isAtEnd()) current++;
    return previous();
}

const token::Token& Parser::peek() {
    if (current >= tokens.size()) {
        std::cerr << "Error: Attempt to access out-of-bounds token at index " << current << std::endl;
        exit(EXIT_FAILURE);
    }
    return tokens[current];
}

const token::Token& Parser::previous() {
    if (current == 0) {
        std::cerr << "Error: Attempt to access previous token when current is 0" << std::endl;
        exit(EXIT_FAILURE);
    }
    return tokens[current - 1];
}

bool Parser::match(token::TokenType type) {
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == token::TokenType::SEMICOLON) return;
        switch (peek().type) {
            case token::TokenType::CLASS:
            case token::TokenType::FUN:
            case token::TokenType::VAR:
            case token::TokenType::FOR:
            case token::TokenType::IF:
            case token::TokenType::WHILE:
            case token::TokenType::PRINT:
            case token::TokenType::RETURN:
                return;
            default:
                break;
        }
        advance();
    }
}

} // namespace parser