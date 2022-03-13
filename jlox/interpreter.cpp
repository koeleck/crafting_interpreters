#include "interpreter.hpp"

#include <cassert>
#include <cstdlib>
#include <stdexcept>

#include "log.hpp"
#include "scanner.hpp"

namespace
{

bool is_truthy(const Interpreter::Value& value) noexcept
{
    if (const bool* const b = std::get_if<bool>(&value)) {
        return *b;
    }
    if (const std::string* const s = std::get_if<std::string>(&value)) {
        return s->empty() == false;
    }
    if (const double* const d = std::get_if<double>(&value)) {
        return *d != 0.;
    }
    if (std::get_if<Interpreter::Nil>(&value)) {
        return false;
    }
    std::abort();
}

template <typename T>
bool is_equal_impl(const Interpreter::Value& lhs, const Interpreter::Value& rhs) noexcept
{
    const T* const l = std::get_if<T>(&lhs);
    if (!l) {
        return false;
    }
    const T* const r = std::get_if<T>(&rhs);
    if (!r) {
        return false;
    }
    return *l == *r;
}

bool is_equal(const Interpreter::Value& lhs, const Interpreter::Value& rhs) noexcept
{
#define CHECK(Type) if (const Type* const l = std::get_if<Type>(&lhs), * const r = std::get_if<Type>(&rhs); l && r) { return *l == *r; }
    CHECK(Interpreter::Nil)
    CHECK(bool)
    CHECK(std::string)
    CHECK(double)
#undef CHECK
    return false;
}

template <typename T>
constexpr
std::string_view type_name() noexcept;

template <>
constexpr
std::string_view type_name<double>() noexcept
{
    return "number";
}

template <>
constexpr
std::string_view type_name<std::string>() noexcept
{
    return "string";
}

template <>
constexpr
std::string_view type_name<Interpreter::Nil>() noexcept
{
    return "nil";
}

template <>
constexpr
std::string_view type_name<bool>() noexcept
{
    return "boolean";
}

constexpr
std::string_view type_name(const Interpreter::Value& value) noexcept
{
    if (std::get_if<std::string>(&value)) {
        return type_name<std::string>();
    }
    if (std::get_if<double>(&value)) {
        return type_name<double>();
    }
    if (std::get_if<bool>(&value)) {
        return type_name<bool>();
    }
    if (std::get_if<Interpreter::Nil>(&value)) {
        return type_name<Interpreter::Nil>();
    }
    std::abort();
}

class InterpreterError final : public std::runtime_error
{
public:
    InterpreterError() : std::runtime_error{""} {}
};


template <typename T>
void check_operand_type(const Interpreter::Value& value,
                        const Token* token,
                        const ScannerResult& scanner_result)
{
    if (nullptr == std::get_if<T>(&value)) {
        const std::string_view got = type_name(value);
        const std::string_view expct = type_name<T>();

        report_error(scanner_result, *token, "Expected operand of type {}, got {}.", expct, got);

        throw InterpreterError{};
    }
}


std::string stringify(const Interpreter::Value& value)
{
    if (std::get_if<Interpreter::Nil>(&value)) {
        return "nil";
    } else if (const double* const d = std::get_if<double>(&value)) {
        return fmt::to_string(*d);
    } else if (const std::string* const s = std::get_if<std::string>(&value)) {
        return *s;
    } else if (const bool* const b = std::get_if<bool>(&value)) {
        return (*b ? "true\n" : "false\n");
    } else {
        std::abort();
    }
}

} // anonymous namespace


Interpreter::~Interpreter() = default;

Interpreter::Interpreter(const ScannerResult& scanner_result)
  : m_scanner_result{scanner_result}
{}

std::optional<Interpreter::Value> Interpreter::execute(Stmt& stmt)
{
    stmt.accept(*this);
    assert(m_stack.empty() == false);
    Value result = std::move(m_stack.back());
    m_stack.pop_back();
    return result;
}

std::optional<Interpreter::Value> Interpreter::evaluate(Expr& expr)
{
    const size_t stack_size = m_stack.size();
    try {
        return evaluate_impl(expr);
    } catch (const InterpreterError&) {
        m_stack.resize(stack_size);
        return {};
    } catch (...) {
        m_stack.resize(stack_size);
        throw;
    }
}

Interpreter::Value Interpreter::evaluate_impl(Expr& expr)
{
    evaluate_impl_nopop(expr);
    Value result = std::move(m_stack.back());
    m_stack.pop_back();
    return result;
}

void Interpreter::evaluate_impl_nopop(Expr& expr)
{
    const size_t prev_stack_size = m_stack.size();
    expr.accept(*this);

    const size_t stack_size = m_stack.size();
    if (stack_size != (prev_stack_size + 1)) {
        // TODO:
        std::abort();
    }
}

