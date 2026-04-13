#ifndef MAC_VALUE_H
#define MAC_VALUE_H

#include <memory>
#include <string>
#include <variant>

namespace callable {
    class MacCallable;
}

namespace instance {
    class MacInstance;
}

namespace value {
    using MacValue = std::variant<std::string, double, bool, std::monostate,
                                  std::shared_ptr<callable::MacCallable>,
                                  std::shared_ptr<instance::MacInstance>>;
}

#endif // MAC_VALUE_H
