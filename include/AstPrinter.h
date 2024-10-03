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

    class AstPrinter : public Visitor, public std::enable_shared_from_this<AstPrinter> {
    public:
        string visitBinaryExpr(expr::Binary* expr) override {
            return parenthesize(expr->operatorToken.lexeme->Value(), expr->left, expr->right);
        }

        string visitUnaryExpr(expr::Unary* expr) override {
            return expr->operatorToken.type == TokenType::NUMBER
                ? parenthesize(expr->operatorToken.lexeme->NumericValue(), expr->right)
                : parenthesize(expr->operatorToken.lexeme->Value(), expr->right);
        }

        string visitLiteralExpr(expr::Literal* expr) override {
            if (expr == nullptr) return "nil";
            return expr->toString();
        }

        string visitVariableExpr(expr::Variable* expr) override {
            return expr->name.lexeme->Value();
        }

        string visitGroupingExpr(expr::Grouping* expr) override {
            return parenthesize("group", expr->expression);
        }

    private:
        template <typename... Exprs>
        string parenthesize(const std::string& name, shared_ptr<Exprs>... exprs) {
            std::ostringstream output;
            output << "(" << name;
            // C++17 fold expression to expand variadic arguments
            ((output << " " << exprs->visit(this->shared_from_this())), ...);
            output << ")";
            return output.str();
        }

        // Overload for numeric values
        string parenthesize(double numericValue, shared_ptr<expr::Expr> expr) {
            std::ostringstream output;
            output << "(" << numericValue;
            output << " " << expr->visit(this->shared_from_this());
            output << ")";
            return output.str();
        }
    };

} // end namespace printer

#endif // ASTPRINTER_H