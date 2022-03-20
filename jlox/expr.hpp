#pragma once

#include <cassert>
#include "tokens.hpp"

class Expr;
struct BinaryExpr;
struct GroupingExpr;
struct LiteralExpr;
struct UnaryExpr;
struct VarExpr;
struct AssignExpr;

class ExprVisitor
{
public:
    virtual ~ExprVisitor() = default;

    virtual void visit(BinaryExpr& binar_expr) = 0;
    virtual void visit(GroupingExpr& grouping_expr) = 0;
    virtual void visit(LiteralExpr& literal_expr) = 0;
    virtual void visit(UnaryExpr& unary_expr) = 0;
    virtual void visit(VarExpr& var_expr) = 0;
    virtual void visit(AssignExpr& assign_expr) = 0;
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

    void accept(ExprVisitor& visitor)
    {
        m_accept(*this, visitor);
    }

    const Token* get_main_token() const noexcept
    {
        return m_get_main_token(*this);
    }

    template <typename T>
    bool is_type() const noexcept
    {
        using TBase = ExprCRTC<T>;
        return m_accept == &TBase::accept_impl;
    }


protected:
    template <typename T>
    friend class ExprCRTC;

    constexpr
    Expr(void (*accept)(Expr&, ExprVisitor&),
         const Token* (*mt)(const Expr&) noexcept) noexcept
      : m_accept{accept}
      , m_get_main_token{mt}
    {
        assert(m_accept);
    }

private:
    void (*m_accept)(Expr&, ExprVisitor&);
    const Token* (*m_get_main_token)(const Expr&) noexcept;
};

template <typename T>
class ExprCRTC : public Expr
{
protected:
    constexpr
    ExprCRTC() noexcept
      : Expr{&ExprCRTC::accept_impl, &ExprCRTC::get_main_token_impl}
    {}

private:
    friend class Expr;

    static
    void accept_impl(Expr& e, ExprVisitor& visitor)
    {
        static_assert(std::is_base_of_v<ExprCRTC<T>, T>);
        visitor.visit(static_cast<T&>(e));
    }

    static
    const Token* get_main_token_impl(const Expr& e) noexcept
    {
        static_assert(std::is_base_of_v<ExprCRTC<T>, T>);
        const T& impl = static_cast<const T&>(e);


        return impl.*(T::main_token);
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

    static constexpr auto main_token = &BinaryExpr::op;
};

struct GroupingExpr : ExprCRTC<GroupingExpr>
{
    constexpr
    GroupingExpr(const Token* parens_beg, Expr* expr, const Token* parens_end) noexcept
      : begin{parens_beg}
      , expr{expr}
      , end{parens_end}
    {
        assert(begin && end && expr &&
               begin->type() == TokenType::LEFT_PAREN &&
               end->type() == TokenType::RIGHT_PAREN);
    }

    const Token* begin;
    Expr* expr;
    const Token* end;

    static constexpr auto main_token = &GroupingExpr::begin;
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
                value->type() == TokenType::NIL));
    }

    const Token* value;
    static constexpr auto main_token = &LiteralExpr::value;
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
    static constexpr auto main_token = &UnaryExpr::op;
};

struct VarExpr : ExprCRTC<VarExpr>
{
    constexpr
    VarExpr(const Token* identifier) noexcept
      : identifier{identifier}
    {
        assert(identifier && identifier->type() == TokenType::IDENTIFIER);
    }

    const Token* identifier;
    static constexpr auto main_token = &VarExpr::identifier;
};

struct AssignExpr : ExprCRTC<AssignExpr>
{
    constexpr
    AssignExpr(const Token* identifier, Expr* value) noexcept
      : identifier{identifier}
      , value{value}
    {
        assert(identifier && identifier->type() == TokenType::IDENTIFIER);
        assert(value);
    }

    const Token* identifier;
    Expr* value;
    static constexpr auto main_token = &AssignExpr::identifier;
};
