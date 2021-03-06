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

void PrintVisitor::visit(VarExpr& var_expr)
{
    m_result.append("(var ");
    m_result += var_expr.identifier->lexeme(m_source);
    m_result.push_back(')');
}

void PrintVisitor::visit(AssignExpr& assign_expr)
{
    m_result.append("(var ");
    m_result += assign_expr.identifier->lexeme(m_source);
    m_result.append(" = ");
    assign_expr.value->accept(*this);
    m_result.push_back(')');
}

void PrintVisitor::visit(LogicalExpr& logical_expr)
{
    m_result.push_back('(');
    m_result += logical_expr.token->lexeme(m_source);

    m_result.append(" (");
    logical_expr.left->accept(*this);
    m_result.append(") (");
    logical_expr.right->accept(*this);
    m_result.push_back(')');
}

void PrintVisitor::visit(CallExpr& call_expr)
{
    m_result.append("(CALL (");
    call_expr.callee->accept(*this);
    m_result.append(")(");
    bool is_first = true;
    for (const auto& e : call_expr.args) {
        if (!is_first) {
            m_result.append(", ");
        }
        is_first = false;
        e->accept(*this);
    }
    m_result.append("))");
}

void PrintVisitor::unkown_expr(Expr& /*expr*/)
{
    m_result += "<unkown expr> ";
}
