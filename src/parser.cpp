#include "../include/parser.hpp"
#include <iostream>
#include <stdexcept>
#include <vector>

Parser::Parser(const std::vector<Token> &tokens_lexer)
    : tokens(tokens_lexer), atual(0) {}

void Parser::parser() {
  try {
    parse_Prog();
  } catch (const std::runtime_error &e) {
    std::cerr << "Falha na analise sintatica: " << e.what() << std::endl;
  }
}

Token Parser::peek() { return tokens[atual]; }

Token Parser::advance() {
  if (!is_eof()) {
    atual++;
  }
  return tokens[atual - 1];
}

void Parser::match(const std::string &tipo) {
  if (peek().type == tipo) {
    advance();
  } else {
    throw std::runtime_error("Syntax error: expected '" + tipo +
                             "', but find '" + peek().value + "'");
  }
}

bool Parser::is_eof() { return peek().type == "EOF"; }

void Parser::parse_Prog() {
  if (peek().type == "IMPORT" || peek().type == "VAR" || peek().type == "FUN" ||
      peek().type == "EOF") {
    parse_ListaGlobal();
    match("EOF");
  } else {
    throw std::runtime_error("Syntax error, expected: fun, import, var, EOF");
  }
}

void Parser::parse_ListaGlobal() {
  if (peek().type == "IMPORT" || peek().type == "VAR" || peek().type == "FUN") {
    parse_NivelGlobal();
    parse_ListaGlobal();
  } else if (peek().type == "EOF") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: fun, import, var, EOF");
  }
}

void Parser::parse_NivelGlobal() {
  if (peek().type == "IMPORT") {
    parse_DeclaraImport();
  } else if (peek().type == "FUN") {
    parse_DeclaraFuncao();
  } else if (peek().type == "VAR") {
    parse_DeclaraGlobal();
  } else {
    throw std::runtime_error("Syntax error, expected: fun, import, var");
  }
}

void Parser::parse_DeclaraImport() {
  if (peek().type == "IMPORT") {
    match("IMPORT");
    match("ID");
    match("SEMICOLON");
  } else {
    throw std::runtime_error("Syntax error, expected: import, id, ';'");
  }
}

void Parser::parse_DeclaraGlobal() {
  if (peek().type == "VAR") {
    parse_DeclaraVariavel();
  } else {
    throw std::runtime_error("Syntax error, expected: var");
  }
}

void Parser::parse_DeclaraFuncao() {
  if (peek().type == "FUN") {
    match("FUN");
    match("ID");
    match("LPAREN");
    parse_ListaParametros();
    match("RPAREN");
    match("COLON");
    parse_Tipo();
    parse_Bloco();
  } else {
    throw std::runtime_error("Syntax error, expected: fun");
  }
}

void Parser::parse_ListaParametros() {
  if (peek().type == "ID") {
    parse_Parametro();
    parse_CaudaParametros();
  } else if (peek().type == "RPAREN") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: id, ')'");
  }
}

void Parser::parse_CaudaParametros() {
  if (peek().type == "COMMA") {
    match("COMMA");
    parse_Parametro();
    parse_CaudaParametros();
  } else if (peek().type == "RPAREN") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: ',', ')'");
  }
}

void Parser::parse_Parametro() {
  if (peek().type == "ID") {
    match("ID");
    match("COLON");
    parse_Tipo();
  } else {
    throw std::runtime_error("Syntax error, expected: id");
  }
}

void Parser::parse_Bloco() {
  if (peek().type == "LBRACE") {
    match("LBRACE");
    parse_ListaComandos();
    match("RBRACE");
  } else {
    throw std::runtime_error("Syntax error, expected: '{'");
  }
}

void Parser::parse_ListaComandos() {
  if (peek().type == "ID" || peek().type == "VAR" || peek().type == "RETURN" ||
      peek().type == "PRINT" || peek().type == "WHILE" || peek().type == "IF" ||
      peek().type == "DO" || peek().type == "FOR") {
    parse_Comando();
    parse_ListaComandos();
  } else if (peek().type == "RBRACE") {
    return;
  } else {
    throw std::runtime_error(
        "Syntax error, expected: id, var, return, print, while, if, for, do");
  }
}

