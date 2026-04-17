#ifndef ANALYZER_TYPES_H
#define ANALYZER_TYPES_H

#include <string>               // string
#include <unordered_map>        // unordered_map (Scope symbol table)
#include <vector>               // vector (result arrays)
#include "MacValue.h"           // value::MacValue (MV type alias)
#include "nlohmann/json.hpp"    // nlohmann::json (JSON serialization)

namespace analyzer {

    using MV = value::MacValue;
    using json = nlohmann::json;

    // ── Structs ───────────────────────────────────────────────────

    struct SymbolDef {
        std::string name, kind, type, description, source, visibility, ownerType;
        int line, col, endCol;
    };

    inline void to_json(json& j, const SymbolDef& s) {
        j = json{
            {"name", s.name}, {"kind", s.kind}, {"type", s.type},
            {"description", s.description}, {"source", s.source},
            {"visibility", s.visibility}, {"ownerType", s.ownerType},
            {"line", s.line}, {"col", s.col}, {"endCol", s.endCol}
        };
    }

    struct Reference {
        int line, col, endCol, defLine, defCol, defEndCol;
        std::string defName, source, defSource, defVisibility, defOwnerType;
    };

    inline void to_json(json& j, const Reference& r) {
        j = json{
            {"line", r.line}, {"col", r.col}, {"endCol", r.endCol},
            {"defLine", r.defLine}, {"defCol", r.defCol}, {"defEndCol", r.defEndCol},
            {"defName", r.defName}, {"source", r.source}, {"defSource", r.defSource},
            {"defVisibility", r.defVisibility}, {"defOwnerType", r.defOwnerType}
        };
    }

    struct Diagnostic {
        int line, col, endCol;
        std::string message, severity, source;
    };

    inline void to_json(json& j, const Diagnostic& d) {
        j = json{
            {"line", d.line}, {"col", d.col}, {"endCol", d.endCol},
            {"message", d.message}, {"severity", d.severity}, {"source", d.source}
        };
    }

    struct PropertyRef {
        int line, col, endCol, defLine, defCol, defEndCol;
        std::string name, ownerType, kind, type, description, source, defSource, visibility;
    };

    inline void to_json(json& j, const PropertyRef& p) {
        j = json{
            {"line", p.line}, {"col", p.col}, {"endCol", p.endCol},
            {"defLine", p.defLine}, {"defCol", p.defCol}, {"defEndCol", p.defEndCol},
            {"name", p.name}, {"ownerType", p.ownerType}, {"kind", p.kind},
            {"type", p.type}, {"description", p.description}, {"source", p.source},
            {"defSource", p.defSource}, {"visibility", p.visibility}
        };
    }

    struct FoldRange {
        int startLine, endLine;
        std::string source;
    };

    inline void to_json(json& j, const FoldRange& f) {
        j = json{
            {"startLine", f.startLine}, {"endLine", f.endLine}, {"source", f.source}
        };
    }

    struct SemanticToken {
        int line, col, length;
        std::string tokenType, source;
    };

    inline void to_json(json& j, const SemanticToken& t) {
        j = json{
            {"line", t.line}, {"col", t.col}, {"length", t.length},
            {"tokenType", t.tokenType}, {"source", t.source}
        };
    }

    struct ParamHint {
        int line, col;
        std::string name, source;
    };

    inline void to_json(json& j, const ParamHint& p) {
        j = json{
            {"line", p.line}, {"col", p.col}, {"name", p.name}, {"source", p.source}
        };
    }

    struct ChainHint {
        int line, endCol;
        std::string type, source;
    };

    inline void to_json(json& j, const ChainHint& c) {
        j = json{
            {"line", c.line}, {"endCol", c.endCol}, {"type", c.type}, {"source", c.source}
        };
    }

    struct Signature {
        std::string name, ownerType, kind, returnType, description, source, visibility;
        int line, col, endCol;
        std::vector<std::string> params;
    };

    inline void to_json(json& j, const Signature& s) {
        j = json{
            {"name", s.name}, {"ownerType", s.ownerType}, {"kind", s.kind},
            {"returnType", s.returnType}, {"description", s.description},
            {"source", s.source}, {"visibility", s.visibility},
            {"line", s.line}, {"col", s.col}, {"endCol", s.endCol},
            {"params", s.params}
        };
    }

    struct ClassMember {
        std::string name, kind, type, returnType, description, source, visibility;
        int line, col, endCol;
        std::vector<std::string> params;
    };

    inline void to_json(json& j, const ClassMember& m) {
        j = json{
            {"name", m.name}, {"kind", m.kind}, {"type", m.type},
            {"returnType", m.returnType}, {"description", m.description},
            {"source", m.source}, {"visibility", m.visibility},
            {"line", m.line}, {"col", m.col}, {"endCol", m.endCol},
            {"params", m.params}
        };
    }

    struct ClassInfo {
        std::string name, superclass, description, source, visibility;
        int line, col, endCol;
        std::vector<ClassMember> members;
    };

    inline void to_json(json& j, const ClassInfo& c) {
        j = json{
            {"name", c.name}, {"superclass", c.superclass},
            {"description", c.description}, {"source", c.source},
            {"visibility", c.visibility}, {"line", c.line},
            {"col", c.col}, {"endCol", c.endCol}, {"members", c.members}
        };
    }

    struct TemplateInfo {
        std::string name, category, description;
    };

    inline void to_json(json& j, const TemplateInfo& t) {
        j = json{
            {"name", t.name}, {"category", t.category}, {"description", t.description}
        };
    }

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
        json j = {
            {"symbols", r.symbols},
            {"references", r.references},
            {"diagnostics", r.diagnostics},
            {"properties", r.properties},
            {"foldingRanges", r.foldingRanges},
            {"semanticTokens", r.semanticTokens},
            {"paramHints", r.paramHints},
            {"chainHints", r.chainHints},
            {"signatures", r.signatures},
            {"classes", r.classes},
            {"templates", r.templates}
        };
        return j.dump();
    }

} // namespace analyzer

#endif // ANALYZER_TYPES_H
