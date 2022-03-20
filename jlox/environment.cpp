#include "environment.hpp"


Environment::Environment() = default;
Environment::~Environment() = default;

void Environment::define(std::string_view name, Value&& value)
{
    m_values.insert_or_assign(name, std::move(value));
}

void Environment::define(std::string_view name, const Value& value)
{
    m_values.insert_or_assign(name, value);
}

const Value* Environment::get(std::string_view name) const noexcept
{
    const auto it = m_values.find(name);
    if (it == m_values.end()) {
        return nullptr;
    }
    return &it->second;
}
