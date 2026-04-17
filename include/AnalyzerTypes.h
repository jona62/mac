#ifndef ANALYZER_TYPES_H
#define ANALYZER_TYPES_H

#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "MacValue.h"

namespace analyzer {

    using MV = value::MacValue;

    // ── JSON helpers ──────────────────────────────────────────────

    inline std::string J(const std::string& s) {
        std::string o = "\"";
        for (unsigned char c : s) {
            switch (c) {
                case '"':  o += "\\\""; break;
                case '\\': o += "\\\\"; break;
                case '\b': o += "\\b"; break;
                case '\f': o += "\\f"; break;
                case '\n': o += "\\n"; break;
                case '\r': o += "\\r"; break;
                case '\t': o += "\\t"; break;
                default:
                    if (c < 0x20) {
                        // JSON requires \u00XX for control characters
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        o += buf;
                    } else {
                        o += static_cast<char>(c);
                    }
            }
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

    template <typename T>
    inline std::string jsonArray(const std::vector<T>& items) {
        std::ostringstream o;
        o << "[";
        for (size_t i = 0; i < items.size(); i++) {
            if (i) o << ",";
            o << items[i].toJson();
        }
        o << "]";
        return o.str();
    }

    // ── Structs ───────────────────────────────────────────────────

    struct SymbolDef {
        std::string name, kind, type, description, source, visibility, ownerType;
        int line, col, endCol;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"name\":" << J(name) << ",\"kind\":" << J(kind)
              << ",\"type\":" << J(type) << ",\"description\":" << J(description)
              << ",\"source\":" << J(source) << ",\"visibility\":" << J(visibility)
              << ",\"ownerType\":" << J(ownerType) << ",\"line\":" << line
              << ",\"col\":" << col << ",\"endCol\":" << endCol << "}";
            return o.str();
        }
    };

    struct Reference {
        int line, col, endCol, defLine, defCol, defEndCol;
        std::string defName, source, defSource, defVisibility, defOwnerType;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"line\":" << line << ",\"col\":" << col << ",\"endCol\":" << endCol
              << ",\"defLine\":" << defLine << ",\"defCol\":" << defCol
              << ",\"defEndCol\":" << defEndCol << ",\"defName\":" << J(defName)
              << ",\"source\":" << J(source) << ",\"defSource\":" << J(defSource)
              << ",\"defVisibility\":" << J(defVisibility)
              << ",\"defOwnerType\":" << J(defOwnerType) << "}";
            return o.str();
        }
    };

