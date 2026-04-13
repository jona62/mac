#include "Parser.h"
#include "ParserError.h"
#include "Interpreter.h"
#include <iostream>

using expr::Unary;
using expr::Binary;

using token::TokenType;
using token::TokenValue;
using errors::ParseError;

using std::shared_ptr;
using std::make_shared;

namespace parser {

Parser::Parser(const std::vector<token::Token>& tokens) : tokens(tokens), current(0) {}

Parser::~Parser() {}

// --- Top-level parsing ---

template <typename T>
std::vector<shared_ptr<stmt::Stmt<T>>> Parser::parse() {
    std::vector<shared_ptr<stmt::Stmt<T>>> statements;
    while (!isAtEnd()) {
        auto decl = declaration<T>();
        if (decl != nullptr) {
            statements.push_back(decl);
        }
    }
    return statements;
}

// --- Statement parsing ---

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::declaration() {
    try {
        if (match(TokenType::CLASS)) return classDeclaration<T>();
        // 2-token lookahead: only treat as function declaration if FUN is followed by IDENTIFIER
        if (peek().type == TokenType::FUN && current + 1 < tokens.size()
            && tokens[current + 1].type == TokenType::IDENTIFIER) {
            advance(); // consume FUN
            return functionDeclaration<T>("function");
        }
        if (match(TokenType::VAR)) return varDeclaration<T>();
        return statement<T>();
    } catch (const ParseError& error) {
        std::cerr << error.what() << std::endl;
        synchronize();
        return nullptr;
    }
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::varDeclaration() {
    consume(TokenType::IDENTIFIER, "Expected variable name.");
    Token name = previous();

    shared_ptr<Expr<T>> initializer = nullptr;
    if (match(TokenType::EQUAL)) {
        initializer = expression<T>();
    }

    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    return make_shared<stmt::VarStmt<T>>(name, initializer);
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::functionDeclaration(const std::string& kind) {
    consume(TokenType::IDENTIFIER, "Expected " + kind + " name.");
    Token name = previous();
    consume(TokenType::LEFT_PAREN, "Expected '(' after " + kind + " name.");

    std::vector<Token> params;
    if (peek().type != TokenType::RIGHT_PAREN) {
        do {
            if (params.size() >= 255) {
                std::cerr << ParseError(peek(), "Can't have more than 255 parameters.").what() << std::endl;
            }
            consume(TokenType::IDENTIFIER, "Expected parameter name.");
            params.push_back(previous());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_PAREN, "Expected ')' after parameters.");

    consume(TokenType::LEFT_BRACE, "Expected '{' before " + kind + " body.");
    auto body = block<T>();
    return make_shared<stmt::FunctionStmt<T>>(name, params, body);
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::classDeclaration() {
    consume(TokenType::IDENTIFIER, "Expected class name.");
    Token name = previous();

    shared_ptr<expr::Variable<T>> superclass = nullptr;
    if (match(TokenType::LESS)) {
        consume(TokenType::IDENTIFIER, "Expected superclass name.");
        superclass = make_shared<expr::Variable<T>>(previous());
    }

    consume(TokenType::LEFT_BRACE, "Expected '{' before class body.");

    std::vector<shared_ptr<stmt::FunctionStmt<T>>> methods;
    while (!isAtEnd() && peek().type != TokenType::RIGHT_BRACE) {
        auto method = functionDeclaration<T>("method");
        methods.push_back(std::dynamic_pointer_cast<stmt::FunctionStmt<T>>(method));
    }

    consume(TokenType::RIGHT_BRACE, "Expected '}' after class body.");
    return make_shared<stmt::ClassStmt<T>>(name, superclass, methods);
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::statement() {
    if (match(TokenType::IF)) return ifStatement<T>();
    if (match(TokenType::PRINT)) return printStatement<T>();
    if (match(TokenType::RETURN)) {
        Token keyword = previous();
        shared_ptr<Expr<T>> value = nullptr;
        if (peek().type != TokenType::SEMICOLON) {
            value = expression<T>();
        }
        consume(TokenType::SEMICOLON, "Expected ';' after return value.");
        return make_shared<stmt::ReturnStmt<T>>(keyword, value);
    }
    if (match(TokenType::BREAK)) {
        Token keyword = previous();
        consume(TokenType::SEMICOLON, "Expected ';' after 'break'.");
        return make_shared<stmt::BreakStmt<T>>(keyword);
    }
    if (match(TokenType::CONTINUE)) {
        Token keyword = previous();
        consume(TokenType::SEMICOLON, "Expected ';' after 'continue'.");
        return make_shared<stmt::ContinueStmt<T>>(keyword);
    }
    if (match(TokenType::WHILE)) return whileStatement<T>();
    if (match(TokenType::FOR)) return forStatement<T>();
    if (match(TokenType::LEFT_BRACE)) return make_shared<stmt::BlockStmt<T>>(block<T>());
    return expressionStatement<T>();
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::printStatement() {
    auto value = expression<T>();
    consume(TokenType::SEMICOLON, "Expected ';' after value.");
    return make_shared<stmt::PrintStmt<T>>(value);
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::ifStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'if'.");
    auto condition = expression<T>();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after if condition.");

    auto thenBranch = statement<T>();
    shared_ptr<stmt::Stmt<T>> elseBranch = nullptr;
    if (match(TokenType::ELSE)) {
        elseBranch = statement<T>();
    }

    return make_shared<stmt::IfStmt<T>>(condition, thenBranch, elseBranch);
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::whileStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'while'.");
    auto condition = expression<T>();
    consume(TokenType::RIGHT_PAREN, "Expected ')' after while condition.");

    auto body = statement<T>();
    return make_shared<stmt::WhileStmt<T>>(condition, body);
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::forStatement() {
    consume(TokenType::LEFT_PAREN, "Expected '(' after 'for'.");

    // Check for for-in: for (var x in collection)
    if (match(TokenType::VAR)) {
        consume(TokenType::IDENTIFIER, "Expected variable name.");
        Token varName = previous();

        if (match(TokenType::IN)) {
            // for-in loop
            auto iterable = expression<T>();
            consume(TokenType::RIGHT_PAREN, "Expected ')' after for-in clause.");
            auto body = statement<T>();
            return make_shared<stmt::ForInStmt<T>>(varName, iterable, body);
        }

        // C-style for with var initializer — we already consumed VAR and IDENTIFIER
        shared_ptr<Expr<T>> init = nullptr;
        if (match(TokenType::EQUAL)) {
            init = expression<T>();
        }
        consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
        auto initializer = make_shared<stmt::VarStmt<T>>(varName, init);

        // Condition
        shared_ptr<expr::Expr<T>> condition = nullptr;
        if (peek().type != TokenType::SEMICOLON) {
            condition = expression<T>();
        }
        consume(TokenType::SEMICOLON, "Expected ';' after loop condition.");

        // Increment
        shared_ptr<expr::Expr<T>> increment = nullptr;
        if (peek().type != TokenType::RIGHT_PAREN) {
            increment = expression<T>();
        }
        consume(TokenType::RIGHT_PAREN, "Expected ')' after for clauses.");

        auto body = statement<T>();

        // Desugar
        if (increment != nullptr) {
            std::vector<shared_ptr<stmt::Stmt<T>>> bodyStatements;
            bodyStatements.push_back(body);
            bodyStatements.push_back(make_shared<stmt::ExpressionStmt<T>>(increment));
            body = make_shared<stmt::BlockStmt<T>>(bodyStatements);
        }
        if (condition == nullptr) {
            condition = make_shared<expr::Literal<T>>(token::TokenValue(true));
        }
        body = make_shared<stmt::WhileStmt<T>>(condition, body);

        std::vector<shared_ptr<stmt::Stmt<T>>> bodyStatements;
        bodyStatements.push_back(initializer);
        bodyStatements.push_back(body);
        return make_shared<stmt::BlockStmt<T>>(bodyStatements);
    }

    // C-style for without var
    shared_ptr<stmt::Stmt<T>> initializer;
    if (match(TokenType::SEMICOLON)) {
        initializer = nullptr;
    } else {
        initializer = expressionStatement<T>();
    }

    shared_ptr<expr::Expr<T>> condition = nullptr;
    if (peek().type != TokenType::SEMICOLON) {
        condition = expression<T>();
    }
    consume(TokenType::SEMICOLON, "Expected ';' after loop condition.");

    shared_ptr<expr::Expr<T>> increment = nullptr;
    if (peek().type != TokenType::RIGHT_PAREN) {
        increment = expression<T>();
    }
    consume(TokenType::RIGHT_PAREN, "Expected ')' after for clauses.");

    auto body = statement<T>();

    if (increment != nullptr) {
        std::vector<shared_ptr<stmt::Stmt<T>>> bodyStatements;
        bodyStatements.push_back(body);
        bodyStatements.push_back(make_shared<stmt::ExpressionStmt<T>>(increment));
        body = make_shared<stmt::BlockStmt<T>>(bodyStatements);
    }
    if (condition == nullptr) {
        condition = make_shared<expr::Literal<T>>(token::TokenValue(true));
    }
    body = make_shared<stmt::WhileStmt<T>>(condition, body);
    if (initializer != nullptr) {
        std::vector<shared_ptr<stmt::Stmt<T>>> bodyStatements;
        bodyStatements.push_back(initializer);
        bodyStatements.push_back(body);
        body = make_shared<stmt::BlockStmt<T>>(bodyStatements);
    }

    return body;
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::expressionStatement() {
    auto expr = expression<T>();
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return make_shared<stmt::ExpressionStmt<T>>(expr);
}

template <typename T>
std::vector<shared_ptr<stmt::Stmt<T>>> Parser::block() {
    std::vector<shared_ptr<stmt::Stmt<T>>> statements;

    while (!isAtEnd() && peek().type != TokenType::RIGHT_BRACE) {
        auto decl = declaration<T>();
        if (decl != nullptr) {
            statements.push_back(decl);
        }
    }

    consume(TokenType::RIGHT_BRACE, "Expected '}' after block.");
    return statements;
}

// --- Utility ---

bool Parser::isAtEnd() {
    return current >= tokens.size();
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

void Parser::consume(const token::TokenType& type, const std::string& message) {
    if (match(type)) return;
    throw ParseError(peek(), message);
}

bool Parser::match(token::TokenType type) {
    if (isAtEnd()) return false;
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

// --- Expression parsing ---

template <typename T>
shared_ptr<Expr<T>> Parser::primary() {
    if (match(TokenType::FALSE)) return make_shared<expr::Literal<T>>(TokenValue(false));
    if (match(TokenType::TRUE)) return make_shared<expr::Literal<T>>(TokenValue(true));
    if (match(TokenType::NIL)) return make_shared<expr::Literal<T>>(TokenValue(monostate {}));

    if (match(TokenType::NUMBER, TokenType::STRING)) return make_shared<expr::Literal<T>>(previous().lexeme);

    if (match(TokenType::SUPER)) {
        Token keyword = previous();
        consume(TokenType::DOT, "Expected '.' after 'super'.");
        consume(TokenType::IDENTIFIER, "Expected superclass method name.");
        Token method = previous();
        return make_shared<expr::Super<T>>(keyword, method);
    }

    if (match(TokenType::THIS)) return make_shared<expr::This<T>>(previous());

    if (match(TokenType::FUN)) {
        // Lambda: fun(params) { body }
        Token funToken = previous();
        consume(TokenType::LEFT_PAREN, "Expected '(' for lambda.");
        std::vector<Token> params;
        if (peek().type != TokenType::RIGHT_PAREN) {
            do {
                consume(TokenType::IDENTIFIER, "Expected parameter name.");
                params.push_back(previous());
            } while (match(TokenType::COMMA));
        }
        consume(TokenType::RIGHT_PAREN, "Expected ')' after lambda parameters.");
        consume(TokenType::LEFT_BRACE, "Expected '{' before lambda body.");
        auto body = block<T>();
        return make_shared<expr::LambdaExpr<T>>(funToken, params, body);
    }

    if (match(TokenType::IDENTIFIER)) return make_shared<expr::Variable<T>>(previous());

    if (match(TokenType::LEFT_PAREN)) {
        auto expr = expression<T>();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
        return make_shared<expr::Grouping<T>>(expr);
    }

    if (match(TokenType::LEFT_BRACKET)) return arrayLiteral<T>();

    if (match(TokenType::LEFT_BRACE)) return mapLiteral<T>();

    throw ParseError(peek(), "Expected expression.");
}

template <typename T>
shared_ptr<Expr<T>> Parser::arrayLiteral() {
    Token bracket = previous();
    std::vector<shared_ptr<Expr<T>>> elements;
    if (peek().type != TokenType::RIGHT_BRACKET) {
        do {
            elements.push_back(expression<T>());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_BRACKET, "Expected ']' after array elements.");
    return make_shared<expr::ArrayExpr<T>>(bracket, elements);
}

template <typename T>
shared_ptr<Expr<T>> Parser::mapLiteral() {
    Token brace = previous();
    std::vector<Token> keys;
    std::vector<shared_ptr<Expr<T>>> values;

    if (peek().type != TokenType::RIGHT_BRACE) {
        do {
            if (match(TokenType::IDENTIFIER) || match(TokenType::STRING)) {
                keys.push_back(previous());
            } else {
                throw ParseError(peek(), "Expected map key (identifier or string).");
            }
            consume(TokenType::COLON, "Expected ':' after map key.");
            values.push_back(expression<T>());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after map entries.");
    return make_shared<expr::MapExpr<T>>(brace, keys, values);
}

template <typename T>
shared_ptr<Expr<T>> Parser::finishCall(shared_ptr<Expr<T>> callee) {
    std::vector<shared_ptr<Expr<T>>> arguments;
    if (peek().type != TokenType::RIGHT_PAREN) {
        do {
            if (arguments.size() >= 255) {
                std::cerr << ParseError(peek(), "Can't have more than 255 arguments.").what() << std::endl;
            }
            arguments.push_back(expression<T>());
        } while (match(TokenType::COMMA));
    }

    consume(TokenType::RIGHT_PAREN, "Expected ')' after arguments.");
    Token paren = previous();
    return make_shared<expr::Call<T>>(callee, paren, arguments);
}

template <typename T>
shared_ptr<Expr<T>> Parser::call() {
    auto expr = primary<T>();

    while (true) {
        if (match(TokenType::LEFT_PAREN)) {
            expr = finishCall<T>(expr);
        } else if (match(TokenType::DOT)) {
            consume(TokenType::IDENTIFIER, "Expected property name after '.'.");
            Token name = previous();
            expr = make_shared<expr::Get<T>>(expr, name);
        } else if (match(TokenType::LEFT_BRACKET)) {
            auto index = expression<T>();
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after index.");
            Token bracket = previous();
            expr = make_shared<expr::IndexGet<T>>(expr, bracket, index);
        } else {
            break;
        }
    }

    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::unary() {
    if (match(TokenType::BANG, TokenType::MINUS)) {
        Token operation = previous();
        auto rightOperand = unary<T>();
        return make_shared<Unary<T>>(operation, rightOperand);
    }
    return call<T>();
}

template <typename T>
shared_ptr<Expr<T>> Parser::factor() {
    auto expr = unary<T>();
    while(match(TokenType::SLASH, TokenType::STAR, TokenType::PERCENT)) {
        Token operation = previous();
        auto rightOperand = unary<T>();
        expr = make_shared<Binary<T>>(expr, operation, rightOperand);
    }
    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::term() {
    auto expr = factor<T>();
    while(match(TokenType::MINUS, TokenType::PLUS)) {
        Token operation = previous();
        auto rightOperand = factor<T>();
        expr = make_shared<Binary<T>>(expr, operation, rightOperand);
    }
    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::comparison() {
    auto expr = term<T>();
    while(match(TokenType::GREATER, TokenType::GREATER_EQUAL, TokenType::LESS, TokenType::LESS_EQUAL)) {
        Token operation = previous();
        auto rightOperand = term<T>();
        expr = make_shared<Binary<T>>(expr, operation, rightOperand);
    }
    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::equality() {
    auto expr = comparison<T>();
    while(match(TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL)) {
        Token operation = previous();
        auto rightOperand = comparison<T>();
        expr = make_shared<Binary<T>>(expr, operation, rightOperand);
    }
    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::logicalOr() {
    auto expr = logicalAnd<T>();
    while (match(TokenType::OR)) {
        Token operation = previous();
        auto right = logicalAnd<T>();
        expr = make_shared<expr::Logical<T>>(expr, operation, right);
    }
    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::logicalAnd() {
    auto expr = equality<T>();
    while (match(TokenType::AND)) {
        Token operation = previous();
        auto right = equality<T>();
        expr = make_shared<expr::Logical<T>>(expr, operation, right);
    }
    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::assignment() {
    auto expr = logicalOr<T>();

    if (match(TokenType::EQUAL)) {
        Token equals = previous();
        auto value = assignment<T>();

        if (auto* varExpr = dynamic_cast<expr::Variable<T>*>(expr.get())) {
            Token name = varExpr->name;
            return make_shared<expr::Assign<T>>(name, value);
        }

        if (auto* getExpr = dynamic_cast<expr::Get<T>*>(expr.get())) {
            return make_shared<expr::Set<T>>(getExpr->object, getExpr->name, value);
        }

        if (auto* indexExpr = dynamic_cast<expr::IndexGet<T>*>(expr.get())) {
            return make_shared<expr::IndexSet<T>>(indexExpr->object, indexExpr->bracket, indexExpr->index, value);
        }

        std::cerr << ParseError(equals, "Invalid assignment target.").what() << std::endl;
    }

    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::expression() {
    return assignment<T>();
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

// Explicit template instantiations for MacValue
using MV = interpreter::MacValue;
template std::vector<shared_ptr<stmt::Stmt<MV>>> Parser::parse<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::declaration<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::varDeclaration<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::functionDeclaration<MV>(const std::string&);
template shared_ptr<stmt::Stmt<MV>> Parser::classDeclaration<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::statement<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::printStatement<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::ifStatement<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::whileStatement<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::forStatement<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::expressionStatement<MV>();
template std::vector<shared_ptr<stmt::Stmt<MV>>> Parser::block<MV>();
template shared_ptr<Expr<MV>> Parser::finishCall<MV>(shared_ptr<Expr<MV>>);
template shared_ptr<Expr<MV>> Parser::call<MV>();
template shared_ptr<Expr<MV>> Parser::primary<MV>();
template shared_ptr<Expr<MV>> Parser::arrayLiteral<MV>();
template shared_ptr<Expr<MV>> Parser::mapLiteral<MV>();
template shared_ptr<Expr<MV>> Parser::unary<MV>();
template shared_ptr<Expr<MV>> Parser::factor<MV>();
template shared_ptr<Expr<MV>> Parser::term<MV>();
template shared_ptr<Expr<MV>> Parser::comparison<MV>();
template shared_ptr<Expr<MV>> Parser::equality<MV>();
template shared_ptr<Expr<MV>> Parser::logicalOr<MV>();
template shared_ptr<Expr<MV>> Parser::logicalAnd<MV>();
template shared_ptr<Expr<MV>> Parser::assignment<MV>();
template shared_ptr<Expr<MV>> Parser::expression<MV>();

} // namespace parser
