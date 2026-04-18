#ifndef ANALYZER_WALK_H
#define ANALYZER_WALK_H

namespace analyzer {

    inline void MacAnalyzer::analyzeStmt(stmt::Stmt<MV>* s) {
        if (!s) return;

        if (auto* p = dynamic_cast<stmt::ExpressionStmt<MV>*>(s)) {
            analyzeExpr(p->expression.get());
        } else if (auto* p = dynamic_cast<stmt::PrintStmt<MV>*>(s)) {
            analyzeExpr(p->expression.get());
        } else if (auto* p = dynamic_cast<stmt::VarStmt<MV>*>(s)) {
            std::string type = "unknown";
            std::string desc;
            if (p->initializer) {
                type = inferType(p->initializer.get());
                if (dynamic_cast<expr::ComposeExpr<MV>*>(p->initializer.get())) {
                    desc = describeCompose(p->initializer.get());
                }
            }
            define(p->name, "variable", type, desc);
            if (p->initializer) analyzeExpr(p->initializer.get());
        } else if (auto* p = dynamic_cast<stmt::BlockStmt<MV>*>(s)) {
            beginScope();
            for (auto& st : p->statements) analyzeStmt(st.get());
            if (p->tailExpr) analyzeExpr(p->tailExpr.get());
            endScope();
        } else if (auto* p = dynamic_cast<stmt::IfStmt<MV>*>(s)) {
            analyzeExpr(p->condition.get());
            analyzeStmt(p->thenBranch.get());
            if (p->elseBranch) analyzeStmt(p->elseBranch.get());
        } else if (auto* p = dynamic_cast<stmt::WhileStmt<MV>*>(s)) {
            analyzeExpr(p->condition.get());
            analyzeStmt(p->body.get());
        } else if (auto* p = dynamic_cast<stmt::FunctionStmt<MV>*>(s)) {
            define(p->name, "function", "fun(" + std::to_string(p->params.size()) + ")");
            addFoldRange(p->name.line, p->body);

            beginScope();
            for (auto& prm : p->params) define(prm, "parameter", "unknown");
            for (auto& st : p->body) analyzeStmt(st.get());
            if (p->tailExpr) analyzeExpr(p->tailExpr.get());
            auto returnType = p->tailExpr ? inferType(p->tailExpr.get()) : inferBlockReturn(p->body);
            addFunctionSignature(p, "function", returnType);
            endScope();
        } else if (auto* p = dynamic_cast<stmt::ReturnStmt<MV>*>(s)) {
            if (p->value) analyzeExpr(p->value.get());
        } else if (auto* p = dynamic_cast<stmt::ClassStmt<MV>*>(s)) {
            auto className = tokName(p->name);
            define(p->name, "class", "class " + className);
            auto* cls = ensureClass(p->name);
            if (p->superclass) {
                auto superName = tokName(p->superclass->name);
                cls->superclass = superName;
                resolveRef(p->superclass->name);
            }

            if (!p->methods.empty()) {
                int endLine = p->name.line;
                for (auto& m : p->methods) {
                    if (!m->body.empty()) endLine = std::max(endLine, lastLineOf(m->body));
                }
                result.foldingRanges.push_back({p->name.line, endLine + 1, currentSource});
            }

            auto prevClass = currentClassName;
            auto prevSuper = currentSuperclassName;
            currentClassName = className;
            currentSuperclassName = cls->superclass;

            beginScope();
            for (auto& m : p->methods) {
                auto methodName = tokName(m->name);
                auto visibility = isInternalName(methodName) ? "internal" : "public";
                define(m->name, "method", "fun(" + std::to_string(m->params.size()) + ")",
                       "", "", visibility, className);
                addFoldRange(m->name.line, m->body);

                beginScope();
                for (auto& prm : m->params) define(prm, "parameter", "unknown");
                for (auto& st : m->body) analyzeStmt(st.get());
                if (m->tailExpr) analyzeExpr(m->tailExpr.get());

                std::vector<std::string> params;
                params.reserve(m->params.size());
                for (auto& prm : m->params) params.push_back(tokName(prm));
                auto returnType = methodName == "init" ? className
                    : (m->tailExpr ? inferType(m->tailExpr.get()) : inferBlockReturn(m->body));
                upsertMember(className, m->name, methodName == "init" ? "constructor" : "method",
                             "fun(" + std::to_string(m->params.size()) + ")", returnType, params,
                             "", visibility);
                addFunctionSignature(m.get(), methodName == "init" ? "constructor" : "method",
                                     returnType, "", className, visibility);
                endScope();
            }

            if (!resolveMember(className, "init")) {
                addSignature(p->name, className, "constructor", constructorParamsFor(className),
                             className, constructorDescriptionFor(className), currentSource, "public", className);
            }

            endScope();
            currentClassName = prevClass;
            currentSuperclassName = prevSuper;
        } else if (auto* p = dynamic_cast<stmt::ForInStmt<MV>*>(s)) {
            analyzeExpr(p->iterable.get());
            auto iterType = inferType(p->iterable.get());
            auto elemType = extractElem(iterType);
            beginScope();
            define(p->varName, "variable", elemType);
            analyzeStmt(p->body.get());
            endScope();
        } else if (auto* p = dynamic_cast<stmt::EnumStmt<MV>*>(s)) {
            if (p->keyword.line > 0 && currentSource == "user") {
                int kc = safeCol(p->keyword);
                result.semanticTokens.push_back({p->keyword.line, kc, 4, "keyword", currentSource});
            }
            define(p->name, "variable", "enum " + tokName(p->name));
        } else if (auto* p = dynamic_cast<stmt::EffectStmt<MV>*>(s)) {
            // Semantic token for 'effect' keyword
            if (p->keyword.line > 0 && currentSource == "user") {
                int kc = safeCol(p->keyword);
                result.semanticTokens.push_back({p->keyword.line, kc, 6, "keyword", currentSource});
            }
            std::string type = "Meme -> Meme";
            std::string desc;
            if (p->value) {
                type = inferType(p->value.get());
                if (dynamic_cast<expr::ComposeExpr<MV>*>(p->value.get()))
                    desc = describeCompose(p->value.get());
            }
            define(p->name, "variable", type, desc);
            if (p->value) analyzeExpr(p->value.get());
        } else if (auto* p = dynamic_cast<stmt::StyleStmt<MV>*>(s)) {
            // Semantic token for 'style' keyword
            if (p->keyword.line > 0 && currentSource == "user") {
                int kc = safeCol(p->keyword);
                result.semanticTokens.push_back({p->keyword.line, kc, 5, "keyword", currentSource});
            }
            // Build description from style properties
            std::string desc;
            for (auto& [key, val] : p->properties) {
                auto keyName = tokName(key);
                if (!desc.empty()) desc += ", ";
                desc += keyName;
                // Try to extract literal value for description
                if (auto* lit = dynamic_cast<expr::Literal<MV>*>(val.get())) {
                    if (auto* sv = std::get_if<std::string>(&lit->value))
                        desc += ": " + *sv;
                    else if (auto* dv = std::get_if<double>(&lit->value)) {
                        std::ostringstream os; os << *dv; desc += ": " + os.str();
                    }
                }
                analyzeExpr(val.get());
            }
            define(p->name, "variable", "Style", desc);
        }
    }

