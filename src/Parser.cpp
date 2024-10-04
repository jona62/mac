#include "Parser.h"
#include <iostream>
#include <AstPrinter.h>

using expr::Unary;
using expr::Binary;

using token::TokenType;
using token::TokenValue;

using std::shared_ptr;
using std::make_shared;

namespace parser {

Parser::Parser(const std::vector<token::Token>& tokens) : tokens(tokens), current(0) {}

Parser::~Parser() {}

void Parser::parse() {
    while (!isAtEnd()) {
        // This is a test case to check if the parser is working correctly
        auto expr = expression();
        auto printer = make_shared<printer::AstPrinter>();
        cout << expr->visit(printer) << endl;
    }
}

bool Parser::isAtEnd() {
    return current >= tokens.size();
}

// TODO(MEDIUM): Use scanner's iterator to get the next token
// This way, we can avoid storing all the tokens in memory
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
    if (isAtEnd()) return false;  // Prevent out-of-bounds access
    if (peek().type == type) {
        advance();
        return true;
    }
    return false;
}

template<typename... Type>
bool Parser::match(Type... types) {
    return (match(types) || ...);
}

shared_ptr<Expr> Parser::primary() {
    if (match(TokenType::FALSE)) return make_shared<expr::Literal>(TokenValue(false));
    if (match(TokenType::TRUE)) return make_shared<expr::Literal>(TokenValue(true));
    if (match(TokenType::NIL)) return make_shared<expr::Literal>(TokenValue(monostate {}));

    if (match(TokenType::NUMBER, TokenType::STRING)) return make_shared<expr::Literal>(previous().lexeme);

    if (match(TokenType::LEFT_PAREN)) {
        auto expr = equality();
        if (!match(TokenType::RIGHT_PAREN)) {
            std::cerr << "Error: Expected ')' after expression" << std::endl;
            exit(EXIT_FAILURE);
        }
        return make_shared<expr::Grouping>(expr);
    }
    std::cerr << "Error: Expected expression" << std::endl;
    exit(EXIT_FAILURE);
}

shared_ptr<Expr> Parser::unary() {
    if (match(TokenType::BANG, TokenType::MINUS)) {
        Token operation = previous();
        auto rightOperand = unary();
        return make_shared<Unary>(operation, rightOperand);
    }
    return primary();
}

shared_ptr<Expr> Parser::factor() {
    auto expr = unary();
    while(match(TokenType::SLASH, TokenType::STAR)) {
        Token operation = previous();
        auto rightOperand = unary();
        expr = make_shared<Binary>(expr, operation, rightOperand);
    }
    return expr;
}

shared_ptr<Expr> Parser::term() {
    auto expr = factor();
    while(match(TokenType::MINUS, TokenType::PLUS)) {
        Token operation = previous();
        auto rightOperand = factor();
        expr = make_shared<Binary>(expr, operation, rightOperand);
    }
    return expr;
}

shared_ptr<Expr> Parser::comparison() {
    auto expr = term();
    while(match(TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL)) {
        Token operation = previous();
        auto rightOperand = term();
        expr = make_shared<Binary>(expr, operation, rightOperand);
    }
    return expr;
}

shared_ptr<Expr> Parser::equality() {
    auto expr = comparison();
    while(match(TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL)) {
        Token operation = previous();
        auto rightOperand = comparison();
        expr = make_shared<Binary>(expr, operation, rightOperand);
    }
    return expr;
}

shared_ptr<Expr> Parser::expression() {
    return equality();
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