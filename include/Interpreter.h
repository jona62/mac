#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <memory>
#include <string>
#include <variant>
#include <vector>
#include <sstream>
#include "Expr.h"
#include "Stmt.h"
#include "MacValue.h"
#include "MacCallable.h"
#include "MacFunction.h"
#include "MacClass.h"
#include "NativeFunctions.h"
#include "Environment.h"
#include "RuntimeError.h"
#include "Return.h"

using std::shared_ptr;
using std::make_shared;
using std::string;
using std::monostate;

namespace interpreter {

    using MacValue = value::MacValue;

    class Interpreter : public expr::Visitor<MacValue>,
                        public stmt::StmtVisitor<MacValue>,
                        public std::enable_shared_from_this<Interpreter> {
    public:
        Interpreter() : globals(make_shared<environment::Environment>()), env(globals) {
            // Define native functions
            globals->define("clock", MacValue(std::static_pointer_cast<callable::MacCallable>(
                make_shared<callable::ClockFunction>())));
        }

        // --- Expression visitors ---

        MacValue visitLiteralExpr(expr::Literal<MacValue>* expr) override {
            return std::visit([](auto&& val) -> MacValue { return val; }, expr->value);
        }

        MacValue visitGroupingExpr(expr::Grouping<MacValue>* expr) override {
            return evaluate(expr->expression);
        }

        MacValue visitUnaryExpr(expr::Unary<MacValue>* expr) override {
            MacValue right = evaluate(expr->right);

            switch (expr->operatorToken.type) {
                case token::TokenType::MINUS:
                    checkNumberOperand(expr->operatorToken, right);
                    return -std::get<double>(right);
                case token::TokenType::BANG:
                    return !isTruthy(right);
                default:
                    break;
            }
            return monostate{};
        }

        MacValue visitBinaryExpr(expr::Binary<MacValue>* expr) override {
            MacValue left = evaluate(expr->left);
            MacValue right = evaluate(expr->right);

            switch (expr->operatorToken.type) {
                case token::TokenType::MINUS:
                    checkNumberOperands(expr->operatorToken, left, right);
                    return std::get<double>(left) - std::get<double>(right);
                case token::TokenType::SLASH:
                    checkNumberOperands(expr->operatorToken, left, right);
                    if (std::get<double>(right) == 0) {
                        throw errors::RuntimeError(expr->operatorToken, "Division by zero.");
                    }
                    return std::get<double>(left) / std::get<double>(right);
                case token::TokenType::STAR:
                    checkNumberOperands(expr->operatorToken, left, right);
                    return std::get<double>(left) * std::get<double>(right);
                case token::TokenType::PLUS:
                    if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) {
                        return std::get<double>(left) + std::get<double>(right);
                    }
                    if (std::holds_alternative<string>(left) && std::holds_alternative<string>(right)) {
                        return std::get<string>(left) + std::get<string>(right);
                    }
                    throw errors::RuntimeError(expr->operatorToken, "Operands must be two numbers or two strings.");
                case token::TokenType::GREATER:
                    checkNumberOperands(expr->operatorToken, left, right);
                    return std::get<double>(left) > std::get<double>(right);
                case token::TokenType::GREATER_EQUAL:
                    checkNumberOperands(expr->operatorToken, left, right);
                    return std::get<double>(left) >= std::get<double>(right);
                case token::TokenType::LESS:
                    checkNumberOperands(expr->operatorToken, left, right);
                    return std::get<double>(left) < std::get<double>(right);
                case token::TokenType::LESS_EQUAL:
                    checkNumberOperands(expr->operatorToken, left, right);
                    return std::get<double>(left) <= std::get<double>(right);
                case token::TokenType::BANG_EQUAL:
                    return !isEqual(left, right);
                case token::TokenType::EQUAL_EQUAL:
                    return isEqual(left, right);
                default:
                    break;
            }
            return monostate{};
        }

        MacValue visitVariableExpr(expr::Variable<MacValue>* expr) override {
            return lookUpVariable(expr->name, expr);
        }

        MacValue visitLogicalExpr(expr::Logical<MacValue>* expr) override {
            MacValue left = evaluate(expr->left);

            if (expr->operatorToken.type == token::TokenType::OR) {
                if (isTruthy(left)) return left;
            } else {
                if (!isTruthy(left)) return left;
            }

            return evaluate(expr->right);
        }

        MacValue visitCallExpr(expr::Call<MacValue>* expr) override {
            MacValue callee = evaluate(expr->callee);

            std::vector<MacValue> arguments;
            for (auto& argument : expr->arguments) {
                arguments.push_back(evaluate(argument));
            }

            if (!std::holds_alternative<shared_ptr<callable::MacCallable>>(callee)) {
                throw errors::RuntimeError(expr->paren, "Can only call functions and classes.");
            }

            auto function = std::get<shared_ptr<callable::MacCallable>>(callee);
            if (static_cast<int>(arguments.size()) != function->arity()) {
                throw errors::RuntimeError(expr->paren,
                    "Expected " + std::to_string(function->arity()) +
                    " arguments but got " + std::to_string(arguments.size()) + ".");
            }

            return function->call(shared_from_this(), arguments);
        }

