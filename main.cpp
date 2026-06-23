#include "include/analisador_lexico.hpp"
#include "include/analisador_semantico.hpp"
#include "include/analisador_sintatico.hpp"
#include "include/gerador_sam.hpp"
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string nome_saida_sam(const std::string &nome) {
  constexpr const char extensao[] = ".sam";
  if (nome.size() >= 4 && nome.compare(nome.size() - 4, 4, extensao) == 0)
    return nome;
  return nome + extensao;
}

void validar_extensao_nir(const std::string &caminho) {
  constexpr char extensao[] = ".nir";
  if (caminho.size() < sizeof(extensao) - 1 ||
      caminho.compare(caminho.size() - (sizeof(extensao) - 1),
                      sizeof(extensao) - 1, extensao) != 0) {
    throw std::runtime_error("Formato de arquivo invalido: '" + caminho +
                             "'. Apenas arquivos .nir sao aceitos.");
  }
}

}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cerr << "Passe o arquivo do programa" << std::endl;
    return 1;
  }

  if (argc > 3) {
    std::cerr << "Uso: " << argv[0]
              << " arquivo.nir [nome_saida]" << std::endl;
    return 1;
  }

  std::string arquivo_saida =
      argc == 3 ? nome_saida_sam(argv[2]) : "saida.sam";

  std::remove(arquivo_saida.c_str());

  try {
    validar_extensao_nir(argv[1]);

    std::fstream arquivo(argv[1]);
    if (!arquivo.is_open())
      throw std::runtime_error("Erro ao abrir: " + std::string(argv[1]));

    std::vector<Token> tokens = AnalisadorLexico::analisar(arquivo);
    AnalisadorSintatico sintatico(tokens);
    std::unique_ptr<ASTNode> raiz(sintatico.analisar());
    AnalisadorSematico semantico;
    semantico.analisar(raiz.get());

    GeradorSAM gerador_sam;
    gerador_sam.gerar(raiz.get());
    gerador_sam.salvar(arquivo_saida);
  } catch (const std::runtime_error &erro) {
    std::cerr << "Erro de compilação\n" << erro.what() << std::endl;
    return 1;
  }
  return 0;
}
