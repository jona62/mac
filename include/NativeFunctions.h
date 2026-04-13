#ifndef NATIVE_FUNCTIONS_H
#define NATIVE_FUNCTIONS_H

#include <chrono>
#include <string>
#include "MacCallable.h"

namespace callable {

    class ClockFunction : public MacCallable {
    public:
        value::MacValue call(std::shared_ptr<interpreter::Interpreter>,
                             std::vector<value::MacValue>) override {
            auto now = std::chrono::system_clock::now();
            auto duration = now.time_since_epoch();
            auto seconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000.0;
            return seconds;
        }

        int arity() override { return 0; }

        std::string toString() override { return "<native fn>"; }
    };

} // namespace callable

#endif // NATIVE_FUNCTIONS_H
