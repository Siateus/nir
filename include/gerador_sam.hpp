#ifndef GERADOR_SAM_HPP
#define GERADOR_SAM_HPP

#include "ast.hpp"
#include <string>
#include <vector>

class GeradorSAM {
private:
  std::vector<std::string> codigo_sam;
  std::vector<std::string> rotulos_interrupcao;
  std::vector<std::string> rotulos_continuacao;
  int contador_rotulos;
  std::string novo_rotulo();
  void liberar_string_local(int deslocamento);
  void liberar_string_global(int deslocamento);

  void traduzir_expressao(ASTNode *no);
  void traduzir_comando(ASTNode *no, bool traduzir_proximos = true);

public:
  GeradorSAM();
  ~GeradorSAM() = default;

  void gerar(ASTNode *raiz);

  void salvar(const std::string &nome_arquivo);
};

#endif
