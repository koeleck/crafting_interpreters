#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "log.hpp"
#include "scanner.hpp"
#include "parser.hpp"
#include "bump_alloc.hpp"

#include "print_visitor.hpp"
#include "interpreter.hpp"

static
int run(std::string_view source)
{
    const auto scan_result = scan_tokens(source);
    if (scan_result.num_errors != 0) {
        std::cerr << "Lexing failed.\n";
        return 1;
    }
    BumpAlloc alloc;
    Expr* const expr = parse(alloc, scan_result);
    if (!expr) {
        std::cerr << "Parsing failed.\n";
        return 1;
    }
    PrintVisitor visitor{scan_result.source};
    expr->accept(visitor);
    std::cout << "AST: " << visitor.get() << '\n';

    Interpreter interpreter{scan_result};

    const std::optional<Interpreter::Value> result = interpreter.evaluate(*expr);
    if (!result) {
        std::cerr << "Interpreter error.\n";
    } else {
        const Interpreter::Value& value = *result;
        std::cout << "-> ";
        if (std::get_if<Interpreter::Nil>(&value)) {
            std::cout << "nil\n";
        } else if (const double* const d = std::get_if<double>(&value)) {
            std::cout << *d << '\n';
        } else if (const std::string* const s = std::get_if<std::string>(&value)) {
            std::cout << *s << '\n';
        } else if (const bool* const b = std::get_if<bool>(&value)) {
            std::cout << (*b ? "true\n" : "false\n");
        } else {
            std::abort();
        }
    }

    return 0;
}


static
int run_file(const char* path)
{
    std::string content;
    {
        std::ifstream input_file{path, std::ios::binary};
        if (!input_file) {
            LOG_ERROR("Failed to open file \"{}\".", path);
            return 1;
        }

        input_file.seekg(0, std::ios::end);
        const ssize_t len = input_file.tellg();
        if (len < 0) {
            LOG_ERROR("Failed to get size of \"{}\"", path);
            return 1;
        }
        input_file.seekg(0, std::ios::beg);

        content.resize(static_cast<size_t>(len));
        input_file.read(&content.front(), len);
    }
    return run(content);
}


static
int run_prompt()
{
    std::cout << "> ";
    for (std::string line; std::getline(std::cin, line); ) {
        const int result = run(line);
        if (result) {
            std::cerr << "Error [" << result << ']';
        }
        std::cout << "\n> ";
    }
    return 0;
}


int main(int argc, char** argv)
{
    if (argc > 2) {
        LOG_ERROR("Usage: jlox [script]");
        return 0;
    } else if (argc == 2) {
        return run_file(argv[1]);
    } else {
        return run_prompt();
    }
}
