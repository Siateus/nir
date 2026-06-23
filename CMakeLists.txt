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

find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_custom_target(exemplos
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/scripts/compile_valid_examples.py
                --compiler $<TARGET_FILE:nir>
                --output-dir ${CMAKE_CURRENT_BINARY_DIR}/exemplos_sam
        DEPENDS nir
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Compilando exemplos Nir em arquivos .sam"
)
