#ifndef PARSER_H
#define PARSER_H

#include <vector>               // vector (token list, parsed statements)
#include "Scanner.h"            // scanner::Scanner (token source)
#include "Expr.h"               // expr::Expr<T>, expression subclasses
#include "Stmt.h"               // stmt::Stmt<T>, statement subclasses

using token::Token;
using expr::Expr;

namespace parser {

    class Parser {
    public:
        Parser(const std::vector<Token>& tokens);
        ~Parser();

        template <typename T>
        std::vector<shared_ptr<stmt::Stmt<T>>> parse();

        static constexpr int MAX_NESTING = 512;

    private:
        const std::vector<Token>& tokens;
        size_t current;
        int nestingDepth = 0;

        bool isAtEnd();
        const Token& advance();
        const Token& peek();
        const Token& previous();
        void consume(const TokenType&, const string&);

        template <typename... Type>
        bool match(Type... types);
        bool match(TokenType type);

        // Statement parsing
        template <typename T>
        shared_ptr<stmt::Stmt<T>> declaration();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> varDeclaration(bool isVal = false);
        template <typename T>
        std::vector<shared_ptr<stmt::Stmt<T>>> varDestructuring(bool isVal = false);
        template <typename T>
        shared_ptr<stmt::Stmt<T>> functionDeclaration(const std::string& kind);
        template <typename T>
        shared_ptr<stmt::Stmt<T>> classDeclaration();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> enumDeclaration();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> statement();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> printStatement();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> ifStatement();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> whileStatement();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> forStatement();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> expressionStatement();
        template <typename T>
        std::vector<shared_ptr<stmt::Stmt<T>>> block();

        // Expression parsing
        template <typename T>
        shared_ptr<Expr<T>> finishCall(shared_ptr<Expr<T>> callee);
        template <typename T>
        shared_ptr<Expr<T>> call();
        template <typename T>
        shared_ptr<Expr<T>> primary();
        template <typename T>
        shared_ptr<Expr<T>> arrayLiteral();
        template <typename T>
        shared_ptr<Expr<T>> mapLiteral();
        template <typename T>
        shared_ptr<Expr<T>> unary();
        template <typename T>
        shared_ptr<Expr<T>> factor();
        template <typename T>
        shared_ptr<Expr<T>> term();
        template <typename T>
        shared_ptr<Expr<T>> comparison();
        template <typename T>
        shared_ptr<Expr<T>> equality();
        template <typename T>
        shared_ptr<Expr<T>> logicalOr();
        template <typename T>
        shared_ptr<Expr<T>> logicalAnd();
        template <typename T>
        shared_ptr<Expr<T>> assignment();
        template <typename T>
        shared_ptr<Expr<T>> compose();
        template <typename T>
        shared_ptr<Expr<T>> saveExpr();
        template <typename T>
        shared_ptr<Expr<T>> pipe();
        template <typename T>
        shared_ptr<Expr<T>> expression();
        template <typename T>
        shared_ptr<Expr<T>> memeLiteral();
        template <typename T>
        shared_ptr<Expr<T>> gifBlock();
        template <typename T>
        shared_ptr<Expr<T>> gridBlock();
        template <typename T>
        shared_ptr<Expr<T>> matchExpression();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> effectDeclaration();
        template <typename T>
        shared_ptr<stmt::Stmt<T>> styleDeclaration();
        template <typename T>
        shared_ptr<Expr<T>> parseInterpolatedString(const std::string& raw, const Token& tok);
        double parseDuration();

        void synchronize();
    };
} // namespace parser

#endif /* PARSER_H */
