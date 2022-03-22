#include "environment.hpp"


Environment::~Environment() = default;

Environment::Environment()
{
    m_scopes.emplace_back();
}

void Environment::define(std::string_view name, Value&& value)
{
    m_scopes.back().insert_or_assign(name, std::move(value));
}

void Environment::define(std::string_view name, const Value& value)
{
    m_scopes.back().insert_or_assign(name, std::move(value));
}

bool Environment::assign(std::string_view name, Value&& value)
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto& scope = *it;
        auto val = scope.find(name);
        if (val != scope.end()) {
            val->second = std::move(value);
            return true;
        }
    }
    return false;
}

bool Environment::assign(std::string_view name, const Value& value)
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto& scope = *it;
        auto val = scope.find(name);
        if (val != scope.end()) {
            val->second = value;
            return true;
        }
    }
    return false;
}

const Value* Environment::get(std::string_view name) const noexcept
{
    for (auto it = m_scopes.rbegin(); it != m_scopes.rend(); ++it) {
        auto& scope = *it;
        auto val = scope.find(name);
        if (val != scope.end()) {
            return &val->second;
        }
    }
    return nullptr;
}

void Environment::open_scope()
{
    m_scopes.emplace_back();
}

void Environment::close_scope()
{
    m_scopes.pop_back();
    assert(!m_scopes.empty());
}
