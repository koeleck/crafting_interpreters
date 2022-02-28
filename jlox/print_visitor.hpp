#pragma once

#include "expr.hpp"

class PrintVisitor final : public Visitor
{
public:
    PrintVisitor(std::string_view source) noexcept;

    void visit(BinaryExpr& binar_expr) override;
    void visit(GroupingExpr& grouping_expr) override;
    void visit(LiteralExpr& literal_expr) override;
    void visit(UnaryExpr& unary_expr) override;
    void unkown_expr(Expr& /*expr*/) override;
    const std::string& get() const noexcept
    {
        return m_result;
    }

private:
    std::string m_result;
    std::string_view m_source;
};
