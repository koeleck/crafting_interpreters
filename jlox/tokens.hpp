#pragma once

#include <cstdint>
#include <cassert>
#include <fmt/format.h>


enum class TokenType: uint8_t
{
    // Single-character tokens
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // One or two character tokens
    BANG, BANG_EQUAL, EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL, LESS, LESS_EQUAL,

    // Literals
    IDENTIFIER, STRING, NUMBER,

    // Keyword
    AND, CLASS, ELSE, FALSE, FUN, FOR, IF, NIL, OR,
    PRINT, RETURN, SUPER, THIS, TRUE, VAR, WHILE,

    END_OF_FILE,
};

std::string_view token_to_string(TokenType token_type) noexcept;

template <>
struct fmt::formatter<TokenType> : formatter<std::string_view>
{
    template <typename FormatContext>
    auto format(TokenType tt, FormatContext& ctx)
    {
        return formatter<std::string_view>::format(token_to_string(tt), ctx);
    }
};


class Token
{
public:
    constexpr
    Token(TokenType token_type, int32_t offset, int32_t length)
      : m_offset{offset}
      , m_length{length}
      , m_type{token_type}
    {
        assert(offset >= 0 && length >= 0);
    }

    constexpr
    TokenType type() const noexcept
    {
        return m_type;
    }

    constexpr
    int32_t offset() const noexcept
    {
        return m_offset;
    }

    constexpr
    int32_t length() const noexcept
    {
        return m_length;
    }

    constexpr
    std::string_view lexeme(std::string_view source) const
    {
        assert(static_cast<size_t>(m_offset + m_length) <= source.size());
        return source.substr(m_offset, m_length);
    }

private:
    int32_t m_offset;
    int32_t m_length;
    TokenType m_type;
    // TODO: literal
};

inline constexpr Token TRUE_TOKEN{TokenType::TRUE, 0, 0};
inline constexpr Token FALSE_TOKEN{TokenType::FALSE, 0, 0};
