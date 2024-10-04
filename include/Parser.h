#ifndef PARSER_H
#define PARSER_H

#include <vector>
#include "Scanner.h"
#include "Expr.h"

using token::Token;
using expr::Expr;

namespace parser {

    class Parser {
    public:
        Parser(const std::vector<Token>& tokens);
        ~Parser();

        void parse();

    private:
        const std::vector<Token>& tokens;
        size_t current;

        bool isAtEnd();
        const Token& advance();
        const Token& peek();
        const Token& previous();
        template <typename... Type>
        bool match(Type... types);
        bool match(TokenType type);
        shared_ptr<Expr> primary();
        shared_ptr<Expr> unary();
        shared_ptr<Expr> factor();
        shared_ptr<Expr> term();
        shared_ptr<Expr> comparison();
        shared_ptr<Expr> equality();
        shared_ptr<Expr> expression();
        void synchronize();
        // Add more parsing functions as needed
    };
} // namespace parser

#endif /* PARSER_H */