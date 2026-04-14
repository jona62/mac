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
        std::string name;
        std::string kind;
        std::string type;
        std::string description;
        std::string source;
        std::string visibility;
        std::string ownerType;
        int line;
        int col;
        int endCol;
    };

    struct Reference {
        int line;
        int col;
        int endCol;
        int defLine;
        int defCol;
        int defEndCol;
        std::string defName;
        std::string source;
        std::string defSource;
        std::string defVisibility;
        std::string defOwnerType;
    };

    struct Diagnostic {
        int line;
        int col;
        int endCol;
        std::string message;
        std::string severity;
        std::string source;
    };

    struct PropertyRef {
        int line;
        int col;
        int endCol;
        int defLine;
        int defCol;
        int defEndCol;
        std::string name;
        std::string ownerType;
        std::string kind;
        std::string type;
        std::string description;
        std::string source;
        std::string defSource;
        std::string visibility;
    };

    struct FoldRange {
        int startLine;
        int endLine;
        std::string source;
    };

    struct SemanticToken {
        int line;
        int col;
        int length;
        std::string tokenType;
        std::string source;
    };

    struct ParamHint {
        int line;
        int col;
        std::string name;
        std::string source;
    };

    struct ChainHint {
        int line;
        int endCol;
        std::string type;
        std::string source;
    };

    struct Signature {
        std::string name;
        std::string ownerType;
        std::string kind;
        std::string returnType;
        std::string description;
        std::string source;
        std::string visibility;
        int line;
        int col;
        int endCol;
        std::vector<std::string> params;
    };

    struct ClassMember {
        std::string name;
        std::string kind;
        std::string type;
        std::string returnType;
        std::string description;
        std::string source;
        std::string visibility;
        int line;
        int col;
        int endCol;
        std::vector<std::string> params;
    };

    struct ClassInfo {
        std::string name;
        std::string superclass;
        std::string description;
        std::string source;
        std::string visibility;
        int line;
        int col;
        int endCol;
        std::vector<ClassMember> members;
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
        std::vector<ClassInfo> classes;
    };

    struct Scope {
        std::unordered_map<std::string, SymbolDef> symbols;
        Scope* parent = nullptr;
    };

    inline std::string J(const std::string& s) {
        std::string o = "\"";
        for (char c : s) {
            if (c == '"') o += "\\\"";
            else if (c == '\\') o += "\\\\";
            else if (c == '\n') o += "\\n";
            else o += c;
        }
        return o + "\"";
    }

    inline std::string jsonParams(const std::vector<std::string>& params) {
        std::ostringstream o;
        o << "[";
        for (size_t i = 0; i < params.size(); i++) {
            if (i) o << ",";
            o << J(params[i]);
        }
        o << "]";
        return o.str();
    }

    inline std::string toJson(const AnalysisResult& r) {
        std::ostringstream o;
        o << "{\"symbols\":[";
        for (size_t i = 0; i < r.symbols.size(); i++) {
            const auto& s = r.symbols[i];
            if (i) o << ",";
            o << "{\"name\":" << J(s.name)
              << ",\"kind\":" << J(s.kind)
              << ",\"type\":" << J(s.type)
              << ",\"description\":" << J(s.description)
              << ",\"source\":" << J(s.source)
              << ",\"visibility\":" << J(s.visibility)
              << ",\"ownerType\":" << J(s.ownerType)
              << ",\"line\":" << s.line
              << ",\"col\":" << s.col
              << ",\"endCol\":" << s.endCol
              << "}";
        }

        o << "],\"references\":[";
        for (size_t i = 0; i < r.references.size(); i++) {
            const auto& v = r.references[i];
            if (i) o << ",";
            o << "{\"line\":" << v.line
              << ",\"col\":" << v.col
              << ",\"endCol\":" << v.endCol
              << ",\"defLine\":" << v.defLine
              << ",\"defCol\":" << v.defCol
              << ",\"defEndCol\":" << v.defEndCol
              << ",\"defName\":" << J(v.defName)
              << ",\"source\":" << J(v.source)
              << ",\"defSource\":" << J(v.defSource)
              << ",\"defVisibility\":" << J(v.defVisibility)
              << ",\"defOwnerType\":" << J(v.defOwnerType)
              << "}";
        }

        o << "],\"diagnostics\":[";
        for (size_t i = 0; i < r.diagnostics.size(); i++) {
            const auto& d = r.diagnostics[i];
            if (i) o << ",";
            o << "{\"line\":" << d.line
              << ",\"col\":" << d.col
              << ",\"endCol\":" << d.endCol
              << ",\"message\":" << J(d.message)
              << ",\"severity\":" << J(d.severity)
              << ",\"source\":" << J(d.source)
              << "}";
        }

        o << "],\"properties\":[";
        for (size_t i = 0; i < r.properties.size(); i++) {
            const auto& p = r.properties[i];
            if (i) o << ",";
            o << "{\"line\":" << p.line
              << ",\"col\":" << p.col
              << ",\"endCol\":" << p.endCol
              << ",\"defLine\":" << p.defLine
              << ",\"defCol\":" << p.defCol
              << ",\"defEndCol\":" << p.defEndCol
              << ",\"name\":" << J(p.name)
              << ",\"ownerType\":" << J(p.ownerType)
              << ",\"kind\":" << J(p.kind)
              << ",\"type\":" << J(p.type)
              << ",\"description\":" << J(p.description)
              << ",\"source\":" << J(p.source)
              << ",\"defSource\":" << J(p.defSource)
              << ",\"visibility\":" << J(p.visibility)
              << "}";
        }

        o << "],\"foldingRanges\":[";
        for (size_t i = 0; i < r.foldingRanges.size(); i++) {
            const auto& f = r.foldingRanges[i];
            if (i) o << ",";
            o << "{\"startLine\":" << f.startLine
              << ",\"endLine\":" << f.endLine
              << ",\"source\":" << J(f.source)
              << "}";
        }

        o << "],\"semanticTokens\":[";
        for (size_t i = 0; i < r.semanticTokens.size(); i++) {
            const auto& t = r.semanticTokens[i];
            if (i) o << ",";
            o << "{\"line\":" << t.line
              << ",\"col\":" << t.col
              << ",\"length\":" << t.length
              << ",\"tokenType\":" << J(t.tokenType)
              << ",\"source\":" << J(t.source)
              << "}";
        }

        o << "],\"paramHints\":[";
        for (size_t i = 0; i < r.paramHints.size(); i++) {
            const auto& h = r.paramHints[i];
            if (i) o << ",";
            o << "{\"line\":" << h.line
              << ",\"col\":" << h.col
              << ",\"name\":" << J(h.name)
              << ",\"source\":" << J(h.source)
              << "}";
        }

        o << "],\"chainHints\":[";
        for (size_t i = 0; i < r.chainHints.size(); i++) {
            const auto& h = r.chainHints[i];
            if (i) o << ",";
            o << "{\"line\":" << h.line
              << ",\"endCol\":" << h.endCol
              << ",\"type\":" << J(h.type)
              << ",\"source\":" << J(h.source)
              << "}";
        }

        o << "],\"signatures\":[";
        for (size_t i = 0; i < r.signatures.size(); i++) {
            const auto& s = r.signatures[i];
            if (i) o << ",";
            o << "{\"name\":" << J(s.name)
              << ",\"ownerType\":" << J(s.ownerType)
              << ",\"kind\":" << J(s.kind)
              << ",\"returnType\":" << J(s.returnType)
              << ",\"description\":" << J(s.description)
              << ",\"source\":" << J(s.source)
              << ",\"visibility\":" << J(s.visibility)
              << ",\"line\":" << s.line
              << ",\"col\":" << s.col
              << ",\"endCol\":" << s.endCol
              << ",\"params\":" << jsonParams(s.params)
              << "}";
        }

        o << "],\"classes\":[";
        for (size_t i = 0; i < r.classes.size(); i++) {
            const auto& c = r.classes[i];
            if (i) o << ",";
            o << "{\"name\":" << J(c.name)
              << ",\"superclass\":" << J(c.superclass)
              << ",\"description\":" << J(c.description)
              << ",\"source\":" << J(c.source)
              << ",\"visibility\":" << J(c.visibility)
              << ",\"line\":" << c.line
              << ",\"col\":" << c.col
              << ",\"endCol\":" << c.endCol
              << ",\"members\":[";
            for (size_t j = 0; j < c.members.size(); j++) {
                const auto& m = c.members[j];
                if (j) o << ",";
                o << "{\"name\":" << J(m.name)
                  << ",\"kind\":" << J(m.kind)
                  << ",\"type\":" << J(m.type)
                  << ",\"returnType\":" << J(m.returnType)
                  << ",\"description\":" << J(m.description)
                  << ",\"source\":" << J(m.source)
                  << ",\"visibility\":" << J(m.visibility)
                  << ",\"line\":" << m.line
                  << ",\"col\":" << m.col
                  << ",\"endCol\":" << m.endCol
                  << ",\"params\":" << jsonParams(m.params)
                  << "}";
            }
            o << "]}";
        }

        o << "]}";
        return o.str();
    }

} // namespace analyzer

#endif // ANALYZER_TYPES_H
