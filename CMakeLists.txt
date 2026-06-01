cmake_minimum_required(VERSION 4.2)
project(nir_lang)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(nir_lang
        include/ast.hpp
        include/codegen.hpp
        include/lexer.hpp
        include/parser.hpp
        include/sematic.hpp
        src/ast.cpp
        src/codegen.cpp
        src/lexer.cpp
        src/parser.cpp
        src/sematic.cpp
        main.cpp)
