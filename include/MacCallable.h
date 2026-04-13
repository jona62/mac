#ifndef MAC_CALLABLE_H
#define MAC_CALLABLE_H

#include <memory>
#include <string>
#include <vector>
#include "MacValue.h"

namespace interpreter {
    class Interpreter;
}

namespace callable {

    class MacCallable {
    public:
        virtual value::MacValue call(std::shared_ptr<interpreter::Interpreter> interpreter,
                                     std::vector<value::MacValue> arguments) = 0;
        virtual int arity() = 0;
        virtual std::string toString() = 0;
        virtual ~MacCallable() = default;
    };

} // namespace callable

#endif // MAC_CALLABLE_H
