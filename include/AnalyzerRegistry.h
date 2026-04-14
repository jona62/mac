#ifndef ANALYZER_REGISTRY_H
#define ANALYZER_REGISTRY_H

#include <functional>
#include <string>
#include <unordered_set>
#include "AnalyzerTypes.h"
#include "NativeRegistry.h"
#include "Token.h"

namespace analyzer {

    using DefFn = std::function<void(const token::Token&, const std::string&, const std::string&,
                                     const std::string&, const std::string&, const std::string&,
                                     const std::string&)>;
    using SigFn = std::function<void(const Signature&)>;

    inline void registerNatives(
        std::unordered_set<std::string>& nativeNames,
        DefFn define,
        SigFn addSignature
    ) {
        for (const auto& native : native_registry::all()) {
            nativeNames.insert(native.name);

            token::Token tok(token::TokenType::IDENTIFIER, token::TokenValue(native.name), 0, 1);
            define(tok, "native", native.symbolType, native.description, "native",
                   native_registry::visibilityName(native.visibility), "");

            for (const auto& overload : native.overloads) {
                Signature sig;
                sig.name = native.name;
                sig.kind = "native";
                sig.returnType = overload.returnType;
                sig.description = overload.description.empty() ? native.description : overload.description;
                sig.source = "native";
                sig.visibility = native_registry::visibilityName(native.visibility);
                sig.line = 0;
                sig.col = 1;
                sig.endCol = static_cast<int>(native.name.size()) + 1;
                sig.params = overload.params;
                addSignature(sig);
            }
        }
    }

} // namespace analyzer

#endif // ANALYZER_REGISTRY_H