void Parser::parse_Comando() {
  if (peek().type == "ID" || peek().type == "VAR" || peek().type == "RETURN" ||
      peek().type == "PRINT") {
    parse_ComandoSimples();
  } else if (peek().type == "WHILE" || peek().type == "IF" ||
             peek().type == "DO" || peek().type == "FOR") {
    parse_ComandoComposto();
  } else {
    throw std::runtime_error(
        "Syntax error, expected: id, var, return, print, while, if, for, do");
  }
}

void Parser::parse_ComandoSimples() {
  if (peek().type == "VAR") {
    parse_DeclaraVariavel();
  } else if (peek().type == "RETURN") {
    parse_ComandoRetorno();
  } else if (peek().type == "ID") {
    match("ID");
    parse_CaudaComandoId();
  } else if (peek().type == "PRINT") {
    parse_ComandoPrint();
  } else {
    throw std::runtime_error("Syntax error, expected: id, var, print, return");
  }
}

void Parser::parse_CaudaComandoId() {
  if (peek().type == "ASSIGN") {
    match("ASSIGN");
    parse_ExpressaoAritmetica();
    match("SEMICOLON");
  } else if (peek().type == "LPAREN") {
    match("LPAREN");
    parse_ListaArgumentos();
    match("RPAREN");
    match("SEMICOLON");
  } else {
    throw std::runtime_error("Syntax error, expected: =, '('");
  }
}

void Parser::parse_ComandoComposto() {
  if (peek().type == "WHILE") {
    parse_ComandoWhile();
  } else if (peek().type == "FOR") {
    parse_ComandoFor();
  } else if (peek().type == "DO") {
    parse_ComandoDoWhile();
  } else if (peek().type == "IF") {
    parse_ComandoIf();
  } else {
    throw std::runtime_error("Syntax error, expected: while, do, for, if");
  }
}

void Parser::parse_DeclaraVariavel() {
  if (peek().type == "VAR") {
    match("VAR");
    match("ID");
    match("COLON");
    parse_Tipo();
    parse_Inicializacao();
    match("SEMICOLON");
  } else {
    throw std::runtime_error("Syntax error, expected: var");
  }
}

void Parser::parse_Inicializacao() {
  if (peek().type == "ASSIGN") {
    match("ASSIGN");
    parse_ExpressaoAritmetica();
  } else if (peek().type == "SEMICOLON") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: =, ';'");
  }
}

void Parser::parse_ComandoRetorno() {
  if (peek().type == "RETURN") {
    match("RETURN");
    parse_ExpressaoAritmetica();
    match("SEMICOLON");
  } else {
    throw std::runtime_error("Syntax error, expected: return");
  }
}

void Parser::parse_ComandoPrint() {
  if (peek().type == "PRINT") {
    match("PRINT");
    match("LPAREN");
    parse_ExpressaoAritmetica();
    match("RPAREN");
    match("SEMICOLON");
  } else {
    throw std::runtime_error("Syntax error, expected: print");
  }
}

void Parser::parse_Tipo() {
  if (peek().type == "TYPE_I32") {
    match("TYPE_I32");
  } else if (peek().type == "TYPE_I64") {
    match("TYPE_I64");
  } else if (peek().type == "TYPE_F32") {
    match("TYPE_F32");
  } else if (peek().type == "TYPE_F64") {
    match("TYPE_F64");
  } else if (peek().type == "TYPE_CHAR") {
    match("TYPE_CHAR");
  } else if (peek().type == "TYPE_VOID") {
    match("TYPE_VOID");
  } else {
    throw std::runtime_error(
        "Syntax error, expected: i32, i64, f32, f64, void, char");
  }
}

void Parser::parse_Condicao() {
  if (peek().type == "NOT" || peek().type == "LPAREN" || peek().type == "ID" ||
      peek().type == "NUMBER") {
    parse_ExpressaoOu();
  } else {
    throw std::runtime_error("Syntax error, expected: '!', '(', id, number");
  }
}

void Parser::parse_ExpressaoOu() {
  if (peek().type == "NOT" || peek().type == "LPAREN" || peek().type == "ID" ||
      peek().type == "NUMBER") {
    parse_ExpressaoE();
    parse_CaudaExpressaoOu();
  } else {
    throw std::runtime_error("Syntax error, expected: '!', '(', id, number");
  }
}

void Parser::parse_CaudaExpressaoOu() {
  if (peek().type == "OR") {
    match("OR");
    parse_ExpressaoE();
    parse_CaudaExpressaoOu();
  } else if (peek().type == "RPAREN") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: '||', ')'");
  }
}

