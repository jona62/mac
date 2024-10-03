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

    // Forward declaration of Expr classes
    class Binary;
    class Unary;
    class Literal;
    class Grouping;
    class Variable;

    class Visitor {
    public:
        virtual string visitBinaryExpr(Binary* expr) = 0;
        virtual string visitUnaryExpr(Unary* expr) = 0;
        virtual string visitLiteralExpr(Literal* expr) = 0;
        virtual string visitGroupingExpr(Grouping* expr) = 0;
        virtual string visitVariableExpr(Variable* expr) = 0;
    };

    class Expr {
    public:
        virtual string visit(shared_ptr<Visitor> visitor) = 0;
    };

    class Binary : public Expr {
    public:
        Binary(shared_ptr<Expr> left, Token operatorToken, shared_ptr<Expr> right)
            : left(left), operatorToken(operatorToken), right(right) {}

        string visit(shared_ptr<Visitor> visitor) override {
            return visitor->visitBinaryExpr(this);
        }

        shared_ptr<Expr> left;
        Token operatorToken;
        shared_ptr<Expr> right;
    };

    class Unary : public Expr {
    public:
        Unary(Token operatorToken, shared_ptr<Expr> right)
            : operatorToken(operatorToken), right(right) {}

        string visit(shared_ptr<Visitor> visitor) override {
            return visitor->visitUnaryExpr(this);
        }

        Token operatorToken;
        shared_ptr<Expr> right;
    };

    class Literal : public Expr {
    public:
        using LiteralValue = variant<string, double>;

        Literal(LiteralValue value) : value(value) {}

        string visit(shared_ptr<Visitor> visitor) override {
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

    class Grouping : public Expr {
    public:
        Grouping(shared_ptr<Expr> expression) : expression(expression) {}

        string visit(shared_ptr<Visitor> visitor) override {
            return visitor->visitGroupingExpr(this);
        }

        shared_ptr<Expr> expression;
    };

    class Variable : public Expr {
    public:
        Variable(Token name) : name(name) {}

        string visit(shared_ptr<Visitor> visitor) override {
            return visitor->visitVariableExpr(this);
        }

        Token name;
    };
} // namespace expr

#endif /* EXPR_H */
