#ifndef MAC_CLASS_H
#define MAC_CLASS_H

#include <memory>               // shared_ptr, enable_shared_from_this
#include <string>               // string
#include <unordered_map>        // unordered_map (method table)
#include "MacCallable.h"        // callable::MacCallable (base class)
#include "MacInstance.h"        // instance::MacInstance (instantiation)
#include "MacFunction.h"        // callable::MacFunction (method storage)

namespace callable {

    class MacClass : public MacCallable, public std::enable_shared_from_this<MacClass> {
    public:
        std::string name;
        std::shared_ptr<MacClass> superclass;
        std::unordered_map<std::string, std::shared_ptr<MacFunction>> methods;

        MacClass(const std::string& name,
                 std::shared_ptr<MacClass> superclass,
                 std::unordered_map<std::string, std::shared_ptr<MacFunction>> methods)
            : name(name), superclass(superclass), methods(methods) {}

        value::MacValue call(std::shared_ptr<interpreter::Interpreter> interpreter,
                             std::vector<value::MacValue> arguments) override {
            auto inst = std::make_shared<instance::MacInstance>(shared_from_this());
            auto initializer = findMethod("init");
            if (initializer != nullptr) {
                initializer->bind(inst)->call(interpreter, arguments);
            }
            return value::MacValue(inst);
        }

        int arity() override {
            auto initializer = findMethod("init");
            if (initializer != nullptr) return initializer->arity();
            return 0;
        }

        std::string toString() override {
            return name;
        }

        std::shared_ptr<MacFunction> findMethod(const std::string& methodName) {
            auto it = methods.find(methodName);
            if (it != methods.end()) return it->second;
            if (superclass != nullptr) return superclass->findMethod(methodName);
            return nullptr;
        }
    };

} // namespace callable

// MacInstance method implementations — need MacClass definition

inline value::MacValue instance::MacInstance::get(const token::Token& name) {
    auto nameStr = std::get<std::string>(name.lexeme);
    auto it = fields.find(nameStr);
    if (it != fields.end()) return it->second;

    auto method = klass->findMethod(nameStr);
    if (method != nullptr) return value::MacValue(
        std::static_pointer_cast<callable::MacCallable>(method->bind(shared_from_this())));

    throw errors::RuntimeError(name, "Undefined property '" + nameStr + "'.");
}

inline std::string instance::MacInstance::toString() {
    return klass->name + " instance";
}

#endif // MAC_CLASS_H
