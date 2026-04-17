#include "Parser.h"                 // parser::Parser, parse<T>()
#include "ParserError.h"            // errors::ParseError
#include "Interpreter.h"            // interpreter::Interpreter (template instantiation)
#include <iostream>             // cerr (error output)

using expr::Unary;
using expr::Binary;

using token::TokenType;
using token::TokenValue;
using errors::ParseError;

using std::shared_ptr;
using std::make_shared;

namespace parser {

static int destructureCounter = 0;

Parser::Parser(const std::vector<token::Token>& tokens) : tokens(tokens), current(0) {}

Parser::~Parser() {}

// --- Top-level parsing ---

template <typename T>
std::vector<shared_ptr<stmt::Stmt<T>>> Parser::parse() {
    std::vector<shared_ptr<stmt::Stmt<T>>> statements;
    while (!isAtEnd()) {
        // Array destructuring: var [a, b] = expr; or val [a, b] = expr;
        if ((peek().type == TokenType::VAR || peek().type == TokenType::VAL)
            && current + 1 < tokens.size()
            && tokens[current + 1].type == TokenType::LEFT_BRACKET) {
            bool isVal = peek().type == TokenType::VAL;
            advance(); // consume VAR/VAL
            auto stmts = varDestructuring<T>(isVal);
            for (auto& s : stmts) statements.push_back(s);
            continue;
        }
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
        if (match(TokenType::VAR)) return varDeclaration<T>(false);
        if (match(TokenType::VAL)) return varDeclaration<T>(true);
        if (match(TokenType::ENUM)) return enumDeclaration<T>();
        if (match(TokenType::EFFECT)) return effectDeclaration<T>();
        if (match(TokenType::STYLE)) return styleDeclaration<T>();
        return statement<T>();
    } catch (const ParseError& error) {
        std::cerr << error.what() << std::endl;
        synchronize();
        return nullptr;
    }
}

template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::varDeclaration(bool isVal) {
    consume(TokenType::IDENTIFIER, "Expected variable name.");
    Token name = previous();

    shared_ptr<Expr<T>> initializer = nullptr;
    if (match(TokenType::EQUAL)) {
        initializer = expression<T>();
    }

    if (isVal && !initializer) {
        throw ParseError(name, "'val' declarations must have an initializer.");
    }

    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");
    return make_shared<stmt::VarStmt<T>>(name, initializer, isVal);
}

template <typename T>
std::vector<shared_ptr<stmt::Stmt<T>>> Parser::varDestructuring(bool isVal) {
    // Already consumed 'var'/'val', next token is '['
    advance(); // consume '['
    int line = previous().line;

    std::vector<Token> names;
    if (peek().type != TokenType::RIGHT_BRACKET) {
        do {
            consume(TokenType::IDENTIFIER, "Expected variable name in destructuring pattern.");
            names.push_back(previous());
        } while (match(TokenType::COMMA));
    }
    consume(TokenType::RIGHT_BRACKET, "Expected ']' after destructuring pattern.");
    consume(TokenType::EQUAL, "Expected '=' after destructuring pattern.");
    auto initializer = expression<T>();
    consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.");

    // Desugar into flat statements (no block, same scope):
    //   var __destructure_N__ = expr;
    //   var a = __destructure_N__[0];
    //   var b = __destructure_N__[1]; ...
    std::string tmpName = "__destructure_" + std::to_string(destructureCounter++) + "__";
    Token tmpToken(TokenType::IDENTIFIER, tmpName, line);
    Token bracketToken(TokenType::LEFT_BRACKET, std::string("["), line);

    std::vector<shared_ptr<stmt::Stmt<T>>> stmts;
    stmts.push_back(make_shared<stmt::VarStmt<T>>(tmpToken, initializer));

    for (size_t i = 0; i < names.size(); i++) {
        auto tmpVar = make_shared<expr::Variable<T>>(tmpToken);
        auto index = make_shared<expr::Literal<T>>(token::TokenValue(static_cast<double>(i)));
        auto indexGet = make_shared<expr::IndexGet<T>>(tmpVar, bracketToken, index);
        stmts.push_back(make_shared<stmt::VarStmt<T>>(names[i], indexGet, isVal));
    }

    return stmts;
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
    auto tailExpr = pendingTailExpr_;
    pendingTailExpr_ = nullptr;
    return make_shared<stmt::FunctionStmt<T>>(name, params, body, tailExpr);
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
shared_ptr<stmt::Stmt<T>> Parser::enumDeclaration() {
    Token keyword = previous();
    consume(TokenType::IDENTIFIER, "Expected enum name.");
    Token name = previous();
    consume(TokenType::LEFT_BRACE, "Expected '{' after enum name.");

    std::vector<typename stmt::EnumStmt<T>::Variant> variants;
    while (peek().type != TokenType::RIGHT_BRACE && !isAtEnd()) {
        consume(TokenType::IDENTIFIER, "Expected variant name.");
        Token variantName = previous();
        std::vector<Token> fields;
        if (match(TokenType::LEFT_PAREN)) {
            do {
                consume(TokenType::IDENTIFIER, "Expected field name.");
                fields.push_back(previous());
            } while (match(TokenType::COMMA));
            consume(TokenType::RIGHT_PAREN, "Expected ')' after variant fields.");
        }
        variants.push_back({variantName, std::move(fields)});
        match(TokenType::COMMA); // optional comma
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after enum variants.");

    return make_shared<stmt::EnumStmt<T>>(keyword, name, std::move(variants));
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
    if (match(TokenType::LEFT_BRACE)) {
        auto body = block<T>();
        auto tail = pendingTailExpr_;
        pendingTailExpr_ = nullptr;
        return make_shared<stmt::BlockStmt<T>>(body, tail);
    }
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

    // Check for for-in: for (var x in collection) or for (var [a, b] in collection)
    if (match(TokenType::VAR)) {
        // Array destructuring in for-in: for (var [a, b] in collection)
        if (peek().type == TokenType::LEFT_BRACKET) {
            advance(); // consume '['
            int line = previous().line;

            std::vector<Token> names;
            if (peek().type != TokenType::RIGHT_BRACKET) {
                do {
                    consume(TokenType::IDENTIFIER, "Expected variable name in destructuring pattern.");
                    names.push_back(previous());
                } while (match(TokenType::COMMA));
            }
            consume(TokenType::RIGHT_BRACKET, "Expected ']' after destructuring pattern.");
            consume(TokenType::IN, "Expected 'in' after destructuring pattern in for loop.");
            auto iterable = expression<T>();
            consume(TokenType::RIGHT_PAREN, "Expected ')' after for-in clause.");
            auto body = statement<T>();

            // Desugar: for (var [a, b] in collection) { body }
            // becomes: for (var __destructure_N__ in collection) {
            //              var a = __destructure_N__[0];
            //              var b = __destructure_N__[1];
            //              body
            //          }
            std::string tmpName = "__destructure_" + std::to_string(destructureCounter++) + "__";
            Token tmpToken(TokenType::IDENTIFIER, tmpName, line);
            Token bracketToken(TokenType::LEFT_BRACKET, std::string("["), line);

            std::vector<shared_ptr<stmt::Stmt<T>>> bodyStmts;
            for (size_t i = 0; i < names.size(); i++) {
                auto tmpVar = make_shared<expr::Variable<T>>(tmpToken);
                auto index = make_shared<expr::Literal<T>>(token::TokenValue(static_cast<double>(i)));
                auto indexGet = make_shared<expr::IndexGet<T>>(tmpVar, bracketToken, index);
                bodyStmts.push_back(make_shared<stmt::VarStmt<T>>(names[i], indexGet));
            }
            bodyStmts.push_back(body);

            auto newBody = make_shared<stmt::BlockStmt<T>>(bodyStmts);
            return make_shared<stmt::ForInStmt<T>>(tmpToken, iterable, newBody);
        }

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
    if (peek().type == TokenType::RIGHT_BRACE) {
        // Tail expression: no semicolon needed before }
        pendingTailExpr_ = expr;
        return nullptr;
    }
    consume(TokenType::SEMICOLON, "Expected ';' after expression.");
    return make_shared<stmt::ExpressionStmt<T>>(expr);
}

template <typename T>
std::vector<shared_ptr<stmt::Stmt<T>>> Parser::block() {
    std::vector<shared_ptr<stmt::Stmt<T>>> statements;
    pendingTailExpr_ = nullptr;

    while (!isAtEnd() && peek().type != TokenType::RIGHT_BRACE) {
        // Array destructuring: var [a, b] = expr; or val [a, b] = expr;
        if ((peek().type == TokenType::VAR || peek().type == TokenType::VAL)
            && current + 1 < tokens.size()
            && tokens[current + 1].type == TokenType::LEFT_BRACKET) {
            bool isVal = peek().type == TokenType::VAL;
            advance(); // consume VAR/VAL
            auto stmts = varDestructuring<T>(isVal);
            for (auto& s : stmts) statements.push_back(s);
            continue;
        }
        auto decl = declaration<T>();
        if (decl != nullptr) {
            statements.push_back(decl);
        } else if (pendingTailExpr_) {
            // Tail expression detected — stop parsing the block
            break;
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
        // Return a synthetic EOF token instead of crashing
        static const token::Token eof(token::TokenType::END_OF_FILE, token::TokenValue(std::string("")), 0);
        return eof;
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

    if (match(TokenType::INTERP_STRING)) {
        Token tok = previous();
        auto raw = std::get<std::string>(tok.lexeme);
        return parseInterpolatedString<T>(raw, tok);
    }

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
        auto tailExpr = pendingTailExpr_;
        pendingTailExpr_ = nullptr;
        return make_shared<expr::LambdaExpr<T>>(funToken, params, body, tailExpr);
    }

    // Arrow function: x -> expr
    if (peek().type == TokenType::IDENTIFIER && current + 1 < tokens.size()
        && tokens[current + 1].type == TokenType::ARROW) {
        advance(); // consume identifier
        Token param = previous();
        advance(); // consume ->
        auto body = expression<T>();
        std::vector<shared_ptr<stmt::Stmt<T>>> stmts;
        stmts.push_back(make_shared<stmt::ReturnStmt<T>>(param, body));
        return make_shared<expr::LambdaExpr<T>>(param, std::vector<Token>{param}, stmts);
    }

    // Contextual keyword blocks: gif, timeline, grid — must check BEFORE generic identifier
    if (peek().type == TokenType::IDENTIFIER) {
        auto* s = std::get_if<std::string>(&peek().lexeme);
        if (s && (*s == "gif" || *s == "grid")) {
            if (current + 1 < tokens.size()) {
                auto nextType = tokens[current + 1].type;
                auto* nextStr = std::get_if<std::string>(&tokens[current + 1].lexeme);
                bool isBlock = (nextType == TokenType::LEFT_BRACE) ||
                    (nextStr && *nextStr == "loop") ||
                    (nextType == TokenType::NUMBER);
                if (isBlock) {
                    advance();
                    if (*s == "gif") return gifBlock<T>();
                    if (*s == "grid") return gridBlock<T>();
                }
            }
        }
    }

    if (match(TokenType::MATCH)) return matchExpression<T>();

    if (match(TokenType::IDENTIFIER)) return make_shared<expr::Variable<T>>(previous());

    if (match(TokenType::LEFT_PAREN)) {
        // Try arrow function: (params) -> expr
        size_t savedPos = current;
        std::vector<Token> arrowParams;
        bool isArrow = false;

        if (peek().type == TokenType::IDENTIFIER || peek().type == TokenType::RIGHT_PAREN) {
            if (peek().type != TokenType::RIGHT_PAREN) {
                bool validParams = true;
                do {
                    if (peek().type != TokenType::IDENTIFIER) { validParams = false; break; }
                    advance();
                    arrowParams.push_back(previous());
                } while (match(TokenType::COMMA));
                if (!validParams) arrowParams.clear();
            }
            if (!arrowParams.empty() || peek().type == TokenType::RIGHT_PAREN) {
                if (peek().type == TokenType::RIGHT_PAREN) {
                    advance(); // consume )
                    if (peek().type == TokenType::ARROW) {
                        isArrow = true;
                    }
                }
            }
        }

        if (isArrow) {
            advance(); // consume ->
            Token arrowToken = previous();
            auto body = expression<T>();
            std::vector<shared_ptr<stmt::Stmt<T>>> stmts;
            stmts.push_back(make_shared<stmt::ReturnStmt<T>>(arrowToken, body));
            return make_shared<expr::LambdaExpr<T>>(arrowToken, arrowParams, stmts);
        }

        // Not an arrow — restore and parse as grouping
        current = savedPos;
        auto expr = expression<T>();
        consume(TokenType::RIGHT_PAREN, "Expected ')' after expression.");
        return make_shared<expr::Grouping<T>>(expr);
    }

    if (match(TokenType::LEFT_BRACKET)) return arrayLiteral<T>();

    if (match(TokenType::LEFT_BRACE)) {
        // Disambiguate: map literal vs block expression
        // Map: {} or { identifier/string : ... }
        // Block: { statements... tailExpr }
        bool isMap = false;
        if (peek().type == TokenType::RIGHT_BRACE) {
            isMap = true; // empty map {}
        } else if ((peek().type == TokenType::IDENTIFIER || peek().type == TokenType::STRING)
                   && current + 1 < tokens.size()
                   && tokens[current + 1].type == TokenType::COLON) {
            isMap = true; // { key: value, ... }
        }
        if (isMap) return mapLiteral<T>();

        // Block expression: { stmts... tailExpr }
        auto body = block<T>();
        auto tail = pendingTailExpr_;
        pendingTailExpr_ = nullptr;
        // Wrap as a block statement inside a lambda that's immediately called
        // Actually, we need a new expr node or we can reuse BlockStmt.
        // Simplest: create a BlockExpr that the interpreter can evaluate.
        // But for now, synthesize as an immediately-invoked lambda with no params.
        std::vector<Token> noParams;
        Token synth(TokenType::FUN, std::string(""), peek().line);
        auto lambda = make_shared<expr::LambdaExpr<T>>(synth, noParams, body, tail);
        // Immediately invoke it: (fun() { ... })()
        Token paren(TokenType::RIGHT_PAREN, std::string(")"), peek().line);
        std::vector<shared_ptr<Expr<T>>> noArgs;
        return make_shared<expr::Call<T>>(lambda, paren, noArgs);
    }

    if (match(TokenType::AT)) return memeLiteral<T>();

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
shared_ptr<Expr<T>> Parser::compose() {
    auto expr = assignment<T>();

    while (match(TokenType::COMPOSE)) {
        Token op = previous();
        auto right = assignment<T>();
        expr = make_shared<expr::ComposeExpr<T>>(expr, op, right);
    }

    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::pipe() {
    auto expr = compose<T>();

    while (match(TokenType::PIPE)) {
        Token op = previous();
        auto right = compose<T>();
        expr = make_shared<expr::PipeExpr<T>>(expr, op, right);
    }

    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::saveExpr() {
    auto expr = pipe<T>();

    if (match(TokenType::FAT_ARROW)) {
        Token op = previous();
        auto path = primary<T>(); // path is just a string literal
        expr = make_shared<expr::SaveExpr<T>>(expr, op, path);
    }

    return expr;
}

template <typename T>
shared_ptr<Expr<T>> Parser::expression() {
    if (++nestingDepth > MAX_NESTING)
        throw ParseError(peek(), "Expression nesting too deep.");
    auto result = saveExpr<T>();
    --nestingDepth;
    return result;
}

// --- Mac v2 syntax parsing ---

// Helper: parse duration like 400ms or 2s — returns milliseconds
double Parser::parseDuration() {
    consume(TokenType::NUMBER, "Expected duration number.");
    double num = std::get<double>(previous().lexeme);
    // Check for ms/s suffix
    if (peek().type == TokenType::IDENTIFIER) {
        auto* unit = std::get_if<std::string>(&peek().lexeme);
        if (unit && (*unit == "ms" || *unit == "s")) {
            advance();
            if (*unit == "s") num *= 1000;
        }
    }
    return num;
}

// style name { key: value, ... }
template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::styleDeclaration() {
    Token keyword = previous(); // the 'style' keyword
    consume(TokenType::IDENTIFIER, "Expected style name.");
    Token name = previous();
    consume(TokenType::LEFT_BRACE, "Expected '{' after style name.");
    std::vector<std::pair<Token, shared_ptr<Expr<T>>>> props;
    while (peek().type != TokenType::RIGHT_BRACE && !isAtEnd()) {
        consume(TokenType::IDENTIFIER, "Expected property name.");
        Token key = previous();
        consume(TokenType::COLON, "Expected ':' after property name.");
        auto value = expression<T>();
        props.push_back({key, value});
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after style block.");
    return make_shared<stmt::StyleStmt<T>>(keyword, name, std::move(props));
}

// effect name = compose_expr;
template <typename T>
shared_ptr<stmt::Stmt<T>> Parser::effectDeclaration() {
    Token keyword = previous(); // the 'effect' keyword
    consume(TokenType::IDENTIFIER, "Expected effect name.");
    Token name = previous();
    consume(TokenType::EQUAL, "Expected '=' after effect name.");
    auto value = compose<T>();
    consume(TokenType::SEMICOLON, "Expected ';' after effect declaration.");
    return make_shared<stmt::EffectStmt<T>>(keyword, name, value);
}

// @templateName [WxH] { top: "...", bottom: "..." } or @templateName "one-liner"
template <typename T>
shared_ptr<Expr<T>> Parser::memeLiteral() {
    // Accept identifier (@two_panel) or string (@"path/to/image.png")
    if (!match(TokenType::IDENTIFIER) && !match(TokenType::STRING)) {
        throw ParseError(peek(), "Expected template name or path after '@'.");
    }
    Token templateName = previous();

    // Dotted template name: @category.name (e.g. @meme.shrek_smirk)
    if (templateName.type == TokenType::IDENTIFIER &&
        peek().type == TokenType::DOT && current + 1 < tokens.size() &&
        tokens[current + 1].type == TokenType::IDENTIFIER) {
        advance(); // consume DOT
        advance(); // consume second IDENTIFIER
        auto first = std::get<std::string>(templateName.lexeme);
        auto second = std::get<std::string>(previous().lexeme);
        templateName.lexeme = token::TokenValue(first + "." + second);
    }

    // Optional size: 400x300
    int width = 0, height = 0;
    if (peek().type == TokenType::NUMBER) {
        // Check if this is WxH (NUMBER then identifier starting with 'x' then NUMBER)
        // or just a number (could be part of a one-liner like @blank 42)
        if (current + 1 < tokens.size() && tokens[current + 1].type == TokenType::IDENTIFIER) {
            auto* xStr = std::get_if<std::string>(&tokens[current + 1].lexeme);
            if (xStr && xStr->length() > 0 && (*xStr)[0] == 'x') {
                advance(); // consume width NUMBER
                width = static_cast<int>(std::get<double>(previous().lexeme));
                // Parse xH: could be "x300" as one token or "x" then NUMBER
                advance(); // consume the x... identifier
                auto xIdent = std::get<std::string>(previous().lexeme);
                if (xIdent.length() > 1) {
                    height = std::stoi(xIdent.substr(1));
                } else {
                    consume(TokenType::NUMBER, "Expected height after 'x'.");
                    height = static_cast<int>(std::get<double>(previous().lexeme));
                }
            }
        }
    }

    // Optional style name: @template [WxH] styleName { ... }
    Token styleName;
    if (peek().type == TokenType::IDENTIFIER) {
        auto* s = std::get_if<std::string>(&peek().lexeme);
        // A style name is an identifier that's NOT a position key and is followed by { or string
        if (s && *s != "top" && *s != "bottom" && *s != "center" && *s != "loop") {
            if (current + 1 < tokens.size()) {
                auto nextType = tokens[current + 1].type;
                if (nextType == TokenType::LEFT_BRACE || nextType == TokenType::STRING) {
                    advance();
                    styleName = previous();
                }
            }
        }
    }

    std::vector<typename expr::MemeLiteralExpr<T>::TextEntry> entries;

    if (match(TokenType::LEFT_BRACE)) {
        // Named positions: top: "...", bottom: "..."
        while (peek().type != TokenType::RIGHT_BRACE && !isAtEnd()) {
            consume(TokenType::IDENTIFIER, "Expected position name (top, bottom, center).");
            Token key = previous();
            consume(TokenType::COLON, "Expected ':' after position name.");
            auto value = expression<T>();
            entries.push_back({key, value});
        }
        consume(TokenType::RIGHT_BRACE, "Expected '}' after meme literal.");
        return make_shared<expr::MemeLiteralExpr<T>>(templateName, entries, false, width, height, styleName);
    }

    // One-liner: @template expr (string literal, variable, or any primary expression)
    if (peek().type != TokenType::SEMICOLON && peek().type != TokenType::END_OF_FILE) {
        auto value = primary<T>();
        Token centerKey(TokenType::IDENTIFIER, token::TokenValue(std::string("center")),
                        templateName.line, templateName.column);
        entries.push_back({centerKey, value});
        return make_shared<expr::MemeLiteralExpr<T>>(templateName, entries, true, width, height, styleName);
    }

    throw ParseError(peek(), "Expected '{' or text after @template.");
}

// gif [loop] { @tmpl "text" : 400ms, --- crossfade 150ms --- ... }
template <typename T>
shared_ptr<Expr<T>> Parser::gifBlock() {
    Token keyword = previous();
    bool loop = false;
    Token loopToken;

    // Check for 'loop' keyword (contextual)
    if (peek().type == TokenType::IDENTIFIER) {
        auto* s = std::get_if<std::string>(&peek().lexeme);
        if (s && *s == "loop") { advance(); loop = true; loopToken = previous(); }
    }

    consume(TokenType::LEFT_BRACE, "Expected '{' after gif.");

    std::vector<typename expr::GifBlockExpr<T>::Entry> entries;
    while (peek().type != TokenType::RIGHT_BRACE && !isAtEnd()) {
        // Check for transition: --- type duration [easing] ---
        if (match(TokenType::TRIPLE_DASH)) {
            consume(TokenType::IDENTIFIER, "Expected transition type after '---'.");
            std::string transType = std::get<std::string>(previous().lexeme);
            double transMs = parseDuration();
            std::string easing = "linear";
            if (peek().type == TokenType::IDENTIFIER) {
                auto* s = std::get_if<std::string>(&peek().lexeme);
                if (s && (*s == "ease" || *s == "easeIn" || *s == "easeOut"
                          || *s == "easeInOut" || *s == "bounce" || *s == "linear")) {
                    advance();
                    easing = *s;
                }
            }
            consume(TokenType::TRIPLE_DASH, "Expected '---' after transition.");
            if (!entries.empty()) {
                auto trans = std::make_shared<typename expr::GifBlockExpr<T>::Transition>();
                trans->type = transType;
                trans->durationMs = transMs;
                trans->easing = easing;
                entries.back().transition = trans;
            }
            continue;
        }

        auto meme = expression<T>();
        consume(TokenType::COLON, "Expected ':' after meme in gif frame.");
        double ms = parseDuration();
        entries.push_back({meme, ms, nullptr});
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after gif block.");

    return make_shared<expr::GifBlockExpr<T>>(keyword, loop, std::move(entries), loopToken);
}

// grid NxM { entries }
template <typename T>
shared_ptr<Expr<T>> Parser::gridBlock() {
    Token keyword = previous();
    // Parse NxM: NUMBER then 'x' then NUMBER, or just NUMBER (assume square)
    consume(TokenType::NUMBER, "Expected grid columns (e.g., 2x2).");
    int cols = static_cast<int>(std::get<double>(previous().lexeme));
    int rows = cols; // default: square
    // Check for 'x' followed by number
    if (peek().type == TokenType::IDENTIFIER) {
        auto* s = std::get_if<std::string>(&peek().lexeme);
        if (s && s->length() > 0 && (*s)[0] == 'x') {
            // Could be "x2" or just "x" followed by number
            if (s->length() > 1) {
                rows = std::stoi(s->substr(1));
                advance();
            } else {
                advance(); // consume 'x'
                consume(TokenType::NUMBER, "Expected row count after 'x'.");
                rows = static_cast<int>(std::get<double>(previous().lexeme));
            }
        }
    }

    consume(TokenType::LEFT_BRACE, "Expected '{' after grid dimensions.");

    std::vector<shared_ptr<Expr<T>>> entries;
    while (peek().type != TokenType::RIGHT_BRACE && !isAtEnd()) {
        entries.push_back(expression<T>());
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after grid block.");

    return make_shared<expr::GridBlockExpr<T>>(keyword, cols, rows, std::move(entries));
}

template <typename T>
shared_ptr<Expr<T>> Parser::matchExpression() {
    Token keyword = previous();
    auto subject = expression<T>();
    consume(TokenType::LEFT_BRACE, "Expected '{' after match expression.");

    std::vector<typename expr::MatchExpr<T>::Arm> arms;
    while (peek().type != TokenType::RIGHT_BRACE && !isAtEnd()) {
        shared_ptr<Expr<T>> pattern = nullptr;
        std::vector<Token> bindings;
        // Check for wildcard _
        if (peek().type == TokenType::IDENTIFIER) {
            auto* s = std::get_if<std::string>(&peek().lexeme);
            if (s && *s == "_") {
                advance(); // consume _
                pattern = nullptr; // wildcard
            } else {
                pattern = expression<T>();
            }
        } else {
            pattern = expression<T>();
        }

        // Check for enum destructuring: if pattern is Call(Get(...), args)
        // e.g. Result.Ok(v) parses as Call(Get(Variable(Result), Ok), [Variable(v)])
        if (pattern) {
            if (auto* callExpr = dynamic_cast<expr::Call<T>*>(pattern.get())) {
                if (dynamic_cast<expr::Get<T>*>(callExpr->callee.get())) {
                    // Extract binding names from the call arguments (they are Variable exprs)
                    for (auto& arg : callExpr->arguments) {
                        if (auto* varExpr = dynamic_cast<expr::Variable<T>*>(arg.get())) {
                            bindings.push_back(varExpr->name);
                        }
                    }
                    // Use the Get expression as the pattern (strip the call)
                    pattern = callExpr->callee;
                }
            }
        }

        consume(TokenType::ARROW, "Expected '->' after match pattern.");
        auto result = expression<T>();
        arms.push_back({pattern, std::move(bindings), result});
    }
    consume(TokenType::RIGHT_BRACE, "Expected '}' after match arms.");

    return make_shared<expr::MatchExpr<T>>(keyword, subject, std::move(arms));
}

template <typename T>
shared_ptr<Expr<T>> Parser::parseInterpolatedString(const std::string& raw, const Token& tok) {
    shared_ptr<Expr<T>> result = nullptr;
    std::string segment;
    size_t i = 0;

    auto makeLit = [&](const std::string& s) {
        return make_shared<expr::Literal<T>>(
            TokenValue(s));
    };

    auto addPart = [&](shared_ptr<Expr<T>> part) {
        if (result) {
            Token plusTok(TokenType::PLUS, TokenValue(std::string("+")), tok.line, tok.column);
            result = make_shared<Binary<T>>(result, plusTok, part);
        } else {
            result = part;
        }
    };

    while (i < raw.size()) {
        // Handle \{ escape — produce literal {
        if (raw[i] == '\\' && i + 1 < raw.size() && raw[i + 1] == '{') {
            segment += '{';
            i += 2;
            continue;
        }
        if (raw[i] == '{') {
            // Emit the text segment accumulated so far
            if (!segment.empty()) {
                addPart(makeLit(segment));
                segment.clear();
            }
            // Find matching }
            i++; // skip opening {
            int depth = 1;
            std::string exprStr;
            while (i < raw.size() && depth > 0) {
                if (raw[i] == '{') depth++;
                else if (raw[i] == '}') {
                    depth--;
                    if (depth == 0) break;
                }
                exprStr += raw[i];
                i++;
            }
            if (i < raw.size()) i++; // skip closing }

            if (exprStr.empty()) {
                // Empty interpolation {} — emit empty string
                addPart(makeLit(""));
                continue;
            }

            // Parse the inner expression
            scanner::Scanner innerScanner(exprStr);
            std::vector<Token> innerTokens;
            for (auto& t : innerScanner) innerTokens.push_back(t);
            innerTokens.push_back(Token(TokenType::END_OF_FILE, TokenValue(std::string("")), tok.line, tok.column));

            Parser innerParser(innerTokens);
            auto expr = innerParser.expression<T>();

            // Start with empty string to ensure string context for +
            if (!result) {
                result = makeLit("");
            }
            addPart(expr);
            continue;
        }
        segment += raw[i];
        i++;
    }

    // Emit trailing text
    if (!segment.empty()) {
        addPart(makeLit(segment));
    }

    if (!result) {
        result = makeLit("");
    }

    return result;
}

void Parser::synchronize() {
    advance();
    while (!isAtEnd()) {
        if (previous().type == token::TokenType::SEMICOLON) return;
        switch (peek().type) {
            case token::TokenType::CLASS:
            case token::TokenType::FUN:
            case token::TokenType::VAL:
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
template shared_ptr<stmt::Stmt<MV>> Parser::varDeclaration<MV>(bool);
template std::vector<shared_ptr<stmt::Stmt<MV>>> Parser::varDestructuring<MV>(bool);
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
template shared_ptr<Expr<MV>> Parser::compose<MV>();
template shared_ptr<Expr<MV>> Parser::saveExpr<MV>();
template shared_ptr<Expr<MV>> Parser::pipe<MV>();
template shared_ptr<Expr<MV>> Parser::expression<MV>();
template shared_ptr<Expr<MV>> Parser::memeLiteral<MV>();
template shared_ptr<Expr<MV>> Parser::gifBlock<MV>();
template shared_ptr<Expr<MV>> Parser::gridBlock<MV>();
template shared_ptr<Expr<MV>> Parser::matchExpression<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::enumDeclaration<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::effectDeclaration<MV>();
template shared_ptr<stmt::Stmt<MV>> Parser::styleDeclaration<MV>();
template shared_ptr<Expr<MV>> Parser::parseInterpolatedString<MV>(const std::string&, const Token&);

} // namespace parser
