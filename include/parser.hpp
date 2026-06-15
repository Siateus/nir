#ifndef PARSER_HPP
#define PARSER_HPP
#include "ast.hpp"
#include "lexer.hpp"
#include <vector>

class Parser {
private:
  std::vector<Token> tokens;
  size_t atual;
  Token peek();
  void match(const std::string &tipo);
  Token advance();
  bool is_eof();
  void erro(const std::string &esperado);

  ASTNode *parse_Prog();
  ASTNode *parse_ListaGlobal();
  ASTNode *parse_NivelGlobal();
  ASTNode *parse_DeclaraImport();
  ASTNode *parse_DeclaraGlobal();
  ASTNode *parse_DeclaraFuncao();
  ASTNode *parse_ListaParametros();
  ASTNode *parse_CaudaParametros();
  ASTNode *parse_Parametro();
  ASTNode *parse_Bloco();
  ASTNode *parse_ListaComandos();
  ASTNode *parse_Comando();
  ASTNode *parse_ComandoSimples();
  ASTNode *parse_CaudaComandoId(const std::string &nome_id);
  ASTNode *parse_ComandoComposto();
  ASTNode *parse_DeclaraVariavel();
  ASTNode *parse_Inicializacao();
  ASTNode *parse_ComandoRetorno();
  ASTNode *parse_ComandoPrint();
  ASTNode *parse_Tipo();
  ASTNode *parse_Condicao();
  ASTNode *parse_ExpressaoOu();
  ASTNode *parse_CaudaExpressaoOu(ASTNode *esq);
  ASTNode *parse_ExpressaoE();
  ASTNode *parse_CaudaExpressaoE(ASTNode *esq);
  ASTNode *parse_ExpLogicaUnaria();
  ASTNode *parse_ExpressaoRelacional();
  ASTNode *parse_CaudaRelacional(ASTNode *esq);
  ASTNode *parse_ExpressaoAritmetica();
  ASTNode *parse_CaudaExpressaoAritmetica(ASTNode *esq);
  ASTNode *parse_Termo();
  ASTNode *parse_CaudaTermo(ASTNode *esq);
  ASTNode *parse_Fator();
  ASTNode *parse_CaudaFator(ASTNode *idNode);
  ASTNode *parse_ListaArgumentos();
  ASTNode *parse_CaudaArgumentos();
  ASTNode *parse_ComandoIf();
  ASTNode *parse_ComandoElse();
  ASTNode *parse_CaudaElse();
  ASTNode *parse_ComandoWhile();
  ASTNode *parse_ComandoDoWhile();
  ASTNode *parse_ComandoFor();

public:
  Parser(const std::vector<Token> &tokens_lexer);
  ASTNode *parser();
};

#endif
