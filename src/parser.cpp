#include "../include/parser.hpp"
#include <stdexcept>
#include <vector>

Parser::Parser(const std::vector<Token> &tokens_lexer)
    : tokens(tokens_lexer), atual(0) {}

ASTNode *Parser::parser() {
  try {
    return parse_Prog();
  } catch (const std::runtime_error &e) {
    throw;
    return nullptr;
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
    erro("'" + tipo + "'");
  }
}

void Parser::erro(const std::string &esperado) {
  Token t = peek();
  std::string encontrado =
      (t.type == "EOF") ? "fim de arquivo (EOF)" : "'" + t.value + "'";
  throw std::runtime_error("Erro de sintaxe na linha " +
                           std::to_string(t.line) + ", coluna " +
                           std::to_string(t.col) + ": esperado " + esperado +
                           ", mas encontrou " + encontrado);
}

bool Parser::is_eof() { return peek().type == "EOF"; }

ASTNode *Parser::parse_Prog() {
  if (peek().type == "IMPORT" || peek().type == "VAR" || peek().type == "FUN" ||
      peek().type == "EOF") {
    ASTNode *listaGlobal = parse_ListaGlobal();
    match("EOF");

    ASTNode *progNode = new ASTNode(NodeType::PROG);
    if (listaGlobal)
      progNode->adopt(listaGlobal);
    return progNode;
  } else {
    erro("fun, import, var ou EOF");
    return nullptr;
  }
}

ASTNode *Parser::parse_ListaGlobal() {
  if (peek().type == "IMPORT" || peek().type == "VAR" || peek().type == "FUN") {
    ASTNode *nivel = parse_NivelGlobal();
    ASTNode *cauda = parse_ListaGlobal();

    if (nivel && cauda) {
      nivel->make_siblings(cauda);
    }
    return nivel ? nivel : cauda;
  } else if (peek().type == "EOF") {
    return nullptr;
  } else {
    erro("fun, import, var ou EOF");
    return nullptr;
  }
}

ASTNode *Parser::parse_NivelGlobal() {
  if (peek().type == "IMPORT") {
    return parse_DeclaraImport();
  } else if (peek().type == "FUN") {
    return parse_DeclaraFuncao();
  } else if (peek().type == "VAR") {
    return parse_DeclaraGlobal();
  } else {
    erro("fun, import ou var");
    return nullptr;
  }
}

// DeclaraImport -> 'import' 'STRING' ';'
ASTNode *Parser::parse_DeclaraImport() {
  if (peek().type == "IMPORT") {
    match("IMPORT");
    std::string caminho = peek().value;
    match("STRING");
    match("SEMICOLON");

    caminho = caminho.substr(1, caminho.size() - 2);
    return new ASTNode(NodeType::IMPORT, caminho);
  } else {
    erro("import, caminho de arquivo entre aspas ou ';'");
    return nullptr;
  }
}

ASTNode *Parser::parse_DeclaraGlobal() {
  if (peek().type == "VAR") {
    return parse_DeclaraVariavel();
  } else {
    erro("'var'");
    return nullptr;
  }
}

