#ifndef STMT_H
#define STMT_H

#include <memory>
#include <vector>
#include <string>
#include "Expr.h"

using std::shared_ptr;
using std::vector;

namespace stmt {

    template <typename T>
    class StmtVisitor;

    template <typename T>
    class Stmt {
    public:
        virtual void accept(shared_ptr<StmtVisitor<T>> visitor) = 0;
        virtual ~Stmt() = default;
    };

    template <typename T>
    class ExpressionStmt : public Stmt<T> {
    public:
        ExpressionStmt(shared_ptr<expr::Expr<T>> expression)
            : expression(expression) {}

        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitExpressionStmt(this);
        }

        shared_ptr<expr::Expr<T>> expression;
    };

    template <typename T>
    class PrintStmt : public Stmt<T> {
    public:
        PrintStmt(shared_ptr<expr::Expr<T>> expression)
            : expression(expression) {}

        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitPrintStmt(this);
        }

        shared_ptr<expr::Expr<T>> expression;
    };

    template <typename T>
    class VarStmt : public Stmt<T> {
    public:
        VarStmt(token::Token name, shared_ptr<expr::Expr<T>> initializer)
            : name(name), initializer(initializer) {}

        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitVarStmt(this);
        }

        token::Token name;
        shared_ptr<expr::Expr<T>> initializer;
    };

    template <typename T>
    class BlockStmt : public Stmt<T> {
    public:
        BlockStmt(vector<shared_ptr<Stmt<T>>> statements)
            : statements(statements) {}

        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitBlockStmt(this);
        }

        vector<shared_ptr<Stmt<T>>> statements;
    };

    template <typename T>
    class IfStmt : public Stmt<T> {
    public:
        IfStmt(shared_ptr<expr::Expr<T>> condition,
               shared_ptr<Stmt<T>> thenBranch,
               shared_ptr<Stmt<T>> elseBranch)
            : condition(condition), thenBranch(thenBranch), elseBranch(elseBranch) {}

        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitIfStmt(this);
        }

        shared_ptr<expr::Expr<T>> condition;
        shared_ptr<Stmt<T>> thenBranch;
        shared_ptr<Stmt<T>> elseBranch;
    };

    template <typename T>
    class WhileStmt : public Stmt<T> {
    public:
        WhileStmt(shared_ptr<expr::Expr<T>> condition, shared_ptr<Stmt<T>> body)
            : condition(condition), body(body) {}

        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitWhileStmt(this);
        }

        shared_ptr<expr::Expr<T>> condition;
        shared_ptr<Stmt<T>> body;
    };

    template <typename T>
    class FunctionStmt : public Stmt<T> {
    public:
        FunctionStmt(token::Token name,
                     vector<token::Token> params,
                     vector<shared_ptr<Stmt<T>>> body)
            : name(name), params(params), body(body) {}

        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitFunctionStmt(this);
        }

        token::Token name;
        vector<token::Token> params;
        vector<shared_ptr<Stmt<T>>> body;
    };

    template <typename T>
    class ReturnStmt : public Stmt<T> {
    public:
        ReturnStmt(token::Token keyword, shared_ptr<expr::Expr<T>> value)
            : keyword(keyword), value(value) {}

        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitReturnStmt(this);
        }

        token::Token keyword;
        shared_ptr<expr::Expr<T>> value;
    };

    template <typename T>
    class ClassStmt : public Stmt<T> {
    public:
        ClassStmt(token::Token name,
                  shared_ptr<expr::Variable<T>> superclass,
                  vector<shared_ptr<FunctionStmt<T>>> methods)
            : name(name), superclass(superclass), methods(methods) {}

        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitClassStmt(this);
        }

        token::Token name;
        shared_ptr<expr::Variable<T>> superclass;
        vector<shared_ptr<FunctionStmt<T>>> methods;
    };

    template <typename T>
    class ForInStmt : public Stmt<T> {
    public:
        ForInStmt(token::Token varName, shared_ptr<expr::Expr<T>> iterable, shared_ptr<Stmt<T>> body)
            : varName(varName), iterable(iterable), body(body) {}
        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitForInStmt(this);
        }
        token::Token varName;
        shared_ptr<expr::Expr<T>> iterable;
        shared_ptr<Stmt<T>> body;
    };

    template <typename T>
    class BreakStmt : public Stmt<T> {
    public:
        BreakStmt(token::Token keyword) : keyword(keyword) {}
        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitBreakStmt(this);
        }
        token::Token keyword;
    };

    template <typename T>
    class ContinueStmt : public Stmt<T> {
    public:
        ContinueStmt(token::Token keyword) : keyword(keyword) {}
        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitContinueStmt(this);
        }
        token::Token keyword;
    };

    // style name { key: value, ... }
    template <typename T>
    class StyleStmt : public Stmt<T> {
    public:
        StyleStmt(token::Token name,
                  std::vector<std::pair<token::Token, std::shared_ptr<expr::Expr<T>>>> properties)
            : name(name), properties(std::move(properties)) {}
        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitStyleStmt(this);
        }
        token::Token name;
        std::vector<std::pair<token::Token, std::shared_ptr<expr::Expr<T>>>> properties;
    };

    // effect name = compose_expr;
    template <typename T>
    class EffectStmt : public Stmt<T> {
    public:
        EffectStmt(token::Token name, std::shared_ptr<expr::Expr<T>> value)
            : name(name), value(value) {}
        void accept(shared_ptr<StmtVisitor<T>> visitor) override {
            visitor->visitEffectStmt(this);
        }
        token::Token name;
        std::shared_ptr<expr::Expr<T>> value;
    };

    template <typename T>
    class StmtVisitor {
    public:
        virtual void visitExpressionStmt(ExpressionStmt<T>* stmt) = 0;
        virtual void visitPrintStmt(PrintStmt<T>* stmt) = 0;
        virtual void visitVarStmt(VarStmt<T>* stmt) = 0;
        virtual void visitBlockStmt(BlockStmt<T>* stmt) = 0;
        virtual void visitIfStmt(IfStmt<T>* stmt) = 0;
        virtual void visitWhileStmt(WhileStmt<T>* stmt) = 0;
        virtual void visitFunctionStmt(FunctionStmt<T>* stmt) = 0;
        virtual void visitReturnStmt(ReturnStmt<T>* stmt) = 0;
        virtual void visitClassStmt(ClassStmt<T>* stmt) = 0;
        virtual void visitForInStmt(ForInStmt<T>* stmt) = 0;
        virtual void visitBreakStmt(BreakStmt<T>* stmt) = 0;
        virtual void visitContinueStmt(ContinueStmt<T>* stmt) = 0;
        virtual void visitEffectStmt(EffectStmt<T>* stmt) = 0;
        virtual void visitStyleStmt(StyleStmt<T>* stmt) = 0;
        virtual ~StmtVisitor() = default;
    };

} // namespace stmt

#endif // STMT_H
