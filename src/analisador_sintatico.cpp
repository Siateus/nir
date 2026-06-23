#include "../include/analisador_sintatico.hpp"
#include <stdexcept>
#include <unordered_map>
#include <vector>

AnalisadorSintatico::AnalisadorSintatico(const std::vector<Token> &tokens_recebidos)
    : tokens(tokens_recebidos), atual(0) {}

AnalisadorSintatico::~AnalisadorSintatico() {
  while (!nos_alocados.empty()) {
    ASTNode *raiz = nullptr;
    for (ASTNode *no : nos_alocados) {
      if (!no->parent && no->leftmostSib == no) {
        raiz = no;
        break;
      }
    }
    if (!raiz)
      raiz = *nos_alocados.begin();
    liberar_arvore_parcial(raiz);
  }
}

ASTNode *AnalisadorSintatico::criar_no(NodeType tipo, const std::string &valor) {
  ASTNode *no = new ASTNode(tipo, valor);
  nos_alocados.insert(no);
  return no;
}

void AnalisadorSintatico::descartar_no(ASTNode *no) {
  nos_alocados.erase(no);
  delete no;
}

void AnalisadorSintatico::liberar_arvore_parcial(ASTNode *no) {
  if (!no || !nos_alocados.count(no))
    return;

  std::vector<ASTNode *> pilha = {no};
  while (!pilha.empty()) {
    ASTNode *no_atual = pilha.back();
    pilha.pop_back();
    if (!no_atual || !nos_alocados.erase(no_atual))
      continue;
    pilha.push_back(no_atual->leftmostChild);
    pilha.push_back(no_atual->next);
  }
  delete no;
}

ASTNode *AnalisadorSintatico::analisar() {
  try {
    ASTNode *raiz = analisar_programa();
    nos_alocados.clear();
    return raiz;
  } catch (const std::runtime_error &e) {
    throw;
    return nullptr;
  }
}

Token AnalisadorSintatico::peek() { return tokens[atual]; }

Token AnalisadorSintatico::advance() {
  Token token_atual = tokens[atual];
  if (atual + 1 < tokens.size()) {
    atual++;
  }
  return token_atual;
}

void AnalisadorSintatico::match(const std::string &tipo) {
  if (peek().tipo == tipo) {
    advance();
  } else {
    erro(descrever_tipo_token(tipo));
  }
}

std::string AnalisadorSintatico::descrever_tipo_token(const std::string &tipo) const {
  static const std::unordered_map<std::string, std::string> descricoes = {
      {"EOF", "fim do arquivo"},
      {"ID", "identificador"},
      {"NUMBER", "numero"},
      {"CHAR", "caractere"},
      {"STRING", "string"},
      {"FUN", "'fun'"},
      {"VAR", "'var'"},
      {"RETURN", "'return'"},
      {"PRINT", "'print'"},
      {"BREAK", "'break'"},
      {"CONTINUE", "'continue'"},
      {"IF", "'if'"},
      {"ELSE", "'else'"},
      {"WHILE", "'while'"},
      {"DO", "'do'"},
      {"FOR", "'for'"},
      {"TYPE_INT", "'int'"},
      {"TYPE_FLOAT", "'float'"},
      {"TYPE_CHAR", "'char'"},
      {"TYPE_VOID", "'void'"},
      {"TYPE_STRING", "'string'"},
      {"PLUS", "'+'"},
      {"MINUS", "'-'"},
      {"MULT", "'*'"},
      {"DIV", "'/'"},
      {"MOD", "'%'"},
      {"EQ", "'=='"},
      {"NEQ", "'!='"},
      {"LT", "'<'"},
      {"GT", "'>'"},
      {"LE", "'<='"},
      {"GE", "'>='"},
      {"OR", "'||'"},
      {"AND", "'&&'"},
      {"NOT", "'!'"},
      {"LBRACE", "'{'"},
      {"RBRACE", "'}'"},
      {"LPAREN", "'('"},
      {"RPAREN", "')'"},
      {"COMMA", "','"},
      {"COLON", "':'"},
      {"SEMICOLON", "';'"},
      {"ASSIGN", "'='"},
  };

  auto encontrado = descricoes.find(tipo);
  return encontrado != descricoes.end() ? encontrado->second : "'" + tipo + "'";
}

