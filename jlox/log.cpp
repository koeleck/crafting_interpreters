#include "log.hpp"

#include <cassert>

#include "scanner.hpp"

namespace detail
{

void report_error_begin(const int line,
                        const int column)
{
    assert(line > 0 && column >= 0);

    if (column > 0) {
        fmt::print(stderr, fg(fmt::color::red), FMT_STRING("[{}:{}] Error: "), line, column);
    } else {
        fmt::print(stderr, fg(fmt::color::red), FMT_STRING("[{}] Error: "), line);
    }
}

void report_error_end(const int line,
                      const int column,
                      std::string_view line_content)
{
    assert(column >= 0);
    fmt::print(stderr, fg(fmt::color::orange), FMT_STRING("\n\n {:5d} | {}\n"), line, line_content);
    if (column > 0) {
        fwrite("         ", 9, 1, stderr);
        for (int i = 0; i < column - 1; ++i) {
            fwrite(" ", 1, 1, stderr);
        }
        fmt::print(stderr, fg(fmt::color::cyan), FMT_STRING("^--- Here.\n"));
    }
}

ErrorContext get_context(const ScannerResult& scanner_result, const Token& token) noexcept
{
    const Position pos = scanner_result.offsets.get_position(token.offset());
    const std::string_view line = get_line_from_offset(scanner_result.source, scanner_result.offsets.get_offset(pos.line));

    return {pos.line, pos.column, line};
}

} // namespace detail
