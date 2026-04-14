#ifndef ANALYZER_INFERENCE_H
#define ANALYZER_INFERENCE_H

// Included at the bottom of MacAnalyzer.h — implements type inference methods.

namespace analyzer {

    inline std::string MacAnalyzer::inferType(expr::Expr<MV>* e) {
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
            return p->elements.empty() ? "[unknown]" : "[" + inferType(p->elements[0].get()) + "]";
        }
        if (auto* p = dynamic_cast<expr::MapExpr<MV>*>(e)) {
            return p->values.empty() ? "{unknown}" : "{" + inferType(p->values[0].get()) + "}";
        }
        if (auto* p = dynamic_cast<expr::PipeExpr<MV>*>(e)) return inferPipeType(p);
        if (auto* p = dynamic_cast<expr::ComposeExpr<MV>*>(e)) {
            auto lt = inferType(p->left.get()), rt = inferType(p->right.get());
            return (lt == "Meme -> Meme" || rt == "Meme -> Meme") ? "Meme -> Meme" : "fun(1)";
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
            if (op == "+") { auto lt = inferType(p->left.get()); if (lt == "string" || lt == "number") return lt; }
        }
        return "unknown";
    }

    inline std::string MacAnalyzer::inferCallType(expr::Call<MV>* c) {
        if (auto* var = dynamic_cast<expr::Variable<MV>*>(c->callee.get())) {
            auto name = tokName(var->name);
            auto* def = resolve(name);
            if (def && def->kind == "class") return def->name;
            static const std::unordered_map<std::string, std::string> ret = {
                {"clock","number"},{"len","number"},{"sqrt","number"},{"abs","number"},
                {"pow","number"},{"floor","number"},{"ceil","number"},
                {"type","string"},{"substr","string"},{"input","string"},
                {"upper","string"},{"lower","string"},{"trim","string"},{"replace","string"},{"join","string"},
                {"split","[string]"},{"range","[number]"},{"any","bool"},{"all","bool"},{"save","bool"},
                {"Timeline","Timeline"},{"animate","Gif"},{"toGrid","Meme"},
                {"beside","Meme"},{"stack","Meme"},{"grid","Meme"},{"pad","Meme"},{"border","Meme"},
                {"invert","Meme"},{"sepia","Meme"},{"sharpen","Meme"},{"vignette","Meme"},
            };
            auto it = ret.find(name); if (it != ret.end()) return it->second;
            static const std::unordered_set<std::string> eff = {"blur","pixelate","noise","saturate","contrast","brightness","jpeg"};
            if (eff.count(name)) return "Meme -> Meme";
            if (name == "zip" && c->arguments.size() >= 2)
                return "[(" + extractElem(inferType(c->arguments[0].get())) + ", " + extractElem(inferType(c->arguments[1].get())) + ")]";
            if (name == "enumerate" && !c->arguments.empty())
                return "[(number, " + extractElem(inferType(c->arguments[0].get())) + ")]";
            static const std::unordered_set<std::string> pres = {"filter","sort","reverse","take","drop"};
            if (pres.count(name) && !c->arguments.empty()) return inferType(c->arguments[0].get());
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
            auto obj = inferType(get->object.get()), m = tokName(get->name);
            if (obj == "Meme" && (m == "text" || m == "resize")) return "Meme";
            if (obj == "Gif" && m == "frame") return "Gif";
            if (obj == "Timeline" && (m == "frame" || m == "transition" || m == "loop" || m == "render" || m == "save")) return "Timeline";
            return obj;
        }
        return "unknown";
    }

    inline std::string MacAnalyzer::inferPipeType(expr::PipeExpr<MV>* p) {
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
                if (name == "reduce" && call->arguments.size() >= 2) return inferType(call->arguments[1].get());
                if ((name == "map" || name == "flatMap") && !call->arguments.empty()) {
                    auto cb = inferCbReturn(call->arguments[0].get(), input);
                    if (cb != "unknown") {
                        if (name == "flatMap" && cb.size() > 2 && cb.front() == '[' && cb.back() == ']') return cb;
                        return "[" + cb + "]";
                    }
                }
                if (name == "zip" && !call->arguments.empty())
                    return "[(" + extractElem(input) + ", " + extractElem(inferType(call->arguments[0].get())) + ")]";
                auto ret = nativeRetForPipe(name, input);
                if (ret != "unknown") return ret;
                if (input == "Meme") return "Meme";
            }
        }
        if (input == "Meme") return "Meme";
        return "unknown";
    }

    inline std::string MacAnalyzer::inferCbReturn(expr::Expr<MV>* cb, const std::string& inputType) {
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

    inline std::string MacAnalyzer::nativeRetForPipe(const std::string& name, const std::string& input) {
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

    inline std::string MacAnalyzer::extractElem(const std::string& t) {
        return (t.size() > 2 && t.front() == '[' && t.back() == ']') ? t.substr(1, t.size() - 2) : t;
    }

    inline std::vector<std::string> MacAnalyzer::splitTuple(const std::string& t) {
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

    // --- Compose description + expression stringification ---

    inline std::string MacAnalyzer::describeCompose(expr::Expr<MV>* e) {
        std::vector<std::string> parts;
        collectCompose(e, parts);
        std::string r;
        for (size_t i = 0; i < parts.size(); i++) { if (i) r += " >> "; r += parts[i]; }
        return r;
    }

    inline void MacAnalyzer::collectCompose(expr::Expr<MV>* e, std::vector<std::string>& parts) {
        if (auto* c = dynamic_cast<expr::ComposeExpr<MV>*>(e)) {
            collectCompose(c->left.get(), parts);
            collectCompose(c->right.get(), parts);
        } else { parts.push_back(exprStr(e)); }
    }

    inline std::string MacAnalyzer::exprStr(expr::Expr<MV>* e) {
        if (auto* v = dynamic_cast<expr::Variable<MV>*>(e)) return tokName(v->name);
        if (auto* c = dynamic_cast<expr::Call<MV>*>(e)) {
            auto callee = exprStr(c->callee.get());
            std::string args;
            for (size_t i = 0; i < c->arguments.size(); i++) { if (i) args += ", "; args += exprStr(c->arguments[i].get()); }
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
            for (size_t i = 0; i < lam->params.size(); i++) { if (i) prms += ", "; prms += tokName(lam->params[i]); }
            if (lam->params.size() > 1) prms = "(" + prms + ")";
            if (lam->body.size() == 1)
                if (auto* r = dynamic_cast<stmt::ReturnStmt<MV>*>(lam->body[0].get()))
                    if (r->value) return prms + " -> " + exprStr(r->value.get());
            return prms + " -> ...";
        }
        if (auto* b = dynamic_cast<expr::Binary<MV>*>(e))
            return exprStr(b->left.get()) + " " + tokName(b->operatorToken) + " " + exprStr(b->right.get());
        if (auto* g = dynamic_cast<expr::Grouping<MV>*>(e)) return "(" + exprStr(g->expression.get()) + ")";
        if (auto* idx = dynamic_cast<expr::IndexGet<MV>*>(e))
            return exprStr(idx->object.get()) + "[" + exprStr(idx->index.get()) + "]";
        if (auto* u = dynamic_cast<expr::Unary<MV>*>(e)) return tokName(u->operatorToken) + exprStr(u->right.get());
        return "...";
    }

} // namespace analyzer

#endif // ANALYZER_INFERENCE_H
