#pragma once

#include <cassert>
#include "tokens.hpp"

class Expr;
struct BinaryExpr;
struct GroupingExpr;
struct LiteralExpr;
struct UnaryExpr;

class Visitor
{
public:
    virtual ~Visitor() = default;

    virtual void visit(BinaryExpr& binar_expr) = 0;
    virtual void visit(GroupingExpr& grouping_expr) = 0;
    virtual void visit(LiteralExpr& literal_expr) = 0;
    virtual void visit(UnaryExpr& unary_expr) = 0;
    virtual void unkown_expr(Expr& expr) = 0;

    void visit(Expr& expr) {
        unkown_expr(expr);
    }
};

template <typename T>
class ExprCRTC;

class Expr
{
public:
    Expr() = delete;

    void accept(Visitor& visitor)
    {
        m_accept(this, visitor);
    }

protected:
    template <typename T>
    friend class ExprCRTC;

    constexpr
    Expr(void (*accept)(Expr*, Visitor&)) noexcept
      : m_accept{accept}
    {
        assert(m_accept);
    }

private:
    void (*m_accept)(Expr*, Visitor&);
};

template <typename T>
class ExprCRTC : public Expr
{
protected:
    constexpr
    ExprCRTC() noexcept
      : Expr{&ExprCRTC::accept_impl}
    {}

private:
    static
    void accept_impl(Expr* e, Visitor& visitor)
    {
        static_assert(std::is_base_of_v<ExprCRTC<T>, T>);
        visitor.visit(*static_cast<T*>(e));
    }
};

struct BinaryExpr : ExprCRTC<BinaryExpr>
{
    constexpr
    BinaryExpr(Expr* left, const Token* op, Expr* right) noexcept
      : left{left}
      , op{op}
      , right{right}
    {
        assert(left && op && right && (
                op->type() == TokenType::EQUAL_EQUAL ||
                op->type() == TokenType::BANG_EQUAL ||
                op->type() == TokenType::LESS ||
                op->type() == TokenType::LESS_EQUAL ||
                op->type() == TokenType::GREATER ||
                op->type() == TokenType::GREATER_EQUAL ||
                op->type() == TokenType::PLUS ||
                op->type() == TokenType::MINUS ||
                op->type() == TokenType::STAR ||
                op->type() == TokenType::SLASH));
    }

    Expr* left;
    const Token* op;
    Expr* right;
};

struct GroupingExpr : ExprCRTC<GroupingExpr>
{
    constexpr
    GroupingExpr(Expr* expr) noexcept
      : expr{expr}
    {
        assert(expr);
    }

    Expr* expr;
};

struct LiteralExpr : ExprCRTC<LiteralExpr>
{
    constexpr
    LiteralExpr(const Token* value) noexcept
      : value{value}
    {
        assert(value && (
                value->type() == TokenType::NUMBER ||
                value->type() == TokenType::STRING ||
                value->type() == TokenType::TRUE ||
                value->type() == TokenType::FALSE ||
                value->type() == TokenType::NIL ||
                value->type() == TokenType::IDENTIFIER));
    }

    const Token* value;
};

struct UnaryExpr : ExprCRTC<UnaryExpr>
{
    constexpr
    UnaryExpr(const Token* op, Expr* right) noexcept
      : op{op}
      , right{right}
    {
        assert(op && right && (
                op->type() == TokenType::MINUS ||
                op->type() == TokenType::BANG));
    }

    const Token* op;
    Expr* right;
};
