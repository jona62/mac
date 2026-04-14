#ifndef MAC_ANALYZER_H
#define MAC_ANALYZER_H

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "Expr.h"
#include "Stmt.h"
#include "MacValue.h"
#include "Token.h"

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

    struct AnalysisResult {
        std::vector<SymbolDef> symbols;
        std::vector<Reference> references;
        std::vector<Diagnostic> diagnostics;
    };

    struct Scope {
        std::unordered_map<std::string, SymbolDef> symbols;
        Scope* parent = nullptr;
    };

    class MacAnalyzer {
    public:
        MacAnalyzer() : currentScope(&globalScope) {
            registerNatives();
            registerPreludeTypes();
        }

        AnalysisResult analyze(const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& stmts) {
            for (auto& s : stmts) analyzeStmt(s.get());
            return result;
        }

        std::string toJson() const {
            std::ostringstream o;
            o << "{\"symbols\":[";
            for (size_t i = 0; i < result.symbols.size(); i++) {
                auto& s = result.symbols[i];
                if (i) o << ",";
                o << "{\"name\":" << J(s.name) << ",\"kind\":" << J(s.kind)
                  << ",\"line\":" << s.line << ",\"col\":" << s.col
                  << ",\"endCol\":" << s.endCol << ",\"type\":" << J(s.type)
                  << ",\"description\":" << J(s.description) << "}";
            }
            o << "],\"references\":[";
            for (size_t i = 0; i < result.references.size(); i++) {
                auto& r = result.references[i];
                if (i) o << ",";
                o << "{\"line\":" << r.line << ",\"col\":" << r.col
                  << ",\"endCol\":" << r.endCol << ",\"defLine\":" << r.defLine
                  << ",\"defCol\":" << r.defCol << ",\"defName\":" << J(r.defName) << "}";
            }
            o << "],\"diagnostics\":[";
            for (size_t i = 0; i < result.diagnostics.size(); i++) {
                auto& d = result.diagnostics[i];
                if (i) o << ",";
                o << "{\"line\":" << d.line << ",\"col\":" << d.col
                  << ",\"endCol\":" << d.endCol << ",\"message\":" << J(d.message)
                  << ",\"severity\":" << J(d.severity) << "}";
            }
            o << "]}";
            return o.str();
        }

    private:
        AnalysisResult result;
        Scope globalScope;
        Scope* currentScope;
        std::unordered_set<std::string> nativeNames;

        // --- AST walking ---

        void analyzeStmt(stmt::Stmt<MV>* s) {
            if (!s) return;
            if (auto* p = dynamic_cast<stmt::ExpressionStmt<MV>*>(s)) { analyzeExpr(p->expression.get()); }
            else if (auto* p = dynamic_cast<stmt::PrintStmt<MV>*>(s)) { analyzeExpr(p->expression.get()); }
            else if (auto* p = dynamic_cast<stmt::VarStmt<MV>*>(s)) {
                std::string type = "unknown", desc;
                if (p->initializer) {
                    type = inferType(p->initializer.get());
                    if (dynamic_cast<expr::ComposeExpr<MV>*>(p->initializer.get()))
                        desc = describeCompose(p->initializer.get());
                }
                define(p->name, "variable", type, desc);
                if (p->initializer) analyzeExpr(p->initializer.get());
            }
            else if (auto* p = dynamic_cast<stmt::BlockStmt<MV>*>(s)) {
                beginScope();
                for (auto& st : p->statements) analyzeStmt(st.get());
                endScope();
            }
            else if (auto* p = dynamic_cast<stmt::IfStmt<MV>*>(s)) {
                analyzeExpr(p->condition.get());
                analyzeStmt(p->thenBranch.get());
                if (p->elseBranch) analyzeStmt(p->elseBranch.get());
            }
            else if (auto* p = dynamic_cast<stmt::WhileStmt<MV>*>(s)) {
                analyzeExpr(p->condition.get());
                analyzeStmt(p->body.get());
            }
            else if (auto* p = dynamic_cast<stmt::FunctionStmt<MV>*>(s)) {
                define(p->name, "function", "fun(" + std::to_string(p->params.size()) + ")");
                beginScope();
                for (auto& prm : p->params) define(prm, "parameter", "unknown");
                for (auto& st : p->body) analyzeStmt(st.get());
                endScope();
            }
            else if (auto* p = dynamic_cast<stmt::ReturnStmt<MV>*>(s)) {
                if (p->value) analyzeExpr(p->value.get());
            }
            else if (auto* p = dynamic_cast<stmt::ClassStmt<MV>*>(s)) {
                define(p->name, "class", "class " + tokName(p->name));
                if (p->superclass) resolveRef(p->superclass->name);
                beginScope();
                for (auto& m : p->methods) {
                    define(m->name, "method", "fun(" + std::to_string(m->params.size()) + ")");
                    beginScope();
                    for (auto& prm : m->params) define(prm, "parameter", "unknown");
                    for (auto& st : m->body) analyzeStmt(st.get());
                    endScope();
                }
                endScope();
            }
            else if (auto* p = dynamic_cast<stmt::ForInStmt<MV>*>(s)) {
                analyzeExpr(p->iterable.get());
                beginScope();
                define(p->varName, "variable", "unknown");
                analyzeStmt(p->body.get());
                endScope();
            }
        }

        void analyzeExpr(expr::Expr<MV>* e) {
            if (!e) return;
            if (auto* p = dynamic_cast<expr::Variable<MV>*>(e)) { resolveRef(p->name); }
            else if (auto* p = dynamic_cast<expr::Assign<MV>*>(e)) { analyzeExpr(p->value.get()); resolveRef(p->name); }
            else if (auto* p = dynamic_cast<expr::Binary<MV>*>(e)) { analyzeExpr(p->left.get()); analyzeExpr(p->right.get()); }
            else if (auto* p = dynamic_cast<expr::Logical<MV>*>(e)) { analyzeExpr(p->left.get()); analyzeExpr(p->right.get()); }
            else if (auto* p = dynamic_cast<expr::Unary<MV>*>(e)) { analyzeExpr(p->right.get()); }
            else if (auto* p = dynamic_cast<expr::Grouping<MV>*>(e)) { analyzeExpr(p->expression.get()); }
            else if (auto* p = dynamic_cast<expr::Call<MV>*>(e)) {
                analyzeExpr(p->callee.get());
                for (auto& a : p->arguments) analyzeExpr(a.get());
            }
            else if (auto* p = dynamic_cast<expr::Get<MV>*>(e)) { analyzeExpr(p->object.get()); }
            else if (auto* p = dynamic_cast<expr::Set<MV>*>(e)) { analyzeExpr(p->object.get()); analyzeExpr(p->value.get()); }
            else if (auto* p = dynamic_cast<expr::ArrayExpr<MV>*>(e)) { for (auto& el : p->elements) analyzeExpr(el.get()); }
            else if (auto* p = dynamic_cast<expr::MapExpr<MV>*>(e)) { for (auto& v : p->values) analyzeExpr(v.get()); }
            else if (auto* p = dynamic_cast<expr::IndexGet<MV>*>(e)) { analyzeExpr(p->object.get()); analyzeExpr(p->index.get()); }
            else if (auto* p = dynamic_cast<expr::IndexSet<MV>*>(e)) { analyzeExpr(p->object.get()); analyzeExpr(p->index.get()); analyzeExpr(p->value.get()); }
            else if (auto* p = dynamic_cast<expr::LambdaExpr<MV>*>(e)) {
                beginScope();
                for (auto& prm : p->params) define(prm, "parameter", "unknown");
                for (auto& st : p->body) analyzeStmt(st.get());
                endScope();
            }
            else if (auto* p = dynamic_cast<expr::PipeExpr<MV>*>(e)) { analyzeExpr(p->value.get()); analyzeExpr(p->func.get()); }
            else if (auto* p = dynamic_cast<expr::ComposeExpr<MV>*>(e)) { analyzeExpr(p->left.get()); analyzeExpr(p->right.get()); }
        }

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
            SymbolDef sym{name, kind, type, desc, tok.line, 1, 1 + static_cast<int>(name.size())};
            currentScope->symbols[name] = sym;
            if (tok.line > 0) result.symbols.push_back(sym);
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
            if (def) {
                int ec = 1 + static_cast<int>(name.size());
                result.references.push_back({tok.line, 1, ec, def->line, def->col, def->name});
            } else if (!nativeNames.count(name) && name != "this" && name != "super") {
                int ec = 1 + static_cast<int>(name.size());
                result.diagnostics.push_back({tok.line, 1, ec,
                    "Undefined variable '" + name + "'.", "warning"});
            }
        }

        // --- Type inference ---

        std::string inferType(expr::Expr<MV>* e) {
            if (!e) return "unknown";
            if (auto* p = dynamic_cast<expr::Literal<MV>*>(e)) {
                return std::visit([](auto&& v) -> std::string {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, double>) return "number";
                    if constexpr (std::is_same_v<T, std::string>) return "string";
                    if constexpr (std::is_same_v<T, bool>) return "bool";
                    return "nil";
                }, p->value);
            }
            if (auto* p = dynamic_cast<expr::Variable<MV>*>(e)) {
                auto* def = resolve(tokName(p->name));
                return def ? def->type : "unknown";
            }
            if (auto* p = dynamic_cast<expr::Call<MV>*>(e)) return inferCallType(p);
            if (auto* p = dynamic_cast<expr::ArrayExpr<MV>*>(e)) {
                if (!p->elements.empty()) return "[" + inferType(p->elements[0].get()) + "]";
                return "[unknown]";
            }
            if (auto* p = dynamic_cast<expr::MapExpr<MV>*>(e)) {
                if (!p->values.empty()) return "{" + inferType(p->values[0].get()) + "}";
                return "{unknown}";
            }
            if (auto* p = dynamic_cast<expr::PipeExpr<MV>*>(e)) return inferPipeType(p);
            if (auto* p = dynamic_cast<expr::ComposeExpr<MV>*>(e)) {
                auto lt = inferType(p->left.get()), rt = inferType(p->right.get());
                if (lt == "Meme -> Meme" || rt == "Meme -> Meme") return "Meme -> Meme";
                return "fun(1)";
            }
            if (auto* p = dynamic_cast<expr::LambdaExpr<MV>*>(e))
                return "fun(" + std::to_string(p->params.size()) + ")";
            if (auto* p = dynamic_cast<expr::IndexGet<MV>*>(e)) {
                auto objType = inferType(p->object.get());
                if (objType.size() > 2 && objType.front() == '(' && objType.back() == ')') {
                    if (auto* lit = dynamic_cast<expr::Literal<MV>*>(p->index.get())) {
                        auto parts = splitTuple(objType);
                        if (auto* val = std::get_if<double>(&lit->value))
                            if (*val >= 0 && (size_t)*val < parts.size()) return parts[(size_t)*val];
                    }
                }
                if (objType.size() > 2 && objType.front() == '[' && objType.back() == ']')
                    return objType.substr(1, objType.size() - 2);
                return "unknown";
            }
            if (auto* p = dynamic_cast<expr::Binary<MV>*>(e)) {
                auto op = tokName(p->operatorToken);
                if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") return "bool";
                if (op == "+") {
                    auto lt = inferType(p->left.get());
                    if (lt == "string") return "string";
                    if (lt == "number") return "number";
                }
            }
            return "unknown";
        }

        std::string inferCallType(expr::Call<MV>* c) {
            if (auto* var = dynamic_cast<expr::Variable<MV>*>(c->callee.get())) {
                auto name = tokName(var->name);
                auto* def = resolve(name);
                if (def && def->kind == "class") return def->name;
                static const std::unordered_map<std::string, std::string> ret = {
                    {"clock","number"},{"len","number"},{"sqrt","number"},{"abs","number"},
                    {"pow","number"},{"floor","number"},{"ceil","number"},
                    {"type","string"},{"substr","string"},{"input","string"},
                    {"upper","string"},{"lower","string"},{"trim","string"},
                    {"replace","string"},{"join","string"},
                    {"split","[string]"},{"range","[number]"},
                    {"any","bool"},{"all","bool"},{"save","bool"},
                    {"Timeline","Timeline"},{"animate","Gif"},{"toGrid","Meme"},
                    {"beside","Meme"},{"stack","Meme"},{"grid","Meme"},
                    {"pad","Meme"},{"border","Meme"},
                    {"invert","Meme"},{"sepia","Meme"},{"sharpen","Meme"},{"vignette","Meme"},
                };
                auto it = ret.find(name);
                if (it != ret.end()) return it->second;
                static const std::unordered_set<std::string> eff = {
                    "blur","pixelate","noise","saturate","contrast","brightness","jpeg"};
                if (eff.count(name)) return "Meme -> Meme";
                // zip(a, b) direct call
                if (name == "zip" && c->arguments.size() >= 2) {
                    auto a = extractElem(inferType(c->arguments[0].get()));
                    auto b = extractElem(inferType(c->arguments[1].get()));
                    return "[(" + a + ", " + b + ")]";
                }
                // enumerate(arr) direct call
                if (name == "enumerate" && !c->arguments.empty()) {
                    auto elem = extractElem(inferType(c->arguments[0].get()));
                    return "[(number, " + elem + ")]";
                }
                // Array-preserving: filter, sort, reverse, take, drop
                static const std::unordered_set<std::string> pres = {"filter","sort","reverse","take","drop"};
                if (pres.count(name) && !c->arguments.empty()) return inferType(c->arguments[0].get());
                // map direct call
                if (name == "map" && c->arguments.size() >= 2) {
                    auto cb = inferCbReturn(c->arguments[1].get(), inferType(c->arguments[0].get()));
                    if (cb != "unknown") return "[" + cb + "]";
                }
                if (name == "reduce" && c->arguments.size() >= 3) return inferType(c->arguments[2].get());
                if (name == "find" && !c->arguments.empty()) return extractElem(inferType(c->arguments[0].get()));
                if (name == "flatten" && !c->arguments.empty()) {
                    auto e = extractElem(inferType(c->arguments[0].get()));
                    return e.front() == '[' ? e : "[" + e + "]";
                }
            }
            if (auto* get = dynamic_cast<expr::Get<MV>*>(c->callee.get())) {
                auto obj = inferType(get->object.get());
                auto m = tokName(get->name);
                if (obj == "Meme" && (m == "text" || m == "resize")) return "Meme";
                if (obj == "Gif" && m == "frame") return "Gif";
                if (obj == "Timeline" && (m == "frame" || m == "transition" || m == "loop" || m == "render" || m == "save")) return "Timeline";
                return obj;
            }
            return "unknown";
        }

        std::string inferPipeType(expr::PipeExpr<MV>* p) {
            auto input = inferType(p->value.get());
            if (auto* var = dynamic_cast<expr::Variable<MV>*>(p->func.get())) {
                auto name = tokName(var->name);
                auto ret = nativeRetForPipe(name, input);
                if (ret != "unknown") return ret;
                auto* def = resolve(name);
                if (def && def->kind == "class") return def->name;
                if (input == "Meme") return "Meme";
                return "unknown";
            }
            if (auto* call = dynamic_cast<expr::Call<MV>*>(p->func.get())) {
                if (auto* callee = dynamic_cast<expr::Variable<MV>*>(call->callee.get())) {
                    auto name = tokName(callee->name);
                    if (name == "reduce" && call->arguments.size() >= 2)
                        return inferType(call->arguments[1].get());
                    if ((name == "map" || name == "flatMap") && !call->arguments.empty()) {
                        auto cb = inferCbReturn(call->arguments[0].get(), input);
                        if (cb != "unknown") {
                            if (name == "flatMap" && cb.size() > 2 && cb.front() == '[' && cb.back() == ']')
                                return cb;
                            return "[" + cb + "]";
                        }
                    }
                    if (name == "zip" && !call->arguments.empty()) {
                        auto a = extractElem(input), b = extractElem(inferType(call->arguments[0].get()));
                        return "[(" + a + ", " + b + ")]";
                    }
                    auto ret = nativeRetForPipe(name, input);
                    if (ret != "unknown") return ret;
                    if (input == "Meme") return "Meme";
                }
            }
            if (input == "Meme") return "Meme";
            return "unknown";
        }

        std::string inferCbReturn(expr::Expr<MV>* cb, const std::string& inputType) {
            if (auto* lambda = dynamic_cast<expr::LambdaExpr<MV>*>(cb)) {
                if (!lambda->body.empty()) {
                    auto elem = extractElem(inputType);
                    beginScope();
                    for (auto& prm : lambda->params) define(prm, "parameter", elem);
                    std::string bodyType = "unknown";
                    auto* last = lambda->body.back().get();
                    if (auto* ret = dynamic_cast<stmt::ReturnStmt<MV>*>(last))
                        bodyType = ret->value ? inferType(ret->value.get()) : "nil";
                    else if (auto* es = dynamic_cast<stmt::ExpressionStmt<MV>*>(last))
                        bodyType = inferType(es->expression.get());
                    endScope();
                    if (bodyType != "unknown") return bodyType;
                }
            }
            if (auto* var = dynamic_cast<expr::Variable<MV>*>(cb)) {
                auto elem = extractElem(inputType);
                if (elem == "Meme") return "Meme";
                auto* def = resolve(tokName(var->name));
                if (def && def->type.find("->") != std::string::npos) return elem;
            }
            return "unknown";
        }

        std::string nativeRetForPipe(const std::string& name, const std::string& input) {
            static const std::unordered_set<std::string> pres = {"filter","sort","reverse","take","drop"};
            if (pres.count(name)) return input;
            if (name == "find" || name == "pop") return extractElem(input);
            if (name == "enumerate") return "[(number, " + extractElem(input) + ")]";
            if (name == "flatten") { auto e = extractElem(input); return e.front() == '[' ? e : "[" + e + "]"; }
            static const std::unordered_map<std::string, std::string> fix = {
                {"join","string"},{"any","bool"},{"all","bool"},{"each","nil"},
                {"upper","string"},{"lower","string"},{"trim","string"},{"save","bool"},
                {"animate","Gif"},{"toGrid","Meme"},{"pad","Meme"},{"border","Meme"},
            };
            auto it = fix.find(name);
            return it != fix.end() ? it->second : "unknown";
        }

        std::string extractElem(const std::string& t) {
            if (t.size() > 2 && t.front() == '[' && t.back() == ']') return t.substr(1, t.size() - 2);
            return t;
        }

        std::vector<std::string> splitTuple(const std::string& t) {
            std::vector<std::string> parts;
            auto inner = t.substr(1, t.size() - 2);
            size_t start = 0; int depth = 0;
            for (size_t i = 0; i < inner.size(); i++) {
                if (inner[i] == '(' || inner[i] == '[') depth++;
                else if (inner[i] == ')' || inner[i] == ']') depth--;
                else if (inner[i] == ',' && depth == 0) {
                    auto p = inner.substr(start, i - start);
                    while (!p.empty() && p[0] == ' ') p = p.substr(1);
                    parts.push_back(p); start = i + 1;
                }
            }
            auto last = inner.substr(start);
            while (!last.empty() && last[0] == ' ') last = last.substr(1);
            parts.push_back(last);
            return parts;
        }

        // --- Compose description ---

        std::string describeCompose(expr::Expr<MV>* e) {
            std::vector<std::string> parts;
            collectCompose(e, parts);
            std::string r;
            for (size_t i = 0; i < parts.size(); i++) { if (i) r += " >> "; r += parts[i]; }
            return r;
        }

        void collectCompose(expr::Expr<MV>* e, std::vector<std::string>& parts) {
            if (auto* c = dynamic_cast<expr::ComposeExpr<MV>*>(e)) {
                collectCompose(c->left.get(), parts);
                collectCompose(c->right.get(), parts);
            } else { parts.push_back(exprStr(e)); }
        }

        std::string exprStr(expr::Expr<MV>* e) {
            if (auto* v = dynamic_cast<expr::Variable<MV>*>(e)) return tokName(v->name);
            if (auto* c = dynamic_cast<expr::Call<MV>*>(e)) {
                auto callee = exprStr(c->callee.get());
                std::string args;
                for (size_t i = 0; i < c->arguments.size(); i++) {
                    if (i) args += ", "; args += exprStr(c->arguments[i].get());
                }
                return callee + "(" + args + ")";
            }
            if (auto* l = dynamic_cast<expr::Literal<MV>*>(e)) {
                return std::visit([](auto&& v) -> std::string {
                    using T = std::decay_t<decltype(v)>;
                    if constexpr (std::is_same_v<T, double>) { std::ostringstream o; o << v; return o.str(); }
                    if constexpr (std::is_same_v<T, std::string>) return "\"" + v + "\"";
                    if constexpr (std::is_same_v<T, bool>) return v ? "true" : "false";
                    return "nil";
                }, l->value);
            }
            if (auto* lam = dynamic_cast<expr::LambdaExpr<MV>*>(e)) {
                std::string prms;
                for (size_t i = 0; i < lam->params.size(); i++) {
                    if (i) prms += ", "; prms += tokName(lam->params[i]);
                }
                if (lam->params.size() > 1) prms = "(" + prms + ")";
                if (lam->body.size() == 1)
                    if (auto* r = dynamic_cast<stmt::ReturnStmt<MV>*>(lam->body[0].get()))
                        if (r->value) return prms + " -> " + exprStr(r->value.get());
                return prms + " -> ...";
            }
            if (auto* b = dynamic_cast<expr::Binary<MV>*>(e))
                return exprStr(b->left.get()) + " " + tokName(b->operatorToken) + " " + exprStr(b->right.get());
            if (auto* g = dynamic_cast<expr::Grouping<MV>*>(e))
                return "(" + exprStr(g->expression.get()) + ")";
            if (auto* idx = dynamic_cast<expr::IndexGet<MV>*>(e))
                return exprStr(idx->object.get()) + "[" + exprStr(idx->index.get()) + "]";
            if (auto* u = dynamic_cast<expr::Unary<MV>*>(e))
                return tokName(u->operatorToken) + exprStr(u->right.get());
            return "...";
        }

        // --- JSON ---

        static std::string J(const std::string& s) {
            std::string o = "\"";
            for (char c : s) {
                if (c == '"') o += "\\\""; else if (c == '\\') o += "\\\\";
                else if (c == '\n') o += "\\n"; else o += c;
            }
            return o + "\"";
        }

        // --- Native registration ---

        void registerNatives() {
            auto reg = [&](const std::string& name, const std::string& desc, const std::string& type = "fun(1)") {
                nativeNames.insert(name);
                token::Token tok(token::TokenType::IDENTIFIER, token::TokenValue(name), 0);
                define(tok, "native", type, desc);
            };
            reg("clock","Current time."); reg("len","Length of string/array.");
            reg("substr","Substring."); reg("split","Split string."); reg("type","Type name.");
            reg("sqrt","Square root."); reg("abs","Absolute value.");
            reg("pow","Exponentiation."); reg("floor","Round down."); reg("ceil","Round up.");
            reg("push","Append to array."); reg("pop","Remove last.");
            reg("map","Map function over array."); reg("filter","Filter by predicate.");
            reg("input","Read input line.");
            reg("range","Generate number array."); reg("reduce","Fold with accumulator.");
            reg("zip","Pair two arrays."); reg("enumerate","Pair with indices.");
            reg("each","Side effects."); reg("flatten","Flatten one level.");
            reg("flatMap","Map then flatten."); reg("sort","Sort."); reg("reverse","Reverse.");
            reg("find","First match."); reg("any","Any match?"); reg("all","All match?");
            reg("take","First n."); reg("drop","Skip first n."); reg("join","Join to string.");
            reg("upper","Uppercase."); reg("lower","Lowercase.");
            reg("trim","Strip whitespace."); reg("replace","Replace all.");
            reg("animate","Memes to GIF."); reg("toGrid","Memes to grid.");
            reg("save","Save to file.");
            reg("blur","Blur. Meme -> Meme.","Meme -> Meme");
            reg("pixelate","Pixelate. Meme -> Meme.","Meme -> Meme");
            reg("noise","Noise. Meme -> Meme.","Meme -> Meme");
            reg("saturate","Saturate. Meme -> Meme.","Meme -> Meme");
            reg("contrast","Contrast. Meme -> Meme.","Meme -> Meme");
            reg("brightness","Brightness. Meme -> Meme.","Meme -> Meme");
            reg("jpeg","JPEG artifacts. Meme -> Meme.","Meme -> Meme");
            reg("invert","Invert. Meme -> Meme.","Meme -> Meme");
            reg("sepia","Sepia. Meme -> Meme.","Meme -> Meme");
            reg("sharpen","Sharpen. Meme -> Meme.","Meme -> Meme");
            reg("vignette","Vignette. Meme -> Meme.","Meme -> Meme");
            reg("beside","Side-by-side."); reg("stack","Vertical stack.");
            reg("grid","Grid layout."); reg("pad","Padding."); reg("border","Border.");
            reg("timeline","Internal.");
            for (auto& n : {"_resolve_template","_meme_save","_gif_save","_save_rendered",
                            "_apply_effect","_compose_layout","_add_padding","_add_border",
                            "_timeline_keyframe","_timeline_transition","_timeline_hold",
                            "_timeline_loop","_timeline_render"})
                reg(n, "Internal.");
        }

        void registerPreludeTypes() {
            auto reg = [&](const std::string& name, const std::string& kind,
                           const std::string& type, const std::string& desc) {
                nativeNames.insert(name);
                token::Token tok(token::TokenType::IDENTIFIER, token::TokenValue(name), 0);
                define(tok, kind, type, desc);
            };
            reg("Size","class","class Size","Pixel dimensions.");
            reg("Duration","class","class Duration","Time in milliseconds.");
            reg("Position","class","class Position","Text position.");
            reg("Format","class","class Format","Output format.");
            reg("Template","class","class Template","Meme template image.");
            reg("Meme","class","class Meme","Meme builder.");
            reg("Frame","class","class Frame","Animation frame.");
            reg("Gif","class","class Gif","GIF builder.");
            reg("Timeline","class","class Timeline","Animation timeline.");
            reg("Top","variable","Position","Top position.");
            reg("Bottom","variable","Position","Bottom position.");
            reg("Center","variable","Position","Center position.");
            reg("PNG","variable","Format","PNG format.");
            reg("JPG","variable","Format","JPG format.");
            reg("GIF","variable","Format","GIF format.");
            reg("deepfry","variable","Meme -> Meme","Composed effect preset.");
            reg("crossfade","variable","string","Transition type.");
            reg("slideLeft","variable","string","Transition type.");
            reg("slideRight","variable","string","Transition type.");
            reg("slideUp","variable","string","Transition type.");
            reg("slideDown","variable","string","Transition type.");
            reg("wipe","variable","string","Transition type.");
        }
    };

} // namespace analyzer

#endif // MAC_ANALYZER_H
