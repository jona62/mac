#ifndef RETURN_H
#define RETURN_H

#include <exception>            // exception (base class)
#include "MacValue.h"           // value::MacValue (return payload)

namespace errors {

    class Return : public std::exception {
    public:
        value::MacValue returnValue;

        Return(value::MacValue value) : returnValue(value) {}

        const char* what() const noexcept override {
            return "Return";
        }
    };

    class BreakException : public std::exception {
    public:
        const char* what() const noexcept override { return "Break"; }
    };

    class ContinueException : public std::exception {
    public:
        const char* what() const noexcept override { return "Continue"; }
    };

} // namespace errors

#endif // RETURN_H
