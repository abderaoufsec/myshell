cmake_minimum_required(VERSION 3.20)

project(myshell
    VERSION 1.0.0
    DESCRIPTION "A Unix shell implemented in C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_executable(myshell
    src/main.cpp
    src/shell.cpp
    src/parser.cpp
    src/executor.cpp
    src/builtins.cpp
    src/jobs.cpp
    src/posix_compat.cpp
)

add_executable(myshell_redirection_tests
    tests/test_redirections.cpp
    src/parser.cpp
    src/executor.cpp
    src/builtins.cpp
    src/jobs.cpp
    src/posix_compat.cpp
)

add_executable(myshell_smoke_tests
    tests/test_shell_smoke.cpp
    src/parser.cpp
    src/executor.cpp
    src/builtins.cpp
    src/jobs.cpp
    src/posix_compat.cpp
)

target_include_directories(myshell
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(myshell_redirection_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_include_directories(myshell_smoke_tests
    PRIVATE
        ${PROJECT_SOURCE_DIR}/include
)

target_compile_options(myshell PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(myshell PRIVATE
        -Wshadow
        -Wconversion
        -Wsign-conversion
    )
endif()

enable_testing()
add_test(NAME redirection_regression COMMAND myshell_redirection_tests)
add_test(NAME shell_smoke COMMAND myshell_smoke_tests)