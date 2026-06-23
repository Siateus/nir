#include "carregador_programa.hpp"
#include "ast.hpp"
#include "analisador_lexico.hpp"
#include "analisador_sintatico.hpp"
#include <fstream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void validar_extensao_nir(const std::string &caminho) {
  constexpr char extensao[] = ".nir";
  if (caminho.size() < sizeof(extensao) - 1 ||
      caminho.compare(caminho.size() - (sizeof(extensao) - 1),
                      sizeof(extensao) - 1, extensao) != 0) {
    throw std::runtime_error("Formato de arquivo invalido: '" + caminho +
                             "'. Apenas arquivos .nir sao aceitos.");
  }
}

ASTNode *analisar_arquivo(const std::string &caminho) {
  validar_extensao_nir(caminho);

  std::fstream arquivo(caminho);
  if (!arquivo.is_open())
    throw std::runtime_error("Erro ao abrir: " + caminho);

  const std::vector<Token> tokens = AnalisadorLexico::analisar(arquivo);
  AnalisadorSintatico analisador_sintatico(tokens);
  return analisador_sintatico.analisar();
}

} // namespace

std::unique_ptr<ASTNode> carregar_programa(const std::string &caminho) {
  return std::unique_ptr<ASTNode>(analisar_arquivo(caminho));
}
