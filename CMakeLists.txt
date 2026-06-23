cmake_minimum_required(VERSION 3.16)
project(nir_lang LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)

include_directories(include)

add_executable(nir
        include/ast.hpp
        include/analisador_lexico.hpp
        include/analisador_semantico.hpp
        include/analisador_sintatico.hpp
        include/carregador_programa.hpp
        include/gerador_sam.hpp
        include/tabela_simbolos.hpp
        src/analisador_lexico.cpp
        src/analisador_semantico.cpp
        src/analisador_sintatico.cpp
        src/carregador_programa.cpp
        src/gerador_sam.cpp
        main.cpp
)

enable_testing()
find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_test(
        NAME example_tests
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/scripts/test_examples.py
                --compiler $<TARGET_FILE:nir>
)
