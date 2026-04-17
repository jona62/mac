#ifndef MAC_LAMBDA_H
#define MAC_LAMBDA_H

#include <memory>               // shared_ptr
#include <string>               // string
#include <vector>               // vector (parameter names)
#include "MacCallable.h"        // callable::MacCallable (base class)
#include "Stmt.h"               // stmt::Stmt (lambda body AST)
#include "Environment.h"        // environment::Environment (closure capture)
#include "Return.h"             // errors::Return (return value exception)

namespace callable {

    class MacLambda : public MacCallable {
    public:
        MacLambda(std::vector<token::Token> params,
                  std::vector<std::shared_ptr<stmt::Stmt<value::MacValue>>> body,
                  std::shared_ptr<environment::Environment> closure)
            : params(params), body(body), closure(closure) {}

        value::MacValue call(std::shared_ptr<interpreter::Interpreter> interpreter,
                             std::vector<value::MacValue> arguments) override;

        int arity() override { return params.size(); }
        std::string toString() override { return "<lambda>"; }

    private:
        std::vector<token::Token> params;
        std::vector<std::shared_ptr<stmt::Stmt<value::MacValue>>> body;
        std::shared_ptr<environment::Environment> closure;
    };

} // namespace callable

#endif // MAC_LAMBDA_H
