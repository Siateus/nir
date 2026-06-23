#ifndef CARREGADOR_PROGRAMA_HPP
#define CARREGADOR_PROGRAMA_HPP

#include "ast.hpp"
#include <memory>
#include <string>

std::unique_ptr<ASTNode> carregar_programa(const std::string &caminho);

#endif