    inline void MacAnalyzer::analyzeExpr(expr::Expr<MV>* e) {
        if (!e) return;

        if (auto* p = dynamic_cast<expr::Variable<MV>*>(e)) {
            resolveRef(p->name);
        } else if (auto* p = dynamic_cast<expr::Assign<MV>*>(e)) {
            analyzeExpr(p->value.get());
            resolveRef(p->name);
        } else if (auto* p = dynamic_cast<expr::Binary<MV>*>(e)) {
            analyzeExpr(p->left.get());
            analyzeExpr(p->right.get());
        } else if (auto* p = dynamic_cast<expr::Logical<MV>*>(e)) {
            analyzeExpr(p->left.get());
            analyzeExpr(p->right.get());
        } else if (auto* p = dynamic_cast<expr::Unary<MV>*>(e)) {
            analyzeExpr(p->right.get());
        } else if (auto* p = dynamic_cast<expr::Grouping<MV>*>(e)) {
            analyzeExpr(p->expression.get());
        } else if (auto* p = dynamic_cast<expr::Call<MV>*>(e)) {
            analyzeExpr(p->callee.get());
            for (auto& a : p->arguments) analyzeExpr(a.get());
            collectParamHints(p);
            collectChainHint(p);
        } else if (auto* p = dynamic_cast<expr::Get<MV>*>(e)) {
            analyzeExpr(p->object.get());
            auto propName = tokName(p->name);
            auto objType = inferType(p->object.get());
            if (const auto* member = resolveMember(objType, propName)) {
                int c = safeCol(p->name);
                result.properties.push_back({
                    p->name.line,
                    c,
                    c + static_cast<int>(propName.size()),
                    member->line,
                    member->col,
                    member->endCol,
                    propName,
                    objType,
                    member->kind,
                    member->kind == "field" ? member->type : member->returnType,
                    member->description,
                    currentSource,
                    member->source,
                    member->visibility,
                });
            }
        } else if (auto* p = dynamic_cast<expr::Set<MV>*>(e)) {
            analyzeExpr(p->object.get());
            analyzeExpr(p->value.get());

            if (dynamic_cast<expr::This<MV>*>(p->object.get()) && !currentClassName.empty()) {
                auto fieldName = tokName(p->name);
                auto fieldType = inferType(p->value.get());
                auto visibility = isInternalName(fieldName) ? "internal" : "public";

                bool exists = false;
                if (auto* cls = resolveClass(currentClassName)) {
                    for (const auto& member : cls->members) {
                        if (member.name == fieldName && member.kind == "field" && member.source == currentSource) {
                            exists = true;
                            break;
                        }
                    }
                }

                upsertMember(currentClassName, p->name, "field", fieldType, "", {}, "", visibility);

                if (!exists) {
                    int c = safeCol(p->name);
                    result.symbols.push_back({
                        fieldName,
                        "field",
                        fieldType,
                        "",
                        currentSource,
                        visibility,
                        currentClassName,
                        p->name.line,
                        c,
                        c + static_cast<int>(fieldName.size()),
                    });
                    if (currentSource == "user" && p->name.line > 0) {
                        result.semanticTokens.push_back({
                            p->name.line,
                            c,
                            static_cast<int>(fieldName.size()),
                            semanticKind("field"),
                            currentSource,
                        });
                    }
                }
            }
        } else if (auto* p = dynamic_cast<expr::This<MV>*>(e)) {
            if (currentClassName.empty()) addDiagnostic(p->keyword, "Can't use 'this' outside of a class.", "error");
        } else if (auto* p = dynamic_cast<expr::Super<MV>*>(e)) {
            if (currentClassName.empty() || currentSuperclassName.empty()) {
                addDiagnostic(p->keyword, "Can't use 'super' without a superclass.", "error");
            } else if (const auto* member = resolveMember(currentSuperclassName, tokName(p->method))) {
                int c = safeCol(p->method);
                result.properties.push_back({
                    p->method.line,
                    c,
                    c + static_cast<int>(tokName(p->method).size()),
                    member->line,
                    member->col,
                    member->endCol,
                    tokName(p->method),
                    currentSuperclassName,
                    member->kind,
                    member->kind == "field" ? member->type : member->returnType,
                    member->description,
                    currentSource,
                    member->source,
                    member->visibility,
                });
            }
        } else if (auto* p = dynamic_cast<expr::ArrayExpr<MV>*>(e)) {
            for (auto& el : p->elements) analyzeExpr(el.get());
        } else if (auto* p = dynamic_cast<expr::MapExpr<MV>*>(e)) {
            for (auto& v : p->values) analyzeExpr(v.get());
        } else if (auto* p = dynamic_cast<expr::IndexGet<MV>*>(e)) {
            analyzeExpr(p->object.get());
            analyzeExpr(p->index.get());
        } else if (auto* p = dynamic_cast<expr::IndexSet<MV>*>(e)) {
            analyzeExpr(p->object.get());
            analyzeExpr(p->index.get());
            analyzeExpr(p->value.get());
        } else if (auto* p = dynamic_cast<expr::LambdaExpr<MV>*>(e)) {
            beginScope();
            for (auto& prm : p->params) define(prm, "parameter", "unknown");
            for (auto& st : p->body) analyzeStmt(st.get());
            if (p->tailExpr) analyzeExpr(p->tailExpr.get());
            endScope();
        } else if (auto* p = dynamic_cast<expr::PipeExpr<MV>*>(e)) {
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
        } else if (auto* p = dynamic_cast<expr::ComposeExpr<MV>*>(e)) {
            analyzeExpr(p->left.get());
            analyzeExpr(p->right.get());
        }
        // --- Mac v2 syntax nodes ---
        else if (auto* p = dynamic_cast<expr::MemeLiteralExpr<MV>*>(e)) {
            auto tname = tokName(p->templateName);
            int tc = safeCol(p->templateName);
            int tec = tc + static_cast<int>(tname.size());

            if (p->templateName.type == token::TokenType::STRING) {
                // String template: @"path/to/image.png" — emit as string token + hover
                if (p->templateName.line > 0 && currentSource == "user") {
                    result.semanticTokens.push_back({p->templateName.line, tc,
                        static_cast<int>(tname.size()) + 2, "string", currentSource});
                    result.symbols.push_back({tname, "variable", "Meme",
                        "Custom image template from file path", currentSource,
                        "public", "", p->templateName.line, tc, tec});
                }
            } else {
                // Identifier template: @two_panel — emit reference + class token
                auto* templateDef = resolve("Template");
                if (templateDef) {
                    result.references.push_back({p->templateName.line, tc, tec,
                        templateDef->line, templateDef->col, templateDef->endCol,
                        templateDef->name, currentSource, templateDef->source,
                        templateDef->visibility, templateDef->ownerType});
                }
                if (p->templateName.line > 0 && currentSource == "user") {
                    result.semanticTokens.push_back({p->templateName.line, tc,
                        static_cast<int>(tname.size()), "class", currentSource});
                }
            }

            // Emit reference for style name if present
            if (p->styleName.type != token::TokenType::NONE) {
                auto sname = tokName(p->styleName);
                if (!sname.empty()) {
                    resolveRef(p->styleName);
                }
            }

            // Emit references for position keys in block syntax (top, bottom, center)
            // Only for block syntax — one-liners don't have user-visible position keys
            if (!p->oneLiner) {
                for (auto& entry : p->entries) {
                    auto keyName = tokName(entry.key);
                    std::string constName;
                    if (keyName == "top") constName = "Top";
                    else if (keyName == "bottom") constName = "Bottom";
                    else if (keyName == "center") constName = "Center";
                    if (!constName.empty()) {
                        auto* posDef = resolve(constName);
                        if (posDef) {
                            int kc = safeCol(entry.key);
                            result.references.push_back({entry.key.line, kc,
                                kc + static_cast<int>(keyName.size()),
                                posDef->line, posDef->col, posDef->endCol,
                                posDef->name, currentSource, posDef->source,
                                posDef->visibility, posDef->ownerType});
                        }
                    }
                    analyzeExpr(entry.value.get());
                }
            } else {
                // One-liner: just analyze the text value
                for (auto& entry : p->entries) analyzeExpr(entry.value.get());
            }
        }
        else if (auto* p = dynamic_cast<expr::SaveExpr<MV>*>(e)) {
            analyzeExpr(p->value.get());
            analyzeExpr(p->path.get());
        }
        else if (auto* p = dynamic_cast<expr::GifBlockExpr<MV>*>(e)) {
            if (p->keyword.line > 0 && currentSource == "user") {
                auto kw = tokName(p->keyword);
                int kc = safeCol(p->keyword);
                result.semanticTokens.push_back({p->keyword.line, kc,
                    static_cast<int>(kw.size()), "keyword", currentSource});
                std::string desc = p->loop ? "Looping GIF block" : "GIF block";
                desc += " — " + std::to_string(p->entries.size()) + " frames";
                result.symbols.push_back({kw, "keyword", "Gif", desc, currentSource,
                    "public", "", p->keyword.line, kc, kc + static_cast<int>(kw.size())});
                // Semantic token for 'loop' keyword
                if (p->loop && p->loopToken.line > 0) {
                    int lc = safeCol(p->loopToken);
                    result.semanticTokens.push_back({p->loopToken.line, lc, 4, "keyword", currentSource});
                }
            }
            for (auto& entry : p->entries) {
                checkFrameType(entry.meme.get(), "gif");
                analyzeExpr(entry.meme.get());
            }
        }
        else if (auto* p = dynamic_cast<expr::MatchExpr<MV>*>(e)) {
            analyzeExpr(p->subject.get());
            for (auto& arm : p->arms) {
                if (arm.pattern) analyzeExpr(arm.pattern.get());
                // Register destructured bindings so the result expression can see them
                for (auto& binding : arm.bindings) {
                    define(binding, "variable", "unknown", "Match binding");
                }
                analyzeExpr(arm.result.get());
            }
            if (p->keyword.line > 0 && currentSource == "user") {
                result.semanticTokens.push_back({
                    p->keyword.line, safeCol(p->keyword),
                    5, "keyword", currentSource}); // "match" is 5 chars
            }
        }
        else if (auto* p = dynamic_cast<expr::GridBlockExpr<MV>*>(e)) {
            if (p->keyword.line > 0 && currentSource == "user") {
                auto kw = tokName(p->keyword);
                int kc = safeCol(p->keyword);
                result.semanticTokens.push_back({p->keyword.line, kc,
                    static_cast<int>(kw.size()), "keyword", currentSource});
                std::string desc = std::to_string(p->cols) + "x" + std::to_string(p->rows) +
                    " grid — " + std::to_string(p->entries.size()) + " entries";
                result.symbols.push_back({kw, "keyword", "Meme", desc, currentSource,
                    "public", "", p->keyword.line, kc, kc + static_cast<int>(kw.size())});
            }
            for (auto& entry : p->entries) {
                checkFrameType(entry.get(), "grid");
                analyzeExpr(entry.get());
            }
        }
    }

