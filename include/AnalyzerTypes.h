#ifndef ANALYZER_TYPES_H
#define ANALYZER_TYPES_H

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "MacValue.h"

namespace analyzer {

    using MV = value::MacValue;

    struct SymbolDef {
        std::string name, kind, type, description;
        int line, col, endCol;
    };

    struct Reference {
        int line, col, endCol, defLine, defCol;
        std::string defName;
    };

    struct Diagnostic {
        int line, col, endCol;
        std::string message, severity;
    };

    struct PropertyRef {
        int line, col, endCol;
        std::string name, ownerType, kind, description;
    };

    struct FoldRange { int startLine, endLine; };
    struct SemanticToken { int line, col, length; std::string tokenType; };
    struct ParamHint { int line, col; std::string name; };
    struct ChainHint { int line, endCol; std::string type; };

    struct Signature {
        std::string name, returnType, description;
        std::vector<std::string> params;
    };

    struct AnalysisResult {
        std::vector<SymbolDef> symbols;
        std::vector<Reference> references;
        std::vector<Diagnostic> diagnostics;
        std::vector<PropertyRef> properties;
        std::vector<FoldRange> foldingRanges;
        std::vector<SemanticToken> semanticTokens;
        std::vector<ParamHint> paramHints;
        std::vector<ChainHint> chainHints;
        std::vector<Signature> signatures;
    };

    struct Scope {
        std::unordered_map<std::string, SymbolDef> symbols;
        Scope* parent = nullptr;
    };

    struct PropInfo { std::string ownerType, kind, description; };

    // JSON helpers

    inline std::string J(const std::string& s) {
        std::string o = "\"";
        for (char c : s) {
            if (c == '"') o += "\\\""; else if (c == '\\') o += "\\\\";
            else if (c == '\n') o += "\\n"; else o += c;
        }
        return o + "\"";
    }

    inline std::string toJson(const AnalysisResult& r) {
        std::ostringstream o;
        o << "{\"symbols\":[";
        for (size_t i = 0; i < r.symbols.size(); i++) {
            auto& s = r.symbols[i]; if (i) o << ",";
            o << "{\"name\":" << J(s.name) << ",\"kind\":" << J(s.kind) << ",\"line\":" << s.line
              << ",\"col\":" << s.col << ",\"endCol\":" << s.endCol << ",\"type\":" << J(s.type)
              << ",\"description\":" << J(s.description) << "}";
        }
        o << "],\"references\":[";
        for (size_t i = 0; i < r.references.size(); i++) {
            auto& v = r.references[i]; if (i) o << ",";
            o << "{\"line\":" << v.line << ",\"col\":" << v.col << ",\"endCol\":" << v.endCol
              << ",\"defLine\":" << v.defLine << ",\"defCol\":" << v.defCol << ",\"defName\":" << J(v.defName) << "}";
        }
        o << "],\"diagnostics\":[";
        for (size_t i = 0; i < r.diagnostics.size(); i++) {
            auto& d = r.diagnostics[i]; if (i) o << ",";
            o << "{\"line\":" << d.line << ",\"col\":" << d.col << ",\"endCol\":" << d.endCol
              << ",\"message\":" << J(d.message) << ",\"severity\":" << J(d.severity) << "}";
        }
        o << "],\"properties\":[";
        for (size_t i = 0; i < r.properties.size(); i++) {
            auto& p = r.properties[i]; if (i) o << ",";
            o << "{\"line\":" << p.line << ",\"col\":" << p.col << ",\"endCol\":" << p.endCol
              << ",\"name\":" << J(p.name) << ",\"ownerType\":" << J(p.ownerType)
              << ",\"kind\":" << J(p.kind) << ",\"description\":" << J(p.description) << "}";
        }
        o << "],\"foldingRanges\":[";
        for (size_t i = 0; i < r.foldingRanges.size(); i++) {
            auto& f = r.foldingRanges[i]; if (i) o << ",";
            o << "{\"startLine\":" << f.startLine << ",\"endLine\":" << f.endLine << "}";
        }
        o << "],\"semanticTokens\":[";
        for (size_t i = 0; i < r.semanticTokens.size(); i++) {
            auto& t = r.semanticTokens[i]; if (i) o << ",";
            o << "{\"line\":" << t.line << ",\"col\":" << t.col << ",\"length\":" << t.length
              << ",\"tokenType\":" << J(t.tokenType) << "}";
        }
        o << "],\"paramHints\":[";
        for (size_t i = 0; i < r.paramHints.size(); i++) {
            auto& h = r.paramHints[i]; if (i) o << ",";
            o << "{\"line\":" << h.line << ",\"col\":" << h.col << ",\"name\":" << J(h.name) << "}";
        }
        o << "],\"chainHints\":[";
        for (size_t i = 0; i < r.chainHints.size(); i++) {
            auto& h = r.chainHints[i]; if (i) o << ",";
            o << "{\"line\":" << h.line << ",\"endCol\":" << h.endCol << ",\"type\":" << J(h.type) << "}";
        }
        o << "],\"signatures\":[";
        for (size_t i = 0; i < r.signatures.size(); i++) {
            auto& s = r.signatures[i]; if (i) o << ",";
            o << "{\"name\":" << J(s.name) << ",\"returnType\":" << J(s.returnType)
              << ",\"description\":" << J(s.description) << ",\"params\":[";
            for (size_t j = 0; j < s.params.size(); j++) { if (j) o << ","; o << J(s.params[j]); }
            o << "]}";
        }
        o << "]}";
        return o.str();
    }

} // namespace analyzer

#endif // ANALYZER_TYPES_H
