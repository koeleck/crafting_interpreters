#pragma once

#include <vector>
#include <string_view>
#include <span>

#include "tokens.hpp"

struct OffsetToLine {
    int32_t offset;
    int32_t line;
};

struct Position
{
    int32_t line;
    int32_t column;

    auto operator<=>(const Position&) const = default;
};

struct ScannerResult
{
    std::string_view source;
    std::vector<Token> tokens;
    std::vector<OffsetToLine> offset_to_line;
};

Position offset_to_position(std::span<const OffsetToLine> map, int32_t offset);
ScannerResult scan_tokens(std::string_view source);
