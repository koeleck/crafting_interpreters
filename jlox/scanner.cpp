#include "scanner.hpp"

#include <algorithm>
#include <cassert>

#include <doctest/doctest.h>

Position offset_to_position(std::span<const OffsetToLine> map, int32_t offset)
{
    assert(offset >= 0);
    assert(map.size() >= 1);

    auto it = std::lower_bound(map.begin(), map.end(), offset,
            [] (const OffsetToLine& otl, int32_t offs) { return otl.offset < offs; });

    if (it != map.end() && it->offset == offset) {
        return {it->line, 1};
    }
    --it;
    return {it->line, 1 + (offset - it->offset)};
}

TEST_CASE("Position from offset")
{
    std::string_view source{"Hello world\n"  // 12 characters
                            "1234567890"     // 10 characters
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"}; // 26 characters
    const OffsetToLine otl[3]{
        {0, 1},
        {12, 2},
        {22, 3}
    };

    CHECK(offset_to_position(otl, 0) == Position{1, 1});
    CHECK(offset_to_position(otl, 11) == Position{1, 12});
    CHECK(offset_to_position(otl, 12) == Position{2, 1});
    CHECK(offset_to_position(otl, 25) == Position{3, 4});
}


ScannerResult scan_tokens(std::string_view source)
{
    ScannerResult result;
    // TODO
    return result;
}
