#pragma once

#include <variant>
#include <string>

struct Nil
{
    constexpr
    bool operator==(Nil) const noexcept { return true; }
};

inline constexpr Nil nil;

using Value = std::variant<Nil, bool, std::string, double>;
