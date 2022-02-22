cmake_minimum_required(VERSION 3.20)

project(crafting_interpreters)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra -g")

add_subdirectory(jlox)
