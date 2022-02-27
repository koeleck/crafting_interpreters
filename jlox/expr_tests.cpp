#include <string>

#include "expr.hpp"
#include "scanner.hpp"
#include "bump_alloc.hpp"

#include <doctest/doctest.h>

namespace
{

class TestVisitor final : public Visitor
{
public:
    TestVisitor(std::string_view source) noexcept
      : m_result{}
      , m_source{source}
    {}

    void visit(BinaryExpr& binar_expr) override
    {
        m_result.push_back('(');
        m_result += binar_expr.op->lexeme(m_source);
        m_result.push_back(' ');
        binar_expr.left->accept(*this);
        m_result.push_back(' ');
        binar_expr.right->accept(*this);
        m_result.push_back(')');
    }

    void visit(GroupingExpr& grouping_expr) override
    {
        m_result += "(group ";
        grouping_expr.expr->accept(*this);
        m_result += ")";
    }

    void visit(LiteralExpr& literal_expr) override
    {
        m_result += literal_expr.value->lexeme(m_source);
    }

    void visit(UnaryExpr& unary_expr) override
    {
        m_result.push_back('(');
        m_result += unary_expr.op->lexeme(m_source);
        unary_expr.right->accept(*this);
        m_result.push_back(')');
    }

    void unkown_expr(Expr& /*expr*/) override
    {
        m_result += "<unkown expr> ";
    }

    const std::string& get() const noexcept
    {
        return m_result;
    }

private:
    std::string m_result;
    std::string_view m_source;
};

TEST_CASE("Expr/Visitor")
{
    std::string_view source{"-123 * (45.67)"};
    auto result = scan_tokens(source);
    CHECK(result.num_errors == 0);
    CHECK(result.tokens.size() == 7);

    LiteralExpr v123{&result.tokens[1]};
    UnaryExpr minus{&result.tokens[0], &v123};

    LiteralExpr v4567{&result.tokens[4]};
    GroupingExpr group{&v4567};

    BinaryExpr binary{&minus, &result.tokens[2], &group};

    TestVisitor visitor{source};
    binary.accept(visitor);

    CHECK(visitor.get() == "(* (-123) (group 45.67))");
}

} // anonymous namespace