    // Check if an expression used as a frame entry is a sequence type (Gif)
    inline void MacAnalyzer::checkFrameType(expr::Expr<MV>* e, const std::string& container) {
        if (!e || currentSource != "user") return;
        auto type = inferType(e);
        if (type == "Gif") {
            int line = 0, col = 1, endCol = 2;
            if (auto* v = dynamic_cast<expr::Variable<MV>*>(e)) {
                line = v->name.line;
                col = safeCol(v->name);
                endCol = col + static_cast<int>(tokName(v->name).size());
            } else if (auto* g = dynamic_cast<expr::GifBlockExpr<MV>*>(e)) {
                line = g->keyword.line;
                col = safeCol(g->keyword);
                endCol = col + static_cast<int>(tokName(g->keyword).size());
            }
            if (line > 0) {
                result.diagnostics.push_back({line, col, endCol,
                    type + " is a sequence type and cannot be used as a frame inside '" +
                    container + "'. Wrap the " + container + " in a 'gif' block instead.",
                    "error", currentSource});
            }
        }
    }

    inline void MacAnalyzer::addFoldRange(
        int startLine, const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body) {
        if (!body.empty()) {
            int endLine = lastLineOf(body);
            if (endLine > startLine) result.foldingRanges.push_back({startLine, endLine, currentSource});
        }
    }

