#include "utf-8.hpp"

#include <doctest/doctest.h>

static
UTF8Char utf8decodewide(const uint32_t initial, const int8_t count, std::string_view text)
{
    uint32_t acc = initial;
    if (static_cast<size_t>(count) >= text.size()) {
        return {'\0', -1};
    }

    for (int8_t i = 1; i < count; ++i) {
        const uint8_t c = static_cast<uint8_t>(text[1]);
        if ((c & 0b11000000) != 0b10000000) {
            return {'\0', 1};
        }
        acc = (acc<<6) | (c & 0b00111111);
    }
    return {acc, count};
}


UTF8Char parse_utf8_char(std::string_view text) noexcept
{
    if (text.empty()) {
        return {'\0', 1};
    }

    const uint8_t first_char = static_cast<uint8_t>(text.front());
    if (!first_char) {
        return {'\0', 1};
    }
    if ((first_char & 0x80) == 0) {
        return {static_cast<uint32_t>(first_char & 0x7f), 1};
    }

    const int32_t num_octets = __builtin_clz(~(static_cast<uint32_t>(first_char)<<24));
    switch (num_octets) {
    case 2:
        return utf8decodewide(static_cast<uint32_t>(first_char & 0x1f), 2, text);
    case 3:
        return utf8decodewide(static_cast<uint32_t>(first_char & 0x0f), 3, text);
    case 4:
        return utf8decodewide(static_cast<uint32_t>(first_char & 0x07), 4, text);
    }
    return {'\0', -1};
}


TEST_CASE("UTF-8 decode")
{
    std::string_view lorem_ipsum = "Λοb";

    {
        const auto c = parse_utf8_char(lorem_ipsum);
        CHECK(c.length == 2);
        CHECK(c.c == 0x039b);
        lorem_ipsum.remove_prefix(c.length);
    }
    {
        const auto c = parse_utf8_char(lorem_ipsum);
        CHECK(c.length == 2);
        CHECK(c.c == 0x03bf);
        lorem_ipsum.remove_prefix(c.length);
    }
    {
        const auto c = parse_utf8_char(lorem_ipsum);
        CHECK(c.length == 1);
        CHECK(c.c == 'b');
        lorem_ipsum.remove_prefix(c.length);
    }

}
