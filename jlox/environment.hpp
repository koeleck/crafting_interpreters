#pragma once

#include <string>
#include <absl/container/flat_hash_map.h>

#include "value.hpp"


class Environment
{
public:
    Environment();
    ~Environment();

    void define(std::string_view name, Value&& value);

    void define(std::string_view name, const Value& value);

    const Value* get(std::string_view name) const noexcept;

    Value* get(std::string_view name) noexcept;

private:
    absl::flat_hash_map<std::string, Value> m_values;
};
