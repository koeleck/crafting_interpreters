#pragma once

#include <string>
#include <vector>
#include <memory>
#include <absl/container/flat_hash_map.h>

#include "value.hpp"


class Environment : public std::enable_shared_from_this<Environment>
{
public:
    Environment(std::shared_ptr<Environment> parent);
    ~Environment();

    void define(std::string_view name, Value&& value);
    void define(std::string_view name, const Value& value);

    bool assign(std::string_view name, Value&& value);
    bool assign(std::string_view name, const Value& value);

    const Value* get(std::string_view name) const noexcept;

    const std::shared_ptr<Environment> parent() const noexcept
    {
        return m_parent;
    }

private:
    using Map = absl::flat_hash_map<std::string, Value>;

    Map m_env;
    std::shared_ptr<Environment> m_parent;
};

class Globals
{
public:
    Globals();
    ~Globals();

    void open_scope();
    void close_scope();

    const std::shared_ptr<Environment>& environment() const noexcept
    {
        return m_env;
    }

    std::shared_ptr<Environment> exchange(std::shared_ptr<Environment> new_env)
    {
        std::shared_ptr<Environment> prev{std::move(m_env)};
        m_env = std::move(new_env);
        return prev;
    }

private:
    std::shared_ptr<Environment> m_env;
};


class NewScope
{
public:
    [[nodiscard]]
    NewScope(Globals& globals) noexcept
      : m_globals{globals}
    {
        m_globals.open_scope();
    }

    ~NewScope()
    {
        m_globals.close_scope();
    }

    NewScope(const NewScope&) = delete;
    NewScope(NewScope&&) = delete;
    NewScope& operator=(const NewScope&) = delete;
    NewScope& operator=(NewScope&&) = delete;
private:
    Globals& m_globals;
};

class AdjustedEnvironment
{
public:
    AdjustedEnvironment(Globals& globals, std::shared_ptr<Environment> new_env)
      : m_globals{globals}
      , m_prev{m_globals.exchange(std::move(new_env))}
    {}

    ~AdjustedEnvironment()
    {
        m_globals.exchange(std::move(m_prev));
    }

    AdjustedEnvironment(const AdjustedEnvironment&) = delete;
    AdjustedEnvironment(AdjustedEnvironment&&) = delete;
    AdjustedEnvironment& operator=(const AdjustedEnvironment&) = delete;
    AdjustedEnvironment& operator=(AdjustedEnvironment&&) = delete;
private:
    Globals& m_globals;
    std::shared_ptr<Environment> m_prev;
};
