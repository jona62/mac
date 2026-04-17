#ifndef ENVIRONMENT_H
#define ENVIRONMENT_H

#include <memory>               // shared_ptr, enable_shared_from_this
#include <string>               // string
#include <unordered_map>        // unordered_map (variable bindings)
#include <unordered_set>        // unordered_set (immutable tracking)
#include "MacValue.h"           // value::MacValue
#include "Token.h"              // token::Token (for error reporting)
#include "RuntimeError.h"       // errors::RuntimeError

namespace environment {

    using value::MacValue;

    class Environment : public std::enable_shared_from_this<Environment> {
    public:
        std::shared_ptr<Environment> enclosing;

        Environment() : enclosing(nullptr) {}
        Environment(std::shared_ptr<Environment> enclosing) : enclosing(enclosing) {}

        void define(const std::string& name, const MacValue& value, bool isVal = false) {
            values[name] = value;
            if (isVal) immutables.insert(name);
        }

        MacValue get(const token::Token& name) {
            auto nameStr = std::get<std::string>(name.lexeme);
            auto it = values.find(nameStr);
            if (it != values.end()) {
                return it->second;
            }
            if (enclosing != nullptr) {
                return enclosing->get(name);
            }
            throw errors::RuntimeError(name, "Undefined variable '" + nameStr + "'.");
        }

        void assign(const token::Token& name, const MacValue& value) {
            auto nameStr = std::get<std::string>(name.lexeme);
            auto it = values.find(nameStr);
            if (it != values.end()) {
                if (immutables.count(nameStr)) {
                    throw errors::RuntimeError(name, "Cannot reassign 'val' binding '" + nameStr + "'.");
                }
                it->second = value;
                return;
            }
            if (enclosing != nullptr) {
                enclosing->assign(name, value);
                return;
            }
            throw errors::RuntimeError(name, "Undefined variable '" + nameStr + "'.");
        }

        MacValue getAt(int distance, const std::string& name) {
            return ancestor(distance)->values[name];
        }

        void assignAt(int distance, const std::string& name, const MacValue& value) {
            auto env = ancestor(distance);
            if (env->immutables.count(name)) {
                token::Token tok(token::TokenType::IDENTIFIER, token::TokenValue(name), 0);
                throw errors::RuntimeError(tok, "Cannot reassign 'val' binding '" + name + "'.");
            }
            env->values[name] = value;
        }

    private:
        std::unordered_map<std::string, MacValue> values;
        std::unordered_set<std::string> immutables;

        std::shared_ptr<Environment> ancestor(int distance) {
            auto env = shared_from_this();
            for (int i = 0; i < distance; i++) {
                env = env->enclosing;
            }
            return env;
        }
    };

} // namespace environment

#endif // ENVIRONMENT_H
