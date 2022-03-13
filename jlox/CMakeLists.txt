find_package(fmt REQUIRED)
find_package(doctest REQUIRED)

add_library(jlox_sources INTERFACE)
target_sources(jlox_sources INTERFACE
    log.hpp
    log.cpp
    tokens.hpp
    tokens.cpp
    scanner.hpp
    scanner.cpp
    utf-8.hpp
    utf-8.cpp
    bump_alloc.hpp
    bump_alloc.cpp
    expr.hpp
    stmt.hpp
    parser.hpp
    parser.cpp

    interpreter.hpp
    interpreter.cpp

    print_visitor.hpp
    print_visitor.cpp
)
target_link_libraries(jlox_sources
    INTERFACE
        fmt::fmt
        doctest::doctest
)

add_executable(jlox
    main.cpp
)
target_compile_definitions(jlox
    PRIVATE
        DOCTEST_CONFIG_DISABLE
)
target_link_libraries(jlox
    PRIVATE
        jlox_sources
)


add_executable(jlox_tests
    test_main.cpp
    expr_tests.cpp
)
target_link_libraries(jlox_tests
    jlox_sources
)
