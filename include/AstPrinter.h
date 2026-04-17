#ifndef ASTPRINTER_H
#define ASTPRINTER_H

#include <string>               // string
#include <sstream>              // ostringstream (S-expression output)
#include <memory>               // shared_ptr
#include "Expr.h"               // expr::Expr<T>, Visitor<T> (AST visiting)

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

        string visitArrayExpr(expr::ArrayExpr<T>* expr) override {
            std::ostringstream out;
            out << "[";
            for (size_t i = 0; i < expr->elements.size(); i++) {
                if (i > 0) out << ", ";
                out << expr->elements[i]->visit(this->shared_from_this());
            }
            out << "]";
            return out.str();
        }

        string visitMapExpr(expr::MapExpr<T>* expr) override {
            std::ostringstream out;
            out << "{";
            for (size_t i = 0; i < expr->keys.size(); i++) {
                if (i > 0) out << ", ";
                out << get<string>(expr->keys[i].lexeme) << ": " << expr->values[i]->visit(this->shared_from_this());
            }
            out << "}";
            return out.str();
        }

        string visitIndexGetExpr(expr::IndexGet<T>* expr) override {
            return expr->object->visit(this->shared_from_this()) + "[" + expr->index->visit(this->shared_from_this()) + "]";
        }

        string visitIndexSetExpr(expr::IndexSet<T>* expr) override {
            return expr->object->visit(this->shared_from_this()) + "[" + expr->index->visit(this->shared_from_this()) + "] = " + expr->value->visit(this->shared_from_this());
        }

        string visitLambdaExpr(expr::LambdaExpr<T>*) override {
            return "<lambda>";
        }

        string visitPipeExpr(expr::PipeExpr<T>* expr) override {
            return "(" + expr->value->visit(this->shared_from_this()) + " |> " + expr->func->visit(this->shared_from_this()) + ")";
        }

        string visitComposeExpr(expr::ComposeExpr<T>* expr) override {
            return "(" + expr->left->visit(this->shared_from_this()) + " >> " + expr->right->visit(this->shared_from_this()) + ")";
        }

        string visitMemeLiteralExpr(expr::MemeLiteralExpr<T>*) override {
            return "<meme>";
        }

        string visitSaveExpr(expr::SaveExpr<T>* expr) override {
            return "(=> " + expr->value->visit(this->shared_from_this()) + " " + expr->path->visit(this->shared_from_this()) + ")";
        }

        string visitGifBlockExpr(expr::GifBlockExpr<T>*) override {
            return "<gif>";
        }

        string visitGridBlockExpr(expr::GridBlockExpr<T>*) override {
            return "<grid>";
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