#ifndef MAC_ANALYZER_H
#define MAC_ANALYZER_H

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Expr.h"
#include "Stmt.h"
#include "Token.h"
#include "AnalyzerTypes.h"
#include "AnalyzerRegistry.h"

namespace analyzer {

    class MacAnalyzer {
    public:
        MacAnalyzer() : currentScope(&globalScope) {
            auto defFn = [this](const token::Token& t, const std::string& k,
                                const std::string& ty, const std::string& d) { define(t, k, ty, d); };
            registerNatives(nativeNames, result, defFn);
            registerPreludeTypes(nativeNames, defFn);
            registerProperties(knownProps);
        }

        AnalysisResult analyze(const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& stmts) {
            for (auto& s : stmts) analyzeStmt(s.get());
            return result;
        }

        std::string toJson() const { return analyzer::toJson(result); }

    private:
        AnalysisResult result;
        Scope globalScope;
        Scope* currentScope;
        std::unordered_set<std::string> nativeNames;
        std::unordered_map<std::string, PropInfo> knownProps;

        // --- Scope ---

        void beginScope() { auto* s = new Scope(); s->parent = currentScope; currentScope = s; }
        void endScope() { auto* old = currentScope; currentScope = old->parent; delete old; }

        static std::string tokName(const token::Token& tok) {
            if (auto* s = std::get_if<std::string>(&tok.lexeme)) return *s;
            return "";
        }

        void define(const token::Token& tok, const std::string& kind,
                    const std::string& type, const std::string& desc = "") {
            auto name = tokName(tok);
            if (name.empty()) return;
            int c = tok.column > 0 ? tok.column : 1;
            SymbolDef sym{name, kind, type, desc, tok.line, c, c + static_cast<int>(name.size())};
            currentScope->symbols[name] = sym;
            result.symbols.push_back(sym);
            if (tok.line > 0)
                result.semanticTokens.push_back({tok.line, c, static_cast<int>(name.size()), semanticKind(kind)});
        }

        SymbolDef* resolve(const std::string& name) {
            for (Scope* s = currentScope; s; s = s->parent) {
                auto it = s->symbols.find(name);
                if (it != s->symbols.end()) return &it->second;
            }
            return nullptr;
        }

        void resolveRef(const token::Token& tok) {
            auto name = tokName(tok);
            if (name.empty()) return;
            auto* def = resolve(name);
            int c = tok.column > 0 ? tok.column : 1;
            int ec = c + static_cast<int>(name.size());
            if (def) {
                result.references.push_back({tok.line, c, ec, def->line, def->col, def->name});
                if (tok.line > 0)
                    result.semanticTokens.push_back({tok.line, c, static_cast<int>(name.size()), semanticKind(def->kind)});
            } else if (!nativeNames.count(name) && name != "this" && name != "super") {
                result.diagnostics.push_back({tok.line, c, ec, "Undefined variable '" + name + "'.", "warning"});
            }
        }

        static std::string semanticKind(const std::string& kind) {
            if (kind == "parameter") return "parameter";
            if (kind == "function" || kind == "native") return "function";
            if (kind == "method") return "method";
            if (kind == "class") return "class";
            return "variable";
        }

        // --- AST walk (statements) ---

        void analyzeStmt(stmt::Stmt<MV>* s);

        // --- AST walk (expressions) ---

        void analyzeExpr(expr::Expr<MV>* e);

        // --- Folding + hints helpers ---

        void addFoldRange(int startLine, const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body);
        void collectParamHints(expr::Call<MV>* call);
        void collectChainHint(expr::Call<MV>* call);
        void addSignature(stmt::FunctionStmt<MV>* fn);
        int lastLineOf(const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body);
        int getExprCol(expr::Expr<MV>* e);
        int getExprLine(expr::Expr<MV>* e);

        // --- Type inference ---

        std::string inferType(expr::Expr<MV>* e);
        std::string inferCallType(expr::Call<MV>* c);
        std::string inferPipeType(expr::PipeExpr<MV>* p);
        std::string inferCbReturn(expr::Expr<MV>* cb, const std::string& inputType);
        std::string nativeRetForPipe(const std::string& name, const std::string& input);
        std::string extractElem(const std::string& t);
        std::vector<std::string> splitTuple(const std::string& t);
        std::string describeCompose(expr::Expr<MV>* e);
        void collectCompose(expr::Expr<MV>* e, std::vector<std::string>& parts);
        std::string exprStr(expr::Expr<MV>* e);
    };

} // namespace analyzer

// Implementation split into separate headers — included here because
// template-heavy code must be visible at the point of use.
#include "AnalyzerWalk.h"
#include "AnalyzerInference.h"

#endif // MAC_ANALYZER_H
