#ifndef EXPR_H
#define EXPR_H

#include <memory>
#include <string>
#include <vector>
#include "Token.h"

using std::shared_ptr;
using std::string;
using token::Token;

// Forward declare for LambdaExpr
namespace stmt {
    template <typename T> class Stmt;
}

namespace expr {

    template <typename T>
    class Visitor;

    template <typename T>
    class Expr {
    public:
        virtual T visit(shared_ptr<Visitor<T>> visitor) = 0;
        virtual ~Expr() = default;
    };

    template <typename T>
    class Binary : public Expr<T> {
    public:
        Binary(shared_ptr<Expr<T>> left, Token operatorToken, shared_ptr<Expr<T>> right)
            : left(left), operatorToken(operatorToken), right(right) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitBinaryExpr(this);
        }

        shared_ptr<Expr<T>> left;
        Token operatorToken;
        shared_ptr<Expr<T>> right;
    };

    template <typename T>
    class Unary : public Expr<T> {
    public:
        Unary(Token operatorToken, shared_ptr<Expr<T>> right)
            : operatorToken(operatorToken), right(right) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitUnaryExpr(this);
        }

        Token operatorToken;
        shared_ptr<Expr<T>> right;
    };

    template <typename T>
    class Literal : public Expr<T> {
    public:
        using LiteralValue = token::TokenValue;

        Literal(LiteralValue value) : value(value) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitLiteralExpr(this);
        }

        LiteralValue value;

        string toString() {
            if (std::holds_alternative<double>(value)) {
                return std::to_string(std::get<double>(value));
            } else if (std::holds_alternative<string>(value)) {
                return std::get<string>(value);
            } else if (std::holds_alternative<bool>(value)) {
                return std::get<bool>(value) ? "true" : "false";
            } else {
                return "nil";
            }
        }
    };

    template <typename T>
    class Variable : public Expr<T> {
    public:
        Variable(Token name) : name(name) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitVariableExpr(this);
        }

        Token name;
    };

    template <typename T>
    class Grouping : public Expr<T> {
    public:
        Grouping(shared_ptr<Expr<T>> expression) : expression(expression) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitGroupingExpr(this);
        }

        shared_ptr<Expr<T>> expression;
    };

    template <typename T>
    class Logical : public Expr<T> {
    public:
        Logical(shared_ptr<Expr<T>> left, Token operatorToken, shared_ptr<Expr<T>> right)
            : left(left), operatorToken(operatorToken), right(right) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitLogicalExpr(this);
        }

        shared_ptr<Expr<T>> left;
        Token operatorToken;
        shared_ptr<Expr<T>> right;
    };

    template <typename T>
    class Call : public Expr<T> {
    public:
        Call(shared_ptr<Expr<T>> callee, Token paren, std::vector<shared_ptr<Expr<T>>> arguments)
            : callee(callee), paren(paren), arguments(arguments) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitCallExpr(this);
        }

        shared_ptr<Expr<T>> callee;
        Token paren;
        std::vector<shared_ptr<Expr<T>>> arguments;
    };

    template <typename T>
    class Assign : public Expr<T> {
    public:
        Assign(Token name, shared_ptr<Expr<T>> value)
            : name(name), value(value) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitAssignExpr(this);
        }

        Token name;
        shared_ptr<Expr<T>> value;
    };

    template <typename T>
    class Get : public Expr<T> {
    public:
        Get(shared_ptr<Expr<T>> object, Token name)
            : object(object), name(name) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitGetExpr(this);
        }

        shared_ptr<Expr<T>> object;
        Token name;
    };

    template <typename T>
    class Set : public Expr<T> {
    public:
        Set(shared_ptr<Expr<T>> object, Token name, shared_ptr<Expr<T>> value)
            : object(object), name(name), value(value) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitSetExpr(this);
        }

        shared_ptr<Expr<T>> object;
        Token name;
        shared_ptr<Expr<T>> value;
    };

    template <typename T>
    class This : public Expr<T> {
    public:
        This(Token keyword) : keyword(keyword) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitThisExpr(this);
        }

        Token keyword;
    };

    template <typename T>
    class Super : public Expr<T> {
    public:
        Super(Token keyword, Token method) : keyword(keyword), method(method) {}

        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitSuperExpr(this);
        }

        Token keyword;
        Token method;
    };

    template <typename T>
    class ArrayExpr : public Expr<T> {
    public:
        ArrayExpr(Token bracket, std::vector<shared_ptr<Expr<T>>> elements)
            : bracket(bracket), elements(elements) {}
        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitArrayExpr(this);
        }
        Token bracket;
        std::vector<shared_ptr<Expr<T>>> elements;
    };

    template <typename T>
    class MapExpr : public Expr<T> {
    public:
        MapExpr(Token brace, std::vector<Token> keys, std::vector<shared_ptr<Expr<T>>> values)
            : brace(brace), keys(keys), values(values) {}
        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitMapExpr(this);
        }
        Token brace;
        std::vector<Token> keys;
        std::vector<shared_ptr<Expr<T>>> values;
    };

    template <typename T>
    class IndexGet : public Expr<T> {
    public:
        IndexGet(shared_ptr<Expr<T>> object, Token bracket, shared_ptr<Expr<T>> index)
            : object(object), bracket(bracket), index(index) {}
        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitIndexGetExpr(this);
        }
        shared_ptr<Expr<T>> object;
        Token bracket;
        shared_ptr<Expr<T>> index;
    };

    template <typename T>
    class IndexSet : public Expr<T> {
    public:
        IndexSet(shared_ptr<Expr<T>> object, Token bracket, shared_ptr<Expr<T>> index, shared_ptr<Expr<T>> value)
            : object(object), bracket(bracket), index(index), value(value) {}
        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitIndexSetExpr(this);
        }
        shared_ptr<Expr<T>> object;
        Token bracket;
        shared_ptr<Expr<T>> index;
        shared_ptr<Expr<T>> value;
    };

    template <typename T>
    class LambdaExpr : public Expr<T> {
    public:
        LambdaExpr(Token funKeyword, std::vector<Token> params,
                   std::vector<shared_ptr<stmt::Stmt<T>>> body)
            : funKeyword(funKeyword), params(params), body(body) {}
        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitLambdaExpr(this);
        }
        Token funKeyword;
        std::vector<Token> params;
        std::vector<shared_ptr<stmt::Stmt<T>>> body;
    };

    template <typename T>
    class PipeExpr : public Expr<T> {
    public:
        PipeExpr(shared_ptr<Expr<T>> value, Token op, shared_ptr<Expr<T>> func)
            : value(value), op(op), func(func) {}
        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitPipeExpr(this);
        }
        shared_ptr<Expr<T>> value;
        Token op;
        shared_ptr<Expr<T>> func;
    };

    template <typename T>
    class ComposeExpr : public Expr<T> {
    public:
        ComposeExpr(shared_ptr<Expr<T>> left, Token op, shared_ptr<Expr<T>> right)
            : left(left), op(op), right(right) {}
        T visit(shared_ptr<Visitor<T>> visitor) override {
            return visitor->visitComposeExpr(this);
        }
        shared_ptr<Expr<T>> left;
        Token op;
        shared_ptr<Expr<T>> right;
    };

    template <typename T>
    class Visitor {
    public:
        virtual T visitBinaryExpr(Binary<T>* expr) = 0;
        virtual T visitUnaryExpr(Unary<T>* expr) = 0;
        virtual T visitLiteralExpr(Literal<T>* expr) = 0;
        virtual T visitVariableExpr(Variable<T>* expr) = 0;
        virtual T visitGroupingExpr(Grouping<T>* expr) = 0;
        virtual T visitLogicalExpr(Logical<T>* expr) = 0;
        virtual T visitCallExpr(Call<T>* expr) = 0;
        virtual T visitAssignExpr(Assign<T>* expr) = 0;
        virtual T visitGetExpr(Get<T>* expr) = 0;
        virtual T visitSetExpr(Set<T>* expr) = 0;
        virtual T visitThisExpr(This<T>* expr) = 0;
        virtual T visitSuperExpr(Super<T>* expr) = 0;
        virtual T visitArrayExpr(ArrayExpr<T>* expr) = 0;
        virtual T visitMapExpr(MapExpr<T>* expr) = 0;
        virtual T visitIndexGetExpr(IndexGet<T>* expr) = 0;
        virtual T visitIndexSetExpr(IndexSet<T>* expr) = 0;
        virtual T visitLambdaExpr(LambdaExpr<T>* expr) = 0;
        virtual T visitPipeExpr(PipeExpr<T>* expr) = 0;
        virtual T visitComposeExpr(ComposeExpr<T>* expr) = 0;
        virtual ~Visitor() = default;
    };
}

#endif // EXPR_H