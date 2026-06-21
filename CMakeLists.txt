cmake_minimum_required(VERSION 3.31.11)
project(nir_lang)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(nir
        include/ast.hpp
        include/codegen.hpp
        include/lexer.hpp
        include/parser.hpp
        include/semantic.hpp
        include/symtab.hpp
        src/codegen.cpp
        src/lexer.cpp
        src/parser.cpp
        src/semantic.cpp
        main.cpp
)

enable_testing()
add_test(NAME regression_tests COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/tests/run_tests.py)

