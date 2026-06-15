#ifndef PARSER_HPP
#define PARSER_HPP
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

  void parse_Prog();
  void parse_ListaGlobal();
  void parse_NivelGlobal();
  void parse_DeclaraImport();
  void parse_DeclaraGlobal();
  void parse_DeclaraFuncao();
  void parse_ListaParametros();
  void parse_CaudaParametros();
  void parse_Parametro();
  void parse_Bloco();
  void parse_ListaComandos();
  void parse_Comando();
  void parse_ComandoSimples();
  void parse_CaudaComandoId();
  void parse_ComandoComposto();
  void parse_DeclaraVariavel();
  void parse_Inicializacao();
  void parse_ComandoRetorno();
  void parse_ComandoPrint();
  void parse_Tipo();
  void parse_Condicao();
  void parse_ExpressaoOu();
  void parse_CaudaExpressaoOu();
  void parse_ExpressaoE();
  void parse_CaudaExpressaoE();
  void parse_ExpLogicaUnaria();
  void parse_ExpressaoRelacional();
  void parse_CaudaRelacional();
  void parse_ExpressaoAritmetica();
  void parse_CaudaExpressaoAritmetica();
  void parse_Termo();
  void parse_CaudaTermo();
  void parse_Fator();
  void parse_CaudaFator();
  void parse_ListaArgumentos();
  void parse_CaudaArgumentos();
  void parse_ComandoIf();
  void parse_ComandoElse();
  void parse_CaudaElse();
  void parse_ComandoWhile();
  void parse_ComandoDoWhile();
  void parse_ComandoFor();

public:
  Parser(const std::vector<Token> &tokens_lexer);
  void parser();
};

#endif
