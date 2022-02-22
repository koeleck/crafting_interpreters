#pragma once

#include <fmt/format.h>
#include <fmt/color.h>


#define LOG_ERROR(message, ...) ::fmt::print(::stderr, fg(::fmt::color::red), FMT_STRING(message "\n") __VA_OPT__(,) __VA_ARGS__)