    inline void MacAnalyzer::collectParamHints(expr::Call<MV>* call) {
        const Signature* sig = nullptr;

        if (auto* var = dynamic_cast<expr::Variable<MV>*>(call->callee.get())) {
            auto name = tokName(var->name);
            for (const auto& candidate : result.signatures) {
                if (candidate.name != name || candidate.visibility == "internal") continue;
                if (candidate.ownerType.empty() &&
                    (candidate.params.size() == call->arguments.size() || candidate.params.size() >= call->arguments.size())) {
                    sig = &candidate;
                    break;
                }
            }
        } else if (auto* get = dynamic_cast<expr::Get<MV>*>(call->callee.get())) {
            auto ownerType = inferType(get->object.get());
            auto methodName = tokName(get->name);
            for (const auto& candidate : result.signatures) {
                if (candidate.name != methodName || candidate.ownerType != ownerType || candidate.visibility == "internal") continue;
                if (candidate.params.size() == call->arguments.size() || candidate.params.size() >= call->arguments.size()) {
                    sig = &candidate;
                    break;
                }
            }
            if (!sig) sig = resolveSignature(methodName, ownerType);
        }

        if (!sig) return;
        for (size_t i = 0; i < sig->params.size() && i < call->arguments.size(); i++) {
            int argCol = getExprCol(call->arguments[i].get());
            int argLine = getExprLine(call->arguments[i].get());
            if (argCol > 0 && argLine > 0 && currentSource == "user") {
                result.paramHints.push_back({argLine, argCol, sig->params[i], currentSource});
            }
        }
    }

