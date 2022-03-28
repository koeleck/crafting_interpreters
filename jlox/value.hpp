#pragma once

#include <variant>
#include <span>
#include <string>
#include <functional>

struct Expr;
class Interpreter;
class Callable;

struct Nil
{
    constexpr
    bool operator==(Nil) const noexcept { return true; }
};

inline constexpr Nil nil;


using Value = std::variant<Nil, bool, std::string, double, Callable>;

class Callable
{
public:
    Callable() = default;

    int32_t arity() const noexcept
    {
        return m_arity;
    }

    Value call(Interpreter& interpreter, std::span<Value> args)
    {
        return m_f(interpreter, args);
    }

private:
    std::function<Value(Interpreter&, std::span<Value>)> m_f;
    int32_t m_arity{-1};
};
