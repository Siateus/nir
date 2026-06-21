#include "../include/codegen.hpp"
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>

struct Variavel {
  int offset;
  bool eh_float;
  bool eh_char;
  bool eh_string;
};

static Variavel variavelParaTipo(ASTNode *tipoNode, int offset) {
  bool is_float = false;
  bool is_char = false;
  bool is_string = false;

  if (tipoNode && tipoNode->type == NodeType::TYPE) {
    if (tipoNode->value == "float")
      is_float = true;
    else if (tipoNode->value == "char")
      is_char = true;
    else if (tipoNode->value == "string")
      is_string = true;
  }

  return {offset, is_float, is_char, is_string};
}

std::unordered_map<std::string, Variavel> offsetsGlobais;
std::unordered_map<std::string, Variavel> tabelaOffsets;
std::unordered_map<ASTNode *, int> offsetsDeclaracoesLocais;
std::vector<int> stringsGlobais;
std::vector<int> stringsLocais;
int proximoOffsetGlobal = 0;
int proximoOffsetLocal = 0;
int parametrosFuncaoAtual = 0;
bool dentroDeFuncao = false;

static void mapearDeclaracoesLocais(ASTNode *no) {
  for (ASTNode *atual = no; atual; atual = atual->next) {
    if (atual->type == NodeType::VAR_DECL) {
      offsetsDeclaracoesLocais[atual] = proximoOffsetLocal++;
    }
    if (atual->leftmostChild)
      mapearDeclaracoesLocais(atual->leftmostChild);
  }
}

GeradorSAM::GeradorSAM() : contadorLabels(0) {}

std::string GeradorSAM::novoLabel() {
  return "L" + std::to_string(contadorLabels++);
}

bool GeradorSAM::eh_expressao_float(ASTNode *no) {
  if (!no)
    return false;

  if (no->inferredType == "float" || no->coercedType == "float")
    return true;

  if (no->type == NodeType::NUMBER)
    return no->value.find('.') != std::string::npos;

  if (no->type == NodeType::ID) {

    auto itLocal = tabelaOffsets.find(no->value);
    if (itLocal != tabelaOffsets.end())
      return itLocal->second.eh_float;

    auto itGlobal = offsetsGlobais.find(no->value);
    if (itGlobal != offsetsGlobais.end())
      return itGlobal->second.eh_float;
  }

  if (no->type == NodeType::BIN_OP)
    return eh_expressao_float(no->leftmostChild) ||
           eh_expressao_float(no->leftmostChild->next);

  return false;
}

// 🧮 1. Processamento de Expressões
void GeradorSAM::traduzir_expressao(ASTNode *no) {
  if (!no)
    return;

  switch (no->type) {
  case NodeType::NUMBER:
    if (no->value.find('.') != std::string::npos)
      codigoSAM.push_back("PUSHIMMF " + no->value);
    else
      codigoSAM.push_back("PUSHIMM " + no->value);
    break;

  case NodeType::CHAR:
    codigoSAM.push_back("PUSHIMMCH " + no->value);
    break;
  case NodeType::STRING:
    codigoSAM.push_back("PUSHIMMSTR " + no->value);
    break;

  case NodeType::ID:
    // 🟢 PRIORIDADE DE SOMBREAMENTO (Shadowing): Verifica primeiro escopo local
    // se estiver em função
    if (dentroDeFuncao &&
        tabelaOffsets.find(no->value) != tabelaOffsets.end()) {
      codigoSAM.push_back("PUSHOFF " +
                          std::to_string(tabelaOffsets[no->value].offset));
    } else if (offsetsGlobais.find(no->value) != offsetsGlobais.end()) {
      codigoSAM.push_back("PUSHABS " +
                          std::to_string(offsetsGlobais[no->value].offset));
    } else if (tabelaOffsets.find(no->value) != tabelaOffsets.end()) {
      codigoSAM.push_back("PUSHOFF " +
                          std::to_string(tabelaOffsets[no->value].offset));
    } else {
      codigoSAM.push_back("PUSHOFF 0");
    }
    break;

  case NodeType::BIN_OP:
    traduzir_expressao(no->leftmostChild);
    traduzir_expressao(no->leftmostChild->next);

    if (no->value == "+")
      codigoSAM.push_back(no->inferredType == "float" ? "ADDF" : "ADD");
    else if (no->value == "-")
      codigoSAM.push_back(no->inferredType == "float" ? "SUBF" : "SUB");
    else if (no->value == "*")
      codigoSAM.push_back(no->inferredType == "float" ? "TIMESF" : "TIMES");
    else if (no->value == "/")
      codigoSAM.push_back(no->inferredType == "float" ? "DIVF" : "DIV");
    else if (no->value == "%")
      codigoSAM.push_back("MOD");
    else if (no->value == "==")
      codigoSAM.push_back("EQUAL");
    else if (no->value == "!=") {
      codigoSAM.push_back("EQUAL");
      codigoSAM.push_back("NOT");
    } else if (no->value == "<")
      codigoSAM.push_back("LESS");
    else if (no->value == "<=") {
      codigoSAM.push_back("GREATER");
      codigoSAM.push_back("NOT");
    } else if (no->value == ">")
      codigoSAM.push_back("GREATER");
    else if (no->value == ">=") {
      codigoSAM.push_back("LESS");
      codigoSAM.push_back("NOT");
    } else if (no->value == "&&") {
      codigoSAM.push_back("TIMES");
    } else if (no->value == "||") {
      codigoSAM.push_back("ADD");
      codigoSAM.push_back("PUSHIMM 0");
      codigoSAM.push_back("GREATER");
    }
    break;

  case NodeType::UN_OP:
    traduzir_expressao(no->leftmostChild);
    if (no->value == "-")
      codigoSAM.push_back(no->inferredType == "float" ? "PUSHIMMF -1.0"
                                                       : "PUSHIMM -1");
    if (no->value == "-")
      codigoSAM.push_back(no->inferredType == "float" ? "TIMESF" : "TIMES");
    else if (no->value == "!")
      codigoSAM.push_back("NOT");
    break;

  case NodeType::CALL: {
    codigoSAM.push_back("ADDSP 1"); // Slot de Retorno

    ASTNode *arg = no->leftmostChild;
    int qtd_args = 0;
    while (arg) {
      traduzir_expressao(arg);
      qtd_args++;
      arg = arg->next;
    }

    codigoSAM.push_back("JSR " + no->value);

    if (qtd_args > 0) {
      codigoSAM.push_back("ADDSP -" + std::to_string(qtd_args));
    }
    break;
  }
  default:
    break;
  }

  if (no->inferredType == "int" && no->coercedType == "float")
    codigoSAM.push_back("ITOF");
}

