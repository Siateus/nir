#include "include/analisador_semantico.hpp"
#include "include/carregador_programa.hpp"
#include "include/gerador_sam.hpp"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>

int main(int quantidade_argumentos, char *argumentos[]) {
  if (quantidade_argumentos < 2) {
    std::cerr << "Passe o arquivo do programa" << std::endl;
    return 1;
  }

  std::remove("saida.sam");

  try {
    std::unique_ptr<ASTNode> raiz = carregar_programa(argumentos[1]);

    AnalisadorSematico semantico;
    semantico.analisar(raiz.get());

    GeradorSAM gerador_sam;
    gerador_sam.gerar(raiz.get());
    gerador_sam.salvar("saida.sam");
  } catch (const std::runtime_error &erro) {
    std::cerr << "Erro de compilação\n" << erro.what() << std::endl;
    return 1;
  }
  return 0;
}
