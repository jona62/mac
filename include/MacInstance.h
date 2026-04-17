#ifndef MAC_INSTANCE_H
#define MAC_INSTANCE_H

#include <memory>               // shared_ptr
#include <string>               // string
#include <unordered_map>        // unordered_map (instance fields)
#include "MacValue.h"           // value::MacValue (field values)
#include "Token.h"              // token::Token (property access errors)
#include "RuntimeError.h"       // errors::RuntimeError

namespace callable {
    class MacClass;
}

namespace instance {

    class MacInstance : public std::enable_shared_from_this<MacInstance> {
    public:
        MacInstance(std::shared_ptr<callable::MacClass> klass) : klass(klass) {}

        std::shared_ptr<callable::MacClass> getClass() const { return klass; }

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
