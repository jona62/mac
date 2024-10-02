#ifndef EXPR_H
#define EXPR_H

#include "Token.h"
#include <variant>
#include <string>
#include <memory>

using token::Token;
using std::string;
using std::variant;
using std::shared_ptr;
using std::make_shared;

namespace expr {

    // Forward declaration of Visitor
    template <typename Result>
    class Visitor;

    template <typename Result>
    class Expr {
    public:
        virtual Result visit(shared_ptr<Visitor<Result>> visitor) = 0;
    };

    template <typename Result>
    class Binary : public Expr<Result> {
    public:
        Binary(shared_ptr<Expr<Result>> left, Token operatorToken, shared_ptr<Expr<Result>> right)
            : left(left), operatorToken(operatorToken), right(right) {}

        Result visit(shared_ptr<Visitor<Result>> visitor) override {
            return visitor->visitBinaryExpr(this);
        }

        shared_ptr<Expr<Result>> left;
        Token operatorToken;
        shared_ptr<Expr<Result>> right;
    };

    template <typename Result>
    class Unary : public Expr<Result> {
    public:
        Unary(Token operatorToken, shared_ptr<Expr<Result>> right)
            : operatorToken(operatorToken), right(right) {}

        Result visit(shared_ptr<Visitor<Result>> visitor) override {
            return visitor->visitUnaryExpr(this);
        }

        Token operatorToken;
        shared_ptr<Expr<Result>> right;
    };

    template <typename Result>
    class Literal : public Expr<Result> {
    public:
        using LiteralValue = variant<string, double>;

        Literal(LiteralValue value) : value(value) {}

        Result visit(shared_ptr<Visitor<Result>> visitor) override {
            return visitor->visitLiteralExpr(this);
        }

        string toString() const {
            if (std::holds_alternative<string>(value)) {
                return std::get<string>(value);
            } else if (std::holds_alternative<double>(value)) {
                return std::to_string(std::get<double>(value));
            }
            return "nil";
        }

        LiteralValue value;
    };

    template <typename Result>
    class Grouping : public Expr<Result> {
    public:
        Grouping(shared_ptr<Expr<Result>> expression) : expression(expression) {}

        Result visit(shared_ptr<Visitor<Result>> visitor) override {
            return visitor->visitGroupingExpr(this);
        }

        shared_ptr<Expr<Result>> expression;
    };

    template <typename Result>
    class Variable : public Expr<Result> {
    public:
        Variable(Token name) : name(name) {}

        Result visit(shared_ptr<Visitor<Result>> visitor) override {
            return visitor->visitVariableExpr(this);
        }

        Token name;
    };

    template <typename Result>
    class Visitor {
    public:
        virtual Result visitBinaryExpr(Binary<Result>* expr) = 0;
        virtual Result visitUnaryExpr(Unary<Result>* expr) = 0;
        virtual Result visitLiteralExpr(Literal<Result>* expr) = 0;
        virtual Result visitVariableExpr(Variable<Result>* expr) = 0;
        virtual Result visitGroupingExpr(Grouping<Result>* expr) = 0;
    };

} // namespace expr

#endif /* EXPR_H */
