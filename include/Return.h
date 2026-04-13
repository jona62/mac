#ifndef RETURN_H
#define RETURN_H

#include <exception>
#include "MacValue.h"

namespace errors {

    class Return : public std::exception {
    public:
        value::MacValue returnValue;

        Return(value::MacValue value) : returnValue(value) {}

        const char* what() const noexcept override {
            return "Return";
        }
    };

} // namespace errors

#endif // RETURN_H
