#include "../include/gerador_sam.hpp"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

struct InformacaoVariavel {
  int deslocamento;
  bool e_string;
};

static InformacaoVariavel criar_informacao_variavel(ASTNode *no_tipo, int deslocamento) {
  bool e_string = false;

  if (no_tipo && no_tipo->type == NodeType::TYPE) {
    if (no_tipo->value == "string")
      e_string = true;
  }

  return {deslocamento, e_string};
}

std::unordered_map<std::string, InformacaoVariavel> deslocamentos_globais;
std::unordered_map<std::string, InformacaoVariavel> tabela_deslocamentos;
std::unordered_map<ASTNode *, int> deslocamentos_declaracoes_locais;
std::vector<int> strings_globais;
std::vector<int> strings_locais;
int proximo_deslocamento_global = 0;
int proximo_deslocamento_local = 0;
int quantidade_parametros_funcao_atual = 0;
bool dentro_de_funcao = false;

static void mapear_declaracoes_locais(ASTNode *no) {
  for (ASTNode *atual = no; atual; atual = atual->next) {
    if (atual->type == NodeType::VAR_DECL) {
      deslocamentos_declaracoes_locais[atual] = proximo_deslocamento_local++;
    }
    if (atual->leftmostChild)
      mapear_declaracoes_locais(atual->leftmostChild);
  }
}

GeradorSAM::GeradorSAM() : contador_rotulos(0) {}

std::string GeradorSAM::novo_rotulo() {
  return "L" + std::to_string(contador_rotulos++);
}

void GeradorSAM::liberar_string_local(int deslocamento) {
  std::string rotulo_liberacao = novo_rotulo();
  std::string rotulo_fim = novo_rotulo();
  codigo_sam.push_back("PUSHOFF " + std::to_string(deslocamento));
  codigo_sam.push_back("JUMPC " + rotulo_liberacao);
  codigo_sam.push_back("JUMP " + rotulo_fim);
  codigo_sam.push_back(rotulo_liberacao + ":");
  codigo_sam.push_back("PUSHOFF " + std::to_string(deslocamento));
  codigo_sam.push_back("FREE");
  codigo_sam.push_back("PUSHIMM 0");
  codigo_sam.push_back("STOREOFF " + std::to_string(deslocamento));
  codigo_sam.push_back(rotulo_fim + ":");
}

void GeradorSAM::liberar_string_global(int deslocamento) {
  std::string rotulo_liberacao = novo_rotulo();
  std::string rotulo_fim = novo_rotulo();
  codigo_sam.push_back("PUSHABS " + std::to_string(deslocamento));
  codigo_sam.push_back("JUMPC " + rotulo_liberacao);
  codigo_sam.push_back("JUMP " + rotulo_fim);
  codigo_sam.push_back(rotulo_liberacao + ":");
  codigo_sam.push_back("PUSHABS " + std::to_string(deslocamento));
  codigo_sam.push_back("FREE");
  codigo_sam.push_back("PUSHIMM 0");
  codigo_sam.push_back("STOREABS " + std::to_string(deslocamento));
  codigo_sam.push_back(rotulo_fim + ":");
}

