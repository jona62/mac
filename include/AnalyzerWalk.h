#ifndef ANALYZER_WALK_H
#define ANALYZER_WALK_H

// Included at the bottom of MacAnalyzer.h — implements AST walking methods.

namespace analyzer {

    inline void MacAnalyzer::analyzeStmt(stmt::Stmt<MV>* s) {
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
            addFoldRange(p->name.line, p->body);
            addSignature(p);
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
            if (!p->methods.empty()) {
                int endLine = p->name.line;
                for (auto& m : p->methods)
                    if (!m->body.empty()) endLine = std::max(endLine, lastLineOf(m->body));
                result.foldingRanges.push_back({p->name.line, endLine + 1});
            }
            beginScope();
            for (auto& m : p->methods) {
                define(m->name, "method", "fun(" + std::to_string(m->params.size()) + ")");
                addFoldRange(m->name.line, m->body);
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

    inline void MacAnalyzer::analyzeExpr(expr::Expr<MV>* e) {
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
            collectParamHints(p);
            collectChainHint(p);
        }
        else if (auto* p = dynamic_cast<expr::Get<MV>*>(e)) {
            analyzeExpr(p->object.get());
            auto propName = tokName(p->name);
            auto objType = inferType(p->object.get());
            auto key = objType + "." + propName;
            auto it = knownProps.find(key);
            if (it != knownProps.end()) {
                int c = p->name.column > 0 ? p->name.column : 1;
                int ec = c + static_cast<int>(propName.size());
                result.properties.push_back({p->name.line, c, ec,
                    propName, it->second.ownerType, it->second.kind, it->second.description});
            }
        }
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
        else if (auto* p = dynamic_cast<expr::PipeExpr<MV>*>(e)) {
            analyzeExpr(p->value.get());
            auto inputType = inferType(p->value.get());
            if (auto* call = dynamic_cast<expr::Call<MV>*>(p->func.get())) {
                analyzeExpr(call->callee.get());
                if (auto* callee = dynamic_cast<expr::Variable<MV>*>(call->callee.get())) {
                    auto name = tokName(callee->name);
                    for (auto& arg : call->arguments) {
                        if (auto* lambda = dynamic_cast<expr::LambdaExpr<MV>*>(arg.get())) {
                            auto elemType = extractElem(inputType);
                            beginScope();
                            if (name == "reduce" && lambda->params.size() >= 2) {
                                auto initType = call->arguments.size() >= 2
                                    ? inferType(call->arguments.back().get()) : "unknown";
                                define(lambda->params[0], "parameter", initType);
                                define(lambda->params[1], "parameter", elemType);
                            } else {
                                for (auto& prm : lambda->params) define(prm, "parameter", elemType);
                            }
                            for (auto& st : lambda->body) analyzeStmt(st.get());
                            endScope();
                        } else {
                            analyzeExpr(arg.get());
                        }
                    }
                } else {
                    for (auto& arg : call->arguments) analyzeExpr(arg.get());
                }
            } else {
                analyzeExpr(p->func.get());
            }
        }
        else if (auto* p = dynamic_cast<expr::ComposeExpr<MV>*>(e)) { analyzeExpr(p->left.get()); analyzeExpr(p->right.get()); }
    }

    // --- Helpers ---

    inline void MacAnalyzer::addFoldRange(int startLine, const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body) {
        if (!body.empty()) {
            int endLine = lastLineOf(body);
            if (endLine > startLine) result.foldingRanges.push_back({startLine, endLine});
        }
    }

    inline int MacAnalyzer::lastLineOf(const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body) {
        if (body.empty()) return 0;
        auto* last = body.back().get();
        if (auto* p = dynamic_cast<stmt::ExpressionStmt<MV>*>(last)) return getExprLine(p->expression.get());
        if (auto* p = dynamic_cast<stmt::VarStmt<MV>*>(last)) return p->name.line;
        if (auto* p = dynamic_cast<stmt::ReturnStmt<MV>*>(last)) return p->keyword.line;
        if (auto* p = dynamic_cast<stmt::FunctionStmt<MV>*>(last)) return p->name.line;
        return 0;
    }

    inline int MacAnalyzer::getExprLine(expr::Expr<MV>* e) {
        if (auto* v = dynamic_cast<expr::Variable<MV>*>(e)) return v->name.line;
        if (auto* c = dynamic_cast<expr::Call<MV>*>(e)) return c->paren.line;
        return 0;
    }

    inline int MacAnalyzer::getExprCol(expr::Expr<MV>* e) {
        if (auto* v = dynamic_cast<expr::Variable<MV>*>(e)) return v->name.column;
        if (auto* c = dynamic_cast<expr::Call<MV>*>(e)) return getExprCol(c->callee.get());
        return 0;
    }

    inline void MacAnalyzer::collectParamHints(expr::Call<MV>* call) {
        if (auto* var = dynamic_cast<expr::Variable<MV>*>(call->callee.get())) {
            auto name = tokName(var->name);
            for (auto& sig : result.signatures) {
                if (sig.name == name) {
                    for (size_t i = 0; i < sig.params.size() && i < call->arguments.size(); i++) {
                        int argCol = getExprCol(call->arguments[i].get());
                        int argLine = getExprLine(call->arguments[i].get());
                        if (argCol > 0 && argLine > 0)
                            result.paramHints.push_back({argLine, argCol, sig.params[i]});
                    }
                    break;
                }
            }
        }
    }

    inline void MacAnalyzer::collectChainHint(expr::Call<MV>* call) {
        if (auto* get = dynamic_cast<expr::Get<MV>*>(call->callee.get())) {
            auto objType = inferType(get->object.get());
            auto method = tokName(get->name);
            auto key = objType + "." + method;
            auto it = knownProps.find(key);
            if (it != knownProps.end() && it->second.kind == "method") {
                result.chainHints.push_back({call->paren.line, call->paren.column + 1, objType});
            }
        }
    }

    inline void MacAnalyzer::addSignature(stmt::FunctionStmt<MV>* fn) {
        Signature sig;
        sig.name = tokName(fn->name);
        sig.returnType = "unknown";
        for (auto& p : fn->params) sig.params.push_back(tokName(p));
        result.signatures.push_back(sig);
    }

} // namespace analyzer

#endif // ANALYZER_WALK_H
