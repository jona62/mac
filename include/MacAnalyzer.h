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
            auto defFn = [this](const token::Token& t, const std::string& k, const std::string& ty,
                                const std::string& d, const std::string& src,
                                const std::string& vis, const std::string& owner) {
                define(t, k, ty, d, src, vis, owner);
            };
            auto sigFn = [this](const Signature& sig) {
                result.signatures.push_back(sig);
            };
            registerNatives(nativeNames, defFn, sigFn);
        }

        AnalysisResult analyze(const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& stmts,
                               const std::string& source = "user") {
            auto prevSource = currentSource;
            currentSource = source;
            for (auto& s : stmts) analyzeStmt(s.get());
            currentSource = prevSource;
            return result;
        }

        void addTemplate(const std::string& name, const std::string& category,
                         const std::string& description) {
            result.templates.push_back({name, category, description});
        }

        std::string toJson() const { return analyzer::toJson(result); }

    private:
        AnalysisResult result;
        Scope globalScope;
        Scope* currentScope;
        std::unordered_set<std::string> nativeNames;
        std::unordered_map<std::string, size_t> classIndices;
        std::string currentSource = "user";
        std::string currentClassName;
        std::string currentSuperclassName;

        void beginScope() {
            auto* s = new Scope();
            s->parent = currentScope;
            currentScope = s;
        }

        void endScope() {
            auto* old = currentScope;
            currentScope = old->parent;
            delete old;
        }

        static std::string tokName(const token::Token& tok) {
            if (auto* s = std::get_if<std::string>(&tok.lexeme)) return *s;
            return "";
        }

        static bool isInternalName(const std::string& name) {
            return !name.empty() && name[0] == '_';
        }

        static std::string defaultVisibility(const std::string& name, const std::string& kind,
                                             const std::string& ownerType) {
            if (kind == "native") return "public";
            if (!ownerType.empty() && isInternalName(name)) return "internal";
            return "public";
        }

        static std::string semanticKind(const std::string& kind) {
            if (kind == "parameter") return "parameter";
            if (kind == "function" || kind == "native") return "function";
            if (kind == "method") return "method";
            if (kind == "class") return "class";
            if (kind == "field") return "property";
            return "variable";
        }

        void define(const token::Token& tok, const std::string& kind,
                    const std::string& type, const std::string& desc = "",
                    const std::string& source = "", const std::string& visibility = "",
                    const std::string& ownerType = "") {
            auto name = tokName(tok);
            if (name.empty()) return;

            const auto resolvedSource = source.empty() ? currentSource : source;
            const auto resolvedVisibility = visibility.empty()
                ? defaultVisibility(name, kind, ownerType)
                : visibility;

            int c = tok.column > 0 ? tok.column : 1;
            SymbolDef sym{
                name,
                kind,
                type,
                desc,
                resolvedSource,
                resolvedVisibility,
                ownerType,
                tok.line,
                c,
                c + static_cast<int>(name.size()),
            };

            currentScope->symbols[name] = sym;
            result.symbols.push_back(sym);

            if (tok.line > 0 && resolvedSource == "user") {
                result.semanticTokens.push_back({
                    tok.line,
                    c,
                    static_cast<int>(name.size()),
                    semanticKind(kind),
                    resolvedSource,
                });
            }
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

            int c = tok.column > 0 ? tok.column : 1;
            int ec = c + static_cast<int>(name.size());
            auto* def = resolve(name);

            if (def) {
                result.references.push_back({
                    tok.line,
                    c,
                    ec,
                    def->line,
                    def->col,
                    def->endCol,
                    def->name,
                    currentSource,
                    def->source,
                    def->visibility,
                    def->ownerType,
                });

                if (tok.line > 0 && currentSource == "user") {
                    result.semanticTokens.push_back({
                        tok.line,
                        c,
                        static_cast<int>(name.size()),
                        semanticKind(def->kind),
                        currentSource,
                    });
                }
            } else if (!nativeNames.count(name) && name != "this" && name != "super") {
                result.diagnostics.push_back({
                    tok.line,
                    c,
                    ec,
                    "Undefined variable '" + name + "'.",
                    "warning",
                    currentSource,
                });
            }
        }

        void addDiagnostic(const token::Token& tok, const std::string& message,
                           const std::string& severity = "warning") {
            auto name = tokName(tok);
            int c = tok.column > 0 ? tok.column : 1;
            int ec = c + static_cast<int>(std::max<size_t>(1, name.size()));
            result.diagnostics.push_back({tok.line, c, ec, message, severity, currentSource});
        }

        ClassInfo* ensureClass(const token::Token& nameTok, const std::string& description = "",
                               const std::string& source = "", const std::string& visibility = "public") {
            auto name = tokName(nameTok);
            if (name.empty()) return nullptr;

            auto it = classIndices.find(name);
            if (it == classIndices.end()) {
                int c = nameTok.column > 0 ? nameTok.column : 1;
                result.classes.push_back({
                    name,
                    "",
                    description,
                    source.empty() ? currentSource : source,
                    visibility,
                    nameTok.line,
                    c,
                    c + static_cast<int>(name.size()),
                    {},
                });
                classIndices[name] = result.classes.size() - 1;
            } else {
                auto& cls = result.classes[it->second];
                if (!description.empty()) cls.description = description;
                if (!source.empty()) cls.source = source;
                cls.visibility = visibility;
                cls.line = nameTok.line;
                cls.col = nameTok.column > 0 ? nameTok.column : 1;
                cls.endCol = cls.col + static_cast<int>(name.size());
            }
            return &result.classes[classIndices[name]];
        }

        ClassInfo* resolveClass(const std::string& name) {
            auto it = classIndices.find(name);
            if (it == classIndices.end()) return nullptr;
            return &result.classes[it->second];
        }

        const ClassInfo* resolveClass(const std::string& name) const {
            auto it = classIndices.find(name);
            if (it == classIndices.end()) return nullptr;
            return &result.classes[it->second];
        }

        ClassMember* upsertMember(const std::string& className, const token::Token& memberTok,
                                  const std::string& kind, const std::string& type,
                                  const std::string& returnType, const std::vector<std::string>& params,
                                  const std::string& description = "",
                                  const std::string& visibility = "") {
            auto* cls = resolveClass(className);
            if (!cls) return nullptr;

            auto memberName = tokName(memberTok);
            if (memberName.empty()) return nullptr;

            auto resolvedVisibility = visibility.empty()
                ? (isInternalName(memberName) ? "internal" : "public")
                : visibility;
            int c = memberTok.column > 0 ? memberTok.column : 1;

            for (auto& member : cls->members) {
                if (member.name != memberName || member.kind != kind) continue;
                if (!type.empty() && member.type != type) {
                    if (member.type.empty() || member.type == "unknown") member.type = type;
                    else if (type != "unknown" && member.type != type) member.type = "unknown";
                }
                if (!returnType.empty()) member.returnType = returnType;
                if (!description.empty()) member.description = description;
                if (!params.empty()) member.params = params;
                member.source = currentSource;
                member.visibility = resolvedVisibility;
                if (member.line <= 0 && memberTok.line > 0) {
                    member.line = memberTok.line;
                    member.col = c;
                    member.endCol = c + static_cast<int>(memberName.size());
                }
                return &member;
            }

            cls->members.push_back({
                memberName,
                kind,
                type,
                returnType,
                description,
                currentSource,
                resolvedVisibility,
                memberTok.line,
                c,
                c + static_cast<int>(memberName.size()),
                params,
            });
            return &cls->members.back();
        }

        ClassMember* resolveMember(const std::string& ownerType, const std::string& name) {
            auto* cls = resolveClass(ownerType);
            if (!cls) return nullptr;
            for (auto& member : cls->members) {
                if (member.name == name) return &member;
            }
            if (!cls->superclass.empty()) return resolveMember(cls->superclass, name);
            return nullptr;
        }

        const ClassMember* resolveMember(const std::string& ownerType, const std::string& name) const {
            auto* cls = resolveClass(ownerType);
            if (!cls) return nullptr;
            for (const auto& member : cls->members) {
                if (member.name == name) return &member;
            }
            if (!cls->superclass.empty()) return resolveMember(cls->superclass, name);
            return nullptr;
        }

        std::vector<std::string> constructorParamsFor(const std::string& className) const {
            auto* cls = resolveClass(className);
            if (!cls) return {};
            for (const auto& member : cls->members) {
                if (member.kind == "constructor") return member.params;
            }
            if (!cls->superclass.empty()) return constructorParamsFor(cls->superclass);
            return {};
        }

        std::string constructorDescriptionFor(const std::string& className) const {
            auto* cls = resolveClass(className);
            if (!cls) return "";
            for (const auto& member : cls->members) {
                if (member.kind == "constructor") return member.description;
            }
            if (!cls->superclass.empty()) return constructorDescriptionFor(cls->superclass);
            return "";
        }

        void addSignature(const token::Token& tok, const std::string& name, const std::string& kind,
                          const std::vector<std::string>& params, const std::string& returnType,
                          const std::string& description = "", const std::string& source = "",
                          const std::string& visibility = "public",
                          const std::string& ownerType = "") {
            int c = tok.column > 0 ? tok.column : 1;
            result.signatures.push_back({
                name,
                ownerType,
                kind,
                returnType,
                description,
                source.empty() ? currentSource : source,
                visibility,
                tok.line,
                c,
                c + static_cast<int>(name.size()),
                params,
            });
        }

        const Signature* resolveSignature(const std::string& name,
                                          const std::string& ownerType = "") const {
            for (const auto& sig : result.signatures) {
                if (sig.name != name) continue;
                if (sig.ownerType == ownerType) return &sig;
            }
            if (!ownerType.empty()) {
                auto* cls = resolveClass(ownerType);
                if (cls && !cls->superclass.empty()) return resolveSignature(name, cls->superclass);
            }
            for (const auto& sig : result.signatures) {
                if (sig.name == name && sig.ownerType.empty()) return &sig;
            }
            return nullptr;
        }

        std::string inferBlockReturn(const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body);
        void collectReturnTypes(stmt::Stmt<MV>* s, std::vector<std::string>& out);

        void analyzeStmt(stmt::Stmt<MV>* s);
        void analyzeExpr(expr::Expr<MV>* e);

        void addFoldRange(int startLine, const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body);
        void collectParamHints(expr::Call<MV>* call);
        void collectChainHint(expr::Call<MV>* call);
        void addFunctionSignature(stmt::FunctionStmt<MV>* fn, const std::string& kind,
                                  const std::string& returnType,
                                  const std::string& description = "",
                                  const std::string& ownerType = "",
                                  const std::string& visibility = "public");
        int lastLineOf(const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body);
        int getExprCol(expr::Expr<MV>* e);
        int getExprLine(expr::Expr<MV>* e);

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

#include "AnalyzerWalk.h"
#include "AnalyzerInference.h"

#endif // MAC_ANALYZER_H
