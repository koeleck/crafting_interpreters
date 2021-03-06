#include "parser.hpp"

#include <cassert>
#include <cstdint>

#include "tokens.hpp"
#include "expr.hpp"
#include "bump_alloc.hpp"
#include "scanner.hpp"
#include "log.hpp"
#include "stmt.hpp"

namespace
{

constexpr
int32_t MAX_N_PARAMS = 255;

constexpr
bool is_left_associative(TokenType /*type*/) noexcept
{
    return true; // TODO for now everything is
}

constexpr
int32_t get_binary_prio(TokenType type) noexcept
{
    // higher is more important
    switch (type) {
    using enum TokenType;
    case EQUAL:
        return 0;

    case OR:
        return 5;

    case AND:
        return 6;

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

class LoxParser
{
public:
    LoxParser(BumpAlloc& alloc,
               const ScannerResult& scanner_result)
      : m_alloc{alloc}
      , m_scanner_result{scanner_result}
      , m_current{0}
    {
        assert(m_scanner_result.tokens.empty() == false &&
               m_scanner_result.tokens.back().type() == TokenType::END_OF_FILE);
    }

    std::vector<Stmt*> parse()
    {
        std::vector<Stmt*> statements;

        while (!eof()) {
            Stmt* const stmt = parse_declaration();
            if (stmt) {
                statements.push_back(stmt);
            } else if (!eof()) {
                advance();
            }
        }

        return statements;
    }

private:

    bool eof() const noexcept
    {
        assert(static_cast<size_t>(m_current) < m_scanner_result.tokens.size());
        return m_scanner_result.tokens[m_current].type() == TokenType::END_OF_FILE;
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

    void unadvance() noexcept
    {
        assert(m_current > 0);
        --m_current;
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

    bool check(TokenType type) const noexcept
    {
        const Token* const nxt = peek();
        return nxt->type() == type;
    }

    template <typename Fmt, typename... Args>
    const Token* consume(TokenType expected, Fmt err_msg, Args&&... args)
    {
        const Token* const lookahead = peek();
        if (lookahead->type() != expected) {
            report_error(lookahead, std::forward<Fmt>(err_msg), std::forward<Args>(args)...);
            return nullptr;
        }
        ++m_current;
        return lookahead;
    }

    Expr* parse_expression()
    {
        Expr* e = parse_primary();
        if (!e) {
            return nullptr;
        }
        return parse_expression_rec(e, 0);
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

            if (op->type() == TokenType::EQUAL) {
                if (lhs->is_type<VarExpr>()) {
                    const Token* const identifier = lhs->get_main_token();
                    lhs = m_alloc.allocate<AssignExpr>(identifier, rhs);
                } else {
                    report_error(op, "Invalid assignment");
                }
            } else if (op->type() == TokenType::AND || op->type() == TokenType::OR) {
                lhs = m_alloc.allocate<LogicalExpr>(lhs, op, rhs);
            } else {
                lhs = m_alloc.allocate<BinaryExpr>(lhs, op, rhs);
            }
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
                return m_alloc.allocate<GroupingExpr>(token, group, lookahead);
            }

        case NIL:
        case TRUE:
        case FALSE:
        case STRING:
        case NUMBER:
            return m_alloc.allocate<LiteralExpr>(token);

        case IDENTIFIER:
            {
                Expr* e = m_alloc.allocate<VarExpr>(token);
                if (!e) {
                    return nullptr;
                }
                if (match(TokenType::LEFT_PAREN)) {
                    e = parse_call(e);
                }
                return e;
            }


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
        unadvance();
        report_error(token, "Unexpected token \"{}\".", token->type());

        return nullptr;
    }

    Expr* parse_call(Expr* callee)
    {
        if (!callee) {
            return nullptr;
        }
        std::vector<Expr*> args;
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                const Token* const lookahead = peek();
                Expr* const arg = parse_expression();
                if (!arg) {
                    return nullptr;
                }
                args.push_back(arg);
                if (args.size() >= MAX_N_PARAMS) {
                    report_error(lookahead, "Can't have more than {} arguments.", MAX_N_PARAMS);
                }
            } while (match(TokenType::COMMA));
        }
        const Token* const paren = consume(TokenType::RIGHT_PAREN, "Expect ')' after arguments.");
        if (!paren) {
            return nullptr;
        }
        return m_alloc.allocate<CallExpr>(callee, paren, std::move(args));
    }


    Stmt* parse_statement()
    {
        if (match(TokenType::IF)) {
            if (!consume(TokenType::LEFT_PAREN, "Expect '(' after 'if'")) {
                return nullptr;
            }
            Expr* const condition = parse_expression();
            if (!condition) {
                return nullptr;
            }
            if (!consume(TokenType::RIGHT_PAREN, "Expect ')' after if condition")) {
                return nullptr;
            }

            Stmt* const then_branch = parse_statement();
            if (!then_branch) {
                return nullptr;
            }
            Stmt* else_branch = nullptr;
            if (match(TokenType::ELSE)) {
                else_branch = parse_statement();
                if (!else_branch) {
                    return nullptr;
                }
            }

            return m_alloc.allocate<IfStmt>(condition, then_branch, else_branch);
        }

        if (match(TokenType::PRINT)) {
            Expr* const expr = parse_expression();
            if (!expr) {
                return nullptr;
            }
            if (!consume(TokenType::SEMICOLON, "Expect ';' after expression.")) {
                return nullptr;
            }
            return m_alloc.allocate<PrintStmt>(expr);
        }

        if (const Token* const token  = match(TokenType::RETURN)) {
            Expr* value{nullptr};
            if (!check(TokenType::SEMICOLON)) {
                value = parse_expression();
                if (!value) {
                    return nullptr;
                }
            }
            if (!consume(TokenType::SEMICOLON, "Expect ';' after return value.")) {
                return nullptr;
            }
            return m_alloc.allocate<ReturnStmt>(token, value);
        }

        if (match(TokenType::LEFT_BRACE)) {
            std::vector<Stmt*> statements;

            while (!eof() && !check(TokenType::RIGHT_BRACE)) {
                Stmt* const s = parse_declaration();
                if (!s) {
                    return nullptr;
                }
                statements.push_back(s);
            }

            if (!consume(TokenType::RIGHT_BRACE, "Expected '}' after block.")) {
                return nullptr;
            }

            return m_alloc.allocate<BlockStmt>(std::move(statements));
        }

        if (match(TokenType::WHILE)) {
            if (!consume(TokenType::LEFT_PAREN, "Expect '(' after 'while'.")) {
                return nullptr;
            }

            Expr* const condition = parse_expression();
            if (!condition) {
                return nullptr;
            }

            if (!consume(TokenType::RIGHT_PAREN, "Expect ')' after condition.")) {
                return nullptr;
            }

            Stmt* const body = parse_statement();
            if (!body) {
                return nullptr;
            }

            return m_alloc.allocate<WhileStmt>(condition, body);
        }

        if (match(TokenType::FOR)) {
            if (!consume(TokenType::LEFT_PAREN, "Expect '(' after 'for'.")) {
                return nullptr;
            }

            Stmt* initializer = nullptr;
            if (match(TokenType::SEMICOLON)) {
                // intentionally empty
            } else if (match(TokenType::VAR)) {
                initializer = parse_var_declaration();
            } else {
                Expr* const expr = parse_expression();
                if (!expr) {
                    return nullptr;
                }
                initializer = m_alloc.allocate<ExprStmt>(expr);
            }

            Expr* condition = nullptr;
            if (!check(TokenType::SEMICOLON)) {
                condition = parse_expression();
                if (!condition) {
                    return nullptr;
                }
            }
            if (!consume(TokenType::SEMICOLON, "Expect ';' after loop condition.")) {
                return nullptr;
            }

            Expr* increment = nullptr;
            if (!check(TokenType::RIGHT_PAREN)) {
                increment = parse_expression();
                if (!increment) {
                    return nullptr;
                }
            }
            if (!consume(TokenType::RIGHT_PAREN, "Expect ')' after for clauses")) {
                return nullptr;
            }

            Stmt* body = parse_statement();
            if (!body) {
                return nullptr;
            }

            if (increment) {
                std::vector<Stmt*> statements{
                    body,
                    m_alloc.allocate<ExprStmt>(increment)
                };
                body = m_alloc.allocate<BlockStmt>(std::move(statements));
            }

            if (!condition) {
                condition = m_alloc.allocate<LiteralExpr>(&TRUE_TOKEN);
            }
            body = m_alloc.allocate<WhileStmt>(condition, body);

            if (initializer) {
                std::vector<Stmt*> statements{
                    initializer,
                    body
                };
                body = m_alloc.allocate<BlockStmt>(std::move(statements));
            }

            return body;
        }

        Expr* const expr = parse_expression();
        if (!expr) {
            return nullptr;
        }
        if (!consume(TokenType::SEMICOLON, "Expect ';' after expression.")) {
            return nullptr;
        }
        return m_alloc.allocate<ExprStmt>(expr);
    }

    Stmt* parse_declaration()
    {
        if (match(TokenType::VAR)) {
            return parse_var_declaration();
        }
        if (match(TokenType::FUN)) {
            return parse_fun_declaration("function");
        }
        return parse_statement();
    }

    Stmt* parse_var_declaration()
    {
        const Token* const identifier = consume(TokenType::IDENTIFIER, "Expected variable name.");
        if (!identifier) {
            return nullptr;
        }

        Expr* initializer = nullptr;
        if (match(TokenType::EQUAL)) {
            initializer = parse_expression();
            if (!initializer) {
                // TODO: report error here?
                return nullptr;
            }
        }

        if (!consume(TokenType::SEMICOLON, "Expected ';' after variable declaration.")) {
            return nullptr;
        }

        return m_alloc.allocate<VarStmt>(identifier, initializer);
    }

    Stmt* parse_fun_declaration(std::string_view kind)
    {
        const Token* const name = consume(TokenType::IDENTIFIER, "Expected {} name.", kind);
        if (!name) {
            return nullptr;
        }

        std::vector<const Token*> params;
        if (!consume(TokenType::LEFT_PAREN, "Expect '(' after {} name", kind)) {
            return nullptr;
        }
        if (!check(TokenType::RIGHT_PAREN)) {
            do {
                if (params.size() >= MAX_N_PARAMS) {
                    report_error(peek(), "Can't have more than {} parameters", MAX_N_PARAMS);
                }
                const Token* const next = consume(TokenType::IDENTIFIER, "Expect parameter name.");
                if (!next) {
                    return nullptr;
                }
                params.push_back(next);
            } while (match(TokenType::COMMA));
        }
        if (!consume(TokenType::RIGHT_PAREN, "Expect ')' after parameters.")) {
            return nullptr;
        }

        std::vector<Stmt*> body;
        if (!consume(TokenType::LEFT_BRACE, "Expect '{' before {} body.", kind)) {
            return nullptr;
        }
        while (!eof() && !check(TokenType::RIGHT_BRACE)) {
            Stmt* const s = parse_declaration();
            if (!s) {
                return nullptr;
            }
            body.push_back(s);
        }

        if (!consume(TokenType::RIGHT_BRACE, "Expected '}' after {} body.", kind)) {
            return nullptr;
        }

        return m_alloc.allocate<FunStmt>(name, std::move(params), std::move(body));
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

std::vector<Stmt*>
parse(BumpAlloc& alloc,
      const ScannerResult& scanner_result)
{
    LoxParser parser{alloc, scanner_result};
    return parser.parse();
}


#include <doctest/doctest.h>
#include "print_visitor.hpp"

TEST_CASE("Parser Test")
{
    std::string_view source{"5 * !6 + 7 * -8 * ((1 + 2) * 3);"};
    auto result = scan_tokens(source);
    CHECK(result.num_errors == 0);

    BumpAlloc alloc;

    //Expr* const expr = parse(alloc, result);
    std::vector<Stmt*> statements = parse(alloc, result);
    CHECK(statements.empty() == false);
    Stmt* const stmt = statements.front();
    CHECK(stmt->is_type<ExprStmt>());

    Expr* const expr = static_cast<ExprStmt*>(stmt)->expr;
    PrintVisitor visitor{result.source};
    expr->accept(visitor);

    CHECK(visitor.get() == "(+ (* 5 (!6)) (* (* 7 (-8)) (group (* (group (+ 1 2)) 3))))");
}
