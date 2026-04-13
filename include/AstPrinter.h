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

    template <typename T>
    class AstPrinter : public Visitor<T>, public std::enable_shared_from_this<AstPrinter<T>> {
    public:
        string visitBinaryExpr(expr::Binary<T>* expr) override {
            return parenthesize(get<string>(expr->operatorToken.lexeme), expr->left, expr->right);
        }

        string visitUnaryExpr(expr::Unary<T>* expr) override {
            return expr->operatorToken.type == TokenType::NUMBER
                ? parenthesize(get<double>(expr->operatorToken.lexeme), expr->right)
                : parenthesize(get<string>(expr->operatorToken.lexeme), expr->right);
        }

        string visitLiteralExpr(expr::Literal<T>* expr) override {
            if (expr == nullptr) return "nil";
            return expr->toString();
        }

        string visitVariableExpr(expr::Variable<T>* expr) override {
            return get<string>(expr->name.lexeme);
        }

        string visitGroupingExpr(expr::Grouping<T>* expr) override {
            return parenthesize("group", expr->expression);
        }

        string visitCallExpr(expr::Call<T>* expr) override {
            std::ostringstream output;
            output << "(call " << expr->callee->visit(this->shared_from_this());
            for (auto& arg : expr->arguments) {
                output << " " << arg->visit(this->shared_from_this());
            }
            output << ")";
            return output.str();
        }

        string visitLogicalExpr(expr::Logical<T>* expr) override {
            return parenthesize(get<string>(expr->operatorToken.lexeme), expr->left, expr->right);
        }

        string visitAssignExpr(expr::Assign<T>* expr) override {
            return "(= " + get<string>(expr->name.lexeme) + " " + expr->value->visit(this->shared_from_this()) + ")";
        }

        string visitGetExpr(expr::Get<T>* expr) override {
            return "(. " + expr->object->visit(this->shared_from_this()) + " " + get<string>(expr->name.lexeme) + ")";
        }

        string visitSetExpr(expr::Set<T>* expr) override {
            return "(= " + expr->object->visit(this->shared_from_this()) + "." + get<string>(expr->name.lexeme)
                + " " + expr->value->visit(this->shared_from_this()) + ")";
        }

        string visitThisExpr(expr::This<T>*) override {
            return "this";
        }

        string visitSuperExpr(expr::Super<T>* expr) override {
            return "(super " + get<string>(expr->method.lexeme) + ")";
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
        string parenthesize(double numericValue, shared_ptr<expr::Expr<T>> expr) {
            std::ostringstream output;
            output << "(" << numericValue;
            output << " " << expr->visit(this->shared_from_this());
            output << ")";
            return output.str();
        }
    };

} // end namespace printer

#endif // ASTPRINTER_H