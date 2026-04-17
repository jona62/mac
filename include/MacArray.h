#ifndef MAC_ARRAY_H
#define MAC_ARRAY_H

#include <memory>               // shared_ptr
#include <string>               // string
#include <sstream>              // ostringstream (toString)
#include <vector>               // vector (element storage)
#include "MacValue.h"           // value::MacValue (element type)

namespace collection {

    class MacArray {
    public:
        std::vector<value::MacValue> elements;

        std::string toString() const;  // Defined after Interpreter (needs stringify)
    };

} // namespace collection

#endif // MAC_ARRAY_H