void Parser::parse_ExpressaoE() {
  if (peek().type == "NOT" || peek().type == "LPAREN" || peek().type == "ID" ||
      peek().type == "NUMBER") {
    parse_ExpLogicaUnaria();
    parse_CaudaExpressaoE();
  } else {
    throw std::runtime_error("Syntax error, expected: '!', '(', id, number");
  }
}

void Parser::parse_CaudaExpressaoE() {
  if (peek().type == "AND") {
    match("AND");
    parse_ExpLogicaUnaria();
    parse_CaudaExpressaoE();
  } else if (peek().type == "RPAREN" || peek().type == "OR") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: '&&', ')', '||'");
  }
}

void Parser::parse_ExpLogicaUnaria() {
  if (peek().type == "NOT") {
    match("NOT");
    parse_ExpLogicaUnaria();
  } else if (peek().type == "LPAREN") {
    match("LPAREN");
    parse_Condicao();
    match("RPAREN");
  } else if (peek().type == "ID" || peek().type == "NUMBER") {
    parse_ExpressaoRelacional();
  } else {
    throw std::runtime_error("Syntax error, expected: '!', '(', id, number");
  }
}

void Parser::parse_ExpressaoRelacional() {
  if (peek().type == "ID" || peek().type == "NUMBER") {
    parse_ExpressaoAritmetica();
    parse_CaudaRelacional();
  } else {
    throw std::runtime_error("Syntax error, expected: id, number");
  }
}

void Parser::parse_CaudaRelacional() {
  if (peek().type == "EQ") {
    match("EQ");
    parse_ExpressaoAritmetica();
  } else if (peek().type == "NEQ") {
    match("NEQ");
    parse_ExpressaoAritmetica();
  } else if (peek().type == "GT") {
    match("GT");
    parse_ExpressaoAritmetica();
  } else if (peek().type == "LT") {
    match("LT");
    parse_ExpressaoAritmetica();
  } else if (peek().type == "GE") {
    match("GE");
    parse_ExpressaoAritmetica();
  } else if (peek().type == "LE") {
    match("LE");
    parse_ExpressaoAritmetica();
  } else if (peek().type == "RPAREN" || peek().type == "AND" ||
             peek().type == "OR") {
    return;
  } else {
    throw std::runtime_error(
        "Syntax error, expected: ==, !=, >=, <=, <, >, '(', '&&', '||'");
  }
}

void Parser::parse_ExpressaoAritmetica() {
  if (peek().type == "ID" || peek().type == "NUMBER") {
    parse_Termo();
    parse_CaudaExpressaoAritmetica();
  } else {
    throw std::runtime_error("Syntax error, expected: id, number");
  }
}

void Parser::parse_CaudaExpressaoAritmetica() {
  if (peek().type == "PLUS") {
    match("PLUS");
    parse_Termo();
    parse_CaudaExpressaoAritmetica();
  } else if (peek().type == "MINUS") {
    match("MINUS");
    parse_Termo();
    parse_CaudaExpressaoAritmetica(); // ;, ), ||, &&, ==, !=, >, <, >=, <=, ,
  } else if (peek().type == "SEMICOLON" || peek().type == "RPAREN" ||
             peek().type == "OR" || peek().type == "AND" ||
             peek().type == "EQ" || peek().type == "NEQ" ||
             peek().type == "GT" || peek().type == "LT" ||
             peek().type == "GE" || peek().type == "LE" ||
             peek().type == "COMMA") {
    return;
  }
  throw std::runtime_error("Syntax error, expected: ',', ;, '||', '&&', ==, "
                           "!=, <, >, <=, >=, ')', -, +");
}

void Parser::parse_Termo() {
  if (peek().type == "ID" || peek().type == "NUMBER") {
    parse_Fator();
    parse_CaudaTermo();
  } else {
    throw std::runtime_error("Syntax error, expected: id, number");
  }
}

