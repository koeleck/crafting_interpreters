#include "environment.hpp"


Environment::~Environment() = default;

Environment::Environment(std::shared_ptr<Environment> parent)
  : m_env{}
  , m_parent{std::move(parent)}
{}

void Environment::define(std::string_view name, Value&& value)
{
    m_env.insert_or_assign(name, std::move(value));
}

void Environment::define(std::string_view name, const Value& value)
{
    m_env.insert_or_assign(name, std::move(value));
}

bool Environment::assign(std::string_view name, Value&& value)
{
    Environment* env = this;
    while (env) {
        const auto it = env->m_env.find(name);
        if (it != env->m_env.end()) {
            it->second = std::move(value);
            return true;
        }
        env = env->parent().get();
    }
    return false;
}

bool Environment::assign(std::string_view name, const Value& value)
{
    Environment* env = this;
    while (env) {
        const auto it = env->m_env.find(name);
        if (it != env->m_env.end()) {
            it->second = std::move(value);
            return true;
        }
        env = env->parent().get();
    }
    return false;
}

const Value* Environment::get(std::string_view name) const noexcept
{
    const Environment* env = this;
    while (env) {
        const auto it = env->m_env.find(name);
        if (it != env->m_env.end()) {
            return &it->second;
        }
        env = env->parent().get();
    }
    return nullptr;
}

//

Globals::~Globals() = default;

Globals::Globals()
  : m_env{std::make_shared<Environment>(nullptr)}
{}

void Globals::open_scope()
{
    std::shared_ptr<Environment> new_env = std::make_shared<Environment>(m_env);
    m_env = std::move(new_env);
}

void Globals::close_scope()
{
    m_env = m_env->parent();
    assert(m_env.get() != nullptr);
}
