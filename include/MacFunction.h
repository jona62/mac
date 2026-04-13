#ifndef MAC_FUNCTION_H
#define MAC_FUNCTION_H

#include <memory>
#include <string>
#include <vector>
#include "MacCallable.h"
#include "MacInstance.h"
#include "Stmt.h"
#include "Environment.h"
#include "Return.h"

namespace interpreter {
    class Interpreter;
}

namespace callable {

    class MacFunction : public MacCallable {
    public:
        MacFunction(stmt::FunctionStmt<value::MacValue>* declaration,
                    std::shared_ptr<environment::Environment> closure)
            : declaration(declaration), closure(closure) {}

        value::MacValue call(std::shared_ptr<interpreter::Interpreter> interpreter,
                             std::vector<value::MacValue> arguments) override;

        int arity() override {
            return declaration->params.size();
        }

        std::shared_ptr<MacFunction> bind(std::shared_ptr<instance::MacInstance> inst) {
            auto env = std::make_shared<environment::Environment>(closure);
            env->define("this", value::MacValue(inst));
            return std::make_shared<MacFunction>(declaration, env);
        }

        std::string toString() override {
            return "<fn " + std::get<std::string>(declaration->name.lexeme) + ">";
        }

    private:
        stmt::FunctionStmt<value::MacValue>* declaration;
        std::shared_ptr<environment::Environment> closure;
    };

} // namespace callable

#endif // MAC_FUNCTION_H
