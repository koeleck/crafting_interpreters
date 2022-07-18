#pragma once

#include <variant>
#include <span>
#include <cassert>
#include <string>
#include <functional>

#include "garbage_collected_heap.hpp"

class Expr;
class Interpreter;
class Callable;
class Environment;


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
    Callable(R (*fptr)(Args...), HeapPtr<Environment> environment)
      : Callable{}
    {
        m_env = std::move(environment);
        m_arity = sizeof...(Args);
        m_f = [fptr=fptr] (Interpreter&, const HeapPtr<Environment>&, std::span<const Value> args) -> Value {
            return invoke(fptr, args, std::make_index_sequence<sizeof...(Args)>{});
        };
    }

    template <typename F>
    explicit
    Callable(F&& fun, int32_t arity, HeapPtr<Environment> environment)
      : m_env{std::move(environment)}
      , m_f{std::forward<F>(fun)}
      , m_arity{arity}
    {}

    int32_t arity() const noexcept
    {
        return m_arity;
    }

    Value call(Interpreter& interpreter, std::span<const Value> args)
    {
        return m_f(interpreter, m_env, args);
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

    HeapPtr<Environment> m_env;
    std::function<Value(Interpreter&, const HeapPtr<Environment>&, std::span<const Value>)> m_f;
    int32_t m_arity{-1};
};
