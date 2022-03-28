#pragma once

#include <variant>
#include <span>
#include <cassert>
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

    template <typename R, typename... Args>
    explicit constexpr
    Callable(R (*fptr)(Args...))
      : Callable{}
    {
        m_arity = sizeof...(Args);
        m_f = [fptr=fptr] (Interpreter&, std::span<const Value> args) -> Value {
            return invoke(fptr, args, std::make_index_sequence<sizeof...(Args)>{});
        };
    }

    int32_t arity() const noexcept
    {
        return m_arity;
    }

    Value call(Interpreter& interpreter, std::span<const Value> args)
    {
        return m_f(interpreter, args);
    }

private:
    template <typename R, typename... Args, size_t... Indices>
    static constexpr
    R invoke(R(*fptr)(Args...), std::span<const Value> args, std::index_sequence<Indices...>)
    {
        assert(args.size() == sizeof...(Indices));
        assert(fptr);
        // TODO: Check parameter types, throw custom exception
        return (*fptr)(std::get<std::remove_cvref_t<Args>>(args[Indices])...);
    }

    std::function<Value(Interpreter&, std::span<const Value>)> m_f;
    int32_t m_arity{-1};
};
