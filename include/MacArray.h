#ifndef MAC_ARRAY_H
#define MAC_ARRAY_H

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include "MacValue.h"

namespace collection {

    class MacArray {
    public:
        std::vector<value::MacValue> elements;

        std::string toString() const;  // Defined after Interpreter (needs stringify)
    };

} // namespace collection

#endif // MAC_ARRAY_H
