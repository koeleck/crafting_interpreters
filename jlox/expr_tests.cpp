#include <string>

#include "expr.hpp"
#include "scanner.hpp"
#include "bump_alloc.hpp"
#include "print_visitor.hpp"

#include <doctest/doctest.h>


TEST_CASE("Expr/Visitor")
{
    std::string_view source{"-123 * (45.67)"};
    auto result = scan_tokens(source);
    CHECK(result.num_errors == 0);
    CHECK(result.tokens.size() == 7);

    LiteralExpr v123{&result.tokens[1]};
    UnaryExpr minus{&result.tokens[0], &v123};

    LiteralExpr v4567{&result.tokens[4]};
    GroupingExpr group{&result.tokens[3], &v4567, &result.tokens[5]};

    BinaryExpr binary{&minus, &result.tokens[2], &group};

    PrintVisitor visitor{source};
    binary.accept(visitor);

    CHECK(visitor.get() == "(* (-123) (group 45.67))");
}
