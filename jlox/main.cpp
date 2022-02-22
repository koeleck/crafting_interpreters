#include <cstdio>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "log.hpp"

static
int run(std::string_view source)
{
    std::cout << source;
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
            return result;
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
