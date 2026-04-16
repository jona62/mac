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
#include "RenderSurface.h"
#include "NativeRegistry.h"
#include "Environment.h"
#include "RuntimeError.h"
#include "Return.h"

using std::shared_ptr;
using std::make_shared;
using std::string;
using std::monostate;

namespace callable {
    class ComposedFunction : public MacCallable {
    public:
        ComposedFunction(std::shared_ptr<MacCallable> first, std::shared_ptr<MacCallable> second)
            : first(first), second(second) {}
        value::MacValue call(std::shared_ptr<interpreter::Interpreter> interp,
                             std::vector<value::MacValue> args) override {
            auto intermediate = first->call(interp, args);
            return second->call(interp, {intermediate});
        }
        int arity() override { return first->arity(); }
        std::string toString() override { return "<composed>"; }
    private:
        std::shared_ptr<MacCallable> first;
        std::shared_ptr<MacCallable> second;
    };
}

namespace interpreter {

    using MacValue = value::MacValue;

    class Interpreter : public expr::Visitor<MacValue>,
                        public stmt::StmtVisitor<MacValue>,
                        public std::enable_shared_from_this<Interpreter> {
    public:
        Interpreter() : globals(make_shared<environment::Environment>()), env(globals) {
            for (const auto& def : native_registry::all()) {
                globals->define(def.name, MacValue(def.factory()));
            }
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
            if (function->arity() != -1 && static_cast<int>(arguments.size()) != function->arity()) {
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

        void visitStyleStmt(stmt::StyleStmt<MacValue>* stmt) override {
            auto map = std::make_shared<collection::MacMap>();
            for (auto& [key, val] : stmt->properties) {
                auto k = std::get<std::string>(key.lexeme);
                auto v = evaluate(val);
                map->set(k, v);
            }
            env->define(std::get<std::string>(stmt->name.lexeme), MacValue(map));
        }

        void visitEffectStmt(stmt::EffectStmt<MacValue>* stmt) override {
            // Same as var declaration — effect is just a named compose
            MacValue value = evaluate(stmt->value);
            env->define(std::get<std::string>(stmt->name.lexeme), value);
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

        MacValue visitPipeExpr(expr::PipeExpr<MacValue>* expr) override {
            MacValue value = evaluate(expr->value);

            // If right side is a Call expr, prepend the piped value to args
            if (auto* callExpr = dynamic_cast<expr::Call<MacValue>*>(expr->func.get())) {
                MacValue callee = evaluate(callExpr->callee);
                std::vector<MacValue> args;
                args.push_back(value); // prepend piped value
                for (auto& arg : callExpr->arguments) {
                    args.push_back(evaluate(arg));
                }
                if (!std::holds_alternative<shared_ptr<callable::MacCallable>>(callee)) {
                    throw errors::RuntimeError(expr->op, "Pipe target must be callable.");
                }
                auto fn = std::get<shared_ptr<callable::MacCallable>>(callee);
                return fn->call(shared_from_this(), args);
            }

            // Otherwise evaluate as callable and call with the value
            MacValue func = evaluate(expr->func);
            if (!std::holds_alternative<shared_ptr<callable::MacCallable>>(func)) {
                throw errors::RuntimeError(expr->op, "Pipe target must be callable.");
            }
            auto fn = std::get<shared_ptr<callable::MacCallable>>(func);
            return fn->call(shared_from_this(), {value});
        }

        MacValue visitComposeExpr(expr::ComposeExpr<MacValue>* expr) override {
            MacValue left = evaluate(expr->left);
            MacValue right = evaluate(expr->right);
            if (!std::holds_alternative<shared_ptr<callable::MacCallable>>(left) ||
                !std::holds_alternative<shared_ptr<callable::MacCallable>>(right)) {
                throw errors::RuntimeError(expr->op, "Compose (>>) requires two callable values.");
            }
            auto fn1 = std::get<shared_ptr<callable::MacCallable>>(left);
            auto fn2 = std::get<shared_ptr<callable::MacCallable>>(right);
            auto composed = make_shared<callable::ComposedFunction>(fn1, fn2);
            return MacValue(std::static_pointer_cast<callable::MacCallable>(composed));
        }

        // --- Mac v2 syntax visitors ---

        MacValue visitMemeLiteralExpr(expr::MemeLiteralExpr<MacValue>* expr) override {
            auto templateName = std::get<std::string>(expr->templateName.lexeme);

            // Call Template(name) then Meme(template) via the prelude classes
            auto templateClass = env->get(token::Token(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("Template")), 0));
            auto memeClass = env->get(token::Token(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("Meme")), 0));

            auto templateFn = std::get<shared_ptr<callable::MacCallable>>(templateClass);
            auto memeFn = std::get<shared_ptr<callable::MacCallable>>(memeClass);

            auto tmpl = templateFn->call(shared_from_this(), {MacValue(templateName)});
            auto meme = memeFn->call(shared_from_this(), {tmpl});

            // Apply text positions by calling .text(position, str) on the meme instance
            auto posTop = env->get(token::Token(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("Top")), 0));
            auto posBottom = env->get(token::Token(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("Bottom")), 0));
            auto posCenter = env->get(token::Token(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("Center")), 0));

