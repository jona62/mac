#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <cmath>
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
#include "MacLambda.h"
#include "MacClass.h"
#include "MacArray.h"
#include "MacMap.h"
#include "MacMeme.h"
#include "MacGif.h"
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
            auto defn = [&](const string& name, shared_ptr<callable::MacCallable> fn) {
                globals->define(name, MacValue(fn));
            };
            defn("clock", make_shared<callable::ClockFunction>());
            defn("len", make_shared<callable::LenFunction>());
            defn("substr", make_shared<callable::SubstrFunction>());
            defn("split", make_shared<callable::SplitFunction>());
            defn("type", make_shared<callable::TypeFunction>());
            defn("sqrt", make_shared<callable::SqrtFunction>());
            defn("abs", make_shared<callable::AbsFunction>());
            defn("pow", make_shared<callable::PowFunction>());
            defn("floor", make_shared<callable::FloorFunction>());
            defn("ceil", make_shared<callable::CeilFunction>());
            defn("push", make_shared<callable::PushFunction>());
            defn("pop", make_shared<callable::PopFunction>());
            defn("map", make_shared<callable::MapArrayFunction>());
            defn("filter", make_shared<callable::FilterFunction>());
            defn("input", make_shared<callable::InputFunction>());
            defn("_resolve_template", make_shared<callable::ResolveTemplateFunction>());
            defn("_meme_save", make_shared<callable::MemeRenderSaveFunction>());
            defn("_gif_save", make_shared<callable::GifRenderSaveFunction>());
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

            // Operator overloading: check for dunder methods on class instances
            if (std::holds_alternative<shared_ptr<instance::MacInstance>>(right)) {
                auto inst = std::get<shared_ptr<instance::MacInstance>>(right);
                std::string dunder;
                switch (expr->operatorToken.type) {
                    case token::TokenType::MINUS: dunder = "__neg__"; break;
                    case token::TokenType::BANG:  dunder = "__not__"; break;
                    default: break;
                }
                if (!dunder.empty()) {
                    auto method = inst->getClass()->findMethod(dunder);
                    if (method) {
                        auto bound = method->bind(inst);
                        return bound->call(shared_from_this(), {});
                    }
                }
            }

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

            // Operator overloading: check for dunder methods on class instances
            if (std::holds_alternative<shared_ptr<instance::MacInstance>>(left)) {
                auto inst = std::get<shared_ptr<instance::MacInstance>>(left);
                std::string dunder;
                switch (expr->operatorToken.type) {
                    case token::TokenType::PLUS:          dunder = "__add__"; break;
                    case token::TokenType::MINUS:         dunder = "__sub__"; break;
                    case token::TokenType::STAR:          dunder = "__mul__"; break;
                    case token::TokenType::SLASH:         dunder = "__div__"; break;
                    case token::TokenType::PERCENT:       dunder = "__mod__"; break;
                    case token::TokenType::EQUAL_EQUAL:   dunder = "__eq__"; break;
                    case token::TokenType::BANG_EQUAL:     dunder = "__ne__"; break;
                    case token::TokenType::LESS:          dunder = "__lt__"; break;
                    case token::TokenType::GREATER:       dunder = "__gt__"; break;
                    case token::TokenType::LESS_EQUAL:    dunder = "__le__"; break;
                    case token::TokenType::GREATER_EQUAL: dunder = "__ge__"; break;
                    default: break;
                }
                if (!dunder.empty()) {
                    auto method = inst->getClass()->findMethod(dunder);
                    if (method) {
                        auto bound = method->bind(inst);
                        return bound->call(shared_from_this(), {right});
                    }
                }
            }

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
                case token::TokenType::PERCENT:
                    checkNumberOperands(expr->operatorToken, left, right);
                    return std::fmod(std::get<double>(left), std::get<double>(right));
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
            if (std::holds_alternative<shared_ptr<collection::MacMap>>(object)) {
                auto map = std::get<shared_ptr<collection::MacMap>>(object);
                auto key = std::get<string>(expr->name.lexeme);
                if (!map->has(key)) {
                    throw errors::RuntimeError(expr->name, "Undefined map key '" + key + "'.");
                }
                return map->get(key);
            }
            throw errors::RuntimeError(expr->name, "Only instances and maps have properties.");
        }

        MacValue visitSetExpr(expr::Set<MacValue>* expr) override {
            MacValue object = evaluate(expr->object);
            if (std::holds_alternative<shared_ptr<instance::MacInstance>>(object)) {
                MacValue value = evaluate(expr->value);
                std::get<shared_ptr<instance::MacInstance>>(object)->set(expr->name, value);
                return value;
            }
            if (std::holds_alternative<shared_ptr<collection::MacMap>>(object)) {
                MacValue value = evaluate(expr->value);
                auto map = std::get<shared_ptr<collection::MacMap>>(object);
                map->set(std::get<string>(expr->name.lexeme), value);
                return value;
            }
            throw errors::RuntimeError(expr->name, "Only instances and maps have fields.");
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
                try {
                    execute(stm->body);
                } catch (const errors::BreakException&) {
                    break;
                } catch (const errors::ContinueException&) {
                    // continue to next iteration
                }
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

        void visitForInStmt(stmt::ForInStmt<MacValue>* stm) override {
            MacValue iterable = evaluate(stm->iterable);

            if (std::holds_alternative<shared_ptr<collection::MacArray>>(iterable)) {
                auto arr = std::get<shared_ptr<collection::MacArray>>(iterable);
                for (auto& elem : arr->elements) {
                    auto blockEnv = make_shared<environment::Environment>(env);
                    blockEnv->define(std::get<string>(stm->varName.lexeme), elem);
                    auto prevEnv = env;
                    env = blockEnv;
                    try {
                        execute(stm->body);
                    } catch (const errors::BreakException&) {
                        env = prevEnv;
                        break;
                    } catch (const errors::ContinueException&) {
                        // continue
                    } catch (...) {
                        env = prevEnv;
                        throw;
                    }
                    env = prevEnv;
                }
            } else if (std::holds_alternative<shared_ptr<collection::MacMap>>(iterable)) {
                auto map = std::get<shared_ptr<collection::MacMap>>(iterable);
                for (auto& [key, val] : map->entries) {
                    auto blockEnv = make_shared<environment::Environment>(env);
                    blockEnv->define(std::get<string>(stm->varName.lexeme), MacValue(key));
                    auto prevEnv = env;
                    env = blockEnv;
                    try {
                        execute(stm->body);
                    } catch (const errors::BreakException&) {
                        env = prevEnv;
                        break;
                    } catch (const errors::ContinueException&) {
                        // continue
                    } catch (...) {
                        env = prevEnv;
                        throw;
                    }
                    env = prevEnv;
                }
            } else {
                throw errors::RuntimeError(stm->varName, "Can only iterate over arrays and maps.");
            }
        }

        void visitBreakStmt(stmt::BreakStmt<MacValue>*) override {
            throw errors::BreakException();
        }

        void visitContinueStmt(stmt::ContinueStmt<MacValue>*) override {
            throw errors::ContinueException();
        }

        MacValue visitArrayExpr(expr::ArrayExpr<MacValue>* expr) override {
            auto arr = std::make_shared<collection::MacArray>();
            for (auto& elem : expr->elements) {
                arr->elements.push_back(evaluate(elem));
            }
            return MacValue(arr);
        }

        MacValue visitMapExpr(expr::MapExpr<MacValue>* expr) override {
            auto map = std::make_shared<collection::MacMap>();
            for (size_t i = 0; i < expr->keys.size(); i++) {
                auto key = std::get<string>(expr->keys[i].lexeme);
                map->set(key, evaluate(expr->values[i]));
            }
            return MacValue(map);
        }

        MacValue visitIndexGetExpr(expr::IndexGet<MacValue>* expr) override {
            MacValue object = evaluate(expr->object);
            MacValue index = evaluate(expr->index);

            if (std::holds_alternative<shared_ptr<collection::MacArray>>(object)) {
                auto arr = std::get<shared_ptr<collection::MacArray>>(object);
                if (!std::holds_alternative<double>(index))
                    throw errors::RuntimeError(expr->bracket, "Array index must be a number.");
                int idx = static_cast<int>(std::get<double>(index));
                if (idx < 0 || idx >= static_cast<int>(arr->elements.size()))
                    throw errors::RuntimeError(expr->bracket, "Array index out of bounds.");
                return arr->elements[idx];
            }
            if (std::holds_alternative<shared_ptr<collection::MacMap>>(object)) {
                auto map = std::get<shared_ptr<collection::MacMap>>(object);
                if (!std::holds_alternative<string>(index))
                    throw errors::RuntimeError(expr->bracket, "Map key must be a string.");
                return map->get(std::get<string>(index));
            }
            throw errors::RuntimeError(expr->bracket, "Only arrays and maps support indexing.");
        }

        MacValue visitIndexSetExpr(expr::IndexSet<MacValue>* expr) override {
            MacValue object = evaluate(expr->object);
            MacValue index = evaluate(expr->index);
            MacValue val = evaluate(expr->value);

            if (std::holds_alternative<shared_ptr<collection::MacArray>>(object)) {
                auto arr = std::get<shared_ptr<collection::MacArray>>(object);
                int idx = static_cast<int>(std::get<double>(index));
                if (idx < 0 || idx >= static_cast<int>(arr->elements.size()))
                    throw errors::RuntimeError(expr->bracket, "Array index out of bounds.");
                arr->elements[idx] = val;
                return val;
            }
            if (std::holds_alternative<shared_ptr<collection::MacMap>>(object)) {
                auto map = std::get<shared_ptr<collection::MacMap>>(object);
                map->set(std::get<string>(index), val);
                return val;
            }
            throw errors::RuntimeError(expr->bracket, "Only arrays and maps support index assignment.");
        }

        MacValue visitLambdaExpr(expr::LambdaExpr<MacValue>* expr) override {
            auto lambda = make_shared<callable::MacLambda>(expr->params, expr->body, env);
            return MacValue(std::static_pointer_cast<callable::MacCallable>(lambda));
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
            if (std::holds_alternative<shared_ptr<collection::MacArray>>(value)) {
                auto arr = std::get<shared_ptr<collection::MacArray>>(value);
                std::ostringstream ss;
                ss << "[";
                for (size_t i = 0; i < arr->elements.size(); i++) {
                    if (i > 0) ss << ", ";
                    ss << stringify(arr->elements[i]);
                }
                ss << "]";
                return ss.str();
            }
            if (std::holds_alternative<shared_ptr<collection::MacMap>>(value)) {
                auto map = std::get<shared_ptr<collection::MacMap>>(value);
                std::ostringstream ss;
                ss << "{";
                for (size_t i = 0; i < map->entries.size(); i++) {
                    if (i > 0) ss << ", ";
                    ss << map->entries[i].first << ": " << stringify(map->entries[i].second);
                }
                ss << "}";
                return ss.str();
            }
            if (std::holds_alternative<shared_ptr<meme::MacMeme>>(value)) {
                return std::get<shared_ptr<meme::MacMeme>>(value)->toString();
            }
            if (std::holds_alternative<shared_ptr<meme::MacGif>>(value)) {
                return std::get<shared_ptr<meme::MacGif>>(value)->toString();
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

// MacLambda::call implementation
inline value::MacValue callable::MacLambda::call(
    std::shared_ptr<interpreter::Interpreter> interpreter,
    std::vector<value::MacValue> arguments)
{
    auto funcEnv = std::make_shared<environment::Environment>(closure);
    for (size_t i = 0; i < params.size(); i++) {
        funcEnv->define(std::get<std::string>(params[i].lexeme), arguments[i]);
    }

    try {
        interpreter->executeBlock(body, funcEnv);
    } catch (const errors::Return& returnValue) {
        return returnValue.returnValue;
    }

    return std::monostate{};
}

#endif // INTERPRETER_H
