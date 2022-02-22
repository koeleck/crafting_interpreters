#include "log.hpp"

#include <cassert>

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

} // namespace detail
