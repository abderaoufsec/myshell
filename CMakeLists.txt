cmake_minimum_required(VERSION 3.20)

project(myshell
    VERSION 0.1.0
    DESCRIPTION "A Unix shell implemented in C++"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(myshell
    src/main.cpp
)