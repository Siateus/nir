#include "../include/semantic.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

static bool eh_numerico(DataType tipo) {
  return tipo == DataType::INT || tipo == DataType::FLOAT;
}

static void marcar_conversao(ASTNode *no, DataType origem, DataType destino) {
  if (no && origem == DataType::INT && destino == DataType::FLOAT)
    no->coercedType = "float";
}

static bool tipo_compativel(ASTNode *expressao, DataType origem,
                            DataType destino) {
  if (origem == destino)
    return true;
  if (origem == DataType::INT && destino == DataType::FLOAT) {
    marcar_conversao(expressao, origem, destino);
    return true;
  }
  return false;
}

AnalisadorSematico::AnalisadorSematico() {
  escopoAtual = new Scope(nullptr);
  funcaoAtualRetorno = DataType::VOID;
}

void AnalisadorSematico::Analisar_sematica(ASTNode *raiz_ast) {
  try {
    analisar_node(raiz_ast);
  } catch (const std::runtime_error &e) {
    throw;
  }
}

void AnalisadorSematico::analisar_node(ASTNode *no) {
  if (!no)
    return;

  switch (no->type) {

  case NodeType::VAR_DECL: {
    std::string nome_var = no->value;
    ASTNode *no_tipo = no->leftmostChild;
    DataType tipo_var = traduzir_para_string(no_tipo->value);

    auto varSym =
        std::make_shared<Symbol>(Symbol{nome_var, tipo_var, false, {}});
    if (!escopoAtual->define(varSym)) {
      throw std::runtime_error("Erro Semântico: Variavel '" + nome_var +
                               "' ja declarada neste escopo.");
    }

    if (no_tipo->next) {
      DataType tipo_init = get_type_node(no_tipo->next);
      if (!tipo_compativel(no_tipo->next, tipo_init, tipo_var)) {
        throw std::runtime_error(
            "Erro Semântico: Tipo incompativel na inicialização de '" +
            nome_var + "'. Esperado: " + type_to_string(tipo_var) +
            ", mas obteve: " + type_to_string(tipo_init));
      }
    }
    break;
  }

  case NodeType::FUN_DECL: {
    std::string nome_fun = no->value;
    ASTNode *no_retorno = no->leftmostChild;
    DataType tipo_retorno =
        no_retorno ? traduzir_para_string(no_retorno->value) : DataType::VOID;

    std::vector<DataType> param_types;
    ASTNode *param = no_retorno ? no_retorno->next : nullptr;
    while (param && param->type == NodeType::PARAM) {
      ASTNode *param_tipo = param->leftmostChild;
      if (param_tipo) {
        param_types.push_back(traduzir_para_string(param_tipo->value));
      }
      param = param->next;
    }

    auto funSym = std::make_shared<Symbol>(
        Symbol{nome_fun, tipo_retorno, true, param_types});
    if (!escopoAtual->define(funSym)) {
      throw std::runtime_error("Erro Semântico: Funcao '" + nome_fun +
                               "' ja declarada.");
    }

    DataType tipoAnterior = funcaoAtualRetorno;
    funcaoAtualRetorno = tipo_retorno;

    Scope *parentScope = escopoAtual;
    escopoAtual = new Scope(parentScope);
    param = no_retorno ? no_retorno->next : nullptr;
    while (param && param->type == NodeType::PARAM) {
      std::string param_nome = param->value;
      ASTNode *param_tipo = param->leftmostChild;
      if (param_tipo) {
        auto paramSym = std::make_shared<Symbol>(Symbol{
            param_nome, traduzir_para_string(param_tipo->value), false, {}});
        escopoAtual->define(paramSym);
      }
      param = param->next;
    }

    ASTNode *body = param;
    if (body) {
      analisar_node(body);
    }

    escopoAtual = parentScope;
    funcaoAtualRetorno = tipoAnterior;
    if (no->next)
      analisar_node(no->next);
    return;
  }

  case NodeType::ASSIGN: {
    ASTNode *no_id = no->leftmostChild;
    auto sym = escopoAtual->resolve(no_id->value);
    if (!sym) {
      throw std::runtime_error("Erro Semântico: Variavel '" + no_id->value +
                               "' nao declarada.");
    }
    if (sym->is_function) {
      throw std::runtime_error("Erro Semântico: Identificador '" +
                               no_id->value +
                               "' e uma funcao e nao pode receber atribuição.");
    }

    DataType tipo_expr = get_type_node(no_id->next);
    if (!tipo_compativel(no_id->next, tipo_expr, sym->type)) {
      throw std::runtime_error("Erro Semântico: Atribuição inválida para '" +
                               no_id->value +
                               "'. Esperado: " + type_to_string(sym->type) +
                               ", mas obteve: " + type_to_string(tipo_expr));
    }
    break;
  }

  case NodeType::RETURN: {
    DataType tipo_expr = DataType::VOID;
    if (no->leftmostChild) {
      tipo_expr = get_type_node(no->leftmostChild);
    }

    if (!tipo_compativel(no->leftmostChild, tipo_expr, funcaoAtualRetorno)) {
      throw std::runtime_error(
          "Erro Semântico: Retorno invalido. A funcao espera retornar '" +
          type_to_string(funcaoAtualRetorno) + "', mas retornou '" +
          type_to_string(tipo_expr) + "'.");
    }
    break;
  }

  case NodeType::IF:
  case NodeType::WHILE: {
    DataType tipo_condicao = get_type_node(no->leftmostChild);
    if (tipo_condicao != DataType::INT) {
      throw std::runtime_error("Erro Semântico: A condição do comando de "
                               "controle precisa ser do tipo 'int'.");
    }

    if (no->leftmostChild->next) {
      analisar_node(no->leftmostChild->next);
    }
    break;
  }

  case NodeType::DO_WHILE: {
    ASTNode *corpo = no->leftmostChild;
    ASTNode *condicao = corpo ? corpo->next : nullptr;
    if (get_type_node(condicao) != DataType::INT) {
      throw std::runtime_error("Erro Semântico: A condição do comando de "
                               "controle precisa ser do tipo 'int'.");
    }
    if (corpo)
      analisar_node(corpo);
    break;
  }

  case NodeType::FOR: {
    Scope *parentScope = escopoAtual;
    escopoAtual = new Scope(parentScope);

    ASTNode *inicializacao = no->leftmostChild;
    ASTNode *condicao = inicializacao ? inicializacao->next : nullptr;
    ASTNode *corpo = condicao ? condicao->next : nullptr;

    if (inicializacao) {
      ASTNode *proximo = inicializacao->next;
      inicializacao->next = nullptr;
      analisar_node(inicializacao);
      inicializacao->next = proximo;
    }
    if (get_type_node(condicao) != DataType::INT) {
      throw std::runtime_error("Erro Semântico: A condição do comando de "
                               "controle precisa ser do tipo 'int'.");
    }
    if (corpo)
      analisar_node(corpo);

    escopoAtual = parentScope;
    if (no->next)
      analisar_node(no->next);
    return;
  }

  case NodeType::BLOCK: {
    Scope *parentScope = escopoAtual;
    escopoAtual = new Scope(parentScope);
    if (no->leftmostChild)
      analisar_node(no->leftmostChild);

    escopoAtual = parentScope;
    if (no->next)
      analisar_node(no->next);
    return;
  }

  case NodeType::PRINT:
  case NodeType::CALL: {
    get_type_node(no);
    break;
  }

  case NodeType::ID: {
    auto sym = escopoAtual->resolve(no->value);
    if (!sym) {
      throw std::runtime_error("Erro Semântico: Identificador '" + no->value +
                               "' nao declarado neste escopo.");
    }
    break;
  }

  default:
    break;
  }

  if (no->leftmostChild)
    analisar_node(no->leftmostChild);
  if (no->next)
    analisar_node(no->next);
}

