#ifndef ANALISADOR_SINTATICO_HPP
#define ANALISADOR_SINTATICO_HPP
#include "ast.hpp"
#include "analisador_lexico.hpp"
#include <unordered_set>
#include <vector>

class AnalisadorSintatico {
private:
  std::vector<Token> tokens;
  std::unordered_set<ASTNode *> nos_alocados;
  size_t atual;
  ASTNode *criar_no(NodeType tipo, const std::string &valor = "");
  void descartar_no(ASTNode *no);
  void liberar_arvore_parcial(ASTNode *no);
  Token peek();
  void match(const std::string &tipo);
  Token advance();
  bool chegou_ao_fim();
  void erro(const std::string &esperado);

  ASTNode *analisar_programa();
  ASTNode *analisar_lista_global();
  ASTNode *analisar_nivel_global();
  ASTNode *analisar_declaracao_import();
  ASTNode *analisar_declaracao_global();
  ASTNode *analisar_declaracao_funcao();
  ASTNode *analisar_lista_parametros();
  ASTNode *analisar_cauda_parametros();
  ASTNode *analisar_parametro();
  ASTNode *analisar_bloco();
  ASTNode *analisar_lista_comandos();
  ASTNode *analisar_comando();
  ASTNode *analisar_comando_simples();
  ASTNode *analisar_cauda_comando_id(const std::string &nome_id);
  ASTNode *analisar_comando_composto();
  ASTNode *analisar_declaracao_variavel();
  ASTNode *analisar_inicializacao();
  ASTNode *analisar_comando_retorno();
  ASTNode *analisar_comando_impressao();
  ASTNode *analisar_comando_interrupcao();
  ASTNode *analisar_comando_continuacao();
  ASTNode *analisar_tipo();
  ASTNode *analisar_condicao();
  ASTNode *analisar_expressao_ou();
  ASTNode *analisar_cauda_expressao_ou(ASTNode *esquerda);
  ASTNode *analisar_expressao_e();
  ASTNode *analisar_cauda_expressao_e(ASTNode *esquerda);
  ASTNode *analisar_expressao_logica_unaria();
  ASTNode *analisar_expressao_relacional();
  ASTNode *analisar_cauda_relacional(ASTNode *esquerda);
  ASTNode *analisar_expressao_aritmetica();
  ASTNode *analisar_cauda_expressao_aritmetica(ASTNode *esquerda);
  ASTNode *analisar_termo();
  ASTNode *analisar_cauda_termo(ASTNode *esquerda);
  ASTNode *analisar_fator();
  ASTNode *analisar_cauda_fator(ASTNode *no_id);
  ASTNode *analisar_lista_argumentos();
  ASTNode *analisar_cauda_argumentos();
  ASTNode *analisar_comando_se();
  ASTNode *analisar_comando_senao();
  ASTNode *analisar_cauda_senao();
  ASTNode *analisar_comando_enquanto();
  ASTNode *analisar_comando_faca_enquanto();
  ASTNode *analisar_comando_para();

public:
  explicit AnalisadorSintatico(const std::vector<Token> &tokens);
  ~AnalisadorSintatico();
  ASTNode *analisar();
};

#endif