ASTNode *Parser::parse_DeclaraFuncao() {
  if (peek().type == "FUN") {
    match("FUN");
    std::string nome_funcao = peek().value;
    match("ID");
    match("LPAREN");
    ASTNode *params = parse_ListaParametros();
    match("RPAREN");
    match("COLON");
    ASTNode *tipoRetorno = parse_Tipo();
    ASTNode *bloco = parse_Bloco();

    ASTNode *funNode = new ASTNode(NodeType::FUN_DECL, nome_funcao);
    if (tipoRetorno)
      funNode->adopt(tipoRetorno);
    if (params)
      funNode->adopt(params);
    if (bloco)
      funNode->adopt(bloco);
    return funNode;
  } else {
    erro("'fun'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ListaParametros() {
  if (peek().type == "ID") {
    ASTNode *param = parse_Parametro();
    ASTNode *cauda = parse_CaudaParametros();

    if (param && cauda) {
      param->make_siblings(cauda);
    }
    return param ? param : cauda;
  } else if (peek().type == "RPAREN") {
    return nullptr;
  } else {
    erro("identificador ou ')'");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaParametros() {
  if (peek().type == "COMMA") {
    match("COMMA");
    ASTNode *param = parse_Parametro();
    ASTNode *cauda = parse_CaudaParametros();

    if (param && cauda) {
      param->make_siblings(cauda);
    }
    return param ? param : cauda;
  } else if (peek().type == "RPAREN") {
    return nullptr;
  } else {
    erro("',' ou ')'");
    return nullptr;
  }
}

ASTNode *Parser::parse_Parametro() {
  if (peek().type == "ID") {
    std::string nome_param = peek().value;
    match("ID");
    match("COLON");
    ASTNode *tipo = parse_Tipo();

    ASTNode *paramNode = new ASTNode(NodeType::PARAM, nome_param);
    if (tipo)
      paramNode->adopt(tipo);
    return paramNode;
  } else {
    erro("identificador");
    return nullptr;
  }
}

ASTNode *Parser::parse_Bloco() {
  if (peek().type == "LBRACE") {
    match("LBRACE");
    ASTNode *comandos = parse_ListaComandos();
    match("RBRACE");

    ASTNode *blocoNode = new ASTNode(NodeType::BLOCK);
    if (comandos)
      blocoNode->adopt(comandos);
    return blocoNode;
  } else {
    erro("'{'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ListaComandos() {
  if (peek().type == "ID" || peek().type == "VAR" || peek().type == "RETURN" ||
      peek().type == "PRINT" || peek().type == "WHILE" || peek().type == "IF" ||
      peek().type == "DO" || peek().type == "FOR") {

    ASTNode *cmd = parse_Comando();
    ASTNode *cauda = parse_ListaComandos();

    if (cmd && cauda) {
      cmd->make_siblings(cauda);
    }
    return cmd ? cmd : cauda;
  } else if (peek().type == "RBRACE") {
    return nullptr;
  } else {
    erro("identificador, 'var', 'return', 'print', 'while', 'if', 'for' ou "
         "'do'");
    return nullptr;
  }
}

ASTNode *Parser::parse_Comando() {
  if (peek().type == "ID" || peek().type == "VAR" || peek().type == "RETURN" ||
      peek().type == "PRINT") {
    return parse_ComandoSimples();
  } else if (peek().type == "WHILE" || peek().type == "IF" ||
             peek().type == "DO" || peek().type == "FOR") {
    return parse_ComandoComposto();
  } else {
    erro("identificador, 'var', 'return', 'print', 'while', 'if', 'for' ou "
         "'do'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ComandoSimples() {
  if (peek().type == "VAR") {
    return parse_DeclaraVariavel();
  } else if (peek().type == "RETURN") {
    return parse_ComandoRetorno();
  } else if (peek().type == "PRINT") {
    return parse_ComandoPrint();
  } else if (peek().type == "ID") {
    std::string nome_id = peek().value;
    match("ID");
    return parse_CaudaComandoId(nome_id);
  } else {
    erro("identificador, 'var', 'print' ou 'return'");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaComandoId(const std::string &nome_id) {
  if (peek().type == "ASSIGN") {
    match("ASSIGN");
    ASTNode *expr = parse_ExpressaoAritmetica();
    match("SEMICOLON");

    ASTNode *assignNode = new ASTNode(NodeType::ASSIGN);
    assignNode->adopt(new ASTNode(NodeType::ID, nome_id));
    if (expr)
      assignNode->adopt(expr);
    return assignNode;
  } else if (peek().type == "LPAREN") {
    match("LPAREN");
    ASTNode *args = parse_ListaArgumentos();
    match("RPAREN");
    match("SEMICOLON");

    ASTNode *callNode = new ASTNode(NodeType::CALL, nome_id);
    if (args)
      callNode->adopt(args);
    return callNode;
  } else {
    erro("'=' ou '('");
    return nullptr;
  }
}

ASTNode *Parser::parse_ComandoComposto() {
  if (peek().type == "WHILE") {
    return parse_ComandoWhile();
  } else if (peek().type == "FOR") {
    return parse_ComandoFor();
  } else if (peek().type == "DO") {
    return parse_ComandoDoWhile();
  } else if (peek().type == "IF") {
    return parse_ComandoIf();
  } else {
    erro("'while', 'do', 'for' ou 'if'");
    return nullptr;
  }
}

ASTNode *Parser::parse_DeclaraVariavel() {
  if (peek().type == "VAR") {
    match("VAR");
    std::string nome_var = peek().value;
    match("ID");
    match("COLON");
    ASTNode *tipo = parse_Tipo();
    ASTNode *init = parse_Inicializacao();
    match("SEMICOLON");

    ASTNode *varNode = new ASTNode(NodeType::VAR_DECL, nome_var);
    if (tipo)
      varNode->adopt(tipo);
    if (init)
      varNode->adopt(init);
    return varNode;
  } else {
    erro("'var'");
    return nullptr;
  }
}

ASTNode *Parser::parse_Inicializacao() {
  if (peek().type == "ASSIGN") {
    match("ASSIGN");
    return parse_ExpressaoAritmetica();
  } else if (peek().type == "SEMICOLON") {
    return nullptr;
  } else {
    erro("'=' ou ';'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ComandoRetorno() {
  if (peek().type == "RETURN") {
    match("RETURN");
    ASTNode *expr = parse_ExpressaoAritmetica();
    match("SEMICOLON");

    ASTNode *retNode = new ASTNode(NodeType::RETURN);
    if (expr)
      retNode->adopt(expr);
    return retNode;
  } else {
    erro("'return'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ComandoPrint() {
  if (peek().type == "PRINT") {
    match("PRINT");
    match("LPAREN");
    ASTNode *expr = parse_ExpressaoAritmetica();
    match("RPAREN");
    match("SEMICOLON");

    ASTNode *printNode = new ASTNode(NodeType::PRINT);
    if (expr)
      printNode->adopt(expr);
    return printNode;
  } else {
    erro("'print'");
    return nullptr;
  }
}

ASTNode *Parser::parse_Tipo() {
  std::string rotulo_tipo = peek().value;
  if (peek().type == "TYPE_INT") {
    match("TYPE_INT");
  } else if (peek().type == "TYPE_FLOAT") {
    match("TYPE_FLOAT");
  } else if (peek().type == "TYPE_CHAR") {
    match("TYPE_CHAR");
  } else if (peek().type == "TYPE_VOID") {
    match("TYPE_VOID");
  } else if (peek().type == "TYPE_STRING") {
    match("TYPE_STRING");
  } else {
    erro("um tipo (como 'int', 'float', 'string', 'void' ou "
         "'char')");
    return nullptr;
  }
  return new ASTNode(NodeType::TYPE, rotulo_tipo);
}

ASTNode *Parser::parse_Condicao() {
  if (peek().type == "NOT" || peek().type == "LPAREN" || peek().type == "ID" ||
      peek().type == "NUMBER" || peek().type == "CHAR" ||
      peek().type == "STRING" || peek().type == "MINUS") {
    return parse_ExpressaoOu();
  } else {
    erro("'!', '(', identificador, número, caractere, string ou '-'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ExpressaoOu() {
  if (peek().type == "NOT" || peek().type == "LPAREN" || peek().type == "ID" ||
      peek().type == "NUMBER" || peek().type == "CHAR" ||
      peek().type == "STRING" || peek().type == "MINUS") {
    ASTNode *esq = parse_ExpressaoE();
    return parse_CaudaExpressaoOu(esq);
  } else {
    erro("'!', '(', identificador, número, caractere, string ou '-'");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaExpressaoOu(ASTNode *esq) {
  if (peek().type == "OR") {
    match("OR");
    ASTNode *dir = parse_ExpressaoE();

    ASTNode *opNode = new ASTNode(NodeType::BIN_OP, "||");
    opNode->adopt(esq);
    if (dir)
      opNode->adopt(dir);

    return parse_CaudaExpressaoOu(opNode);
  } else if (peek().type == "RPAREN") {
    return esq; // ε
  } else {
    erro("'||' ou ')'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ExpressaoE() {
  if (peek().type == "NOT" || peek().type == "LPAREN" || peek().type == "ID" ||
      peek().type == "NUMBER" || peek().type == "CHAR" ||
      peek().type == "STRING" || peek().type == "MINUS") {
    ASTNode *esq = parse_ExpLogicaUnaria();
    return parse_CaudaExpressaoE(esq);
  } else {
    erro("'!', '(', identificador, número, caractere, string ou '-'");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaExpressaoE(ASTNode *esq) {
  if (peek().type == "AND") {
    match("AND");
    ASTNode *dir = parse_ExpLogicaUnaria();

    ASTNode *opNode = new ASTNode(NodeType::BIN_OP, "&&");
    opNode->adopt(esq);
    if (dir)
      opNode->adopt(dir);

    return parse_CaudaExpressaoE(opNode);
  } else if (peek().type == "RPAREN" || peek().type == "OR") {
    return esq;
  } else {
    erro("'&&', ')' ou '||'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ExpLogicaUnaria() {
  if (peek().type == "NOT") {
    match("NOT");
    ASTNode *operando = parse_ExpLogicaUnaria();

    ASTNode *unNode = new ASTNode(NodeType::UN_OP, "!");
    if (operando)
      unNode->adopt(operando);
    return unNode;
  } else if (peek().type == "LPAREN") {
    match("LPAREN");
    ASTNode *cond = parse_Condicao();
    match("RPAREN");
    return cond;
  } else if (peek().type == "ID" || peek().type == "NUMBER" ||
             peek().type == "CHAR" || peek().type == "STRING" ||
             peek().type == "MINUS") {
    return parse_ExpressaoRelacional();
  } else {
    erro("'!', '(', identificador, número, caractere, string ou '-'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ExpressaoRelacional() {
  if (peek().type == "ID" || peek().type == "NUMBER" || peek().type == "CHAR" ||
      peek().type == "STRING" || peek().type == "MINUS") {
    ASTNode *esq = parse_ExpressaoAritmetica();
    return parse_CaudaRelacional(esq);
  } else {
    erro("identificador, número, caractere, string ou '-'");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaRelacional(ASTNode *esq) {
  if (peek().type == "EQ" || peek().type == "NEQ" || peek().type == "GT" ||
      peek().type == "LT" || peek().type == "GE" || peek().type == "LE") {

    std::string op = peek().value;
    advance();
    ASTNode *dir = parse_ExpressaoAritmetica();

    ASTNode *relNode = new ASTNode(NodeType::BIN_OP, op);
    relNode->adopt(esq);
    if (dir)
      relNode->adopt(dir);
    return relNode;
  } else if (peek().type == "RPAREN" || peek().type == "AND" ||
             peek().type == "OR") {
    return esq; // ε
  } else {
    erro("'==', '!=', '>=', '<=', '<', '>', ')', '&&' ou '||'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ExpressaoAritmetica() {
  if (peek().type == "ID" || peek().type == "NUMBER" || peek().type == "CHAR" ||
      peek().type == "STRING" || peek().type == "MINUS") {
    ASTNode *esq = parse_Termo();
    return parse_CaudaExpressaoAritmetica(esq);
  } else {
    erro("uma expressão");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaExpressaoAritmetica(ASTNode *esq) {
  if (peek().type == "PLUS" || peek().type == "MINUS") {
    std::string op = peek().value;
    advance();
    ASTNode *dir = parse_Termo();

    ASTNode *opNode = new ASTNode(NodeType::BIN_OP, op);
    opNode->adopt(esq);
    if (dir)
      opNode->adopt(dir);

    return parse_CaudaExpressaoAritmetica(opNode);
  } else if (peek().type == "SEMICOLON" || peek().type == "RPAREN" ||
             peek().type == "OR" || peek().type == "AND" ||
             peek().type == "EQ" || peek().type == "NEQ" ||
             peek().type == "GT" || peek().type == "LT" ||
             peek().type == "GE" || peek().type == "LE" ||
             peek().type == "COMMA") {
    return esq;
  } else {
    erro("',', ';', '||', '&&', '==', '!=', '<', '>', '<=', '>=', ')', '-' ou "
         "'+'");
    return nullptr;
  }
}

ASTNode *Parser::parse_Termo() {
  if (peek().type == "ID" || peek().type == "NUMBER" || peek().type == "CHAR" ||
      peek().type == "STRING" || peek().type == "MINUS") {
    ASTNode *esq = parse_Fator();
    return parse_CaudaTermo(esq);
  } else {
    erro("identificador, número, caractere, string ou '-'");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaTermo(ASTNode *esq) {
  if (peek().type == "MULT" || peek().type == "DIV" || peek().type == "MOD") {
    std::string op = peek().value;
    advance();
    ASTNode *dir = parse_Fator();

    ASTNode *opNode = new ASTNode(NodeType::BIN_OP, op);
    opNode->adopt(esq);
    if (dir)
      opNode->adopt(dir);

    return parse_CaudaTermo(opNode);
  } else if (peek().type == "SEMICOLON" || peek().type == "RPAREN" ||
             peek().type == "OR" || peek().type == "AND" ||
             peek().type == "EQ" || peek().type == "NEQ" ||
             peek().type == "GT" || peek().type == "LT" ||
             peek().type == "GE" || peek().type == "LE" ||
             peek().type == "COMMA" || peek().type == "PLUS" ||
             peek().type == "MINUS") {
    return esq; // ε
  } else {
    erro("',', ';', '||', '&&', '==', '!=', '<', '>', '<=', '>=', ')', '-', "
         "'+', '*', '/' ou '%'");
    return nullptr;
  }
}

ASTNode *Parser::parse_Fator() {
  if (peek().type == "ID") {
    std::string nome_id = peek().value;
    match("ID");
    ASTNode *idNode = new ASTNode(NodeType::ID, nome_id);
    return parse_CaudaFator(idNode);
  } else if (peek().type == "NUMBER") {
    std::string val = peek().value;
    match("NUMBER");
    return new ASTNode(NodeType::NUMBER, val);
  } else if (peek().type == "CHAR") {
    std::string val = peek().value;
    match("CHAR");
    return new ASTNode(NodeType::CHAR, val);
  } else if (peek().type == "STRING") {
    std::string val = peek().value;
    match("STRING");
    return new ASTNode(NodeType::STRING, val);
  } else if (peek().type == "MINUS") {
    match("MINUS");
    ASTNode *operando = parse_Fator();
    ASTNode *unNode = new ASTNode(NodeType::UN_OP, "-");
    if (operando)
      unNode->adopt(operando);
    return unNode;
  } else {
    erro("identificador, número, caractere, string ou '-'");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaFator(ASTNode *idNode) {
  if (peek().type == "LPAREN") {
    match("LPAREN");
    ASTNode *args = parse_ListaArgumentos();
    match("RPAREN");

    ASTNode *callNode = new ASTNode(NodeType::CALL, idNode->value);
    delete idNode;
    if (args)
      callNode->adopt(args);
    return callNode;
  } else if (peek().type == "SEMICOLON" || peek().type == "RPAREN" ||
             peek().type == "OR" || peek().type == "AND" ||
             peek().type == "EQ" || peek().type == "NEQ" ||
             peek().type == "GT" || peek().type == "LT" ||
             peek().type == "GE" || peek().type == "LE" ||
             peek().type == "COMMA" || peek().type == "PLUS" ||
             peek().type == "MINUS" || peek().type == "MULT" ||
             peek().type == "DIV" || peek().type == "MOD") {
    return idNode;
  } else {
    erro("',', ';', '||', '&&', '==', '!=', '<', '>', '<=', '>=', ')', '-', "
         "'+', '*', '/', '%' ou '('");
    return nullptr;
  }
}

ASTNode *Parser::parse_ListaArgumentos() {
  if (peek().type == "ID" || peek().type == "NUMBER" || peek().type == "CHAR" ||
      peek().type == "STRING" || peek().type == "MINUS") {
    ASTNode *expr = parse_ExpressaoAritmetica();
    ASTNode *cauda = parse_CaudaArgumentos();

    if (expr && cauda) {
      expr->make_siblings(cauda);
    }
    return expr ? expr : cauda;
  } else if (peek().type == "RPAREN") {
    return nullptr;
  } else {
    erro("uma lista de argumentos válida");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaArgumentos() {
  if (peek().type == "COMMA") {
    match("COMMA");
    ASTNode *expr = parse_ExpressaoAritmetica();
    ASTNode *cauda = parse_CaudaArgumentos();

    if (expr && cauda) {
      expr->make_siblings(cauda);
    }
    return expr ? expr : cauda;
  } else if (peek().type == "RPAREN") {
    return nullptr; // ε
  } else {
    erro("identificador, número ou ')'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ComandoIf() {
  if (peek().type == "IF") {
    match("IF");
    match("LPAREN");
    ASTNode *cond = parse_Condicao();
    match("RPAREN");
    ASTNode *blocoThen = parse_Bloco();
    ASTNode *blocoElse = parse_ComandoElse();

    ASTNode *ifNode = new ASTNode(NodeType::IF);
    if (cond)
      ifNode->adopt(cond);
    if (blocoThen)
      ifNode->adopt(blocoThen);
    ifNode->adopt(blocoElse ? blocoElse : new ASTNode(NodeType::EMPTY));
    return ifNode;
  } else {
    erro("'if'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ComandoElse() {
  if (peek().type == "ELSE") {
    match("ELSE");
    return parse_CaudaElse();
  } else if (peek().type == "RBRACE" || peek().type == "ID" ||
             peek().type == "VAR" || peek().type == "RETURN" ||
             peek().type == "PRINT" || peek().type == "WHILE" ||
             peek().type == "IF" || peek().type == "DO" ||
             peek().type == "FOR") {
    return nullptr;
  } else {
    erro("'else' ou o próximo comando");
    return nullptr;
  }
}

ASTNode *Parser::parse_CaudaElse() {
  if (peek().type == "LBRACE") {
    return parse_Bloco();
  } else if (peek().type == "IF") {
    return parse_ComandoIf();
  } else {
    erro("'{' para início de bloco ou 'if' para comando condicional composto");
    return nullptr;
  }
}

ASTNode *Parser::parse_ComandoWhile() {
  if (peek().type == "WHILE") {
    match("WHILE");
    match("LPAREN");
    ASTNode *cond = parse_Condicao();
    match("RPAREN");
    ASTNode *corpo = parse_Bloco();

    ASTNode *whileNode = new ASTNode(NodeType::WHILE);
    if (cond)
      whileNode->adopt(cond);
    if (corpo)
      whileNode->adopt(corpo);
    return whileNode;
  } else {
    erro("'while'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ComandoDoWhile() {
  if (peek().type == "DO") {
    match("DO");
    ASTNode *corpo = parse_Bloco();
    match("WHILE");
    match("LPAREN");
    ASTNode *cond = parse_Condicao();
    match("RPAREN");
    match("SEMICOLON");

    ASTNode *doNode = new ASTNode(NodeType::DO_WHILE);
    if (corpo)
      doNode->adopt(corpo);
    if (cond)
      doNode->adopt(cond);
    return doNode;
  } else {
    erro("'do'");
    return nullptr;
  }
}

ASTNode *Parser::parse_ComandoFor() {
  if (peek().type == "FOR") {
    match("FOR");
    match("LPAREN");
    ASTNode *init = parse_DeclaraVariavel();
    ASTNode *cond = parse_Condicao();
    match("RPAREN");
    ASTNode *corpo = parse_Bloco();

    return make_family(NodeType::FOR, {init, cond, corpo});
  } else {
    erro("'for'");
    return nullptr;
  }
}