            // Collect positioned text entries (text: "..." x: N y: N)
            std::vector<meme::PositionedText> positionedTexts;
            for (size_t ei = 0; ei < expr->entries.size(); ei++) {
                auto key = std::get<std::string>(expr->entries[ei].key.lexeme);

                if (key == "text") {
                    // Positioned text: look ahead for x, y, and optional fontSize
                    auto textVal = evaluate(expr->entries[ei].value);
                    auto content = std::get<std::string>(textVal);
                    int px = 0, py = 0;
                    float fontSizeOverride = 0;
                    while (ei + 1 < expr->entries.size()) {
                        auto nk = std::get<std::string>(expr->entries[ei + 1].key.lexeme);
                        if (nk == "x") {
                            px = static_cast<int>(std::get<double>(evaluate(expr->entries[ei + 1].value)));
                            ei++;
                            continue;
                        }
                        if (nk == "y") {
                            py = static_cast<int>(std::get<double>(evaluate(expr->entries[ei + 1].value)));
                            ei++;
                            continue;
                        }
                        if (nk == "fontSize") {
                            auto fontSizeVal = evaluate(expr->entries[ei + 1].value);
                            if (std::holds_alternative<std::string>(fontSizeVal)) {
                                auto fontSize = std::get<std::string>(fontSizeVal);
                                if (fontSize == "sm") fontSizeOverride = -1;
                                else if (fontSize == "md") fontSizeOverride = -2;
                                else if (fontSize == "lg") fontSizeOverride = -3;
                                else if (fontSize == "xlg") fontSizeOverride = -4;
                            } else if (std::holds_alternative<double>(fontSizeVal)) {
                                fontSizeOverride = static_cast<float>(std::get<double>(fontSizeVal));
                            }
                            ei++;
                            continue;
                        }
                        break;
                    }
                    positionedTexts.push_back({content, px, py, fontSizeOverride});
                    continue;
                }

                if (key == "x" || key == "y" || key == "fontSize") continue; // consumed by text above

                auto textVal = evaluate(expr->entries[ei].value);
                MacValue position;
                if (key == "top") position = posTop;
                else if (key == "bottom") position = posBottom;
                else position = posCenter;

                auto inst = std::get<shared_ptr<instance::MacInstance>>(meme);
                token::Token textTok(token::TokenType::IDENTIFIER,
                    token::TokenValue(std::string("text")), 0);
                auto method = inst->get(textTok);
                auto fn = std::get<shared_ptr<callable::MacCallable>>(method);
                meme = fn->call(shared_from_this(), {position, textVal});
            }

            // Apply style if specified: @template styleName { ... }
            if (expr->styleName.type != token::TokenType::NONE) {
                auto styleNameStr = std::get<std::string>(expr->styleName.lexeme);
                auto styleVal = env->get(expr->styleName);
                if (std::holds_alternative<shared_ptr<collection::MacMap>>(styleVal)) {
                    auto styleMap = std::get<shared_ptr<collection::MacMap>>(styleVal);
                    // Store style on the meme instance for rendering
                    auto inst = std::get<shared_ptr<instance::MacInstance>>(meme);
                    token::Token styleTok(token::TokenType::IDENTIFIER,
                        token::TokenValue(std::string("_style")), 0);
                    inst->set(styleTok, styleVal);
                }
            }

            // Apply size if specified: @template WxH { ... }
            if (expr->width > 0 && expr->height > 0) {
                auto sizeClass = env->get(token::Token(token::TokenType::IDENTIFIER,
                    token::TokenValue(std::string("Size")), 0));
                auto sizeFn = std::get<shared_ptr<callable::MacCallable>>(sizeClass);
                auto size = sizeFn->call(shared_from_this(), {
                    MacValue(static_cast<double>(expr->width)),
                    MacValue(static_cast<double>(expr->height))
                });

                auto inst = std::get<shared_ptr<instance::MacInstance>>(meme);
                token::Token resizeTok(token::TokenType::IDENTIFIER,
                    token::TokenValue(std::string("resize")), 0);
                auto method = inst->get(resizeTok);
                auto fn = std::get<shared_ptr<callable::MacCallable>>(method);
                meme = fn->call(shared_from_this(), {size});
            }

            // Store positioned texts AFTER style/resize (which create new instances)
            if (!positionedTexts.empty()) {
                auto inst = std::get<shared_ptr<instance::MacInstance>>(meme);
                auto arr = std::make_shared<collection::MacArray>();
                for (auto& pt : positionedTexts) {
                    auto map = std::make_shared<collection::MacMap>();
                    map->set("content", MacValue(pt.content));
                    map->set("x", MacValue(static_cast<double>(pt.x)));
                    map->set("y", MacValue(static_cast<double>(pt.y)));
                    if (pt.fontSizeOverride != 0) {
                        if (pt.fontSizeOverride == -1) map->set("fontSize", MacValue(std::string("sm")));
                        else if (pt.fontSizeOverride == -2) map->set("fontSize", MacValue(std::string("md")));
                        else if (pt.fontSizeOverride == -3) map->set("fontSize", MacValue(std::string("lg")));
                        else if (pt.fontSizeOverride == -4) map->set("fontSize", MacValue(std::string("xlg")));
                        else map->set("fontSize", MacValue(static_cast<double>(pt.fontSizeOverride)));
                    }
                    arr->elements.push_back(MacValue(map));
                }
                token::Token ptTok(token::TokenType::IDENTIFIER,
                    token::TokenValue(std::string("positionedTexts")), 0);
                inst->set(ptTok, MacValue(arr));
            }

