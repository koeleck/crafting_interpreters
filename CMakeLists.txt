cmake_minimum_required(VERSION 3.20)

project(crafting_interpreters)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(WITH_ASAN "Use address sanitizer" ON)
option(WITH_LSAN "Use lean sanitizer" ON)
option(WITH_UBSAN "use undefined behavior sanitizer" ON)

string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -g")
if(WITH_ASAN)
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize=address")
endif()
if(WITH_LSAN)
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize=leak")
endif()
if(WITH_UBSAN)
    string(APPEND CMAKE_CXX_FLAGS " -fsanitize=undefined")
endif()

add_subdirectory(jlox)
