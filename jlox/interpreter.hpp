#pragma once

#include <optional>
#include <vector>
#include <string>

#include "expr.hpp"
#include "stmt.hpp"
#include "value.hpp"
#include "environment.hpp"

struct ScannerResult;

class Interpreter final : public ExprVisitor
                        , public StmtVisitor
{
public:
    Interpreter(const ScannerResult& scanner_result, Environment& environment);
    ~Interpreter();

    [[nodiscard]]
    std::optional<Value> execute(Stmt& stmt);

    void visit(BinaryExpr& binar_expr) override;
    void visit(GroupingExpr& grouping_expr) override;
    void visit(LiteralExpr& literal_expr) override;
    void visit(UnaryExpr& unary_expr) override;
    void visit(VarExpr& var_expr) override;
    void unkown_expr(Expr& expr) override;

    void visit(ExprStmt& expr_stmt) override;
    void visit(PrintStmt& expr_stmt) override;
    void visit(VarStmt& var_stmt) override;
    void unkown_stmt(Stmt& stmt) override;

private:
    [[nodiscard]]
    Value evaluate_impl(Expr& expr);

    void evaluate_impl_nopop(Expr& expr);

    template <typename T>
    void check_operand_type(const Value&, const Expr*) const;

    template <typename T>
    void check_operand_type(const Value&, const Value&, const BinaryExpr*) const;

    std::vector<Value> m_stack;
    const ScannerResult& m_scanner_result;
    Environment& m_env;
};