void Parser::parse_CaudaTermo() {
  if (peek().type == "MULT") {
    match("MULT");
    parse_Fator();
    parse_CaudaTermo();
  } else if (peek().type == "DIV") {
    match("DIV");
    parse_Fator();
    parse_CaudaTermo();
  } else if (peek().type == "MOD") {
    match("MOD");
    parse_Fator();
    parse_CaudaTermo();
  } else if (peek().type == "SEMICOLON" || peek().type == "RPAREN" ||
             peek().type == "OR" || peek().type == "AND" ||
             peek().type == "EQ" || peek().type == "NEQ" ||
             peek().type == "GT" || peek().type == "LT" ||
             peek().type == "GE" || peek().type == "LE" ||
             peek().type == "COMMA" || peek().type == "PLUS" ||
             peek().type == "MINUS") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: ',', ;, '||', '&&', ==, "
                             "!=, <, >, <=, >=, ')', -, +, *, /, %");
  }
}

void Parser::parse_Fator() {
  if (peek().type == "ID") {
    match("ID");
    parse_CaudaFator();
  } else if (peek().type == "NUMBER") {
    match("NUMBER");
  } else {
    throw std::runtime_error("Syntax error, expected: id, number");
  }
}

void Parser::parse_CaudaFator() {
  if (peek().type == "LPAREN") {
    match("LPAREN");
    parse_ListaArgumentos();
    match("RPAREN");
  } else if (peek().type == "SEMICOLON" || peek().type == "RPAREN" ||
             peek().type == "OR" || peek().type == "AND" ||
             peek().type == "EQ" || peek().type == "NEQ" ||
             peek().type == "GT" || peek().type == "LT" ||
             peek().type == "GE" || peek().type == "LE" ||
             peek().type == "COMMA" || peek().type == "PLUS" ||
             peek().type == "MINUS" || peek().type == "MULT" ||
             peek().type == "DIV" || peek().type == "MOD") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: ',', ;, '||', '&&', ==, "
                             "!=, <, >, <=, >=, ')', -, +, *, /, %, '('");
  }
}

void Parser::parse_ListaArgumentos() {
  if (peek().type == "ID" || peek().type == "NUMBER") {
    parse_ExpressaoAritmetica();
    parse_CaudaArgumentos();
  } else if (peek().type == "RPAREN") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: id, number, ')'");
  }
}

void Parser::parse_CaudaArgumentos() {
  if (peek().type == "COMMA") {
    match("COMMA");
    parse_ExpressaoAritmetica();
    parse_CaudaArgumentos();
  } else if (peek().type == "RPAREN") {
    return;
  } else {
    throw std::runtime_error("Syntax error, expected: id, number, ')'");
  }
}

void Parser::parse_ComandoIf() {
  if (peek().type == "IF") {
    match("IF");
    match("LPAREN");
    parse_Condicao();
    match("RPAREN");
    parse_Bloco();
    parse_ComandoElse();
  } else {
    throw std::runtime_error("Syntax error, expected: 'if'");
  }
}

void Parser::parse_ComandoElse() {
  if (peek().type == "ELSE") {
    match("ELSE");
    parse_CaudaElse();
  } else if (peek().type == "RBRACE" || peek().type == "ID" ||
             peek().type == "VAR" || peek().type == "RETURN" ||
             peek().type == "PRINT" || peek().type == "WHILE" ||
             peek().type == "IF" || peek().type == "DO" ||
             peek().type == "FOR") {
    return;
  } else {
    throw std::runtime_error(
        "Syntax error after if block, expected 'else' or next command");
  }
}

void Parser::parse_CaudaElse() {
  if (peek().type == "LBRACE") {
    parse_Bloco();
  } else if (peek().type == "IF") {
    parse_ComandoIf();
  } else {
    throw std::runtime_error("Syntax error after 'else': expected '{' for "
                             "block scope or 'if' for statement");
  }
}

void Parser::parse_ComandoWhile() {
  if (peek().type == "WHILE") {
    match("WHILE");
    match("LPAREN");
    parse_Condicao();
    match("RPAREN");
    parse_Bloco();
  } else {
    throw std::runtime_error("Syntax error: expected 'while'");
  }
}

void Parser::parse_ComandoDoWhile() {
  if (peek().type == "DO") {
    match("DO");
    parse_Bloco();
    match("WHILE");
    match("LPAREN");
    parse_Condicao();
    match("RPAREN");
    match("SEMICOLON");
  } else {
    throw std::runtime_error("Syntax error: expected 'do'");
  }
}

void Parser::parse_ComandoFor() {
  if (peek().type == "FOR") {
    match("FOR");
    match("LPAREN");
    parse_DeclaraVariavel();
    parse_Condicao();
    match("RPAREN");
    parse_Bloco();
  } else {
    throw std::runtime_error("Syntax error: expected 'for'");
  }
}