// 🧮 1. Processamento de Expressões
void GeradorSAM::traduzir_expressao(ASTNode *no) {
  if (!no)
    return;

  switch (no->type) {
  case NodeType::NUMBER:
    if (no->value.find('.') != std::string::npos)
      codigo_sam.push_back("PUSHIMMF " + no->value);
    else
      codigo_sam.push_back("PUSHIMM " + no->value);
    break;

  case NodeType::CHAR:
    codigo_sam.push_back("PUSHIMMCH " + no->value);
    break;
  case NodeType::STRING:
    codigo_sam.push_back("PUSHIMMSTR " + no->value);
    break;

  case NodeType::ID:
    // 🟢 PRIORIDADE DE SOMBREAMENTO (Shadowing): Verifica primeiro escopo variavel_local
    // se estiver em função
    if (dentro_de_funcao &&
        tabela_deslocamentos.find(no->value) != tabela_deslocamentos.end()) {
      codigo_sam.push_back("PUSHOFF " +
                          std::to_string(tabela_deslocamentos[no->value].deslocamento));
    } else if (deslocamentos_globais.find(no->value) != deslocamentos_globais.end()) {
      codigo_sam.push_back("PUSHABS " +
                          std::to_string(deslocamentos_globais[no->value].deslocamento));
    } else if (tabela_deslocamentos.find(no->value) != tabela_deslocamentos.end()) {
      codigo_sam.push_back("PUSHOFF " +
                          std::to_string(tabela_deslocamentos[no->value].deslocamento));
    } else {
      throw std::runtime_error("Erro interno: identificador sem deslocamento '" +
                               no->value + "'.");
    }
    break;

  case NodeType::BIN_OP:
    if (no->value == "&&") {
      std::string rotulo_direita = novo_rotulo();
      std::string rotulo_verdadeiro = novo_rotulo();
      std::string rotulo_fim = novo_rotulo();
      traduzir_expressao(no->leftmostChild);
      codigo_sam.push_back("JUMPC " + rotulo_direita);
      codigo_sam.push_back("PUSHIMM 0");
      codigo_sam.push_back("JUMP " + rotulo_fim);
      codigo_sam.push_back(rotulo_direita + ":");
      traduzir_expressao(no->leftmostChild->next);
      codigo_sam.push_back("JUMPC " + rotulo_verdadeiro);
      codigo_sam.push_back("PUSHIMM 0");
      codigo_sam.push_back("JUMP " + rotulo_fim);
      codigo_sam.push_back(rotulo_verdadeiro + ":");
      codigo_sam.push_back("PUSHIMM 1");
      codigo_sam.push_back(rotulo_fim + ":");
      break;
    }
    if (no->value == "||") {
      std::string rotulo_verdadeiro = novo_rotulo();
      std::string rotulo_fim = novo_rotulo();
      traduzir_expressao(no->leftmostChild);
      codigo_sam.push_back("JUMPC " + rotulo_verdadeiro);
      traduzir_expressao(no->leftmostChild->next);
      codigo_sam.push_back("JUMPC " + rotulo_verdadeiro);
      codigo_sam.push_back("PUSHIMM 0");
      codigo_sam.push_back("JUMP " + rotulo_fim);
      codigo_sam.push_back(rotulo_verdadeiro + ":");
      codigo_sam.push_back("PUSHIMM 1");
      codigo_sam.push_back(rotulo_fim + ":");
      break;
    }

    traduzir_expressao(no->leftmostChild);
    traduzir_expressao(no->leftmostChild->next);

    if (no->value == "+")
      codigo_sam.push_back(no->inferredType == "float" ? "ADDF" : "ADD");
    else if (no->value == "-")
      codigo_sam.push_back(no->inferredType == "float" ? "SUBF" : "SUB");
    else if (no->value == "*")
      codigo_sam.push_back(no->inferredType == "float" ? "TIMESF" : "TIMES");
    else if (no->value == "/")
      codigo_sam.push_back(no->inferredType == "float" ? "DIVF" : "DIV");
    else if (no->value == "%")
      codigo_sam.push_back("MOD");
    else if (no->value == "==")
      codigo_sam.push_back("EQUAL");
    else if (no->value == "!=") {
      codigo_sam.push_back("EQUAL");
      codigo_sam.push_back("NOT");
    } else if (no->value == "<")
      codigo_sam.push_back("LESS");
    else if (no->value == "<=") {
      codigo_sam.push_back("GREATER");
      codigo_sam.push_back("NOT");
    } else if (no->value == ">")
      codigo_sam.push_back("GREATER");
    else if (no->value == ">=") {
      codigo_sam.push_back("LESS");
      codigo_sam.push_back("NOT");
    }
    break;

  case NodeType::UN_OP:
    traduzir_expressao(no->leftmostChild);
    if (no->value == "-")
      codigo_sam.push_back(no->inferredType == "float" ? "PUSHIMMF -1.0"
                                                       : "PUSHIMM -1");
    if (no->value == "-")
      codigo_sam.push_back(no->inferredType == "float" ? "TIMESF" : "TIMES");
    else if (no->value == "!")
      codigo_sam.push_back("NOT");
    break;

  case NodeType::CALL: {
    codigo_sam.push_back("ADDSP 1"); // Slot de Retorno

    ASTNode *argumento = no->leftmostChild;
    int quantidade_argumentos = 0;
    while (argumento) {
      traduzir_expressao(argumento);
      quantidade_argumentos++;
      argumento = argumento->next;
    }

    codigo_sam.push_back("JSR " + no->value);

    if (quantidade_argumentos > 0) {
      codigo_sam.push_back("ADDSP -" + std::to_string(quantidade_argumentos));
    }
    break;
  }
  default:
    break;
  }

  if (no->inferredType == "int" && no->coercedType == "float")
    codigo_sam.push_back("ITOF");
}

