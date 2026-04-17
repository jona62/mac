#ifndef MAC_ENUM_H
#define MAC_ENUM_H

#include <memory>               // shared_ptr
#include <string>               // string
#include <vector>               // vector (variant fields, field values)
#include "MacValue.h"           // value::MacValue (field value type)

namespace enumeration {

    struct VariantDef {
        std::string name;
        std::vector<std::string> fields; // empty for simple variants like Red
    };

    struct MacEnumDef {
        std::string name; // e.g. "Result"
        std::vector<VariantDef> variants;

        const VariantDef* getVariant(const std::string& variantName) const {
            for (auto& v : variants)
                if (v.name == variantName) return &v;
            return nullptr;
        }
    };

    class MacEnum {
    public:
        std::shared_ptr<MacEnumDef> def;
        std::string tag; // variant name, e.g. "Ok"
        std::vector<value::MacValue> fieldValues;
    };

} // namespace enumeration

#endif // MAC_ENUM_H
