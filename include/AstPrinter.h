#ifndef ASTPRINTER_H
#define ASTPRINTER_H

#include <string>
#include "Expr.h"

using expr::Visitor;
using std::string;

namespace printer {

    template <typename Result>
    class AstPrinter : public Visitor<Result> {
    public:
        Result visitBinaryExpr(expr::Binary<Result>* expr) override {
            return parenthesize(expr->operatorToken.lexeme, expr->left, expr->right);
        }

        Result visitUnaryExpr(expr::Unary<Result>* expr) override {
            return parenthesize(expr->operatorToken.lexeme, expr->right);
        }

        Result visitLiteralExpr(expr::Literal<Result>* expr) override {
            if (expr == NULL) return "nil";
            return expr->toString();
        }

        Result visitVariableExpr(expr::Variable<Result>* expr) override {
            return expr->name.lexeme;
        }

        Result visitGroupingExpr(expr::Grouping<Result>* expr) override {
            return parenthesize("grouping", expr->expression);
        }
    private:
        template <typename... Exprs>
        Result parenthesize(const std::string& name, Exprs*... exprs) {
            std::ostringstream output;
            output << "(" << name;
            // C++17 fold expression to expand variadic arguments
            ((output << " " << exprs->visit(this)), ...);
            output << ")";
            return output.str();
        }
    };
} // end namespace printer

#endif // ASTPRINTER_H