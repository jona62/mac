#ifndef MAC_INSTANCE_H
#define MAC_INSTANCE_H

#include <memory>
#include <string>
#include <unordered_map>
#include "MacValue.h"
#include "Token.h"
#include "RuntimeError.h"

namespace callable {
    class MacClass;
}

namespace instance {

    class MacInstance : public std::enable_shared_from_this<MacInstance> {
    public:
        MacInstance(std::shared_ptr<callable::MacClass> klass) : klass(klass) {}

        value::MacValue get(const token::Token& name);

        void set(const token::Token& name, const value::MacValue& val) {
            fields[std::get<std::string>(name.lexeme)] = val;
        }

        std::string toString();

    private:
        std::shared_ptr<callable::MacClass> klass;
        std::unordered_map<std::string, value::MacValue> fields;
    };

} // namespace instance

#endif // MAC_INSTANCE_H