    inline void MacAnalyzer::collectChainHint(expr::Call<MV>* call) {
        if (!dynamic_cast<expr::Get<MV>*>(call->callee.get())) return;
        auto resultType = inferCallType(call);
        if (resultType != "unknown" && currentSource == "user") {
            result.chainHints.push_back({
                call->paren.line,
                call->paren.column + 1,
                resultType,
                currentSource,
            });
        }
    }

    inline void MacAnalyzer::addFunctionSignature(stmt::FunctionStmt<MV>* fn, const std::string& kind,
                                                  const std::string& returnType,
                                                  const std::string& description,
                                                  const std::string& ownerType,
                                                  const std::string& visibility) {
        std::vector<std::string> params;
        params.reserve(fn->params.size());
        for (auto& prm : fn->params) params.push_back(tokName(prm));
        addSignature(fn->name, tokName(fn->name), kind, params, returnType, description,
                     currentSource, visibility, ownerType);
    }

    inline void MacAnalyzer::collectReturnTypes(stmt::Stmt<MV>* s, std::vector<std::string>& out) {
        if (!s) return;
        if (auto* ret = dynamic_cast<stmt::ReturnStmt<MV>*>(s)) {
            out.push_back(ret->value ? inferType(ret->value.get()) : "nil");
            return;
        }
        if (auto* block = dynamic_cast<stmt::BlockStmt<MV>*>(s)) {
            for (auto& st : block->statements) collectReturnTypes(st.get(), out);
            return;
        }
        if (auto* ifStmt = dynamic_cast<stmt::IfStmt<MV>*>(s)) {
            collectReturnTypes(ifStmt->thenBranch.get(), out);
            if (ifStmt->elseBranch) collectReturnTypes(ifStmt->elseBranch.get(), out);
            return;
        }
        if (auto* whileStmt = dynamic_cast<stmt::WhileStmt<MV>*>(s)) {
            collectReturnTypes(whileStmt->body.get(), out);
            return;
        }
        if (auto* forIn = dynamic_cast<stmt::ForInStmt<MV>*>(s)) {
            collectReturnTypes(forIn->body.get(), out);
            return;
        }
    }