// 📑 2. Processamento de Comandos
void GeradorSAM::traduzir_comando(ASTNode *no, bool traduzir_proximos) {
  if (!no)
    return;

  switch (no->type) {

  case NodeType::FUN_DECL: {
    codigo_sam.push_back(no->value + ":");
    tabela_deslocamentos.clear();
    deslocamentos_declaracoes_locais.clear();
    strings_locais.clear();
    proximo_deslocamento_local = 1;

    bool escopo_pai_era_funcao = dentro_de_funcao;
    dentro_de_funcao = true;

    if (no->value == "main") {
      codigo_sam.push_back("LINK");

      quantidade_parametros_funcao_atual = 0;
      ASTNode *corpo = no->leftmostChild;
      if (corpo &&
          (corpo->type == NodeType::TYPE || corpo->type == NodeType::ID))
        corpo = corpo->next;

      mapear_declaracoes_locais(corpo);
      int quantidade_locais = proximo_deslocamento_local - 1;

      if (quantidade_locais > 0)
        codigo_sam.push_back("ADDSP " + std::to_string(quantidade_locais));

      if (corpo)
        traduzir_comando(corpo);

      for (int deslocamento : strings_locais) {
        liberar_string_local(deslocamento);
      }

      if (quantidade_locais > 0)
        codigo_sam.push_back("ADDSP -" + std::to_string(quantidade_locais));
      codigo_sam.push_back("UNLINK");
      for (int deslocamento : strings_globais) {
        liberar_string_global(deslocamento);
      }
      if (proximo_deslocamento_global > 1)
        codigo_sam.push_back("ADDSP -" +
                            std::to_string(proximo_deslocamento_global - 1));
      codigo_sam.push_back("STOP");
    } else {
      std::vector<std::string> nomes_parametros;
      ASTNode *parametro = no->leftmostChild;
      if (parametro &&
          (parametro->type == NodeType::TYPE || parametro->type == NodeType::ID))
        parametro = parametro->next;

      while (parametro && parametro->type == NodeType::PARAM) {
        nomes_parametros.push_back(parametro->value);
        parametro = parametro->next;
      }

      int total_parametros = static_cast<int>(nomes_parametros.size());
      quantidade_parametros_funcao_atual = total_parametros;

      for (int i = 0; i < total_parametros; i++) {
        ASTNode *tipo_parametro = nullptr;
        ASTNode *no_parametro =
            no->leftmostChild ? no->leftmostChild->next : nullptr;
        for (int j = 0; j < i && no_parametro; j++)
          no_parametro = no_parametro->next;
        if (no_parametro)
          tipo_parametro = no_parametro->leftmostChild;
        tabela_deslocamentos[nomes_parametros[i]] =
            criar_informacao_variavel(tipo_parametro, -2 - (total_parametros - 1 - i));
      }

      codigo_sam.push_back("LINK");

      ASTNode *corpo = parametro;
      mapear_declaracoes_locais(corpo);
      int quantidade_locais = proximo_deslocamento_local - 1;

      if (quantidade_locais > 0)
        codigo_sam.push_back("ADDSP " + std::to_string(quantidade_locais));

      if (corpo)
        traduzir_comando(corpo);

      for (int deslocamento : strings_locais) {
        liberar_string_local(deslocamento);
      }

      if (quantidade_locais > 0)
        codigo_sam.push_back("ADDSP -" + std::to_string(quantidade_locais));

      codigo_sam.push_back("UNLINK");
      codigo_sam.push_back("RST");
    }

    dentro_de_funcao = escopo_pai_era_funcao;

    if (traduzir_proximos && no->next)
      traduzir_comando(no->next);
    return;
  }

  case NodeType::RETURN: {
    if (no->leftmostChild) {
      traduzir_expressao(no->leftmostChild);

      int deslocamento_retorno = -2 - quantidade_parametros_funcao_atual;

      codigo_sam.push_back("STOREOFF " + std::to_string(deslocamento_retorno));
    }

    int quantidade_locais = proximo_deslocamento_local - 1;

    for (int deslocamento : strings_locais) {
      liberar_string_local(deslocamento);
    }

    if (quantidade_locais > 0)
      codigo_sam.push_back("ADDSP -" + std::to_string(quantidade_locais));

    codigo_sam.push_back("UNLINK");
    codigo_sam.push_back("RST");

    return;
  }
  case NodeType::VAR_DECL: {
    InformacaoVariavel informacao_variavel = criar_informacao_variavel(no->leftmostChild, 0);

    if (!dentro_de_funcao) {
      informacao_variavel.deslocamento = proximo_deslocamento_global++;
      deslocamentos_globais[no->value] = informacao_variavel;
      codigo_sam.push_back("ADDSP 1");
      if (informacao_variavel.e_string)
        strings_globais.push_back(deslocamentos_globais[no->value].deslocamento);

      if (no->leftmostChild && no->leftmostChild->next) {
        traduzir_expressao(no->leftmostChild->next);
        codigo_sam.push_back("STOREABS " +
                            std::to_string(deslocamentos_globais[no->value].deslocamento));
      }
    } else {
      int deslocamento_atual = deslocamentos_declaracoes_locais.at(no);
      informacao_variavel.deslocamento = deslocamento_atual;
      tabela_deslocamentos[no->value] = informacao_variavel;
      if (informacao_variavel.e_string) {
        if (std::find(strings_locais.begin(), strings_locais.end(),
                      deslocamento_atual) == strings_locais.end())
          strings_locais.push_back(deslocamento_atual);
        liberar_string_local(deslocamento_atual);
      }

      if (no->leftmostChild && no->leftmostChild->next) {
        traduzir_expressao(no->leftmostChild->next);
        codigo_sam.push_back("STOREOFF " + std::to_string(deslocamento_atual));
      }
    }
    break;
  }

  case NodeType::BLOCK: {
    auto tabela_antes_do_bloco = tabela_deslocamentos;
    if (no->leftmostChild)
      traduzir_comando(no->leftmostChild);
    tabela_deslocamentos = std::move(tabela_antes_do_bloco);
    break;
  }

  case NodeType::ASSIGN: {
    ASTNode *no_id = no->leftmostChild;
    auto variavel_local = tabela_deslocamentos.find(no_id->value);
    auto variavel_global = deslocamentos_globais.find(no_id->value);
    if (variavel_local != tabela_deslocamentos.end() &&
        variavel_local->second.e_string)
      liberar_string_local(variavel_local->second.deslocamento);
    else if (variavel_global != deslocamentos_globais.end() &&
             variavel_global->second.e_string)
      liberar_string_global(variavel_global->second.deslocamento);

    traduzir_expressao(no_id->next);
    // locais/globais
    if (dentro_de_funcao &&
        tabela_deslocamentos.find(no_id->value) != tabela_deslocamentos.end()) {
      codigo_sam.push_back("STOREOFF " +
                          std::to_string(tabela_deslocamentos[no_id->value].deslocamento));
    } else {
      codigo_sam.push_back("STOREABS " +
                          std::to_string(deslocamentos_globais.at(no_id->value).deslocamento));
    }
    break;
  }

  case NodeType::PRINT: {
    if (no->leftmostChild && no->leftmostChild->type == NodeType::STRING) {
      int deslocamento_temporario = proximo_deslocamento_local;
      codigo_sam.push_back("ADDSP 1");
      traduzir_expressao(no->leftmostChild);
      codigo_sam.push_back("STOREOFF " + std::to_string(deslocamento_temporario));
      codigo_sam.push_back("PUSHOFF " + std::to_string(deslocamento_temporario));
      codigo_sam.push_back("WRITESTR");
      codigo_sam.push_back("PUSHOFF " + std::to_string(deslocamento_temporario));
      codigo_sam.push_back("FREE");
      codigo_sam.push_back("ADDSP -1");
      break;
    }

    traduzir_expressao(no->leftmostChild);

    if (no->leftmostChild->inferredType == "float") {
      codigo_sam.push_back("WRITEF");
    } else if (no->leftmostChild->inferredType == "char") {
      codigo_sam.push_back("WRITECH");
    } else if (no->leftmostChild->inferredType == "string") {
      codigo_sam.push_back("WRITESTR");
    } else {
      codigo_sam.push_back("WRITE");
    }
    break;
  }

  case NodeType::CALL:
    traduzir_expressao(no);
    codigo_sam.push_back("ADDSP -1");
    break;

  case NodeType::BREAK:
    codigo_sam.push_back("JUMP " + rotulos_interrupcao.back());
    break;

  case NodeType::CONTINUE:
    codigo_sam.push_back("JUMP " + rotulos_continuacao.back());
    break;

  case NodeType::IF: {
    ASTNode *condicao = no->leftmostChild;
    ASTNode *bloco_entao = condicao ? condicao->next : nullptr;
    ASTNode *bloco_senao = bloco_entao ? bloco_entao->next : nullptr;
    std::string rotulo_entao = novo_rotulo();
    std::string rotulo_senao = novo_rotulo();
    std::string rotulo_fim = novo_rotulo();

    traduzir_expressao(condicao);
    codigo_sam.push_back("JUMPC " + rotulo_entao);
    codigo_sam.push_back("JUMP " + rotulo_senao);
    codigo_sam.push_back(rotulo_entao + ":");
    traduzir_comando(bloco_entao, false);
    codigo_sam.push_back("JUMP " + rotulo_fim);
    codigo_sam.push_back(rotulo_senao + ":");
    if (bloco_senao && bloco_senao->type != NodeType::EMPTY)
      traduzir_comando(bloco_senao, false);
    codigo_sam.push_back(rotulo_fim + ":");
    break;
  }

  case NodeType::WHILE: {
    ASTNode *condicao = no->leftmostChild;
    ASTNode *corpo = condicao ? condicao->next : nullptr;
    std::string rotulo_inicio = novo_rotulo();
    std::string rotulo_corpo = novo_rotulo();
    std::string rotulo_fim = novo_rotulo();

    codigo_sam.push_back(rotulo_inicio + ":");
    traduzir_expressao(condicao);
    codigo_sam.push_back("JUMPC " + rotulo_corpo);
    codigo_sam.push_back("JUMP " + rotulo_fim);
    codigo_sam.push_back(rotulo_corpo + ":");
    rotulos_interrupcao.push_back(rotulo_fim);
    rotulos_continuacao.push_back(rotulo_inicio);
    traduzir_comando(corpo, false);
    rotulos_continuacao.pop_back();
    rotulos_interrupcao.pop_back();
    codigo_sam.push_back("JUMP " + rotulo_inicio);
    codigo_sam.push_back(rotulo_fim + ":");
    break;
  }

  case NodeType::DO_WHILE: {
    ASTNode *corpo = no->leftmostChild;
    ASTNode *condicao = corpo ? corpo->next : nullptr;
    std::string rotulo_inicio = novo_rotulo();
    std::string rotulo_condicao = novo_rotulo();
    std::string rotulo_fim = novo_rotulo();

    codigo_sam.push_back(rotulo_inicio + ":");
    rotulos_interrupcao.push_back(rotulo_fim);
    rotulos_continuacao.push_back(rotulo_condicao);
    traduzir_comando(corpo, false);
    rotulos_continuacao.pop_back();
    rotulos_interrupcao.pop_back();
    codigo_sam.push_back(rotulo_condicao + ":");
    traduzir_expressao(condicao);
    codigo_sam.push_back("JUMPC " + rotulo_inicio);
    codigo_sam.push_back(rotulo_fim + ":");
    break;
  }

  case NodeType::FOR: {
    ASTNode *inicializacao = no->leftmostChild;
    ASTNode *condicao = inicializacao ? inicializacao->next : nullptr;
    ASTNode *corpo = condicao ? condicao->next : nullptr;
    std::string rotulo_inicio = novo_rotulo();
    std::string rotulo_corpo = novo_rotulo();
    std::string rotulo_fim = novo_rotulo();

    auto tabela_antes_do_para = tabela_deslocamentos;
    traduzir_comando(inicializacao, false);
    codigo_sam.push_back(rotulo_inicio + ":");
    traduzir_expressao(condicao);
    codigo_sam.push_back("JUMPC " + rotulo_corpo);
    codigo_sam.push_back("JUMP " + rotulo_fim);
    codigo_sam.push_back(rotulo_corpo + ":");
    rotulos_interrupcao.push_back(rotulo_fim);
    rotulos_continuacao.push_back(rotulo_inicio);
    traduzir_comando(corpo, false);
    rotulos_continuacao.pop_back();
    rotulos_interrupcao.pop_back();
    codigo_sam.push_back("JUMP " + rotulo_inicio);
    codigo_sam.push_back(rotulo_fim + ":");
    tabela_deslocamentos = std::move(tabela_antes_do_para);
    break;
  }

  default:
    if (no->leftmostChild)
      traduzir_comando(no->leftmostChild);
    break;
  }

  if (traduzir_proximos && no->next)
    traduzir_comando(no->next);
}

