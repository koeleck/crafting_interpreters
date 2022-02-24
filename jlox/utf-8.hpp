#pragma once

#include <cstdint>
#include <string_view>

struct UTF8Char
{
    uint32_t c{0};
    int32_t length{0}; // negative length for error
};

/**
 * Parse utf-8 character
 */
UTF8Char parse_utf8_char(std::string_view text) noexcept;
