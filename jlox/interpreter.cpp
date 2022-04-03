#include "interpreter.hpp"

#include <cassert>
#include <cstdlib>
#include <chrono>
#include <stdexcept>

#include "log.hpp"
#include "scanner.hpp"

namespace
{

bool is_truthy(const Value& value) noexcept
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
    if (std::get_if<Nil>(&value)) {
        return false;
    }
    std::abort();
}

template <typename T>
bool is_equal_impl(const Value& lhs, const Value& rhs) noexcept
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

bool is_equal(const Value& lhs, const Value& rhs) noexcept
{
#define CHECK(Type) if (const Type* const l = std::get_if<Type>(&lhs), * const r = std::get_if<Type>(&rhs); l && r) { return *l == *r; }
    CHECK(Nil)
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
std::string_view type_name<Nil>() noexcept
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
std::string_view type_name(const Value& value) noexcept
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
    if (std::get_if<Nil>(&value)) {
        return type_name<Nil>();
    }
    std::abort();
}

class InterpreterError final : public std::runtime_error
{
public:
    InterpreterError() : std::runtime_error{""} {}
};


template <typename T>
void check_operand_type(const Value& value,
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


std::string stringify(const Value& value)
{
    if (std::get_if<Nil>(&value)) {
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

double clock_impl()
{
    const auto now = std::chrono::steady_clock::now();
    const int64_t now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    return static_cast<double>(now_us) / 1000000.0;
}

} // anonymous namespace


Interpreter::~Interpreter() = default;

Interpreter::Interpreter(const ScannerResult& scanner_result, Environment& environment)
  : m_scanner_result{scanner_result}
  , m_env{environment}
{
    m_env.define("clock", Callable{&clock_impl});
}

bool Interpreter::execute(Stmt& stmt)
{
    assert(m_stack.empty() == true);
    try {
        const bool do_return = stmt.accept(*this);
        if (do_return) {
            m_stack.pop_back();
        }
    } catch (const InterpreterError&) {
        assert(m_stack.empty() == true);
        return false;
    }
    assert(m_stack.empty() == true);
    return true;
}

Value Interpreter::evaluate_impl(Expr& expr)
{
    evaluate_impl_nopop(expr);
    Value result = std::move(m_stack.back());
    m_stack.pop_back();
    return result;
}

void Interpreter::evaluate_impl_nopop(Expr& expr)
{
    const size_t prev_stack_size = m_stack.size();

    try {
        expr.accept(*this);
    } catch (...) {
        m_stack.resize(prev_stack_size);
        throw;
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
        // TODO: Handle 0xafd, 0b111, 0777...
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


void Interpreter::visit(VarExpr& var_expr)
{
    const Value* const value = m_env.get(var_expr.identifier->lexeme(m_scanner_result.source));
    if (!value) {
        report_error(m_scanner_result, *var_expr.identifier, "Identifier not found");
        throw InterpreterError{};
    }
    m_stack.push_back(*value);
}

void Interpreter::visit(AssignExpr& assign_expr)
{
    const std::string_view name = assign_expr.identifier->lexeme(m_scanner_result.source);

    evaluate_impl_nopop(*assign_expr.value);

    const bool result = m_env.assign(name, std::move(m_stack.back()));
    m_stack.pop_back();

    if (!result) {
        report_error(m_scanner_result, *assign_expr.identifier, "Undefined variable '{}'.", name);
        throw InterpreterError{};
    }

}

void Interpreter::visit(LogicalExpr& logical_expr)
{
    const bool left = is_truthy(evaluate_impl(*logical_expr.left));
    const bool is_and = logical_expr.token->type() == TokenType::AND;
    if (is_and && !left) {
        m_stack.push_back(false);
        return;
    } else if (!is_and && left) {
        m_stack.push_back(true);
        return;
    }

    const bool right = is_truthy(evaluate_impl(*logical_expr.right));
    m_stack.push_back(right);
}

void Interpreter::visit(CallExpr& call_expr)
{
    Value callee = evaluate_impl(*call_expr.callee);

    Callable* const callable_callee = std::get_if<Callable>(&callee);
    if (!callable_callee) {
        report_error(m_scanner_result, *call_expr.callee->get_main_token(),
                     "Value not callable");
        throw InterpreterError{};
    }
    if (static_cast<int32_t>(call_expr.args.size()) != callable_callee->arity()) {
        report_error(m_scanner_result, *call_expr.callee->get_main_token(),
                     "Expected {} arguments but got {}.", callable_callee->arity(),
                     call_expr.args.size());
        throw InterpreterError{};
    }


    std::vector<Value> args;
    args.reserve(call_expr.args.size());
    for (Expr* arg : call_expr.args) {
        args.push_back(evaluate_impl(*arg));
    }

    m_stack.push_back(callable_callee->call(*this, args));
}

void Interpreter::unkown_expr(Expr& expr)
{
    static_cast<void>(expr);
    std::abort();
}


bool Interpreter::visit(ExprStmt& expr_stmt)
{
    assert(expr_stmt.expr);
    evaluate_impl_nopop(*expr_stmt.expr);
    m_stack.clear();
    return false;
}


bool Interpreter::visit(PrintStmt& print_stmt)
{
    assert(print_stmt.expr);
    evaluate_impl_nopop(*print_stmt.expr);

    assert(m_stack.empty() == false);

    std::string tmp;
    std::string* sptr = std::get_if<std::string>(&m_stack.back());
    if (!sptr) {
        Value value = std::move(m_stack.back());
        tmp = stringify(value);
        sptr = &tmp;
    }
    fmt::print(" :: {}\n", *sptr);
    m_stack.pop_back();

    return false;
}


bool Interpreter::visit(VarStmt& var_stmt)
{
    Value val{nil};
    if (var_stmt.initializer) {
        val = evaluate_impl(*var_stmt.initializer);
    }
    m_env.define(var_stmt.identifier->lexeme(m_scanner_result.source),
                 std::move(val));

    return false;
}

bool Interpreter::visit(BlockStmt& block_stmt)
{
    NewScope new_scope(m_env);

    for (Stmt* stmt : block_stmt.statements) {
        const bool do_return = stmt->accept(*this);
        if (do_return) {
            return true;
        }
    }
    return false;
}

bool Interpreter::visit(IfStmt& if_stmt)
{
    if (is_truthy(evaluate_impl(*if_stmt.condition))) {
        return if_stmt.then_branch->accept(*this);
    } else if (if_stmt.else_branch) {
        return if_stmt.else_branch->accept(*this);
    }
    return false;
}

bool Interpreter::visit(WhileStmt& while_stmt)
{
    while (is_truthy(evaluate_impl(*while_stmt.condition))) {
        const bool do_return = while_stmt.body->accept(*this);
        if (do_return) {
            return true;
        }
    }
    return false;
}

bool Interpreter::visit(FunStmt& fun_stmt)
{
    const int32_t arity = static_cast<int32_t>(fun_stmt.params.size());
    auto f = [params=std::move(fun_stmt.params), body=std::move(fun_stmt.body)] (Interpreter& interpreter, std::span<const Value> args) -> Value {
        assert(params.size() == args.size());
        NewScope new_scope(interpreter.m_env);
        for (size_t i = 0, count = params.size(); i != count; ++i) {
            interpreter.m_env.define(params[i]->lexeme(interpreter.m_scanner_result.source), args[i]);
        }
        bool has_return_value = false;
        for (Stmt* stmt : body) {
            if (stmt->accept(interpreter)) {
                has_return_value = true;
                break;
            }

        }
        if (has_return_value) {
            Value return_value = std::move(interpreter.m_stack.back());
            interpreter.m_stack.pop_back();
            return return_value;
        }
        return nil;
    };

    m_env.define(fun_stmt.name->lexeme(m_scanner_result.source), Callable{std::move(f), arity});

    return false;
}

bool Interpreter::visit(ReturnStmt& return_stmt)
{
    if (return_stmt.expr) {
        evaluate_impl_nopop(*return_stmt.expr);
    } else {
        m_stack.emplace_back(nil);
    }
    return true;
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
