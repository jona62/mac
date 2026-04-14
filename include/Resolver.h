#ifndef RESOLVER_H
#define RESOLVER_H

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "Expr.h"
#include "Stmt.h"
#include "MacValue.h"

// Forward declare — full definition included after class for resolveLocal
namespace interpreter {
    class Interpreter;
}

namespace resolver {

    using MV = value::MacValue;

    class Resolver : public expr::Visitor<MV>,
                     public stmt::StmtVisitor<MV>,
                     public std::enable_shared_from_this<Resolver> {
    public:
        Resolver(std::shared_ptr<interpreter::Interpreter> interpreter)
            : interpreter(interpreter) {}

        void resolve(const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& statements) {
            for (auto& statement : statements) {
                resolveStmt(statement);
            }
        }

        // --- Expression visitors ---

        MV visitLiteralExpr(expr::Literal<MV>*) override {
            return std::monostate{};
        }

        MV visitGroupingExpr(expr::Grouping<MV>* expr) override {
            resolveExpr(expr->expression);
            return std::monostate{};
        }

        MV visitUnaryExpr(expr::Unary<MV>* expr) override {
            resolveExpr(expr->right);
            return std::monostate{};
        }

        MV visitBinaryExpr(expr::Binary<MV>* expr) override {
            resolveExpr(expr->left);
            resolveExpr(expr->right);
            return std::monostate{};
        }

        MV visitVariableExpr(expr::Variable<MV>* expr) override {
            if (!scopes.empty()) {
                auto& scope = scopes.back();
                auto nameStr = std::get<std::string>(expr->name.lexeme);
                auto it = scope.find(nameStr);
                if (it != scope.end() && it->second == false) {
                    std::cerr << "[line " << expr->name.line
                              << "] Error: Can't read local variable in its own initializer." << std::endl;
                }
            }
            resolveLocal(expr, expr->name);
            return std::monostate{};
        }

        MV visitLogicalExpr(expr::Logical<MV>* expr) override {
            resolveExpr(expr->left);
            resolveExpr(expr->right);
            return std::monostate{};
        }

        MV visitCallExpr(expr::Call<MV>* expr) override {
            resolveExpr(expr->callee);
            for (auto& argument : expr->arguments) {
                resolveExpr(argument);
            }
            return std::monostate{};
        }

        MV visitGetExpr(expr::Get<MV>* expr) override {
            resolveExpr(expr->object);
            return std::monostate{};
        }

        MV visitSetExpr(expr::Set<MV>* expr) override {
            resolveExpr(expr->value);
            resolveExpr(expr->object);
            return std::monostate{};
        }

        MV visitThisExpr(expr::This<MV>* expr) override {
            resolveLocal(expr, expr->keyword);
            return std::monostate{};
        }

        MV visitSuperExpr(expr::Super<MV>* expr) override {
            resolveLocal(expr, expr->keyword);
            return std::monostate{};
        }

        MV visitArrayExpr(expr::ArrayExpr<MV>* expr) override {
            for (auto& elem : expr->elements) resolveExpr(elem);
            return std::monostate{};
        }

        MV visitMapExpr(expr::MapExpr<MV>* expr) override {
            for (auto& val : expr->values) resolveExpr(val);
            return std::monostate{};
        }

        MV visitIndexGetExpr(expr::IndexGet<MV>* expr) override {
            resolveExpr(expr->object);
            resolveExpr(expr->index);
            return std::monostate{};
        }

        MV visitIndexSetExpr(expr::IndexSet<MV>* expr) override {
            resolveExpr(expr->value);
            resolveExpr(expr->object);
            resolveExpr(expr->index);
            return std::monostate{};
        }

        MV visitLambdaExpr(expr::LambdaExpr<MV>* expr) override {
            beginScope();
            for (auto& param : expr->params) {
                declare(param);
                define(param);
            }
            resolve(expr->body);
            endScope();
            return std::monostate{};
        }

        MV visitPipeExpr(expr::PipeExpr<MV>* expr) override {
            resolveExpr(expr->value);
            resolveExpr(expr->func);
            return std::monostate{};
        }

        MV visitComposeExpr(expr::ComposeExpr<MV>* expr) override {
            resolveExpr(expr->left);
            resolveExpr(expr->right);
            return std::monostate{};
        }

        MV visitMemeLiteralExpr(expr::MemeLiteralExpr<MV>* expr) override {
            for (auto& e : expr->entries) resolveExpr(e.value);
            return std::monostate{};
        }
        MV visitSaveExpr(expr::SaveExpr<MV>* expr) override {
            resolveExpr(expr->value); resolveExpr(expr->path);
            return std::monostate{};
        }
        MV visitGifBlockExpr(expr::GifBlockExpr<MV>* expr) override {
            for (auto& f : expr->frames) resolveExpr(f.meme);
            return std::monostate{};
        }
        MV visitTimelineBlockExpr(expr::TimelineBlockExpr<MV>* expr) override {
            for (auto& e : expr->entries) resolveExpr(e.frame.meme);
            return std::monostate{};
        }
        MV visitGridBlockExpr(expr::GridBlockExpr<MV>* expr) override {
            for (auto& e : expr->entries) resolveExpr(e);
            return std::monostate{};
        }

        MV visitAssignExpr(expr::Assign<MV>* expr) override {
            resolveExpr(expr->value);
            resolveLocal(expr, expr->name);
            return std::monostate{};
        }

        // --- Statement visitors ---

        void visitExpressionStmt(stmt::ExpressionStmt<MV>* stm) override {
            resolveExpr(stm->expression);
        }

        void visitPrintStmt(stmt::PrintStmt<MV>* stm) override {
            resolveExpr(stm->expression);
        }

        void visitVarStmt(stmt::VarStmt<MV>* stm) override {
            declare(stm->name);
            if (stm->initializer != nullptr) {
                resolveExpr(stm->initializer);
            }
            define(stm->name);
        }

        void visitBlockStmt(stmt::BlockStmt<MV>* stm) override {
            beginScope();
            resolve(stm->statements);
            endScope();
        }

        void visitIfStmt(stmt::IfStmt<MV>* stm) override {
            resolveExpr(stm->condition);
            resolveStmt(stm->thenBranch);
            if (stm->elseBranch != nullptr) resolveStmt(stm->elseBranch);
        }

        void visitWhileStmt(stmt::WhileStmt<MV>* stm) override {
            resolveExpr(stm->condition);
            resolveStmt(stm->body);
        }

        void visitFunctionStmt(stmt::FunctionStmt<MV>* stm) override {
            declare(stm->name);
            define(stm->name);
            resolveFunction(stm);
        }

        void visitReturnStmt(stmt::ReturnStmt<MV>* stm) override {
            if (stm->value != nullptr) {
                resolveExpr(stm->value);
            }
        }

        void visitForInStmt(stmt::ForInStmt<MV>* stm) override {
            resolveExpr(stm->iterable);
            beginScope();
            declare(stm->varName);
            define(stm->varName);
            resolveStmt(stm->body);
            endScope();
        }

        void visitBreakStmt(stmt::BreakStmt<MV>*) override {}
        void visitContinueStmt(stmt::ContinueStmt<MV>*) override {}
        void visitEffectStmt(stmt::EffectStmt<MV>* stm) override {
            declare(stm->name); resolveExpr(stm->value); define(stm->name);
        }
        void visitStyleStmt(stmt::StyleStmt<MV>* stm) override {
            declare(stm->name);
            for (auto& [key, val] : stm->properties) resolveExpr(val);
            define(stm->name);
        }

        void visitClassStmt(stmt::ClassStmt<MV>* stm) override {
            declare(stm->name);
            define(stm->name);

            if (stm->superclass != nullptr) {
                resolveExpr(stm->superclass);
                beginScope();
                scopes.back()["super"] = true;
            }

            beginScope();
            scopes.back()["this"] = true;

            for (auto& method : stm->methods) {
                resolveFunction(method.get());
            }

            endScope();

            if (stm->superclass != nullptr) {
                endScope();
            }
        }

    private:
        std::shared_ptr<interpreter::Interpreter> interpreter;
        std::vector<std::unordered_map<std::string, bool>> scopes;

        void beginScope() {
            scopes.push_back(std::unordered_map<std::string, bool>());
        }

        void endScope() {
            scopes.pop_back();
        }

        void declare(const token::Token& name) {
            if (scopes.empty()) return;
            auto& scope = scopes.back();
            auto nameStr = std::get<std::string>(name.lexeme);
            if (scope.count(nameStr)) {
                std::cerr << "[line " << name.line
                          << "] Error: Already a variable with this name in this scope." << std::endl;
            }
            scope[nameStr] = false;
        }

        void define(const token::Token& name) {
            if (scopes.empty()) return;
            scopes.back()[std::get<std::string>(name.lexeme)] = true;
        }

        void resolveLocal(expr::Expr<MV>* expr, const token::Token& name);

        void resolveStmt(std::shared_ptr<stmt::Stmt<MV>> stmt) {
            stmt->accept(shared_from_this());
        }

        void resolveExpr(std::shared_ptr<expr::Expr<MV>> expr) {
            expr->visit(shared_from_this());
        }

        void resolveFunction(stmt::FunctionStmt<MV>* function) {
            beginScope();
            for (auto& param : function->params) {
                declare(param);
                define(param);
            }
            resolve(function->body);
            endScope();
        }
    };

} // namespace resolver

// Include full Interpreter definition for resolveLocal
#include "Interpreter.h"

inline void resolver::Resolver::resolveLocal(expr::Expr<MV>* expr, const token::Token& name) {
    auto nameStr = std::get<std::string>(name.lexeme);
    for (int i = static_cast<int>(scopes.size()) - 1; i >= 0; i--) {
        if (scopes[i].count(nameStr)) {
            interpreter->resolve(expr, static_cast<int>(scopes.size()) - 1 - i);
            return;
        }
    }
}

#endif // RESOLVER_H
