#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <absl/hash/hash.h>

#include "value.hpp"
#include "garbage_collected_heap.hpp"

struct StringHash {
    using is_transparent = void;
    [[nodiscard]] size_t operator()(const char *txt) const {
        return std::hash<std::string_view>{}(txt);
    }
    [[nodiscard]] size_t operator()(std::string_view txt) const {
        return std::hash<std::string_view>{}(txt);
    }
    [[nodiscard]] size_t operator()(const std::string &txt) const {
        return std::hash<std::string>{}(txt);
    }
};

class Environment
{
public:
    Environment(HeapPtr<Environment> parent);
    ~Environment();

    void define(std::string_view name, Value&& value);
    void define(std::string_view name, const Value& value);

    bool assign(std::string_view name, Value&& value);
    bool assign(std::string_view name, const Value& value);

    const Value* get(std::string_view name) const noexcept;

    const HeapPtr<Environment>& parent() const noexcept
    {
        return m_parent;
    }

private:
    using Map = std::unordered_map<std::string, Value,
            StringHash,
            std::equal_to<>,
            GarbageCollectedAllocator<std::pair<const std::string, Value>>>;
    /*
    using Map = absl::flat_hash_map<std::string, Value,
          absl::container_internal::hash_default_hash<std::string>,
          absl::container_internal::hash_default_eq<std::string>,
          GarbageCollectedAllocator<std::pair<const std::string, Value>>>;
    */

    Map m_env;
    HeapPtr<Environment> m_parent;
};

class Globals
{
public:
    Globals();
    ~Globals();

    void open_scope();
    void close_scope();

    const HeapPtr<Environment>& environment() const noexcept
    {
        return m_env;
    }

    HeapPtr<Environment> exchange(HeapPtr<Environment> new_env)
    {
        HeapPtr<Environment> prev{std::move(m_env)};
        m_env = std::move(new_env);
        return prev;
    }

private:
    HeapPtr<Environment> m_env;
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
    AdjustedEnvironment(Globals& globals, HeapPtr<Environment> new_env)
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
    HeapPtr<Environment> m_prev;
};
