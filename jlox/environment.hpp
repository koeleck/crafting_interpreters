#pragma once

#include <string>
#include <vector>
#include <absl/container/flat_hash_map.h>

#include "value.hpp"


class Environment
{
public:
    Environment();
    ~Environment();

    void define(std::string_view name, Value&& value);
    void define(std::string_view name, const Value& value);

    bool assign(std::string_view name, Value&& value);
    bool assign(std::string_view name, const Value& value);

    const Value* get(std::string_view name) const noexcept;

    void open_scope();
    void close_scope();

private:
    using Map = absl::flat_hash_map<std::string, Value>;
    std::vector<Map> m_scopes;
};


class NewScope
{
public:
    [[nodiscard]]
    NewScope(Environment& env) noexcept
      : m_env{env}
    {
        m_env.open_scope();
    }

    ~NewScope()
    {
        m_env.close_scope();
    }

    NewScope(const NewScope&) = delete;
    NewScope(NewScope&&) = delete;
    NewScope& operator=(const NewScope&) = delete;
    NewScope& operator=(NewScope&&) = delete;
private:
    Environment& m_env;
};
