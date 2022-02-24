#pragma once

#include <vector>
#include <string_view>
#include <span>

#include "tokens.hpp"

struct Position
{
    int32_t line;
    int32_t column;

    auto operator<=>(const Position&) const = default;
};

class OffsetToLine
{
public:
    explicit
    OffsetToLine(std::vector<int32_t> offsets) noexcept
      : m_offsets{std::move(offsets)}
    {}

    Position get_position(int32_t offset) const noexcept;

    int32_t num_lines() const noexcept
    {
        return static_cast<int32_t>(m_offsets.size());
    }

    int32_t get_offset(int32_t line) const noexcept;

private:
    std::vector<int32_t> m_offsets;
};

struct ScannerResult
{
    std::string_view source;
    std::vector<Token> tokens;
    OffsetToLine offsets;
    int32_t num_errors{0};
};

ScannerResult scan_tokens(std::string_view source);
