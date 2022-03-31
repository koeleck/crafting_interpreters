#pragma once

#include <cassert>
#include <vector>
#include "tokens.hpp"

class Expr;
class Stmt;
struct ExprStmt;
struct PrintStmt;
struct VarStmt;
struct FunStmt;
struct BlockStmt;
struct IfStmt;
struct WhileStmt;

class StmtVisitor
{
public:
    virtual ~StmtVisitor() = default;

    virtual void visit(ExprStmt& expr_stmt) = 0;
    virtual void visit(PrintStmt& print_stmt) = 0;
    virtual void visit(VarStmt& var_stmt) = 0;
    virtual void visit(BlockStmt& block_stmt) = 0;
    virtual void visit(IfStmt& if_stmt) = 0;
    virtual void visit(WhileStmt& while_stmt) = 0;
    virtual void visit(FunStmt& fun_stmt) = 0;
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
    VarStmt(const Token* identifier, Expr* initializer) noexcept
      : identifier{identifier}
      , initializer{initializer}
    {
        assert(identifier && identifier->type() == TokenType::IDENTIFIER);
    }

    const Token* identifier{nullptr};
    Expr* initializer{nullptr};
};

struct BlockStmt : StmtCRTC<BlockStmt>
{
    BlockStmt(std::vector<Stmt*> statements) noexcept
      : statements{std::move(statements)}
    {}

    std::vector<Stmt*> statements;
};

struct IfStmt : StmtCRTC<IfStmt>
{
    constexpr
    IfStmt(Expr* condition, Stmt* then_branch, Stmt* else_branch) noexcept
      : condition{condition}
      , then_branch{then_branch}
      , else_branch{else_branch}
    {
        assert(condition && then_branch);
    }

    constexpr
    IfStmt(Expr* condition, Stmt* then_branch) noexcept
      : IfStmt{condition, then_branch, nullptr}
    {}

    Expr* condition;
    Stmt* then_branch;
    Stmt* else_branch;
};


struct WhileStmt : StmtCRTC<WhileStmt>
{
    constexpr
    WhileStmt(Expr* condition, Stmt* body) noexcept
      : condition{condition}
      , body{body}
    {
        assert(condition && body);
    }

    Expr* condition;
    Stmt* body;
};

struct FunStmt : StmtCRTC<FunStmt>
{
    FunStmt(const Token* name, std::vector<const Token*> params,
            std::vector<Stmt*> body) noexcept
      : name{name}
      , params{std::move(params)}
      , body{std::move(body)}
    {
        assert(name);
    }

    const Token* name;
    std::vector<const Token*> params;
    std::vector<Stmt*> body;
};