DataType AnalisadorSematico::get_type_node(ASTNode *node) {
  if (!node)
    return DataType::VOID;

  auto marcar_tipo = [node](DataType tipo) {
    switch (tipo) {
    case DataType::INT:
      node->inferredType = "int";
      break;
    case DataType::FLOAT:
      node->inferredType = "float";
      break;
    case DataType::CHAR:
      node->inferredType = "char";
      break;
    case DataType::STRING:
      node->inferredType = "string";
      break;
    case DataType::VOID:
      node->inferredType = "void";
      break;
    }
    return tipo;
  };

  switch (node->type) {
  case NodeType::NUMBER:
    if (node->value.find('.') != std::string::npos)
      return marcar_tipo(DataType::FLOAT);
    return marcar_tipo(DataType::INT);

  case NodeType::CHAR:
    return marcar_tipo(DataType::CHAR);
  case NodeType::STRING:
    return marcar_tipo(DataType::STRING);

  case NodeType::ID: {
    auto sym = escopoAtual->resolve(node->value);
    if (sym)
      return marcar_tipo(sym->type);
    throw std::runtime_error("Erro Semântico: Variavel '" + node->value +
                             "' nao declarada neste escopo.");
  }

  case NodeType::CALL: {
    auto sym = escopoAtual->resolve(node->value);
    if (!sym || !sym->is_function) {
      throw std::runtime_error("Erro Semântico: Funcao '" + node->value +
                               "' nao declarada.");
    }

    std::vector<DataType> arg_types;
    ASTNode *arg = node->leftmostChild;
    while (arg) {
      arg_types.push_back(get_type_node(arg));
      arg = arg->next;
    }

    if (arg_types.size() != sym->param_types.size()) {
      throw std::runtime_error(
          "Erro Semântico: Quantidade incorreta de argumentos para a funcao '" +
          node->value +
          "'. Esperado: " + std::to_string(sym->param_types.size()) +
          ", mas recebeu: " + std::to_string(arg_types.size()));
    }

    for (size_t i = 0; i < arg_types.size(); ++i) {
      ASTNode *arg = node->leftmostChild;
      for (size_t j = 0; j < i; ++j)
        arg = arg->next;

      if (!tipo_compativel(arg, arg_types[i], sym->param_types[i])) {
        throw std::runtime_error(
            "Erro Semântico: Tipo de argumento incompativel na funcao '" +
            node->value + "' na posicao " + std::to_string(i + 1) +
            ". Esperado: " + type_to_string(sym->param_types[i]) +
            ", obteve: " + type_to_string(arg_types[i]));
      }
    }
    return marcar_tipo(sym->type);
  }

  case NodeType::BIN_OP: {
    DataType t_esq = get_type_node(node->leftmostChild);
    DataType t_dir = get_type_node(node->leftmostChild->next);

    if (node->value == "&&" || node->value == "||") {
      if (t_esq != DataType::INT || t_dir != DataType::INT) {
        throw std::runtime_error("Erro Semântico: Operador lógico '" +
                                 node->value +
                                 "' exige operandos do tipo 'int'.");
      }
      return marcar_tipo(DataType::INT);
    }

    if (node->value == "==" || node->value == "!=") {
      if (t_esq != t_dir && eh_numerico(t_esq) && eh_numerico(t_dir)) {
        if (t_esq == DataType::INT)
          marcar_conversao(node->leftmostChild, t_esq, DataType::FLOAT);
        if (t_dir == DataType::INT)
          marcar_conversao(node->leftmostChild->next, t_dir, DataType::FLOAT);
      } else if (t_esq != t_dir) {
        throw std::runtime_error("Erro Semântico: Comparação '" + node->value +
                                 "' invalida entre tipos diferentes.");
      }
      return marcar_tipo(DataType::INT);
    }

    if (node->value == "<" || node->value == ">" || node->value == "<=" ||
        node->value == ">=") {
      bool esquerdaNumerica =
          t_esq == DataType::INT || t_esq == DataType::FLOAT;
      bool direitaNumerica =
          t_dir == DataType::INT || t_dir == DataType::FLOAT;
      if (!esquerdaNumerica || !direitaNumerica) {
        throw std::runtime_error(
            "Erro Semântico: Comparação relacional '" + node->value +
            "' exige operandos numéricos.");
      }
      if (t_esq == DataType::INT && t_dir == DataType::FLOAT)
        marcar_conversao(node->leftmostChild, t_esq, DataType::FLOAT);
      if (t_esq == DataType::FLOAT && t_dir == DataType::INT)
        marcar_conversao(node->leftmostChild->next, t_dir, DataType::FLOAT);
      return marcar_tipo(DataType::INT);
    }

    if (node->value == "%") {
      if (t_esq != DataType::INT || t_dir != DataType::INT) {
        throw std::runtime_error(
            "Erro Semântico: Operador '%' exige operandos do tipo 'int'.");
      }
      return marcar_tipo(DataType::INT);
    }

    bool esquerdaNumerica =
        t_esq == DataType::INT || t_esq == DataType::FLOAT;
    bool direitaNumerica =
        t_dir == DataType::INT || t_dir == DataType::FLOAT;
    if (!esquerdaNumerica || !direitaNumerica) {
      throw std::runtime_error("Erro Semântico: Operação '" + node->value +
                               "' exige operandos numéricos.");
    }

    DataType tipo_resultado =
        t_esq == DataType::FLOAT || t_dir == DataType::FLOAT
            ? DataType::FLOAT
            : DataType::INT;
    marcar_conversao(node->leftmostChild, t_esq, tipo_resultado);
    marcar_conversao(node->leftmostChild->next, t_dir, tipo_resultado);
    return marcar_tipo(tipo_resultado);
  }

  case NodeType::UN_OP: {
    DataType t = get_type_node(node->leftmostChild);
    if (node->value == "-" && t != DataType::INT && t != DataType::FLOAT) {
      throw std::runtime_error(
          "Erro Semântico: Operador unário '-' exige tipo numérico.");
    }
    if (node->value == "!" && t != DataType::INT) {
      throw std::runtime_error(
          "Erro Semântico: Operador unário '!' exige tipo 'int'.");
    }
    return marcar_tipo(t);
  }

  default:
    return marcar_tipo(DataType::VOID);
  }
}

DataType AnalisadorSematico::traduzir_para_string(const std::string &str_tipo) {
  if (str_tipo == "int")
    return DataType::INT;
  if (str_tipo == "float")
    return DataType::FLOAT;
  if (str_tipo == "char")
    return DataType::CHAR;
  if (str_tipo == "string")
    return DataType::STRING;
  if (str_tipo == "void")
    return DataType::VOID;
  throw std::runtime_error("Tipo desconhecido: " + str_tipo);
}

std::string AnalisadorSematico::type_to_string(DataType t) {
  switch (t) {
  case DataType::INT:
    return "int";
  case DataType::FLOAT:
    return "float";
  case DataType::CHAR:
    return "char";
  case DataType::STRING:
    return "string";
  case DataType::VOID:
    return "void";
  }
  return "desconhecido";
}