        MacValue visitAssignExpr(expr::Assign<MacValue>* expr) override {
            MacValue value = evaluate(expr->value);

            auto it = locals.find(static_cast<expr::Expr<MacValue>*>(expr));
            if (it != locals.end()) {
                env->assignAt(it->second, std::get<string>(expr->name.lexeme), value);
            } else {
                globals->assign(expr->name, value);
            }

            return value;
        }

        MacValue visitGetExpr(expr::Get<MacValue>* expr) override {
            MacValue object = evaluate(expr->object);
            if (std::holds_alternative<shared_ptr<instance::MacInstance>>(object)) {
                return std::get<shared_ptr<instance::MacInstance>>(object)->get(expr->name);
            }
            throw errors::RuntimeError(expr->name, "Only instances have properties.");
        }

        MacValue visitSetExpr(expr::Set<MacValue>* expr) override {
            MacValue object = evaluate(expr->object);
            if (!std::holds_alternative<shared_ptr<instance::MacInstance>>(object)) {
                throw errors::RuntimeError(expr->name, "Only instances have fields.");
            }
            MacValue value = evaluate(expr->value);
            std::get<shared_ptr<instance::MacInstance>>(object)->set(expr->name, value);
            return value;
        }

        MacValue visitThisExpr(expr::This<MacValue>* expr) override {
            return lookUpVariable(expr->keyword, expr);
        }

        MacValue visitSuperExpr(expr::Super<MacValue>* expr) override {
            int distance = locals[static_cast<expr::Expr<MacValue>*>(expr)];
            auto superclass = std::get<shared_ptr<callable::MacCallable>>(
                env->getAt(distance, "super"));
            auto superclassPtr = std::dynamic_pointer_cast<callable::MacClass>(superclass);
            auto object = std::get<shared_ptr<instance::MacInstance>>(
                env->getAt(distance - 1, "this"));

            auto method = superclassPtr->findMethod(std::get<string>(expr->method.lexeme));
            if (method == nullptr) {
                throw errors::RuntimeError(expr->method,
                    "Undefined property '" + std::get<string>(expr->method.lexeme) + "'.");
            }
            return MacValue(std::static_pointer_cast<callable::MacCallable>(method->bind(object)));
        }

        // --- Statement visitors ---

        void visitExpressionStmt(stmt::ExpressionStmt<MacValue>* stm) override {
            evaluate(stm->expression);
        }

        void visitPrintStmt(stmt::PrintStmt<MacValue>* stm) override {
            MacValue value = evaluate(stm->expression);
            std::cout << stringify(value) << std::endl;
        }

        void visitVarStmt(stmt::VarStmt<MacValue>* stm) override {
            MacValue value = monostate{};
            if (stm->initializer != nullptr) {
                value = evaluate(stm->initializer);
            }
            env->define(std::get<string>(stm->name.lexeme), value);
        }

        void visitBlockStmt(stmt::BlockStmt<MacValue>* stm) override {
            auto blockEnv = make_shared<environment::Environment>(env);
            executeBlock(stm->statements, blockEnv);
        }

        void visitIfStmt(stmt::IfStmt<MacValue>* stm) override {
            if (isTruthy(evaluate(stm->condition))) {
                execute(stm->thenBranch);
            } else if (stm->elseBranch != nullptr) {
                execute(stm->elseBranch);
            }
        }

        void visitWhileStmt(stmt::WhileStmt<MacValue>* stm) override {
            while (isTruthy(evaluate(stm->condition))) {
                execute(stm->body);
            }
        }

        void visitFunctionStmt(stmt::FunctionStmt<MacValue>* stm) override {
            auto function = make_shared<callable::MacFunction>(stm, env);
            env->define(std::get<string>(stm->name.lexeme),
                        MacValue(std::static_pointer_cast<callable::MacCallable>(function)));
        }

        void visitReturnStmt(stmt::ReturnStmt<MacValue>* stm) override {
            MacValue value = monostate{};
            if (stm->value != nullptr) {
                value = evaluate(stm->value);
            }
            throw errors::Return(value);
        }

