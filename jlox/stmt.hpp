#pragma once

#include <cassert>
#include "tokens.hpp"

class Expr;
class Stmt;
struct ExprStmt;
struct PrintStmt;
struct VarStmt;

class StmtVisitor
{
public:
    virtual ~StmtVisitor() = default;

    virtual void visit(ExprStmt& expr_stmt) = 0;
    virtual void visit(PrintStmt& print_stmt) = 0;
    virtual void visit(VarStmt& var_stmt) = 0;
    virtual void unkown_stmt(Stmt& stmt) = 0;

    void visit(Stmt& stmt) {
        unkown_stmt(stmt);
    }
};

template <typename T>
class StmtCRTC;

class Stmt
{
public:
    Stmt() = delete;

    void accept(StmtVisitor& visitor)
    {
        m_accept(*this, visitor);
    }

    template <typename T>
    bool is_type() const noexcept
    {
        using TBase = StmtCRTC<T>;
        return m_accept == &TBase::accept_impl;
    }

protected:
    template <typename T>
    friend class StmtCRTC;

    constexpr
    Stmt(void (*accept)(Stmt&, StmtVisitor&)) noexcept
      : m_accept{accept}
    {
        assert(m_accept);
    }

private:
    void (*m_accept)(Stmt&, StmtVisitor&);
};

template <typename T>
class StmtCRTC : public Stmt
{
protected:
    constexpr
    StmtCRTC() noexcept
      : Stmt{&StmtCRTC::accept_impl}
    {}

private:
    friend class Stmt;

    static
    void accept_impl(Stmt& e, StmtVisitor& visitor)
    {
        static_assert(std::is_base_of_v<StmtCRTC<T>, T>);
        visitor.visit(static_cast<T&>(e));
    }
};

struct ExprStmt : StmtCRTC<ExprStmt>
{
    constexpr
    ExprStmt(Expr* expr) noexcept
      : expr{expr}
    {
        assert(expr);
    }

    Expr* expr{nullptr};
};

struct PrintStmt : StmtCRTC<PrintStmt>
{
    constexpr
    PrintStmt(Expr* expr) noexcept
      : expr{expr}
    {
        assert(expr);
    }

    Expr* expr{nullptr};
};

struct VarStmt : StmtCRTC<VarStmt>
{
    constexpr
    VarStmt(const Token* identifier, Expr* expr) noexcept
      : identifier{identifier}
      , expr{expr}
    {
        assert(identifier && identifier->type() == TokenType::IDENTIFIER);
    }

    const Token* identifier{nullptr};
    Expr* expr{nullptr};
};