std::string AnalisadorSintatico::descrever_token(const Token &token) const {
  if (token.tipo == "EOF")
    return "fim do arquivo";

  std::string descricao = descrever_tipo_token(token.tipo);
  if (token.valor.empty() || descricao == "'" + token.valor + "'")
    return descricao;

  return "'" + token.valor + "' (" + descricao + ")";
}

void AnalisadorSintatico::erro(const std::string &esperado) {
  Token t = peek();
  std::string mensagem = "Erro sintatico na linha " + std::to_string(t.linha) +
                         ", coluna " + std::to_string(t.coluna) + ".\n";
  mensagem += "Esperado: " + esperado + ".\n";
  mensagem += "Encontrado: " + descrever_token(t) + ".";
  throw std::runtime_error(mensagem);
}

bool AnalisadorSintatico::chegou_ao_fim() { return peek().tipo == "EOF"; }

ASTNode *AnalisadorSintatico::analisar_programa() {
  if (peek().tipo == "VAR" || peek().tipo == "FUN" || peek().tipo == "EOF") {
    ASTNode *lista_global = analisar_lista_global();
    match("EOF");

    ASTNode *no_programa = criar_no(NodeType::PROG);
    if (lista_global)
      no_programa->adopt(lista_global);
    return no_programa;
  } else {
    erro("declaracao global, declaracao de funcao ou fim do arquivo");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_lista_global() {
  if (peek().tipo == "VAR" || peek().tipo == "FUN") {
    ASTNode *nivel = analisar_nivel_global();
    ASTNode *cauda = analisar_lista_global();

    if (nivel && cauda) {
      nivel->make_siblings(cauda);
    }
    return nivel ? nivel : cauda;
  } else if (peek().tipo == "EOF") {
    return nullptr;
  } else {
    erro("declaracao global, declaracao de funcao ou fim do arquivo");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_nivel_global() {
  if (peek().tipo == "FUN") {
    return analisar_declaracao_funcao();
  } else if (peek().tipo == "VAR") {
    return analisar_declaracao_global();
  } else {
    erro("declaracao global iniciada por 'var' ou funcao iniciada por 'fun'");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_declaracao_global() {
  if (peek().tipo == "VAR") {
    return analisar_declaracao_variavel();
  } else {
    erro("declaracao global iniciada por 'var'");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_declaracao_funcao() {
  if (peek().tipo == "FUN") {
    match("FUN");
    std::string nome_funcao = peek().valor;
    match("ID");
    match("LPAREN");
    ASTNode *parametros = analisar_lista_parametros();
    match("RPAREN");
    match("COLON");
    ASTNode *tipo_retorno = analisar_tipo();
    ASTNode *bloco = analisar_bloco();

    ASTNode *no_funcao = criar_no(NodeType::FUN_DECL, nome_funcao);
    if (tipo_retorno)
      no_funcao->adopt(tipo_retorno);
    if (parametros)
      no_funcao->adopt(parametros);
    if (bloco)
      no_funcao->adopt(bloco);
    return no_funcao;
  } else {
    erro("declaracao de funcao valida");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_lista_parametros() {
  if (peek().tipo == "ID") {
    ASTNode *parametro = analisar_parametro();
    ASTNode *cauda = analisar_cauda_parametros();

    if (parametro && cauda) {
      parametro->make_siblings(cauda);
    }
    return parametro ? parametro : cauda;
  } else if (peek().tipo == "RPAREN") {
    return nullptr;
  } else {
    erro("lista de parametros valida ou ')' para funcao sem parametros");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_parametros() {
  if (peek().tipo == "COMMA") {
    match("COMMA");
    ASTNode *parametro = analisar_parametro();
    ASTNode *cauda = analisar_cauda_parametros();

    if (parametro && cauda) {
      parametro->make_siblings(cauda);
    }
    return parametro ? parametro : cauda;
  } else if (peek().tipo == "RPAREN") {
    return nullptr;
  } else {
    erro("',' para separar parametros ou ')' para encerrar a lista");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_parametro() {
  if (peek().tipo == "ID") {
    std::string nome_parametro = peek().valor;
    match("ID");
    match("COLON");
    ASTNode *tipo = analisar_tipo();

    ASTNode *no_parametro = criar_no(NodeType::PARAM, nome_parametro);
    if (tipo)
      no_parametro->adopt(tipo);
    return no_parametro;
  } else {
    erro("parametro no formato nome: tipo");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_bloco() {
  if (peek().tipo == "LBRACE") {
    match("LBRACE");
    ASTNode *comandos = analisar_lista_comandos();
    match("RBRACE");

    ASTNode *no_bloco = criar_no(NodeType::BLOCK);
    if (comandos)
      no_bloco->adopt(comandos);
    return no_bloco;
  } else {
    erro("bloco iniciado por '{'");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_lista_comandos() {
  if (peek().tipo == "ID" || peek().tipo == "VAR" || peek().tipo == "RETURN" ||
      peek().tipo == "PRINT" || peek().tipo == "BREAK" ||
      peek().tipo == "CONTINUE" || peek().tipo == "WHILE" ||
      peek().tipo == "IF" || peek().tipo == "DO" || peek().tipo == "FOR") {

    ASTNode *comando = analisar_comando();
    ASTNode *cauda = analisar_lista_comandos();

    if (comando && cauda) {
      comando->make_siblings(cauda);
    }
    return comando ? comando : cauda;
  } else if (peek().tipo == "RBRACE") {
    return nullptr;
  } else {
    erro("comando valido ou '}' para encerrar o bloco");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando() {
  if (peek().tipo == "ID" || peek().tipo == "VAR" || peek().tipo == "RETURN" ||
      peek().tipo == "PRINT" || peek().tipo == "BREAK" ||
      peek().tipo == "CONTINUE") {
    return analisar_comando_simples();
  } else if (peek().tipo == "WHILE" || peek().tipo == "IF" ||
             peek().tipo == "DO" || peek().tipo == "FOR") {
    return analisar_comando_composto();
  } else {
    erro("comando simples ou comando composto");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_simples() {
  if (peek().tipo == "VAR") {
    return analisar_declaracao_variavel();
  } else if (peek().tipo == "RETURN") {
    return analisar_comando_retorno();
  } else if (peek().tipo == "PRINT") {
    return analisar_comando_impressao();
  } else if (peek().tipo == "BREAK") {
    return analisar_comando_interrupcao();
  } else if (peek().tipo == "CONTINUE") {
    return analisar_comando_continuacao();
  } else if (peek().tipo == "ID") {
    std::string nome_id = peek().valor;
    match("ID");
    return analisar_cauda_comando_id(nome_id);
  } else {
    erro("comando simples valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_comando_id(const std::string &nome_id) {
  if (peek().tipo == "ASSIGN") {
    match("ASSIGN");
    ASTNode *expressao = analisar_condicao();
    match("SEMICOLON");

    ASTNode *no_atribuicao = criar_no(NodeType::ASSIGN);
    no_atribuicao->adopt(criar_no(NodeType::ID, nome_id));
    if (expressao)
      no_atribuicao->adopt(expressao);
    return no_atribuicao;
  } else if (peek().tipo == "LPAREN") {
    match("LPAREN");
    ASTNode *argumentos = analisar_lista_argumentos();
    match("RPAREN");
    match("SEMICOLON");

    ASTNode *no_chamada = criar_no(NodeType::CALL, nome_id);
    if (argumentos)
      no_chamada->adopt(argumentos);
    return no_chamada;
  } else {
    erro("atribuicao ou chamada de funcao apos o identificador '" + nome_id + "'");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_composto() {
  if (peek().tipo == "WHILE") {
    return analisar_comando_enquanto();
  } else if (peek().tipo == "FOR") {
    return analisar_comando_para();
  } else if (peek().tipo == "DO") {
    return analisar_comando_faca_enquanto();
  } else if (peek().tipo == "IF") {
    return analisar_comando_se();
  } else {
    erro("comando composto valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_declaracao_variavel() {
  if (peek().tipo == "VAR") {
    match("VAR");
    std::string nome_variavel = peek().valor;
    match("ID");
    match("COLON");
    ASTNode *tipo = analisar_tipo();
    ASTNode *inicializacao = analisar_inicializacao();
    match("SEMICOLON");

    ASTNode *no_variavel = criar_no(NodeType::VAR_DECL, nome_variavel);
    if (tipo)
      no_variavel->adopt(tipo);
    if (inicializacao)
      no_variavel->adopt(inicializacao);
    return no_variavel;
  } else {
    erro("declaracao de variavel valida");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_inicializacao() {
  if (peek().tipo == "ASSIGN") {
    match("ASSIGN");
    return analisar_condicao();
  } else if (peek().tipo == "SEMICOLON") {
    return nullptr;
  } else {
    erro("inicializacao com '=' ou ';' para declaracao sem valor inicial");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_retorno() {
  if (peek().tipo == "RETURN") {
    match("RETURN");
    ASTNode *expressao = analisar_condicao();
    match("SEMICOLON");

    ASTNode *no_retorno = criar_no(NodeType::RETURN);
    if (expressao)
      no_retorno->adopt(expressao);
    return no_retorno;
  } else {
    erro("comando de retorno valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_impressao() {
  if (peek().tipo == "PRINT") {
    match("PRINT");
    match("LPAREN");
    ASTNode *expressao = analisar_condicao();
    match("RPAREN");
    match("SEMICOLON");

    ASTNode *no_impressao = criar_no(NodeType::PRINT);
    if (expressao)
      no_impressao->adopt(expressao);
    return no_impressao;
  } else {
    erro("comando de impressao valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_interrupcao() {
  match("BREAK");
  match("SEMICOLON");
  return criar_no(NodeType::BREAK);
}

ASTNode *AnalisadorSintatico::analisar_comando_continuacao() {
  match("CONTINUE");
  match("SEMICOLON");
  return criar_no(NodeType::CONTINUE);
}

ASTNode *AnalisadorSintatico::analisar_tipo() {
  std::string rotulo_tipo = peek().valor;
  if (peek().tipo == "TYPE_INT") {
    match("TYPE_INT");
  } else if (peek().tipo == "TYPE_FLOAT") {
    match("TYPE_FLOAT");
  } else if (peek().tipo == "TYPE_CHAR") {
    match("TYPE_CHAR");
  } else if (peek().tipo == "TYPE_VOID") {
    match("TYPE_VOID");
  } else if (peek().tipo == "TYPE_STRING") {
    match("TYPE_STRING");
  } else {
    erro("um tipo: 'int', 'float', 'char', 'string' ou 'void'");
    return nullptr;
  }
  return criar_no(NodeType::TYPE, rotulo_tipo);
}

ASTNode *AnalisadorSintatico::analisar_condicao() {
  if (peek().tipo == "NOT" || peek().tipo == "LPAREN" || peek().tipo == "ID" ||
      peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
      peek().tipo == "STRING" || peek().tipo == "MINUS") {
    return analisar_expressao_ou();
  } else {
    erro("expressao valida");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_expressao_ou() {
  if (peek().tipo == "NOT" || peek().tipo == "LPAREN" || peek().tipo == "ID" ||
      peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
      peek().tipo == "STRING" || peek().tipo == "MINUS") {
    ASTNode *esquerda = analisar_expressao_e();
    return analisar_cauda_expressao_ou(esquerda);
  } else {
    erro("expressao valida");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_expressao_ou(ASTNode *esquerda) {
  if (peek().tipo == "OR") {
    match("OR");
    if (!(peek().tipo == "NOT" || peek().tipo == "LPAREN" ||
          peek().tipo == "ID" || peek().tipo == "NUMBER" ||
          peek().tipo == "CHAR" || peek().tipo == "STRING" ||
          peek().tipo == "MINUS"))
      erro("expressao valida apos operador '||'");
    ASTNode *direita = analisar_expressao_e();

    ASTNode *no_operador = criar_no(NodeType::BIN_OP, "||");
    no_operador->adopt(esquerda);
    if (direita)
      no_operador->adopt(direita);

    return analisar_cauda_expressao_ou(no_operador);
  } else if (peek().tipo == "RPAREN" || peek().tipo == "SEMICOLON" ||
             peek().tipo == "COMMA" || peek().tipo == "RBRACE" ||
             peek().tipo == "EOF") {
    return esquerda; // ε
  } else {
    erro("operador logico ou delimitador valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_expressao_e() {
  if (peek().tipo == "NOT" || peek().tipo == "LPAREN" || peek().tipo == "ID" ||
      peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
      peek().tipo == "STRING" || peek().tipo == "MINUS") {
    ASTNode *esquerda = analisar_expressao_logica_unaria();
    return analisar_cauda_expressao_e(esquerda);
  } else {
    erro("expressao valida");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_expressao_e(ASTNode *esquerda) {
  if (peek().tipo == "AND") {
    match("AND");
    if (!(peek().tipo == "NOT" || peek().tipo == "LPAREN" ||
          peek().tipo == "ID" || peek().tipo == "NUMBER" ||
          peek().tipo == "CHAR" || peek().tipo == "STRING" ||
          peek().tipo == "MINUS"))
      erro("expressao valida apos operador '&&'");
    ASTNode *direita = analisar_expressao_logica_unaria();

    ASTNode *no_operador = criar_no(NodeType::BIN_OP, "&&");
    no_operador->adopt(esquerda);
    if (direita)
      no_operador->adopt(direita);

    return analisar_cauda_expressao_e(no_operador);
  } else if (peek().tipo == "RPAREN" || peek().tipo == "OR" ||
             peek().tipo == "SEMICOLON" || peek().tipo == "COMMA" ||
             peek().tipo == "RBRACE" || peek().tipo == "EOF") {
    return esquerda;
  } else {
    erro("operador logico ou delimitador valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_expressao_logica_unaria() {
  if (peek().tipo == "NOT") {
    match("NOT");
    ASTNode *operando = analisar_expressao_logica_unaria();

    ASTNode *no_unario = criar_no(NodeType::UN_OP, "!");
    if (operando)
      no_unario->adopt(operando);
    return no_unario;
  } else if (peek().tipo == "LPAREN" || peek().tipo == "ID" ||
             peek().tipo == "NUMBER" ||
             peek().tipo == "CHAR" || peek().tipo == "STRING" ||
             peek().tipo == "MINUS") {
    return analisar_expressao_relacional();
  } else {
    erro("expressao valida");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_expressao_relacional() {
  if (peek().tipo == "LPAREN" || peek().tipo == "ID" ||
      peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
      peek().tipo == "STRING" || peek().tipo == "MINUS") {
    ASTNode *esquerda = analisar_expressao_aritmetica();
    return analisar_cauda_relacional(esquerda);
  } else {
    erro("expressao relacional valida");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_relacional(ASTNode *esquerda) {
  if (peek().tipo == "EQ" || peek().tipo == "NEQ" || peek().tipo == "GT" ||
      peek().tipo == "LT" || peek().tipo == "GE" || peek().tipo == "LE") {

    std::string operador = peek().valor;
    advance();
    if (!(peek().tipo == "LPAREN" || peek().tipo == "ID" ||
          peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
          peek().tipo == "STRING" || peek().tipo == "MINUS"))
      erro("expressao valida apos operador '" + operador + "'");
    ASTNode *direita = analisar_expressao_aritmetica();

    ASTNode *no_relacional = criar_no(NodeType::BIN_OP, operador);
    no_relacional->adopt(esquerda);
    if (direita)
      no_relacional->adopt(direita);
    return no_relacional;
  } else if (peek().tipo == "RPAREN" || peek().tipo == "AND" ||
             peek().tipo == "OR" || peek().tipo == "SEMICOLON" ||
             peek().tipo == "COMMA" || peek().tipo == "RBRACE" ||
             peek().tipo == "EOF") {
    return esquerda; // ε
  } else {
    erro("operador relacional ou delimitador valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_expressao_aritmetica() {
  if (peek().tipo == "LPAREN" || peek().tipo == "ID" ||
      peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
      peek().tipo == "STRING" || peek().tipo == "MINUS") {
    ASTNode *esquerda = analisar_termo();
    return analisar_cauda_expressao_aritmetica(esquerda);
  } else {
    erro("expressao aritmetica valida");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_expressao_aritmetica(ASTNode *esquerda) {
  if (peek().tipo == "PLUS" || peek().tipo == "MINUS") {
    std::string operador = peek().valor;
    advance();
    if (!(peek().tipo == "LPAREN" || peek().tipo == "ID" ||
          peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
          peek().tipo == "STRING" || peek().tipo == "MINUS"))
      erro("operando valido apos operador '" + operador + "'");
    ASTNode *direita = analisar_termo();

    ASTNode *no_operador = criar_no(NodeType::BIN_OP, operador);
    no_operador->adopt(esquerda);
    if (direita)
      no_operador->adopt(direita);

    return analisar_cauda_expressao_aritmetica(no_operador);
  } else if (peek().tipo == "SEMICOLON" || peek().tipo == "RPAREN" ||
             peek().tipo == "OR" || peek().tipo == "AND" ||
             peek().tipo == "EQ" || peek().tipo == "NEQ" ||
             peek().tipo == "GT" || peek().tipo == "LT" ||
             peek().tipo == "GE" || peek().tipo == "LE" ||
             peek().tipo == "COMMA" || peek().tipo == "RBRACE" ||
             peek().tipo == "EOF") {
    return esquerda;
  } else {
    erro("operador aritmetico ou delimitador valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_termo() {
  if (peek().tipo == "LPAREN" || peek().tipo == "ID" ||
      peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
      peek().tipo == "STRING" || peek().tipo == "MINUS") {
    ASTNode *esquerda = analisar_fator();
    return analisar_cauda_termo(esquerda);
  } else {
    erro("termo de expressao valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_termo(ASTNode *esquerda) {
  if (peek().tipo == "MULT" || peek().tipo == "DIV" || peek().tipo == "MOD") {
    std::string operador = peek().valor;
    advance();
    if (!(peek().tipo == "LPAREN" || peek().tipo == "ID" ||
          peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
          peek().tipo == "STRING" || peek().tipo == "MINUS"))
      erro("operando valido apos operador '" + operador + "'");
    ASTNode *direita = analisar_fator();

    ASTNode *no_operador = criar_no(NodeType::BIN_OP, operador);
    no_operador->adopt(esquerda);
    if (direita)
      no_operador->adopt(direita);

    return analisar_cauda_termo(no_operador);
  } else if (peek().tipo == "SEMICOLON" || peek().tipo == "RPAREN" ||
             peek().tipo == "OR" || peek().tipo == "AND" ||
             peek().tipo == "EQ" || peek().tipo == "NEQ" ||
             peek().tipo == "GT" || peek().tipo == "LT" ||
             peek().tipo == "GE" || peek().tipo == "LE" ||
             peek().tipo == "COMMA" || peek().tipo == "PLUS" ||
             peek().tipo == "MINUS" || peek().tipo == "RBRACE" ||
             peek().tipo == "EOF") {
    return esquerda; // ε
  } else {
    erro("operador de termo ou delimitador valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_fator() {
  if (peek().tipo == "ID") {
    std::string nome_id = peek().valor;
    match("ID");
    ASTNode *no_id = criar_no(NodeType::ID, nome_id);
    return analisar_cauda_fator(no_id);
  } else if (peek().tipo == "NUMBER") {
    std::string valor = peek().valor;
    match("NUMBER");
    return criar_no(NodeType::NUMBER, valor);
  } else if (peek().tipo == "CHAR") {
    std::string valor = peek().valor;
    match("CHAR");
    return criar_no(NodeType::CHAR, valor);
  } else if (peek().tipo == "STRING") {
    std::string valor = peek().valor;
    match("STRING");
    return criar_no(NodeType::STRING, valor);
  } else if (peek().tipo == "MINUS") {
    match("MINUS");
    ASTNode *operando = analisar_fator();
    ASTNode *no_unario = criar_no(NodeType::UN_OP, "-");
    if (operando)
      no_unario->adopt(operando);
    return no_unario;
  } else if (peek().tipo == "LPAREN") {
    match("LPAREN");
    ASTNode *expressao = analisar_condicao();
    match("RPAREN");
    return expressao;
  } else {
    erro("fator de expressao valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_fator(ASTNode *no_id) {
  if (peek().tipo == "LPAREN") {
    match("LPAREN");
    ASTNode *argumentos = analisar_lista_argumentos();
    match("RPAREN");

    ASTNode *no_chamada = criar_no(NodeType::CALL, no_id->value);
    descartar_no(no_id);
    if (argumentos)
      no_chamada->adopt(argumentos);
    return no_chamada;
  } else if (peek().tipo == "SEMICOLON" || peek().tipo == "RPAREN" ||
             peek().tipo == "OR" || peek().tipo == "AND" ||
             peek().tipo == "EQ" || peek().tipo == "NEQ" ||
             peek().tipo == "GT" || peek().tipo == "LT" ||
             peek().tipo == "GE" || peek().tipo == "LE" ||
             peek().tipo == "COMMA" || peek().tipo == "PLUS" ||
             peek().tipo == "MINUS" || peek().tipo == "MULT" ||
             peek().tipo == "DIV" || peek().tipo == "MOD" ||
             peek().tipo == "RBRACE" || peek().tipo == "EOF") {
    return no_id;
  } else {
    erro("chamada de funcao, operador ou delimitador valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_lista_argumentos() {
  if (peek().tipo == "ID" || peek().tipo == "NUMBER" || peek().tipo == "CHAR" ||
      peek().tipo == "STRING" || peek().tipo == "MINUS" ||
      peek().tipo == "LPAREN" || peek().tipo == "NOT") {
    ASTNode *expressao = analisar_condicao();
    ASTNode *cauda = analisar_cauda_argumentos();

    if (expressao && cauda) {
      expressao->make_siblings(cauda);
    }
    return expressao ? expressao : cauda;
  } else if (peek().tipo == "RPAREN") {
    return nullptr;
  } else {
    erro("lista de argumentos valida");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_argumentos() {
  if (peek().tipo == "COMMA") {
    match("COMMA");
    ASTNode *expressao = analisar_condicao();
    ASTNode *cauda = analisar_cauda_argumentos();

    if (expressao && cauda) {
      expressao->make_siblings(cauda);
    }
    return expressao ? expressao : cauda;
  } else if (peek().tipo == "RPAREN") {
    return nullptr; // ε
  } else {
    erro("',' para outro argumento ou ')' para encerrar a chamada");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_se() {
  if (peek().tipo == "IF") {
    match("IF");
    match("LPAREN");
    ASTNode *condicao = analisar_condicao();
    match("RPAREN");
    ASTNode *bloco_entao = analisar_bloco();
    ASTNode *bloco_senao = analisar_comando_senao();

    ASTNode *no_se = criar_no(NodeType::IF);
    if (condicao)
      no_se->adopt(condicao);
    if (bloco_entao)
      no_se->adopt(bloco_entao);
    no_se->adopt(bloco_senao ? bloco_senao : criar_no(NodeType::EMPTY));
    return no_se;
  } else {
    erro("comando condicional valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_senao() {
  if (peek().tipo == "ELSE") {
    match("ELSE");
    return analisar_cauda_senao();
  } else if (peek().tipo == "RBRACE" || peek().tipo == "ID" ||
             peek().tipo == "VAR" || peek().tipo == "RETURN" ||
             peek().tipo == "PRINT" || peek().tipo == "BREAK" ||
             peek().tipo == "CONTINUE" || peek().tipo == "WHILE" ||
             peek().tipo == "IF" || peek().tipo == "DO" ||
             peek().tipo == "FOR") {
    return nullptr;
  } else {
    erro("'else', proximo comando ou fim do bloco");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_cauda_senao() {
  if (peek().tipo == "LBRACE") {
    return analisar_bloco();
  } else if (peek().tipo == "IF") {
    return analisar_comando_se();
  } else {
    erro("bloco ou comando 'if' apos 'else'");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_enquanto() {
  if (peek().tipo == "WHILE") {
    match("WHILE");
    match("LPAREN");
    ASTNode *condicao = analisar_condicao();
    match("RPAREN");
    ASTNode *corpo = analisar_bloco();

    ASTNode *no_enquanto = criar_no(NodeType::WHILE);
    if (condicao)
      no_enquanto->adopt(condicao);
    if (corpo)
      no_enquanto->adopt(corpo);
    return no_enquanto;
  } else {
    erro("comando while valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_faca_enquanto() {
  if (peek().tipo == "DO") {
    match("DO");
    ASTNode *corpo = analisar_bloco();
    match("WHILE");
    match("LPAREN");
    ASTNode *condicao = analisar_condicao();
    match("RPAREN");
    match("SEMICOLON");

    ASTNode *no_faca_enquanto = criar_no(NodeType::DO_WHILE);
    if (corpo)
      no_faca_enquanto->adopt(corpo);
    if (condicao)
      no_faca_enquanto->adopt(condicao);
    return no_faca_enquanto;
  } else {
    erro("comando do-while valido");
    return nullptr;
  }
}

ASTNode *AnalisadorSintatico::analisar_comando_para() {
  if (peek().tipo == "FOR") {
    match("FOR");
    match("LPAREN");
    ASTNode *inicializacao = analisar_declaracao_variavel();
    ASTNode *condicao = analisar_condicao();
    match("SEMICOLON");
    std::string nome_atualizacao = peek().valor;
    match("ID");
    match("ASSIGN");
    ASTNode *expressao_atualizacao = analisar_condicao();
    match("RPAREN");
    ASTNode *corpo = analisar_bloco();

    ASTNode *atualizacao = criar_no(NodeType::ASSIGN);
    atualizacao->adopt(criar_no(NodeType::ID, nome_atualizacao));
    if (expressao_atualizacao)
      atualizacao->adopt(expressao_atualizacao);

    ASTNode *no_para = criar_no(NodeType::FOR);
    if (inicializacao)
      no_para->adopt(inicializacao);
    if (condicao)
      no_para->adopt(condicao);
    no_para->adopt(atualizacao);
    if (corpo)
      no_para->adopt(corpo);
    return no_para;
  } else {
    erro("comando for valido");
    return nullptr;
  }
}
