#include "print_visitor.hpp"

PrintVisitor::PrintVisitor(std::string_view source) noexcept
  : m_result{}
  , m_source{source}
{}

void PrintVisitor::visit(BinaryExpr& binar_expr)
{
    m_result.push_back('(');
    m_result += binar_expr.op->lexeme(m_source);
    m_result.push_back(' ');
    binar_expr.left->accept(*this);
    m_result.push_back(' ');
    binar_expr.right->accept(*this);
    m_result.push_back(')');
}

void PrintVisitor::visit(GroupingExpr& grouping_expr)
{
    m_result += "(group ";
    grouping_expr.expr->accept(*this);
    m_result += ")";
}

void PrintVisitor::visit(LiteralExpr& literal_expr)
{
    m_result += literal_expr.value->lexeme(m_source);
}

void PrintVisitor::visit(UnaryExpr& unary_expr)
{
    m_result.push_back('(');
    m_result += unary_expr.op->lexeme(m_source);
    unary_expr.right->accept(*this);
    m_result.push_back(')');
}

void PrintVisitor::unkown_expr(Expr& /*expr*/)
{
    m_result += "<unkown expr> ";
}
