#pragma once

#include <variant>
#include <optional>
#include <vector>
#include <string>
#include "expr.hpp"

struct ScannerResult;

class Interpreter final : public Visitor
{
public:
    Interpreter(const ScannerResult& scanner_result);
    ~Interpreter();

    struct Nil {
        constexpr
        bool operator==(Nil) const noexcept { return true; }
    };
    using Value = std::variant<Nil, bool, std::string, double>;

    [[nodiscard]]
    std::optional<Value> evaluate(Expr& expr);

    void visit(BinaryExpr& binar_expr) override;
    void visit(GroupingExpr& grouping_expr) override;
    void visit(LiteralExpr& literal_expr) override;
    void visit(UnaryExpr& unary_expr) override;
    void unkown_expr(Expr& expr) override;

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
};
