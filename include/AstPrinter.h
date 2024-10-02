#ifndef ASTPRINTER_H
#define ASTPRINTER_H

#include <string>
#include <sstream>
#include <memory>
#include "Expr.h"

using expr::Visitor;
using std::string;
using std::shared_ptr;

namespace printer {

    template <typename Result>
    class AstPrinter : public Visitor<Result>, public std::enable_shared_from_this<AstPrinter<Result>> {
    public:
        Result visitBinaryExpr(expr::Binary<Result>* expr) override {
            return parenthesize(expr->operatorToken.lexeme->Value(), expr->left, expr->right);
        }

        Result visitUnaryExpr(expr::Unary<Result>* expr) override {
            return parenthesize(expr->operatorToken.lexeme->Value(), expr->right);
        }

        Result visitLiteralExpr(expr::Literal<Result>* expr) override {
            if (expr == nullptr) return "nil";
            return expr->toString();
        }

        Result visitVariableExpr(expr::Variable<Result>* expr) override {
            return expr->name.lexeme->Value();
        }

        Result visitGroupingExpr(expr::Grouping<Result>* expr) override {
            return parenthesize("grouping", expr->expression);
        }

    private:
        template <typename... Exprs>
        Result parenthesize(const std::string& name, shared_ptr<Exprs>... exprs) {
            std::ostringstream output;
            output << "(" << name;
            // C++17 fold expression to expand variadic arguments
            ((output << " " << exprs->visit(this->shared_from_this())), ...);
            output << ")";
            return output.str();
        }
    };

} // end namespace printer

#endif // ASTPRINTER_H