void GeradorSAM::gerar(ASTNode *raiz) {
  codigo_sam.clear();
  contador_rotulos = 0;
  proximo_deslocamento_global = 1;
  deslocamentos_globais.clear();
  strings_globais.clear();
  strings_locais.clear();
  deslocamentos_declaracoes_locais.clear();
  rotulos_interrupcao.clear();
  rotulos_continuacao.clear();
  dentro_de_funcao = false;

  ASTNode *nivel_global = raiz;
  if (raiz && raiz->type == NodeType::PROG)
    nivel_global = raiz->leftmostChild;

  codigo_sam.push_back("PUSHIMM 0");

  for (ASTNode *no = nivel_global; no; no = no->next) {
    if (no->type != NodeType::VAR_DECL)
      continue;
    InformacaoVariavel informacao_variavel = criar_informacao_variavel(no->leftmostChild, proximo_deslocamento_global++);
    deslocamentos_globais[no->value] = informacao_variavel;
    codigo_sam.push_back("ADDSP 1");
    if (informacao_variavel.e_string)
      strings_globais.push_back(informacao_variavel.deslocamento);
  }

  for (ASTNode *no = nivel_global; no; no = no->next) {
    if (no->type != NodeType::VAR_DECL || !no->leftmostChild ||
        !no->leftmostChild->next)
      continue;
    traduzir_expressao(no->leftmostChild->next);
    codigo_sam.push_back("STOREABS " +
                        std::to_string(deslocamentos_globais.at(no->value).deslocamento));
  }

  codigo_sam.push_back("JUMP main");

  for (ASTNode *no = nivel_global; no; no = no->next) {
    if (no->type == NodeType::FUN_DECL)
      traduzir_comando(no, false);
  }
}

void GeradorSAM::salvar(const std::string &nome_arquivo) {
  std::ofstream arquivo(nome_arquivo);
  if (!arquivo.is_open())
    throw std::runtime_error("Não foi possível criar o arquivo SaM '" +
                             nome_arquivo + "'.");
  for (const auto &instrucao : codigo_sam) {
    arquivo << instrucao << "\n";
  }
  std::cout << "Código SaM v2.6 gerado em: " << nome_arquivo << std::endl;
}