void Interpreter::visit(BinaryExpr& binar_expr)
{
    Value lhs = evaluate_impl(*binar_expr.left);
    Value rhs = evaluate_impl(*binar_expr.right);

    switch (binar_expr.op->type()) {
    using enum TokenType;
    case PLUS:
        if (const double* const ld = std::get_if<double>(&lhs), * const rd = std::get_if<double>(&rhs);
            ld && rd)
        {
            m_stack.emplace_back(*ld + *rd);
        } else if (const std::string* const ls = std::get_if<std::string>(&lhs),
                   * const rs = std::get_if<std::string>(&rhs);
                   ls && rs)
        {
            m_stack.emplace_back(*ls + *rs);
        } else {
            const std::string_view ltype = type_name(lhs);
            const std::string_view rtype = type_name(rhs);
            report_error(m_scanner_result, *binar_expr.op, "Operands to (+) must be two numbers or two strings. Got {} and {}.",
                         ltype, rtype);
            throw InterpreterError{};
        }
        break;

    case MINUS:
        check_operand_type<double>(lhs, rhs, &binar_expr);
        m_stack.push_back(std::get<double>(lhs) - std::get<double>(rhs));
        break;

    case SLASH:
        check_operand_type<double>(lhs, rhs, &binar_expr);
        m_stack.push_back(std::get<double>(lhs) / std::get<double>(rhs));
        break;

    case STAR:
        check_operand_type<double>(lhs, rhs, &binar_expr);
        m_stack.push_back(std::get<double>(lhs) * std::get<double>(rhs));
        break;

    case EQUAL_EQUAL:
        m_stack.push_back(is_equal(lhs, rhs));
        break;

    case BANG_EQUAL:
        m_stack.push_back(!is_equal(lhs, rhs));
        break;

    case GREATER:
        check_operand_type<double>(lhs, rhs, &binar_expr);
        m_stack.push_back(std::get<double>(lhs) > std::get<double>(rhs));
        break;
    case GREATER_EQUAL:
        check_operand_type<double>(lhs, rhs, &binar_expr);
        m_stack.push_back(std::get<double>(lhs) >= std::get<double>(rhs));
        break;

    case LESS:
        check_operand_type<double>(lhs, rhs, &binar_expr);
        m_stack.push_back(std::get<double>(lhs) < std::get<double>(rhs));
        break;

    case LESS_EQUAL:
        check_operand_type<double>(lhs, rhs, &binar_expr);
        m_stack.push_back(std::get<double>(lhs) <= std::get<double>(rhs));
        break;

    default:
        std::abort();
    }
}

void Interpreter::visit(GroupingExpr& grouping_expr)
{
    evaluate_impl_nopop(*grouping_expr.expr);
}

void Interpreter::visit(LiteralExpr& literal_expr)
{
    assert(literal_expr.value != nullptr);

    std::string_view lexeme = literal_expr.value->lexeme(m_scanner_result.source);

    switch (literal_expr.value->type()) {
    using enum TokenType;
    case STRING:
        assert(lexeme.size() >= 2);
        m_stack.emplace_back(std::in_place_type_t<std::string>{}, lexeme.substr(1, lexeme.size() - 2));
        break;

    case NUMBER:
        assert(lexeme.size() >= 1);
        m_stack.emplace_back(std::strtod(lexeme.data(), nullptr));
        break;

    case TRUE:
        m_stack.emplace_back(true);
        break;

    case FALSE:
        m_stack.emplace_back(false);
        break;

    case NIL:
        m_stack.emplace_back(Nil{});
        break;

    case IDENTIFIER:
        report_error(m_scanner_result, *literal_expr.get_main_token(), "Identifiers not implemented yet");
        throw InterpreterError{};

    default:
        std::abort();
    }
}

void Interpreter::visit(UnaryExpr& unary_expr)
{
    Value val = evaluate_impl(*unary_expr.right);

    switch (unary_expr.op->type()) {
    using enum TokenType;
    case MINUS:
        check_operand_type<double>(val, unary_expr.right);
        m_stack.emplace_back(-std::get<double>(val));
        break;

    case BANG:
        m_stack.emplace_back(!is_truthy(val));
        break;

    default:
        std::abort();
    }
}

void Interpreter::unkown_expr(Expr& expr)
{
    static_cast<void>(expr);
    std::abort();
}


void Interpreter::visit(ExprStmt& expr_stmt)
{
    assert(expr_stmt.expr);
    evaluate_impl_nopop(*expr_stmt.expr);
}


void Interpreter::visit(PrintStmt& print_stmt)
{
    assert(print_stmt.expr);
    evaluate_impl_nopop(*print_stmt.expr);

    assert(m_stack.empty() == false);
    if (std::get_if<std::string>(&m_stack.back()) == nullptr) {
        Value value = std::move(m_stack.back());
        m_stack.pop_back();

        m_stack.emplace_back(stringify(value));
    }

    const std::string* s = std::get_if<std::string>(&m_stack.back());
    fmt::print(" :: {}\n", *s);
}

void Interpreter::unkown_stmt(Stmt&)
{
    std::abort();
}

template <typename T>
void Interpreter::check_operand_type(const Value& value, const Expr* expr) const
{
    ::check_operand_type<T>(value, expr->get_main_token(), m_scanner_result);
}

template <typename T>
void Interpreter::check_operand_type(const Value& lhs, const Value& rhs, const BinaryExpr* expr) const
{
    ::check_operand_type<T>(lhs, expr->left->get_main_token(), m_scanner_result);
    ::check_operand_type<T>(rhs, expr->right->get_main_token(), m_scanner_result);
}
