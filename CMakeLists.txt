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
    
)

target_include_directories(myshell
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