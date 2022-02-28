#include "scanner.hpp"

#include <algorithm>
#include <cassert>
#include <cctype>

#include <doctest/doctest.h>

#include "log.hpp"
#include "utf-8.hpp"

#define STRINGIFY_(x) #x
#define STRINGIFY(x) STRINGIFY_(x)

namespace
{

Position offset_to_position(std::span<const int32_t> map, int32_t offset)
{
    assert(offset >= 0);
    assert(map.size() >= 1);

    auto it = std::lower_bound(map.begin(), map.end(), offset);

    if (it != map.end() && *it == offset) {
        const size_t line = std::distance(map.begin(), it) + 1;
        return {static_cast<int32_t>(line), 1};
    }
    --it;
    const size_t line = std::distance(map.begin(), it) + 1;
    return {static_cast<int32_t>(line), 1 + (offset - *it)};
}

TEST_CASE("Position from offset")
{
    std::string_view source{"Hello world\n"  // 12 characters
                            "1234567890"     // 10 characters
                            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"}; // 26 characters
    const int32_t otl[3]{
        0,
        12,
        22
    };

    CHECK(offset_to_position(otl, 0) == Position{1, 1});
    CHECK(offset_to_position(otl, 11) == Position{1, 12});
    CHECK(offset_to_position(otl, 12) == Position{2, 1});
    CHECK(offset_to_position(otl, 25) == Position{3, 4});
}

class Reader
{
public:
    Reader(std::string_view source) noexcept
      : m_source{source}
      , m_offset{0}
      , m_line{1}
      , m_column{1}
    {
        m_offsets.push_back(0);
    }

    constexpr
    int32_t offset() const noexcept
    {
        return m_offset;
    }

    constexpr
    int32_t line() const noexcept
    {
        return m_line;
    }

    constexpr
    int32_t column() const noexcept
    {
        return m_column;
    }

    constexpr
    bool eof() const noexcept
    {
        return static_cast<size_t>(m_offset) == m_source.size();
    }

    constexpr
    char peek() const noexcept
    {
        if (eof()) {
            return '\0';
        }
        assert(static_cast<size_t>(m_offset) < m_source.size());
        return m_source[m_offset];
    }

    constexpr
    char peek(int32_t n) const noexcept
    {
        assert(n >= 0);
        if (static_cast<size_t>(m_offset + n) >= m_source.size()) {
            return '\0';
        }
        return m_source[m_offset + n];
    }

    constexpr
    std::string_view remaining_source() const noexcept
    {
        return m_source.substr(m_offset);
    }

    /**
     * Compare current character with `c`.
     * Advance position and return true if equal.
     */
    bool match(char c) noexcept
    {
        if (eof() || m_source[m_offset] != c) {
            return false;
        }
        return advance();
    }

    //! Advance position and return previous character.
    char advance() noexcept
    {
        assert(static_cast<size_t>(m_offset) != m_source.size());
        const char c = m_source[m_offset++];
        ++m_column;
        if (c == '\n') {
            m_offsets.push_back(m_offset);
            ++m_line;
            m_column = 1;
        }
        return c;
    }

    char advance(int32_t n) noexcept
    {
        assert(n >= 1);
        char c;
        while (n-- > 0) {
            c = advance();
        }
        return c;
    }

    std::vector<int32_t>&& get_offsets() && noexcept
    {
        return std::move(m_offsets);
    }

    /**
     * Extract line.
     */
    std::string_view get_line(int32_t line) const
    {
        assert(line > 0 && line <= m_line);
        assert(static_cast<size_t>(line - 1) < m_offsets.size());

        const int32_t offset = m_offsets[static_cast<size_t>(line - 1)];
        const auto end = m_source.find('\n', offset);
        if (end == m_source.npos) {
            return m_source.substr(offset);
        }
        return m_source.substr(offset, end - offset);
    }


private:
    std::vector<int32_t> m_offsets;
    std::string_view m_source;
    int32_t m_offset;
    int32_t m_line;
    int32_t m_column;
};

/**
 * Parse strings. String may contain UTF-8.
 */
int32_t parse_string(Reader& reader)
{
    // At this pointe we're at the first character AFTER the
    // opening quote.
    int32_t length = 0;

    while (!reader.eof()) {

        UTF8Char c = parse_utf8_char(reader.remaining_source());
        if (c.length <= 0) {
            return -1;
        }

        reader.advance(c.length);
        if (c.c == '"') {
            return length;
        }
        length += c.length;
    }
    return -1;
}

/**
 * Parse number returns length
 */
int32_t parse_number(Reader& reader)
{
    bool has_dot = false;

    int32_t length = 0;
    while (!reader.eof()) {
        const char c = reader.peek();
        if (c == '.') {
            if (!std::isdigit(reader.peek(1))) {
                break;
            }
            if (has_dot) {
                break;
            }
            has_dot = true;
        } else if (!std::isdigit(c)) {
            break;
        }
        ++length;
        reader.advance();
    }
    return length;
}

bool is_alpha(char c)
{
    return std::isalpha(c) || c == '_';
}

/**
 * parse identifier
 */
int32_t parse_identifier(Reader& reader)
{
    int32_t length = 0;
    while (!reader.eof()) {
        const char c = reader.peek();
        if (!is_alpha(c)) {
            break;
        }
        ++length;
        reader.advance();
    }
    return length;
}

TokenType get_type_of_identifier(std::string_view source,
                                 int32_t offset,
                                 int32_t length)
{
    using enum TokenType;
    assert(offset > 0 && length > 0);
    assert(static_cast<size_t>(offset + length) <= source.size());
    std::string_view tok = source.substr(offset, length);
#define CHK_KEYWRD(str, type) if (STRINGIFY(str) == tok) { return type; }
    CHK_KEYWRD(and, AND);
    CHK_KEYWRD(class, CLASS);
    CHK_KEYWRD(else, ELSE);
    CHK_KEYWRD(false, FALSE);
    CHK_KEYWRD(for, FOR);
    CHK_KEYWRD(fun, FUN);
    CHK_KEYWRD(if, IF);
    CHK_KEYWRD(nil, NIL);
    CHK_KEYWRD(or, OR);
    CHK_KEYWRD(print, PRINT);
    CHK_KEYWRD(return, RETURN);
    CHK_KEYWRD(super, SUPER);
    CHK_KEYWRD(this, THIS);
    CHK_KEYWRD(true, TRUE);
    CHK_KEYWRD(var, VAR);
    CHK_KEYWRD(while, WHILE);
#undef CHK_KEYWRD
    return IDENTIFIER;
}

} // anonymous namespace


ScannerResult scan_tokens(std::string_view source)
{
    int32_t num_errors{0};

    std::vector<Token> tokens;
    auto add_token = [&] (TokenType type, int32_t offset, int32_t length) {
        tokens.emplace_back(type, offset, length);
    };

    Reader reader{source};
    while (!reader.eof()) {
        using enum TokenType;
        const size_t offset = reader.offset();
        const int32_t column = reader.column();
        const int32_t line = reader.line();
        const char c = reader.advance();

        switch (c) {
        case '(':
            add_token(LEFT_PAREN, offset, 1);
            break;

        case ')':
            add_token(RIGHT_PAREN, offset, 1);
            break;

        case '{':
            add_token(LEFT_BRACE, offset, 1);
            break;

        case '}':
            add_token(RIGHT_BRACE, offset, 1);
            break;

        case ',':
            add_token(COMMA, offset, 1);
            break;

        case '.':
            add_token(DOT, offset, 1);
            break;

        case '-':
            add_token(MINUS, offset, 1);
            break;

        case '+':
            add_token(PLUS, offset, 1);
            break;

        case ';':
            add_token(SEMICOLON, offset, 1);
            break;

        case '*':
            add_token(STAR, offset, 1);
            break;

        case '!':
            if (reader.match('=')) {
                add_token(BANG_EQUAL, offset, 2);
            } else {
                add_token(BANG, offset, 1);
            }
            break;

        case '=':
            if (reader.match('=')) {
                add_token(EQUAL_EQUAL, offset, 2);
            } else {
                add_token(EQUAL, offset, 1);
            }
            break;

        case '<':
            if (reader.match('=')) {
                add_token(LESS_EQUAL, offset, 2);
            } else {
                add_token(LESS, offset, 1);
            }
            break;

        case '>':
            if (reader.match('=')) {
                add_token(GREATER_EQUAL, offset, 2);
            } else {
                add_token(GREATER, offset, 1);
            }
            break;

        case '/':
            if (reader.match('/')) {
                while (!reader.eof() && reader.peek() != '\n') {
                    reader.advance();
                }
            } else {
                add_token(SLASH, offset, 1);
            }
            break;

        case '"':
            {
                const int32_t str_length = parse_string(reader);
                if (str_length < 0) {
                    num_errors++;
                    report_error(line,
                                 column,
                                 reader.get_line(line),
                                 "Unterminated string.");
                } else {
                    add_token(STRING, offset, str_length + 2);
                }
            }
            break;

        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            {
                add_token(NUMBER, offset, 1 + parse_number(reader));
            }
            break;

        case ' ':
        case '\r':
        case '\t':
        case '\n':
            break;

        default:
            if (is_alpha(c)) {
                const int32_t l = parse_identifier(reader);
                const TokenType type = get_type_of_identifier(source, offset, 1 + l);
                add_token(type, offset, 1 + l);
            } else {
                ++num_errors;
                report_error(line,
                             column,
                             reader.get_line(line), "Unexpected character: \"{}\"", c);
            }
            break;
        }
    }

    add_token(TokenType::END_OF_FILE, source.size(), 0);

    return {source,
            std::move(tokens),
            OffsetToLine{std::move(reader).get_offsets()},
            num_errors};
}

TEST_CASE("scanner")
{
    SUBCASE("First example from book") {
        std::string_view source = "// this is a comment\n"
                                  "(( )){} // grouping stuff\n"
                                  "!*-/=<> <= == // operators\n";
        auto result = scan_tokens(source);
        CHECK(result.num_errors == 0);
        CHECK(result.offsets.num_lines() == 4);

        CHECK(result.offsets.get_offset(1) == 0);
        CHECK(result.offsets.get_offset(4) == 74);

        CHECK(result.tokens.size() == 16);
        CHECK(result.tokens[0].type() == TokenType::LEFT_PAREN);
        CHECK(result.tokens[0].length() == 1);
        CHECK(result.tokens[0].offset() == 21);
        CHECK(source[result.tokens[0].offset()] == '(');

        CHECK(result.tokens[14].type() == TokenType::EQUAL_EQUAL);
        CHECK(result.tokens[14].length() == 2);
        CHECK(result.tokens[14].offset() == 58);
        CHECK(source[result.tokens[14].offset()] == '=');
        CHECK(source[result.tokens[14].offset() + 1] == '=');

        CHECK(result.tokens[15].type() == TokenType::END_OF_FILE);
        CHECK(result.tokens[15].length() == 0);
        CHECK(result.tokens[15].offset() == 74);
    }

    SUBCASE("Strings") {
        std::string_view source = " \n"
                                  " \"ΛΛΛ\" ";

        auto result = scan_tokens(source);
        CHECK(result.num_errors == 0);
        CHECK(result.offsets.num_lines() == 2);
        CHECK(result.offsets.get_offset(2) == 2);

        CHECK(result.tokens.size() == 2);
        CHECK(result.tokens[0].type() == TokenType::STRING);
        CHECK(result.tokens[0].length() == 8); // 6 bytes for UTF-8 string + 2 bytes for the quotes
        CHECK(result.tokens[0].offset() == 3);
    }

    SUBCASE("Numbers") {
        std::string_view source = " 12\n"
                                  "  092.2 \n"
                                  "  .1. ";

        auto result = scan_tokens(source);
        CHECK(result.num_errors == 1); // Leading zero not allowed
        CHECK(result.offsets.num_lines() == 3);
        CHECK(result.offsets.get_offset(3) == 13);

        CHECK(result.tokens.size() == 6);

        CHECK(result.tokens[0].type() == TokenType::NUMBER);
        CHECK(result.tokens[0].length() == 2);
        CHECK(result.tokens[0].offset() == 1);

        CHECK(result.tokens[1].type() == TokenType::NUMBER);
        CHECK(result.tokens[1].length() == 4);
        CHECK(result.tokens[1].offset() == 7);

        CHECK(result.tokens[2].type() == TokenType::DOT);
        CHECK(result.tokens[2].length() == 1);
        CHECK(result.tokens[2].offset() == 15);

        CHECK(result.tokens[3].type() == TokenType::NUMBER);
        CHECK(result.tokens[3].length() == 1);
        CHECK(result.tokens[3].offset() == 16);

        CHECK(result.tokens[4].type() == TokenType::DOT);
        CHECK(result.tokens[4].length() == 1);
        CHECK(result.tokens[4].offset() == 17);
    }

    SUBCASE("Identifiers and keywords") {
        std::string_view source = " var \n"
                                  " true \n"
                                  " TRUE \n";

        auto result = scan_tokens(source);
        CHECK(result.num_errors == 0);
        CHECK(result.offsets.num_lines() == 4);
        CHECK(result.offsets.get_offset(4) == 20);

        CHECK(result.tokens.size() == 4);

        CHECK(result.tokens[0].type() == TokenType::VAR);
        CHECK(result.tokens[0].length() == 3);
        CHECK(result.tokens[0].offset() == 1);

        CHECK(result.tokens[1].type() == TokenType::TRUE);
        CHECK(result.tokens[1].length() == 4);
        CHECK(result.tokens[1].offset() == 7);

        CHECK(result.tokens[2].type() == TokenType::IDENTIFIER);
        CHECK(result.tokens[2].length() == 4);
        CHECK(result.tokens[2].offset() == 14);
    }
}


Position OffsetToLine::get_position(const int32_t offset) const noexcept
{
    return offset_to_position(m_offsets, offset);
}

int32_t OffsetToLine::get_offset(const int32_t line) const noexcept
{
    assert(line > 0 && static_cast<size_t>(line) <= m_offsets.size());

    return m_offsets[static_cast<size_t>(line - 1)];
}


std::string_view get_line_from_offset(std::string_view source, const int32_t offset)
{
    const auto end = source.find('\n', offset);
    if (end == source.npos) {
        return source.substr(offset);
    }
    return source.substr(offset, end - offset);
}
