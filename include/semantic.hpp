#ifndef SEMATIC_HPP
#define SEMATIC_HPP

#include "ast.hpp"
#include "symtab.hpp"
#include <string>
class AnalisadorSematico {
private:
  Scope *escopoAtual;

  void analisar_node(ASTNode *node);
  DataType get_type_node(ASTNode *node);
  DataType traduzir_para_string(const std::string &str_tipo);
  std::string type_to_string(DataType t);
  DataType funcaoAtualRetorno;

public:
  AnalisadorSematico();
  ~AnalisadorSematico() = default;

  void Analisar_sematica(ASTNode *raiz_ast);
};
#endif // SEMATIC_HPP