    struct Diagnostic {
        int line, col, endCol;
        std::string message, severity, source;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"line\":" << line << ",\"col\":" << col << ",\"endCol\":" << endCol
              << ",\"message\":" << J(message) << ",\"severity\":" << J(severity)
              << ",\"source\":" << J(source) << "}";
            return o.str();
        }
    };

    struct PropertyRef {
        int line, col, endCol, defLine, defCol, defEndCol;
        std::string name, ownerType, kind, type, description, source, defSource, visibility;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"line\":" << line << ",\"col\":" << col << ",\"endCol\":" << endCol
              << ",\"defLine\":" << defLine << ",\"defCol\":" << defCol
              << ",\"defEndCol\":" << defEndCol << ",\"name\":" << J(name)
              << ",\"ownerType\":" << J(ownerType) << ",\"kind\":" << J(kind)
              << ",\"type\":" << J(type) << ",\"description\":" << J(description)
              << ",\"source\":" << J(source) << ",\"defSource\":" << J(defSource)
              << ",\"visibility\":" << J(visibility) << "}";
            return o.str();
        }
    };

    struct FoldRange {
        int startLine, endLine;
        std::string source;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"startLine\":" << startLine << ",\"endLine\":" << endLine
              << ",\"source\":" << J(source) << "}";
            return o.str();
        }
    };

    struct SemanticToken {
        int line, col, length;
        std::string tokenType, source;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"line\":" << line << ",\"col\":" << col << ",\"length\":" << length
              << ",\"tokenType\":" << J(tokenType) << ",\"source\":" << J(source) << "}";
            return o.str();
        }
    };

    struct ParamHint {
        int line, col;
        std::string name, source;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"line\":" << line << ",\"col\":" << col
              << ",\"name\":" << J(name) << ",\"source\":" << J(source) << "}";
            return o.str();
        }
    };

    struct ChainHint {
        int line, endCol;
        std::string type, source;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"line\":" << line << ",\"endCol\":" << endCol
              << ",\"type\":" << J(type) << ",\"source\":" << J(source) << "}";
            return o.str();
        }
    };

    struct Signature {
        std::string name, ownerType, kind, returnType, description, source, visibility;
        int line, col, endCol;
        std::vector<std::string> params;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"name\":" << J(name) << ",\"ownerType\":" << J(ownerType)
              << ",\"kind\":" << J(kind) << ",\"returnType\":" << J(returnType)
              << ",\"description\":" << J(description) << ",\"source\":" << J(source)
              << ",\"visibility\":" << J(visibility) << ",\"line\":" << line
              << ",\"col\":" << col << ",\"endCol\":" << endCol
              << ",\"params\":" << jsonParams(params) << "}";
            return o.str();
        }
    };

    struct ClassMember {
        std::string name, kind, type, returnType, description, source, visibility;
        int line, col, endCol;
        std::vector<std::string> params;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"name\":" << J(name) << ",\"kind\":" << J(kind)
              << ",\"type\":" << J(type) << ",\"returnType\":" << J(returnType)
              << ",\"description\":" << J(description) << ",\"source\":" << J(source)
              << ",\"visibility\":" << J(visibility) << ",\"line\":" << line
              << ",\"col\":" << col << ",\"endCol\":" << endCol
              << ",\"params\":" << jsonParams(params) << "}";
            return o.str();
        }
    };

    struct ClassInfo {
        std::string name, superclass, description, source, visibility;
        int line, col, endCol;
        std::vector<ClassMember> members;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"name\":" << J(name) << ",\"superclass\":" << J(superclass)
              << ",\"description\":" << J(description) << ",\"source\":" << J(source)
              << ",\"visibility\":" << J(visibility) << ",\"line\":" << line
              << ",\"col\":" << col << ",\"endCol\":" << endCol
              << ",\"members\":" << jsonArray(members) << "}";
            return o.str();
        }
    };

    struct TemplateInfo {
        std::string name, category, description;

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"name\":" << J(name) << ",\"category\":" << J(category)
              << ",\"description\":" << J(description) << "}";
            return o.str();
        }
    };

    // ── Analysis result ───────────────────────────────────────────

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
        std::vector<TemplateInfo> templates;
    };

    struct Scope {
        std::unordered_map<std::string, SymbolDef> symbols;
        Scope* parent = nullptr;
    };

    inline std::string toJson(const AnalysisResult& r) {
        std::ostringstream o;
        o << "{\"symbols\":" << jsonArray(r.symbols)
          << ",\"references\":" << jsonArray(r.references)
          << ",\"diagnostics\":" << jsonArray(r.diagnostics)
          << ",\"properties\":" << jsonArray(r.properties)
          << ",\"foldingRanges\":" << jsonArray(r.foldingRanges)
          << ",\"semanticTokens\":" << jsonArray(r.semanticTokens)
          << ",\"paramHints\":" << jsonArray(r.paramHints)
          << ",\"chainHints\":" << jsonArray(r.chainHints)
          << ",\"signatures\":" << jsonArray(r.signatures)
          << ",\"classes\":" << jsonArray(r.classes)
          << ",\"templates\":" << jsonArray(r.templates)
          << "}";
        return o.str();
    }

} // namespace analyzer

#endif // ANALYZER_TYPES_H
