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

        if (auto* p = dynamic_cast<expr::Assign<MV>*>(e)) {
            return inferType(p->value.get());
        }

        if (auto* p = dynamic_cast<expr::Call<MV>*>(e)) return inferCallType(p);

        if (auto* p = dynamic_cast<expr::Get<MV>*>(e)) {
            auto ownerType = inferType(p->object.get());
            auto memberName = tokName(p->name);
            if (const auto* member = resolveMember(ownerType, memberName)) {
                if (member->kind == "field") return member->type;
                if (member->kind == "constructor") return ownerType;
                if (!member->returnType.empty()) return member->returnType;
                return member->type.empty() ? "unknown" : member->type;
            }
            return "unknown";
        }

        if (auto* p = dynamic_cast<expr::Set<MV>*>(e)) {
            return inferType(p->value.get());
        }

        if (dynamic_cast<expr::This<MV>*>(e)) {
            return currentClassName.empty() ? "unknown" : currentClassName;
        }

        if (auto* p = dynamic_cast<expr::Super<MV>*>(e)) {
            if (currentSuperclassName.empty()) return "unknown";
            if (const auto* member = resolveMember(currentSuperclassName, tokName(p->method))) {
                if (member->kind == "field") return member->type;
                if (member->kind == "constructor") return currentSuperclassName;
                if (!member->returnType.empty()) return member->returnType;
                return member->type.empty() ? "unknown" : member->type;
            }
            return currentSuperclassName;
        }

        if (auto* p = dynamic_cast<expr::ArrayExpr<MV>*>(e)) {
            return p->elements.empty() ? "[unknown]" : "[" + inferType(p->elements[0].get()) + "]";
        }

        if (auto* p = dynamic_cast<expr::MapExpr<MV>*>(e)) {
            return p->values.empty() ? "{unknown}" : "{" + inferType(p->values[0].get()) + "}";
        }

        if (auto* p = dynamic_cast<expr::PipeExpr<MV>*>(e)) return inferPipeType(p);

        if (auto* p = dynamic_cast<expr::ComposeExpr<MV>*>(e)) {
            auto leftType = inferType(p->left.get());
            auto rightType = inferType(p->right.get());
            return (leftType == "Meme -> Meme" || rightType == "Meme -> Meme") ? "Meme -> Meme" : "fun(1)";
        }

        if (auto* p = dynamic_cast<expr::LambdaExpr<MV>*>(e)) {
            return "fun(" + std::to_string(p->params.size()) + ")";
        }

        if (auto* p = dynamic_cast<expr::IndexGet<MV>*>(e)) {
            auto objType = inferType(p->object.get());
            if (objType.size() > 2 && objType.front() == '(' && objType.back() == ')') {
                if (auto* lit = dynamic_cast<expr::Literal<MV>*>(p->index.get())) {
                    auto parts = splitTuple(objType);
                    if (auto* val = std::get_if<double>(&lit->value)) {
                        if (*val >= 0 && static_cast<size_t>(*val) < parts.size()) {
                            return parts[static_cast<size_t>(*val)];
                        }
                    }
                }
            }
            if (objType.size() > 2 && objType.front() == '[' && objType.back() == ']') {
                return objType.substr(1, objType.size() - 2);
            }
            return "unknown";
        }

        if (auto* p = dynamic_cast<expr::Binary<MV>*>(e)) {
            auto op = tokName(p->operatorToken);
            if (op == "==" || op == "!=" || op == "<" || op == ">" || op == "<=" || op == ">=") return "bool";
            if (op == "+") {
                auto leftType = inferType(p->left.get());
                auto rightType = inferType(p->right.get());
                if (leftType == "string" || rightType == "string") return "string";
                if (leftType == "number" && rightType == "number") return "number";
            }
        }

        // Mac v2 syntax nodes
        if (dynamic_cast<expr::MemeLiteralExpr<MV>*>(e)) return "Meme";
        if (dynamic_cast<expr::GifBlockExpr<MV>*>(e)) return "Gif";
        if (dynamic_cast<expr::TimelineBlockExpr<MV>*>(e)) return "Timeline";
        if (dynamic_cast<expr::GridBlockExpr<MV>*>(e)) return "Meme";
        if (auto* p = dynamic_cast<expr::SaveExpr<MV>*>(e)) return "bool";

        return "unknown";
    }

    inline std::string MacAnalyzer::inferCallType(expr::Call<MV>* c) {
        std::function<const Signature*(const std::string&, const std::string&, size_t)> findSignature =
            [&](const std::string& name,
                const std::string& ownerType,
                size_t argCount) -> const Signature* {
            const Signature* fallback = nullptr;
            for (const auto& sig : result.signatures) {
                if (sig.name != name || sig.visibility == "internal") continue;
                if (sig.ownerType != ownerType) continue;
                if (sig.params.size() == argCount) return &sig;
                if (!fallback) fallback = &sig;
            }
            if (!ownerType.empty()) {
                if (const auto* cls = resolveClass(ownerType)) {
                    if (!cls->superclass.empty()) return findSignature(name, cls->superclass, argCount);
                }
            }
            return fallback;
        };

        auto inferGenericReturn = [&](const std::string& name) -> std::string {
            if (name == "zip" && c->arguments.size() >= 2) {
                return "[(" + extractElem(inferType(c->arguments[0].get()))
                    + ", " + extractElem(inferType(c->arguments[1].get())) + ")]";
            }
            if (name == "enumerate" && !c->arguments.empty()) {
                return "[(number, " + extractElem(inferType(c->arguments[0].get())) + ")]";
            }
            if ((name == "filter" || name == "sort" || name == "reverse" ||
                 name == "take" || name == "drop") && !c->arguments.empty()) {
                return inferType(c->arguments[0].get());
            }
            if (name == "map" && c->arguments.size() >= 2) {
                auto cb = inferCbReturn(c->arguments[1].get(), inferType(c->arguments[0].get()));
                if (cb != "unknown") return "[" + cb + "]";
            }
            if (name == "flatMap" && c->arguments.size() >= 2) {
                auto cb = inferCbReturn(c->arguments[1].get(), inferType(c->arguments[0].get()));
                if (cb != "unknown") return (cb.size() > 2 && cb.front() == '[' && cb.back() == ']') ? cb : "[" + cb + "]";
            }
            if (name == "reduce" && c->arguments.size() >= 3) return inferType(c->arguments[2].get());
            if (name == "find" && !c->arguments.empty()) return extractElem(inferType(c->arguments[0].get()));
            if (name == "flatten" && !c->arguments.empty()) {
                auto elem = extractElem(inferType(c->arguments[0].get()));
                return (!elem.empty() && elem.front() == '[') ? elem : "[" + elem + "]";
            }
            return "unknown";
        };

        if (auto* var = dynamic_cast<expr::Variable<MV>*>(c->callee.get())) {
            auto name = tokName(var->name);
            if (auto* def = resolve(name)) {
                if (def->kind == "class") return def->name;
                if (def->type == "Meme -> Meme") return "Meme";
            }

            if (const auto* sig = findSignature(name, "", c->arguments.size())) {
                auto generic = inferGenericReturn(name);
                if (generic != "unknown") return generic;
                if (!sig->returnType.empty()) return sig->returnType;
            }

            auto generic = inferGenericReturn(name);
            if (generic != "unknown") return generic;
        }

        if (auto* get = dynamic_cast<expr::Get<MV>*>(c->callee.get())) {
            auto ownerType = inferType(get->object.get());
            auto methodName = tokName(get->name);
            if (const auto* sig = findSignature(methodName, ownerType, c->arguments.size())) {
                if (!sig->returnType.empty()) return sig->returnType;
            }
            if (const auto* member = resolveMember(ownerType, methodName)) {
                if (member->kind == "constructor") return ownerType;
                if (!member->returnType.empty()) return member->returnType;
                if (!member->type.empty()) return member->type;
            }
        }

        if (auto* sup = dynamic_cast<expr::Super<MV>*>(c->callee.get())) {
            if (currentSuperclassName.empty()) return "unknown";
            auto methodName = tokName(sup->method);
            if (const auto* sig = findSignature(methodName, currentSuperclassName, c->arguments.size())) {
                if (!sig->returnType.empty()) return sig->returnType;
            }
            if (const auto* member = resolveMember(currentSuperclassName, methodName)) {
                if (!member->returnType.empty()) return member->returnType;
                if (!member->type.empty()) return member->type;
            }
        }

        if (auto* lambda = dynamic_cast<expr::LambdaExpr<MV>*>(c->callee.get())) {
            if (!lambda->body.empty()) {
                auto* last = lambda->body.back().get();
                if (auto* ret = dynamic_cast<stmt::ReturnStmt<MV>*>(last)) {
                    return ret->value ? inferType(ret->value.get()) : "nil";
                }
                if (auto* exprStmt = dynamic_cast<stmt::ExpressionStmt<MV>*>(last)) {
                    return inferType(exprStmt->expression.get());
                }
            }
        }

        return "unknown";
    }

    inline std::string MacAnalyzer::inferPipeType(expr::PipeExpr<MV>* p) {
        auto input = inferType(p->value.get());

        if (auto* var = dynamic_cast<expr::Variable<MV>*>(p->func.get())) {
            auto name = tokName(var->name);
            auto ret = nativeRetForPipe(name, input);
            if (ret != "unknown") return ret;

            if (auto* def = resolve(name)) {
                if (def->kind == "class") return def->name;
                if (def->type == "Meme -> Meme") return "Meme";
            }

            if (const auto* sig = resolveSignature(name)) {
                if (sig->returnType == "Meme -> Meme") return "Meme";
                if (!sig->returnType.empty()) return sig->returnType;
            }

            return input == "Meme" ? "Meme" : "unknown";
        }

        if (auto* call = dynamic_cast<expr::Call<MV>*>(p->func.get())) {
            if (auto* callee = dynamic_cast<expr::Variable<MV>*>(call->callee.get())) {
                auto name = tokName(callee->name);
                if (name == "reduce" && !call->arguments.empty()) {
                    return inferType(call->arguments.back().get());
                }
                if ((name == "map" || name == "flatMap") && !call->arguments.empty()) {
                    auto cb = inferCbReturn(call->arguments[0].get(), input);
                    if (cb != "unknown") {
                        if (name == "flatMap" && cb.size() > 2 && cb.front() == '[' && cb.back() == ']') return cb;
                        return "[" + cb + "]";
                    }
                }
                if (name == "zip" && !call->arguments.empty()) {
                    return "[(" + extractElem(input)
                        + ", " + extractElem(inferType(call->arguments[0].get())) + ")]";
                }

                auto ret = nativeRetForPipe(name, input);
                if (ret != "unknown") return ret;

                if (const auto* sig = resolveSignature(name)) {
                    if (sig->returnType == "Meme -> Meme") return "Meme";
                    if (!sig->returnType.empty()) return sig->returnType;
                }

                if (input == "Meme") return "Meme";
            }
        }

        return input == "Meme" ? "Meme" : "unknown";
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
            auto name = tokName(var->name);
            if (elem == "Meme") {
                if (auto* def = resolve(name)) {
                    if (def->type == "Meme -> Meme") return "Meme";
                }
            }
            if (const auto* sig = resolveSignature(name)) {
                if (!sig->returnType.empty() && sig->returnType != "T") return sig->returnType;
            }
        }

        return "unknown";
    }

    inline std::string MacAnalyzer::nativeRetForPipe(const std::string& name, const std::string& input) {
        static const std::unordered_set<std::string> preserves = {"filter", "sort", "reverse", "take", "drop"};
        if (preserves.count(name)) return input;
        if (name == "find" || name == "pop") return extractElem(input);
        if (name == "enumerate") return "[(number, " + extractElem(input) + ")]";
        if (name == "flatten") {
            auto elem = extractElem(input);
            return (!elem.empty() && elem.front() == '[') ? elem : "[" + elem + "]";
        }

        static const std::unordered_map<std::string, std::string> fixed = {
            {"join", "string"},
            {"any", "bool"},
            {"all", "bool"},
            {"each", "nil"},
            {"upper", "string"},
            {"lower", "string"},
            {"trim", "string"},
            {"replace", "string"},
            {"save", "bool"},
            {"animate", "Gif"},
            {"toGrid", "Meme"},
            {"pad", "Meme"},
            {"border", "Meme"},
        };

        auto it = fixed.find(name);
        return it != fixed.end() ? it->second : "unknown";
    }

    inline std::string MacAnalyzer::extractElem(const std::string& t) {
        return (t.size() > 2 && t.front() == '[' && t.back() == ']') ? t.substr(1, t.size() - 2) : t;
    }

    inline std::vector<std::string> MacAnalyzer::splitTuple(const std::string& t) {
        std::vector<std::string> parts;
        auto inner = t.substr(1, t.size() - 2);
        size_t start = 0;
        int depth = 0;
        for (size_t i = 0; i < inner.size(); i++) {
            if (inner[i] == '(' || inner[i] == '[') depth++;
            else if (inner[i] == ')' || inner[i] == ']') depth--;
            else if (inner[i] == ',' && depth == 0) {
                auto part = inner.substr(start, i - start);
                while (!part.empty() && part[0] == ' ') part = part.substr(1);
                parts.push_back(part);
                start = i + 1;
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
        std::string resultStr;
        for (size_t i = 0; i < parts.size(); i++) {
            if (i) resultStr += " >> ";
            resultStr += parts[i];
        }
        return resultStr;
    }

    inline void MacAnalyzer::collectCompose(expr::Expr<MV>* e, std::vector<std::string>& parts) {
        if (auto* c = dynamic_cast<expr::ComposeExpr<MV>*>(e)) {
            collectCompose(c->left.get(), parts);
            collectCompose(c->right.get(), parts);
        } else {
            parts.push_back(exprStr(e));
        }
    }

    inline std::string MacAnalyzer::exprStr(expr::Expr<MV>* e) {
        if (auto* v = dynamic_cast<expr::Variable<MV>*>(e)) return tokName(v->name);
        if (auto* c = dynamic_cast<expr::Call<MV>*>(e)) {
            auto callee = exprStr(c->callee.get());
            std::string args;
            for (size_t i = 0; i < c->arguments.size(); i++) {
                if (i) args += ", ";
                args += exprStr(c->arguments[i].get());
            }
            return callee + "(" + args + ")";
        }
        if (auto* l = dynamic_cast<expr::Literal<MV>*>(e)) {
            return std::visit([](auto&& v) -> std::string {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, double>) {
                    std::ostringstream out;
                    out << v;
                    return out.str();
                }
                if constexpr (std::is_same_v<T, std::string>) return "\"" + v + "\"";
                if constexpr (std::is_same_v<T, bool>) return v ? "true" : "false";
                return "nil";
            }, l->value);
        }
        if (auto* lam = dynamic_cast<expr::LambdaExpr<MV>*>(e)) {
            std::string params;
            for (size_t i = 0; i < lam->params.size(); i++) {
                if (i) params += ", ";
                params += tokName(lam->params[i]);
            }
            if (lam->params.size() > 1) params = "(" + params + ")";
            if (lam->body.size() == 1) {
                if (auto* ret = dynamic_cast<stmt::ReturnStmt<MV>*>(lam->body[0].get())) {
                    if (ret->value) return params + " -> " + exprStr(ret->value.get());
                }
            }
            return params + " -> ...";
        }
        if (auto* b = dynamic_cast<expr::Binary<MV>*>(e)) {
            return exprStr(b->left.get()) + " " + tokName(b->operatorToken) + " " + exprStr(b->right.get());
        }
        if (auto* g = dynamic_cast<expr::Grouping<MV>*>(e)) return "(" + exprStr(g->expression.get()) + ")";
        if (auto* idx = dynamic_cast<expr::IndexGet<MV>*>(e)) {
            return exprStr(idx->object.get()) + "[" + exprStr(idx->index.get()) + "]";
        }
        if (auto* get = dynamic_cast<expr::Get<MV>*>(e)) {
            return exprStr(get->object.get()) + "." + tokName(get->name);
        }
        if (auto* u = dynamic_cast<expr::Unary<MV>*>(e)) return tokName(u->operatorToken) + exprStr(u->right.get());
        if (dynamic_cast<expr::This<MV>*>(e)) return "this";
        return "...";
    }

} // namespace analyzer

#endif // ANALYZER_INFERENCE_H