        void visitClassStmt(stmt::ClassStmt<MacValue>* stm) override {
            shared_ptr<callable::MacClass> superclassPtr = nullptr;

            if (stm->superclass != nullptr) {
                MacValue superVal = evaluate(stm->superclass);
                if (!std::holds_alternative<shared_ptr<callable::MacCallable>>(superVal)) {
                    throw errors::RuntimeError(stm->superclass->name, "Superclass must be a class.");
                }
                superclassPtr = std::dynamic_pointer_cast<callable::MacClass>(
                    std::get<shared_ptr<callable::MacCallable>>(superVal));
                if (!superclassPtr) {
                    throw errors::RuntimeError(stm->superclass->name, "Superclass must be a class.");
                }
            }

            env->define(std::get<string>(stm->name.lexeme), monostate{});

            if (stm->superclass != nullptr) {
                env = make_shared<environment::Environment>(env);
                env->define("super", MacValue(std::static_pointer_cast<callable::MacCallable>(superclassPtr)));
            }

            std::unordered_map<string, shared_ptr<callable::MacFunction>> methods;
            for (auto& method : stm->methods) {
                auto function = make_shared<callable::MacFunction>(method.get(), env);
                methods[std::get<string>(method->name.lexeme)] = function;
            }

            auto klass = make_shared<callable::MacClass>(
                std::get<string>(stm->name.lexeme), superclassPtr, methods);

            if (stm->superclass != nullptr) {
                env = env->enclosing;
            }

            env->assign(stm->name, MacValue(std::static_pointer_cast<callable::MacCallable>(klass)));
        }

        // --- Public API ---

        void resolve(expr::Expr<MacValue>* expr, int depth) {
            locals[expr] = depth;
        }

        void interpret(const std::vector<shared_ptr<stmt::Stmt<MacValue>>>& statements) {
            try {
                for (auto& statement : statements) {
                    execute(statement);
                }
            } catch (const errors::RuntimeError& error) {
                std::cerr << error.what() << std::endl;
            }
        }

        void executeBlock(const std::vector<shared_ptr<stmt::Stmt<MacValue>>>& statements,
                          shared_ptr<environment::Environment> blockEnv) {
            auto previousEnv = env;
            try {
                env = blockEnv;
                for (auto& statement : statements) {
                    execute(statement);
                }
                env = previousEnv;
            } catch (...) {
                env = previousEnv;
                throw;
            }
        }

        string stringify(const MacValue& value) {
            if (std::holds_alternative<monostate>(value)) return "nil";
            if (std::holds_alternative<bool>(value)) {
                return std::get<bool>(value) ? "true" : "false";
            }
            if (std::holds_alternative<double>(value)) {
                std::ostringstream ss;
                ss << std::get<double>(value);
                return ss.str();
            }
            if (std::holds_alternative<shared_ptr<callable::MacCallable>>(value)) {
                return std::get<shared_ptr<callable::MacCallable>>(value)->toString();
            }
            if (std::holds_alternative<shared_ptr<instance::MacInstance>>(value)) {
                return std::get<shared_ptr<instance::MacInstance>>(value)->toString();
            }
            return std::get<string>(value);
        }

    private:
        shared_ptr<environment::Environment> globals;
        shared_ptr<environment::Environment> env;
        std::unordered_map<expr::Expr<MacValue>*, int> locals;

        MacValue evaluate(shared_ptr<expr::Expr<MacValue>> expr) {
            return expr->visit(shared_from_this());
        }

        void execute(shared_ptr<stmt::Stmt<MacValue>> stmt) {
            stmt->accept(shared_from_this());
        }

        bool isTruthy(const MacValue& value) {
            if (std::holds_alternative<monostate>(value)) return false;
            if (std::holds_alternative<bool>(value)) return std::get<bool>(value);
            return true;
        }

        bool isEqual(const MacValue& a, const MacValue& b) {
            return a == b;
        }

        void checkNumberOperand(const token::Token& op, const MacValue& operand) {
            if (std::holds_alternative<double>(operand)) return;
            throw errors::RuntimeError(op, "Operand must be a number.");
        }

        void checkNumberOperands(const token::Token& op, const MacValue& left, const MacValue& right) {
            if (std::holds_alternative<double>(left) && std::holds_alternative<double>(right)) return;
            throw errors::RuntimeError(op, "Operands must be numbers.");
        }

        MacValue lookUpVariable(const token::Token& name, expr::Expr<MacValue>* expr) {
            auto it = locals.find(expr);
            if (it != locals.end()) {
                return env->getAt(it->second, std::get<string>(name.lexeme));
            } else {
                return globals->get(name);
            }
        }
    };

} // namespace interpreter

// MacFunction::call implementation — needs full Interpreter definition
inline value::MacValue callable::MacFunction::call(
    std::shared_ptr<interpreter::Interpreter> interpreter,
    std::vector<value::MacValue> arguments)
{
    auto funcEnv = std::make_shared<environment::Environment>(closure);
    for (size_t i = 0; i < declaration->params.size(); i++) {
        funcEnv->define(std::get<std::string>(declaration->params[i].lexeme), arguments[i]);
    }

    try {
        interpreter->executeBlock(declaration->body, funcEnv);
    } catch (const errors::Return& returnValue) {
        return returnValue.returnValue;
    }

    return std::monostate{};
}

#endif // INTERPRETER_H