            return meme;
        }

        MacValue visitSaveExpr(expr::SaveExpr<MacValue>* expr) override {
            // Desugar: expr => "path" becomes save(expr, "path")
            // The global save() function handles all types: Meme instances, Gif, Timeline, MacMap
            auto value = evaluate(expr->value);
            auto pathVal = evaluate(expr->path);
            auto saveFn = env->get(token::Token(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("save")), 0));
            auto fn = std::get<shared_ptr<callable::MacCallable>>(saveFn);
            return fn->call(shared_from_this(), {value, pathVal});
        }

        MacValue visitGifBlockExpr(expr::GifBlockExpr<MacValue>* expr) override {
            // Build GIF directly using C++ MacGif to handle both
            // Meme instances and rendered maps without temp files.
            auto rawGif = std::make_shared<meme::MacGif>();

            for (auto& frame : expr->frames) {
                auto meme = evaluate(frame.meme);
                int durationMs = static_cast<int>(frame.durationMs);
                rawGif->addFrame(callable::getRenderSurface(meme), durationMs);
            }

            // Wrap in a prelude Gif instance so => and .save() work consistently
            auto gifClass = env->get(token::Token(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("Gif")), 0));
            auto gifFn = std::get<shared_ptr<callable::MacCallable>>(gifClass);
            auto gif = gifFn->call(shared_from_this(), {});
            auto inst = std::get<shared_ptr<instance::MacInstance>>(gif);
            token::Token gifTok(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("_gif")), 0);
            inst->set(gifTok, MacValue(rawGif));
            // Set frameCount to match
            token::Token fcTok(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("frameCount")), 0);
            inst->set(fcTok, MacValue(static_cast<double>(expr->frames.size())));

            return gif;
        }

        MacValue visitTimelineBlockExpr(expr::TimelineBlockExpr<MacValue>* expr) override {
            // Build timeline directly using C++ MacTimeline to handle
            // both Meme instances and rendered maps without temp files.
            auto rawTl = std::make_shared<meme::MacTimeline>();

            for (auto& entry : expr->entries) {
                auto meme = evaluate(entry.frame.meme);
                int holdMs = static_cast<int>(entry.frame.durationMs);

                rawTl->addKeyframe(callable::getRenderSurface(meme));
                rawTl->addHold(holdMs);

                if (entry.transition) {
                    std::string easing = entry.transition->easing.empty() ? "linear" : entry.transition->easing;
                    rawTl->setTransition(static_cast<int>(entry.transition->durationMs),
                                         entry.transition->type, easing);
                }
            }

            if (expr->loop) rawTl->setLoop(0);

            // Wrap in a Timeline prelude instance so => and .render() work
            auto tlClass = env->get(token::Token(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("Timeline")), 0));
            auto tlFn = std::get<shared_ptr<callable::MacCallable>>(tlClass);
            auto tl = tlFn->call(shared_from_this(), {});
            // Replace the internal _tl with our populated one
            auto inst = std::get<shared_ptr<instance::MacInstance>>(tl);
            token::Token tlTok(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("_tl")), 0);
            inst->set(tlTok, MacValue(rawTl));

            return tl;
        }

        MacValue visitGridBlockExpr(expr::GridBlockExpr<MacValue>* expr) override {
            // Evaluate all entries into an array
            auto arr = std::make_shared<collection::MacArray>();
            for (auto& e : expr->entries) {
                arr->elements.push_back(evaluate(e));
            }

            // Call toGrid(arr, cols, rows)
            auto toGridFn = env->get(token::Token(token::TokenType::IDENTIFIER,
                token::TokenValue(std::string("toGrid")), 0));
            auto fn = std::get<shared_ptr<callable::MacCallable>>(toGridFn);
            return fn->call(shared_from_this(), {
                MacValue(arr),
                MacValue(static_cast<double>(expr->cols)),
                MacValue(static_cast<double>(expr->rows))
            });
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
            if (std::holds_alternative<shared_ptr<meme::RenderSurface>>(value)) {
                return std::get<shared_ptr<meme::RenderSurface>>(value)->toString();
            }
            if (std::holds_alternative<shared_ptr<meme::MacMeme>>(value)) {
                return std::get<shared_ptr<meme::MacMeme>>(value)->toString();
            }
            if (std::holds_alternative<shared_ptr<meme::MacGif>>(value)) {
                return std::get<shared_ptr<meme::MacGif>>(value)->toString();
            }
            if (std::holds_alternative<shared_ptr<meme::MacTimeline>>(value)) {
                return std::get<shared_ptr<meme::MacTimeline>>(value)->toString();
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
