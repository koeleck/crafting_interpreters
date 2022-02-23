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
)
target_link_libraries(jlox_tests
    jlox_sources
)
