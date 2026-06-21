#ifndef CODEGEN_HPP
#define CODEGEN_HPP

#include "ast.hpp"
#include <string>
#include <vector>

class GeradorSAM {
private:
  std::vector<std::string> codigoSAM;
  int contadorLabels;
  std::string novoLabel();

  bool eh_expressao_float(ASTNode *no);

  void traduzir_expressao(ASTNode *no);

  void traduzir_comando(ASTNode *no, bool traduzirProximos = true);

public:
  GeradorSAM();
  ~GeradorSAM() = default;

  void gerar(ASTNode *raiz);

  void salvar(const std::string &nomeArquivo);
};

#endif // !CODEGEN_HPP
