#ifndef EXPR_H
#define EXPR_H

#include "Token.h" // for token::Token
#include <variant>
#include <string>

using token::Token;
using std::string;
using std::variant;
using std::holds_alternative;
using std::get;
using std::to_string;

namespace expr {
    using ObjectValue = variant<string, double>;

    // Forward declaration of Visitor
    template <typename Result>
    class Visitor;

    template <typename Result = void>
    class Expr {
    public:
        virtual Result visit(Visitor<Result>* visitor) = 0;
    };

    template <typename Result>
    class Binary : public Expr<Result> {
    public:
        Binary(Expr<Result>* left, const Token &operatorToken, Expr<Result>* right)
            : left(left), operatorToken(operatorToken), right(right) {}

        Result visit(Visitor<Result>* visitor) override {
            return visitor->visitBinaryExpr(this);
        }

        Expr<Result>* left;
        const Token operatorToken;
        Expr<Result>* right;
    };

    template <typename Result>
    class Unary : public Expr<Result> {
    public:
        Unary(const Token &operatorToken, Expr<Result>* right)
            : operatorToken(operatorToken), right(right) {}

        Result visit(Visitor<Result>* visitor) override {
            return visitor->visitUnaryExpr(this);
        }

        const Token operatorToken;
        Expr<Result>* right;
    };

    template <typename Result>
    class Literal : public Expr<Result> {
    public:
        Literal(const ObjectValue& literal)
            : literal(literal) {}

        Result visit(Visitor<Result>* visitor) override {
            return visitor->visitLiteralExpr(this);
        }

        string toString() const {
            if (holds_alternative<string>(literal)) {
                return get<string>(literal);
            } else if (holds_alternative<double>(literal)) {
                return to_string(get<double>(literal));
            }
            return "None";
        }

        ObjectValue literal;
    };

    template <typename Result>
    class Grouping : public Expr<Result> {
    public:
        Grouping(Expr<Result>* expression)
            : expression(expression) {}

        Result visit(Visitor<Result>* visitor) override {
            return visitor->visitGroupingExpr(this);
        }

        Expr<Result>* expression;
    };

    template <typename Result>
    class Variable : public Expr<Result> {
    public:
        Variable(const Token &name)
            : name(name) {}

        Result visit(Visitor<Result>* visitor) override {
            return visitor->visitVariableExpr(this);
        }

        Token name;
    };
    
    template <typename Result>
    class Visitor {
    public:
        virtual Result visitBinaryExpr(Binary<Result>* expr) = 0;
        virtual Result visitUnaryExpr(Unary<Result>* expr) = 0;
        virtual Result visitGroupingExpr(Grouping<Result>* expr) = 0;
        virtual Result visitLiteralExpr(Literal<Result>* expr) = 0;
        virtual Result visitVariableExpr(Variable<Result>* expr) = 0;
    };

} // namespace expr

#endif /* EXPR_H */