    inline std::string MacAnalyzer::inferBlockReturn(
        const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body) {
        std::vector<std::string> returns;
        for (auto& st : body) collectReturnTypes(st.get(), returns);
        if (returns.empty()) return "nil";

        std::string first = returns.front();
        for (const auto& type : returns) {
            if (type != first) return "unknown";
        }
        return first;
    }

    inline int MacAnalyzer::lastLineOf(const std::vector<std::shared_ptr<stmt::Stmt<MV>>>& body) {
        if (body.empty()) return 0;
        auto* last = body.back().get();
        if (auto* p = dynamic_cast<stmt::ExpressionStmt<MV>*>(last)) return getExprLine(p->expression.get());
        if (auto* p = dynamic_cast<stmt::VarStmt<MV>*>(last)) return p->name.line;
        if (auto* p = dynamic_cast<stmt::ReturnStmt<MV>*>(last)) return p->keyword.line;
        if (auto* p = dynamic_cast<stmt::FunctionStmt<MV>*>(last)) return p->name.line;
        if (auto* p = dynamic_cast<stmt::ClassStmt<MV>*>(last)) return p->name.line;
        return 0;
    }

    inline int MacAnalyzer::getExprLine(expr::Expr<MV>* e) {
        if (auto* v = dynamic_cast<expr::Variable<MV>*>(e)) return v->name.line;
        if (auto* c = dynamic_cast<expr::Call<MV>*>(e)) return c->paren.line;
        if (auto* g = dynamic_cast<expr::Get<MV>*>(e)) return g->name.line;
        if (auto* s = dynamic_cast<expr::Set<MV>*>(e)) return s->name.line;
        if (auto* t = dynamic_cast<expr::This<MV>*>(e)) return t->keyword.line;
        if (auto* s = dynamic_cast<expr::Super<MV>*>(e)) return s->method.line;
        return 0;
    }

    inline int MacAnalyzer::getExprCol(expr::Expr<MV>* e) {
        if (auto* v = dynamic_cast<expr::Variable<MV>*>(e)) return v->name.column;
        if (auto* c = dynamic_cast<expr::Call<MV>*>(e)) return getExprCol(c->callee.get());
        if (auto* g = dynamic_cast<expr::Get<MV>*>(e)) return g->name.column;
        if (auto* s = dynamic_cast<expr::Set<MV>*>(e)) return s->name.column;
        if (auto* t = dynamic_cast<expr::This<MV>*>(e)) return t->keyword.column;
        if (auto* s = dynamic_cast<expr::Super<MV>*>(e)) return s->method.column;
        return 0;
    }

} // namespace analyzer

#endif // ANALYZER_WALK_H
