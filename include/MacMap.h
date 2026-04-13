#ifndef MAC_MAP_H
#define MAC_MAP_H

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include "MacValue.h"
#include "RuntimeError.h"

namespace collection {

    class MacMap {
    public:
        std::vector<std::pair<std::string, value::MacValue>> entries;
        std::unordered_map<std::string, size_t> index;

        value::MacValue get(const std::string& key) const {
            auto it = index.find(key);
            if (it == index.end()) {
                return std::monostate{};
            }
            return entries[it->second].second;
        }

        void set(const std::string& key, const value::MacValue& val) {
            auto it = index.find(key);
            if (it != index.end()) {
                entries[it->second].second = val;
            } else {
                index[key] = entries.size();
                entries.push_back({key, val});
            }
        }

        bool has(const std::string& key) const {
            return index.count(key) > 0;
        }

        std::string toString() const;  // Defined after Interpreter (needs stringify)
    };

} // namespace collection

#endif // MAC_MAP_H