// 📑 2. Processamento de Comandos
void GeradorSAM::traduzir_comando(ASTNode *no, bool traduzirProximos) {
  if (!no)
    return;

  switch (no->type) {

  case NodeType::FUN_DECL: {
    codigoSAM.push_back(no->value + ":");
    tabelaOffsets.clear();
    offsetsDeclaracoesLocais.clear();
    stringsLocais.clear();
    proximoOffsetLocal = 1;

    bool escopoPaiEraFuncao = dentroDeFuncao;
    dentroDeFuncao = true;

    if (no->value == "main") {
      codigoSAM.push_back("LINK");

      parametrosFuncaoAtual = 0;
      ASTNode *corpo = no->leftmostChild;
      if (corpo &&
          (corpo->type == NodeType::TYPE || corpo->type == NodeType::ID))
        corpo = corpo->next;

      mapearDeclaracoesLocais(corpo);
      int qtdLocais = proximoOffsetLocal - 1;

      if (qtdLocais > 0)
        codigoSAM.push_back("ADDSP " + std::to_string(qtdLocais));

      if (corpo)
        traduzir_comando(corpo);

      for (int offset : stringsLocais) {
        codigoSAM.push_back("PUSHOFF " + std::to_string(offset));
        codigoSAM.push_back("FREE");
      }

      if (qtdLocais > 0)
        codigoSAM.push_back("ADDSP -" + std::to_string(qtdLocais));
      codigoSAM.push_back("UNLINK");
      for (int offset : stringsGlobais) {
        codigoSAM.push_back("PUSHABS " + std::to_string(offset));
        codigoSAM.push_back("FREE");
      }
      if (proximoOffsetGlobal > 1)
        codigoSAM.push_back("ADDSP -" +
                            std::to_string(proximoOffsetGlobal - 1));
      codigoSAM.push_back("STOP");
    } else {
      std::vector<std::string> nomes_params;
      ASTNode *param = no->leftmostChild;
      if (param &&
          (param->type == NodeType::TYPE || param->type == NodeType::ID))
        param = param->next;

      while (param && param->type == NodeType::PARAM) {
        nomes_params.push_back(param->value);
        param = param->next;
      }

      int total_params = nomes_params.size();
      parametrosFuncaoAtual = total_params;

      for (int i = 0; i < total_params; i++) {
        ASTNode *tipoParam = nullptr;
        ASTNode *paramNode =
            no->leftmostChild ? no->leftmostChild->next : nullptr;
        for (int j = 0; j < i && paramNode; j++)
          paramNode = paramNode->next;
        if (paramNode)
          tipoParam = paramNode->leftmostChild;
        tabelaOffsets[nomes_params[i]] =
            variavelParaTipo(tipoParam, -2 - (total_params - 1 - i));
      }

      codigoSAM.push_back("LINK");

      ASTNode *corpo = param;
      mapearDeclaracoesLocais(corpo);
      int qtdLocais = proximoOffsetLocal - 1;

      if (qtdLocais > 0)
        codigoSAM.push_back("ADDSP " + std::to_string(qtdLocais));

      if (corpo)
        traduzir_comando(corpo);

      for (int offset : stringsLocais) {
        codigoSAM.push_back("PUSHOFF " + std::to_string(offset));
        codigoSAM.push_back("FREE");
      }

      if (qtdLocais > 0)
        codigoSAM.push_back("ADDSP -" + std::to_string(qtdLocais));

      codigoSAM.push_back("UNLINK");
      codigoSAM.push_back("RST");
    }

    dentroDeFuncao = escopoPaiEraFuncao;

    if (traduzirProximos && no->next)
      traduzir_comando(no->next);
    return;
  }

  case NodeType::RETURN: {
    if (no->leftmostChild) {
      traduzir_expressao(no->leftmostChild);

      int offset_retorno = -2 - parametrosFuncaoAtual;

      codigoSAM.push_back("STOREOFF " + std::to_string(offset_retorno));
    }

    int qtdLocais = proximoOffsetLocal - 1;

    for (int offset : stringsLocais) {
      codigoSAM.push_back("PUSHOFF " + std::to_string(offset));
      codigoSAM.push_back("FREE");
    }

    if (qtdLocais > 0)
      codigoSAM.push_back("ADDSP -" + std::to_string(qtdLocais));

    codigoSAM.push_back("UNLINK");
    codigoSAM.push_back("RST");

    return;
  }
  case NodeType::VAR_DECL: {
    Variavel varInfo = variavelParaTipo(no->leftmostChild, 0);

    if (!dentroDeFuncao) {
      varInfo.offset = proximoOffsetGlobal++;
      offsetsGlobais[no->value] = varInfo;
      codigoSAM.push_back("ADDSP 1");

      if (no->leftmostChild && no->leftmostChild->next) {
        traduzir_expressao(no->leftmostChild->next);
        codigoSAM.push_back("STOREABS " +
                            std::to_string(offsetsGlobais[no->value].offset));
        if (varInfo.eh_string)
          stringsGlobais.push_back(offsetsGlobais[no->value].offset);
      }
    } else {
      int offset_atual = offsetsDeclaracoesLocais.at(no);
      varInfo.offset = offset_atual;
      tabelaOffsets[no->value] = varInfo;

      if (no->leftmostChild && no->leftmostChild->next) {
        traduzir_expressao(no->leftmostChild->next);
        codigoSAM.push_back("STOREOFF " + std::to_string(offset_atual));
        if (varInfo.eh_string)
          stringsLocais.push_back(offset_atual);
      }
    }
    break;
  }

  case NodeType::BLOCK: {
    auto tabelaAntesDoBloco = tabelaOffsets;
    if (no->leftmostChild)
      traduzir_comando(no->leftmostChild);
    tabelaOffsets = std::move(tabelaAntesDoBloco);
    break;
  }

  case NodeType::ASSIGN: {
    ASTNode *no_id = no->leftmostChild;
    traduzir_expressao(no_id->next);
    // locais/globais
    if (dentroDeFuncao &&
        tabelaOffsets.find(no_id->value) != tabelaOffsets.end()) {
      codigoSAM.push_back("STOREOFF " +
                          std::to_string(tabelaOffsets[no_id->value].offset));
    } else {
      codigoSAM.push_back("STOREABS " +
                          std::to_string(offsetsGlobais[no_id->value].offset));
    }
    break;
  }

  case NodeType::PRINT: {
    if (no->leftmostChild && no->leftmostChild->type == NodeType::STRING) {
      int tempOffset = proximoOffsetLocal;
      codigoSAM.push_back("ADDSP 1");
      traduzir_expressao(no->leftmostChild);
      codigoSAM.push_back("STOREOFF " + std::to_string(tempOffset));
      codigoSAM.push_back("PUSHOFF " + std::to_string(tempOffset));
      codigoSAM.push_back("WRITESTR");
      codigoSAM.push_back("PUSHOFF " + std::to_string(tempOffset));
      codigoSAM.push_back("FREE");
      codigoSAM.push_back("ADDSP -1");
      break;
    }

    traduzir_expressao(no->leftmostChild);

    if (eh_expressao_float(no->leftmostChild)) {
      codigoSAM.push_back("WRITEF");
    } else if (no->leftmostChild->type == NodeType::CHAR ||
               (no->leftmostChild->type == NodeType::ID &&
                tabelaOffsets.count(no->leftmostChild->value) &&
                tabelaOffsets[no->leftmostChild->value].eh_char)) {
      codigoSAM.push_back("WRITECH");
    } else if (no->leftmostChild->type == NodeType::STRING ||
               (no->leftmostChild->type == NodeType::ID &&
                tabelaOffsets.count(no->leftmostChild->value) &&
                tabelaOffsets[no->leftmostChild->value].eh_string)) {
      codigoSAM.push_back("WRITESTR");
    } else {
      codigoSAM.push_back("WRITE");
    }
    break;
  }

  case NodeType::IF: {
    ASTNode *cond = no->leftmostChild;
    ASTNode *blocoThen = cond ? cond->next : nullptr;
    ASTNode *blocoElse = blocoThen ? blocoThen->next : nullptr;
    std::string labelThen = novoLabel();
    std::string labelElse = novoLabel();
    std::string labelFim = novoLabel();

    traduzir_expressao(cond);
    codigoSAM.push_back("JUMPC " + labelThen);
    codigoSAM.push_back("JUMP " + labelElse);
    codigoSAM.push_back(labelThen + ":");
    traduzir_comando(blocoThen, false);
    codigoSAM.push_back("JUMP " + labelFim);
    codigoSAM.push_back(labelElse + ":");
    if (blocoElse && blocoElse->type != NodeType::EMPTY)
      traduzir_comando(blocoElse, false);
    codigoSAM.push_back(labelFim + ":");
    break;
  }

  case NodeType::WHILE: {
    ASTNode *cond = no->leftmostChild;
    ASTNode *corpo = cond ? cond->next : nullptr;
    std::string labelInicio = novoLabel();
    std::string labelCorpo = novoLabel();
    std::string labelFim = novoLabel();

    codigoSAM.push_back(labelInicio + ":");
    traduzir_expressao(cond);
    codigoSAM.push_back("JUMPC " + labelCorpo);
    codigoSAM.push_back("JUMP " + labelFim);
    codigoSAM.push_back(labelCorpo + ":");
    traduzir_comando(corpo, false);
    codigoSAM.push_back("JUMP " + labelInicio);
    codigoSAM.push_back(labelFim + ":");
    break;
  }

  case NodeType::DO_WHILE: {
    ASTNode *corpo = no->leftmostChild;
    ASTNode *cond = corpo ? corpo->next : nullptr;
    std::string labelInicio = novoLabel();

    codigoSAM.push_back(labelInicio + ":");
    traduzir_comando(corpo, false);
    traduzir_expressao(cond);
    codigoSAM.push_back("JUMPC " + labelInicio);
    break;
  }

  case NodeType::FOR: {
    ASTNode *init = no->leftmostChild;
    ASTNode *cond = init ? init->next : nullptr;
    ASTNode *corpo = cond ? cond->next : nullptr;
    std::string labelInicio = novoLabel();
    std::string labelCorpo = novoLabel();
    std::string labelFim = novoLabel();

    auto tabelaAntesDoFor = tabelaOffsets;
    traduzir_comando(init, false);
    codigoSAM.push_back(labelInicio + ":");
    traduzir_expressao(cond);
    codigoSAM.push_back("JUMPC " + labelCorpo);
    codigoSAM.push_back("JUMP " + labelFim);
    codigoSAM.push_back(labelCorpo + ":");
    traduzir_comando(corpo, false);
    codigoSAM.push_back("JUMP " + labelInicio);
    codigoSAM.push_back(labelFim + ":");
    tabelaOffsets = std::move(tabelaAntesDoFor);
    break;
  }

  default:
    if (no->leftmostChild)
      traduzir_comando(no->leftmostChild);
    break;
  }

  if (traduzirProximos && no->next)
    traduzir_comando(no->next);
}

void GeradorSAM::gerar(ASTNode *raiz) {
  codigoSAM.clear();
  contadorLabels = 0;
  proximoOffsetGlobal = 1;
  offsetsGlobais.clear();
  stringsGlobais.clear();
  stringsLocais.clear();
  offsetsDeclaracoesLocais.clear();
  dentroDeFuncao = false;

  ASTNode *topLevel = raiz;
  if (raiz && raiz->type == NodeType::PROG)
    topLevel = raiz->leftmostChild;

  codigoSAM.push_back("PUSHIMM 0");

  for (ASTNode *no = topLevel; no; no = no->next) {
    if (no->type == NodeType::VAR_DECL)
      traduzir_comando(no, false);
  }

  codigoSAM.push_back("JUMP main");

  for (ASTNode *no = topLevel; no; no = no->next) {
    if (no->type == NodeType::FUN_DECL)
      traduzir_comando(no, false);
  }
}

void GeradorSAM::salvar(const std::string &nomeArquivo) {
  std::ofstream arquivo(nomeArquivo);
  for (const auto &instrucao : codigoSAM) {
    arquivo << instrucao << "\n";
  }
  std::cout << "Código SaM v2.6 gerado em: " << nomeArquivo << std::endl;
}
