find_package(fmt REQUIRED)

add_executable(jlox
    main.cpp

    log.hpp
    log.cpp
)

target_link_libraries(jlox
    PRIVATE
        fmt::fmt
)
