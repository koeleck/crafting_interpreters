#include "parser.hpp"

#include <cassert>
#include <cstdint>

#include "tokens.hpp"
#include "expr.hpp"
#include "bump_alloc.hpp"
#include "scanner.hpp"
#include "log.hpp"

namespace
{

constexpr
bool is_left_associative(TokenType /*type*/) noexcept
{
    return true; // TODO for now everything is
}

constexpr
int32_t get_binary_prio(TokenType type) noexcept
{
    switch (type) {
    using enum TokenType;
    case EQUAL_EQUAL:
    case BANG_EQUAL:
        return 10;

    case GREATER:
    case GREATER_EQUAL:
    case LESS:
    case LESS_EQUAL:
        return 20;

    case MINUS:
    case PLUS:
        return 30;

    case SLASH:
    case STAR:
        return 40;

    default:
        break;
    }
    return -1;
}

class ExprParser
{
public:
    ExprParser(BumpAlloc& alloc,
               const ScannerResult& scanner_result)
      : m_alloc{alloc}
      , m_scanner_result{scanner_result}
      , m_current{0}
    {
        assert(m_scanner_result.tokens.empty() == false &&
               m_scanner_result.tokens.back().type() == TokenType::END_OF_FILE);
    }

    Expr* parse()
    {
        if (eof()) {
            return nullptr;
        }

        Expr* const result = parse_expression();
        if (!result) {
            synchronize();
        }
        // TODO: Remove me
        if (!eof()) {
        }
        return result;
    }

private:

    bool eof() const noexcept
    {
        return static_cast<size_t>(m_current) >= m_scanner_result.tokens.size();
    }

    void synchronize()
    {
        while (!eof()) {
            advance();
        }
    }

    const Token* peek() const noexcept
    {
        if (static_cast<size_t>(m_current) >= m_scanner_result.tokens.size()) {
            return &m_scanner_result.tokens.back();
        }
        return &m_scanner_result.tokens[m_current];
    }

    const Token* advance() noexcept
    {
        if (static_cast<size_t>(m_current) >= m_scanner_result.tokens.size()) {
            return &m_scanner_result.tokens.back();
        }
        return &m_scanner_result.tokens[m_current++];
    }

    const Token* match(TokenType type) noexcept
    {
        const Token* const nxt = peek();
        if (nxt->type() == type) {
            ++m_current;
            return nxt;
        }
        return nullptr;
    }

    Expr* parse_expression()
    {
        return parse_expression_rec(parse_primary(), 0);
    }

    Expr* parse_expression_rec(Expr* lhs, const int32_t min_priority = 0)
    {
        if (lhs == nullptr) {
            return nullptr;
        }
        while (true) {
            const Token* lookahead = peek();
            const int32_t op_prio = get_binary_prio(lookahead->type());
            if (op_prio < min_priority) {
                break;
            }
            const Token* const op = advance();

            Expr* rhs = parse_primary();
            if (!rhs) {
                return nullptr;
            }

            lookahead = peek();
            while (true) {
                const int32_t nxt_op_prio = get_binary_prio(lookahead->type());
                const bool is_right_assoc = !is_left_associative(lookahead->type());
                if ((!is_right_assoc && nxt_op_prio > op_prio) ||
                    (is_right_assoc && nxt_op_prio >= op_prio))
                {
                    rhs = parse_expression_rec(rhs, op_prio + (nxt_op_prio > op_prio));
                    if (!rhs) {
                        return nullptr;
                    }
                    lookahead = peek();
                } else {
                    break;
                }
            }
            lhs = m_alloc.allocate<BinaryExpr>(lhs, op, rhs);
        }
        return lhs;
    }

    Expr* parse_primary()
    {
        const Token* const token = advance();
        switch (token->type()) {
        using enum TokenType;
        case LEFT_PAREN:
            {
                Expr* const group = parse_expression();
                if (!group) {
                    return nullptr;
                }
                const Token* const lookahead = peek();
                if (lookahead->type() != RIGHT_PAREN) {
                    report_error(lookahead, "Expected ')', got \"{}\".", token_to_string(lookahead->type()));
                    return nullptr;
                }
                advance();
                return m_alloc.allocate<GroupingExpr>(group);
            }

        case NIL:
        case TRUE:
        case FALSE:
        case IDENTIFIER:
        case STRING:
        case NUMBER:
            return m_alloc.allocate<LiteralExpr>(token);

        case MINUS:
        case BANG:
            {
                Expr* prim = parse_primary();
                if (!prim) {
                    return nullptr;
                }
                return m_alloc.allocate<UnaryExpr>(token, prim);
            }

        default:
            break;
        }
        report_error(token, "Unexpected token \"{}\".", token->type());

        return nullptr;
    }

    template <typename Fmt, typename... Args>
    void report_error(const Token* token, Fmt&& format_str, Args&&... args)
    {
        const Position pos = m_scanner_result.offsets.get_position(token->offset());
        const int32_t line_offset = m_scanner_result.offsets.get_offset(pos.line);
        std::string_view src_line = get_line_from_offset(m_scanner_result.source, line_offset);

        ::report_error(pos.line, pos.column, src_line, std::forward<Fmt>(format_str), std::forward<Args>(args)...);
    }

    BumpAlloc& m_alloc;
    const ScannerResult& m_scanner_result;
    int32_t m_current;
};


} // namespace

Expr*
parse(BumpAlloc& alloc,
      const ScannerResult& scanner_result)
{
    ExprParser parser{alloc, scanner_result};
    return parser.parse();
}


#include <doctest/doctest.h>
#include "print_visitor.hpp"

TEST_CASE("Parser Test")
{
    std::string_view source{"5 * !6 + 7 * -8 * ((1 + 2) * 3)"};
    auto result = scan_tokens(source);
    CHECK(result.num_errors == 0);

    BumpAlloc alloc;

    Expr* const expr = parse(alloc, result);
    CHECK(expr != nullptr);

    PrintVisitor visitor{result.source};
    expr->accept(visitor);

    CHECK(visitor.get() == "(+ (* 5 (!6)) (* (* 7 (-8)) (group (* (group (+ 1 2)) 3))))");
}
