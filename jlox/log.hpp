#pragma once

#include <string_view>

#include <fmt/format.h>
#include <fmt/color.h>

class Token;
struct ScannerResult;

#define LOG_ERROR(message, ...) ::fmt::print(::stderr, fg(::fmt::color::red), FMT_STRING(message "\n") __VA_OPT__(,) __VA_ARGS__)

namespace detail
{

void report_error_begin(int line, int column);
void report_error_end(int line, int column, std::string_view line_content);

struct ErrorContext
{
    int line;
    int column;
    std::string_view line_content;
};

ErrorContext get_context(const ScannerResult& scanner_result, const Token& token) noexcept;

} // namespace detail

/**
 * Report error in source code.
 * \param line Line number (>= 1).
 * \param column Column number. Set to zero if not applicable.
 * \param line_content Content of line.
 * \param format_str Error message as fmt string.
 * \param args Additional arguments to format string.
 */
template <typename Fmt, typename... Args>
inline
void report_error(int line, int column, std::string_view line_content, Fmt&& format_str, Args&&... args)
{
    detail::report_error_begin(line, column);
    fmt::print(stderr, fg(fmt::color::red), std::forward<Fmt>(format_str), std::forward<Args>(args)...);
    detail::report_error_end(line, column, line_content);
}


template <typename Fmt, typename... Args>
inline
void report_error(const ScannerResult& scanner_result, const Token& token, Fmt&& format_str, Args&&... args)
{
    const detail::ErrorContext ctx = detail::get_context(scanner_result, token);
    report_error(ctx.line, ctx.column, ctx.line_content, std::forward<Fmt>(format_str), std::forward<Args>(args)...);
}
