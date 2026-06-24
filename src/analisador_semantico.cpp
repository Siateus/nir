#include "../include/analisador_semantico.hpp"
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

static bool e_numerico(TipoDado tipo) {
  return tipo == TipoDado::INT || tipo == TipoDado::FLOAT;
}

static void marcar_conversao(ASTNode *no, TipoDado origem, TipoDado destino) {
  if (no && origem == TipoDado::INT && destino == TipoDado::FLOAT)
    no->coercedType = "float";
}

static bool tipo_compativel(ASTNode *expressao, TipoDado origem,
                            TipoDado destino) {
  if (origem == destino)
    return true;
  if (origem == TipoDado::INT && destino == TipoDado::FLOAT) {
    marcar_conversao(expressao, origem, destino);
    return true;
  }
  return false;
}

static void validar_funcao_main(ASTNode *raiz) {
  ASTNode *nivel = raiz;
  if (nivel && nivel->type == NodeType::PROG)
    nivel = nivel->leftmostChild;

  ASTNode *funcao_principal = nullptr;
  for (ASTNode *atual = nivel; atual; atual = atual->next) {
    if (atual->type == NodeType::FUN_DECL && atual->value == "main") {
      funcao_principal = atual;
      break;
    }
  }

  if (!funcao_principal) {
    throw std::runtime_error(
        "Erro Semântico: O programa precisa declarar a função "
        "'fun main(): void'.");
  }

  ASTNode *retorno = funcao_principal->leftmostChild;
  if (!retorno || retorno->type != NodeType::TYPE ||
      retorno->value != "void") {
    throw std::runtime_error(
        "Erro Semântico: A função 'main' precisa retornar 'void'.");
  }

  ASTNode *proximo = retorno->next;
  if (proximo && proximo->type == NodeType::PARAM) {
    throw std::runtime_error(
        "Erro Semântico: A função 'main' não pode receber parâmetros.");
  }
}

static bool esta_dentro_de_laco(ASTNode *no) {
  for (ASTNode *pai = no ? no->parent : nullptr; pai; pai = pai->parent) {
    if (pai->type == NodeType::WHILE || pai->type == NodeType::DO_WHILE ||
        pai->type == NodeType::FOR)
      return true;
  }
  return false;
}

AnalisadorSematico::AnalisadorSematico() {
  escopo_global = new Escopo(nullptr);
  escopo_atual = escopo_global;
  tipo_retorno_funcao_atual = TipoDado::VOID;
}

AnalisadorSematico::~AnalisadorSematico() { delete escopo_global; }

AnalisadorSematico::EstadoInicializacao
AnalisadorSematico::capturar_estado_inicializacao() const {
  EstadoInicializacao estado;
  escopo_global->coletar_estado_inicializacao(estado);
  return estado;
}

void AnalisadorSematico::restaurar_estado_inicializacao(
    const EstadoInicializacao &estado) {
  escopo_global->aplicar_estado_inicializacao(estado);
}

void AnalisadorSematico::intersectar_estado_inicializacao(
    const EstadoInicializacao &base, const EstadoInicializacao &primeiro,
    const EstadoInicializacao &segundo) {
  EstadoInicializacao resultado = base;

  for (const auto &entrada : base) {
    const Simbolo *simbolo = entrada.first;
    bool inicializado_base = entrada.second;

    bool inicializado_primeiro = inicializado_base;
    auto encontrado_primeiro = primeiro.find(simbolo);
    if (encontrado_primeiro != primeiro.end())
      inicializado_primeiro = encontrado_primeiro->second;

    bool inicializado_segundo = inicializado_base;
    auto encontrado_segundo = segundo.find(simbolo);
    if (encontrado_segundo != segundo.end())
      inicializado_segundo = encontrado_segundo->second;

    resultado[simbolo] = inicializado_primeiro && inicializado_segundo;
  }

  restaurar_estado_inicializacao(resultado);
}

void AnalisadorSematico::analisar_no_isolado(ASTNode *no) {
  if (!no)
    return;

  ASTNode *proximo = no->next;
  no->next = nullptr;
  analisar_no(no);
  no->next = proximo;
}

void AnalisadorSematico::registrar_simbolos_globais(ASTNode *raiz) {
  ASTNode *nivel = raiz && raiz->type == NodeType::PROG ? raiz->leftmostChild
                                                        : raiz;
  for (ASTNode *atual = nivel; atual; atual = atual->next) {
    if (atual->type == NodeType::VAR_DECL) {
      ASTNode *no_tipo = atual->leftmostChild;
      TipoDado tipo = converter_texto_para_tipo(no_tipo->value);
      if (tipo == TipoDado::VOID) {
        throw std::runtime_error("Erro Semântico: Variavel '" + atual->value +
                                 "' não pode possuir tipo 'void'.");
      }
      auto simbolo = std::make_shared<Simbolo>(
          Simbolo{atual->value, tipo, false, false, {}});
      if (!escopo_atual->definir(simbolo)) {
        throw std::runtime_error("Erro Semântico: Identificador global '" +
                                 atual->value + "' já declarado.");
      }
      continue;
    }

    if (atual->type != NodeType::FUN_DECL)
      continue;

    ASTNode *no_retorno = atual->leftmostChild;
    TipoDado retorno = converter_texto_para_tipo(no_retorno->value);
    if (retorno == TipoDado::STRING) {
      throw std::runtime_error(
          "Erro Semântico: Funções ainda não podem retornar 'string'.");
    }

    std::vector<TipoDado> parametros;
    ASTNode *parametro = no_retorno->next;
    while (parametro && parametro->type == NodeType::PARAM) {
      TipoDado tipo = converter_texto_para_tipo(parametro->leftmostChild->value);
      if (tipo == TipoDado::VOID || tipo == TipoDado::STRING) {
        throw std::runtime_error(
            "Erro Semântico: Parametro '" + parametro->value +
            "' não pode possuir tipo '" + converter_tipo_para_texto(tipo) + "'.");
      }
      parametros.push_back(tipo);
      parametro = parametro->next;
    }

    auto simbolo = std::make_shared<Simbolo>(
        Simbolo{atual->value, retorno, true, true, parametros});
    if (!escopo_atual->definir(simbolo)) {
      throw std::runtime_error("Erro Semântico: Identificador global '" +
                               atual->value + "' já declarado.");
    }
  }
}

void AnalisadorSematico::analisar(ASTNode *raiz) {
  try {
    validar_funcao_main(raiz);
    registrar_simbolos_globais(raiz);
    analisar_no(raiz);
  } catch (const std::runtime_error &erro) {
    throw;
  }
}

bool AnalisadorSematico::garante_retorno(ASTNode *no) {
  if (!no)
    return false;
  if (no->type == NodeType::RETURN)
    return true;
  if (no->type == NodeType::BLOCK) {
    for (ASTNode *comando = no->leftmostChild; comando;
         comando = comando->next) {
      if (garante_retorno(comando))
        return true;
    }
    return false;
  }
  if (no->type == NodeType::IF) {
    ASTNode *condicao = no->leftmostChild;
    ASTNode *entao = condicao ? condicao->next : nullptr;
    ASTNode *senao = entao ? entao->next : nullptr;
    return garante_retorno(entao) && senao &&
           senao->type != NodeType::EMPTY && garante_retorno(senao);
  }
  return false;
}

void AnalisadorSematico::analisar_no(ASTNode *no) {
  if (!no)
    return;

  switch (no->type) {

  case NodeType::VAR_DECL: {
    std::string nome_variavel = no->value;
    ASTNode *no_tipo = no->leftmostChild;
    TipoDado tipo_variavel = converter_texto_para_tipo(no_tipo->value);

    if (tipo_variavel == TipoDado::VOID) {
      throw std::runtime_error("Erro Semântico: Variavel '" + nome_variavel +
                               "' não pode possuir tipo 'void'.");
    }

    bool e_global = no->parent && no->parent->type == NodeType::PROG;
    if (!e_global) {
      auto simbolo_variavel =
          std::make_shared<Simbolo>(Simbolo{nome_variavel, tipo_variavel, false,
                                            no_tipo->next != nullptr, {}});
      if (!escopo_atual->definir(simbolo_variavel)) {
        throw std::runtime_error("Erro Semântico: Variavel '" + nome_variavel +
                                 "' ja declarada neste escopo.");
      }
    }

    if (no_tipo->next) {
      TipoDado tipo_init = obter_tipo_no(no_tipo->next);
      if (tipo_variavel == TipoDado::STRING &&
          no_tipo->next->type != NodeType::STRING) {
        throw std::runtime_error(
            "Erro Semântico: Strings só podem receber literais de string.");
      }
      if (!tipo_compativel(no_tipo->next, tipo_init, tipo_variavel)) {
        throw std::runtime_error(
            "Erro Semântico: Tipo incompativel na inicialização de '" +
            nome_variavel + "'. Esperado: " + converter_tipo_para_texto(tipo_variavel) +
            ", mas obteve: " + converter_tipo_para_texto(tipo_init));
      }
    }

    if (e_global) {
      auto simbolo_global = escopo_atual->resolver(nome_variavel);
      if (simbolo_global)
        simbolo_global->inicializado = (no_tipo->next != nullptr);
    }
    break;
  }

  case NodeType::FUN_DECL: {
    std::string nome_funcao = no->value;
    ASTNode *no_retorno = no->leftmostChild;
    TipoDado tipo_retorno =
        no_retorno ? converter_texto_para_tipo(no_retorno->value) : TipoDado::VOID;

    ASTNode *parametro = no_retorno ? no_retorno->next : nullptr;

    TipoDado tipo_anterior = tipo_retorno_funcao_atual;
    tipo_retorno_funcao_atual = tipo_retorno;

    Escopo *escopo_pai = escopo_atual;
    escopo_atual = new Escopo(escopo_pai);
    parametro = no_retorno ? no_retorno->next : nullptr;
    while (parametro && parametro->type == NodeType::PARAM) {
      std::string nome_parametro = parametro->value;
      ASTNode *tipo_parametro = parametro->leftmostChild;
      if (tipo_parametro) {
        TipoDado tipo_parametro_dado = converter_texto_para_tipo(tipo_parametro->value);
        auto simbolo_parametro = std::make_shared<Simbolo>(Simbolo{
            nome_parametro, tipo_parametro_dado, false, true, {}});
        if (!escopo_atual->definir(simbolo_parametro)) {
          throw std::runtime_error("Erro Semântico: Parametro '" + nome_parametro +
                                   "' repetido na função '" + nome_funcao + "'.");
        }
      }
      parametro = parametro->next;
    }

    ASTNode *corpo = parametro;
    if (corpo) {
      analisar_no(corpo);
    }
    if (tipo_retorno != TipoDado::VOID && !garante_retorno(corpo)) {
      throw std::runtime_error("Erro Semântico: A função '" + nome_funcao +
                               "' pode terminar sem retornar '" +
                               converter_tipo_para_texto(tipo_retorno) + "'.");
    }

    escopo_atual = escopo_pai;
    tipo_retorno_funcao_atual = tipo_anterior;
    if (no->next)
      analisar_no(no->next);
    return;
  }

  case NodeType::ASSIGN: {
    ASTNode *no_id = no->leftmostChild;
    auto simbolo_resolvido = escopo_atual->resolver(no_id->value);
    if (!simbolo_resolvido) {
      throw std::runtime_error("Erro Semântico: Variavel '" + no_id->value +
                               "' nao declarada.");
    }
    if (simbolo_resolvido->e_funcao) {
      throw std::runtime_error("Erro Semântico: Identificador '" +
                               no_id->value +
                               "' erro uma funcao erro nao pode receber atribuição.");
    }

    TipoDado tipo_expressao = obter_tipo_no(no_id->next);
    if (simbolo_resolvido->tipo == TipoDado::STRING &&
        no_id->next->type != NodeType::STRING) {
      throw std::runtime_error(
          "Erro Semântico: Strings só podem receber literais de string.");
    }
    if (!tipo_compativel(no_id->next, tipo_expressao, simbolo_resolvido->tipo)) {
      throw std::runtime_error("Erro Semântico: Atribuição inválida para '" +
                               no_id->value +
                               "'. Esperado: " + converter_tipo_para_texto(simbolo_resolvido->tipo) +
                               ", mas obteve: " + converter_tipo_para_texto(tipo_expressao));
    }
    simbolo_resolvido->inicializado = true;
    break;
  }

  case NodeType::RETURN: {
    TipoDado tipo_expressao = TipoDado::VOID;
    if (no->leftmostChild) {
      tipo_expressao = obter_tipo_no(no->leftmostChild);
    }

    if (!tipo_compativel(no->leftmostChild, tipo_expressao, tipo_retorno_funcao_atual)) {
      throw std::runtime_error(
          "Erro Semântico: Retorno invalido. A funcao espera retornar '" +
          converter_tipo_para_texto(tipo_retorno_funcao_atual) + "', mas retornou '" +
          converter_tipo_para_texto(tipo_expressao) + "'.");
    }
    break;
  }

  case NodeType::BREAK:
    if (!esta_dentro_de_laco(no)) {
      throw std::runtime_error(
          "Erro Semântico: O comando 'break' só pode ser usado dentro de um "
          "laço.");
    }
    break;

  case NodeType::CONTINUE:
    if (!esta_dentro_de_laco(no)) {
      throw std::runtime_error(
          "Erro Semântico: O comando 'continue' só pode ser usado dentro de "
          "um laço.");
    }
    break;

  case NodeType::IF:
  case NodeType::WHILE: {
    TipoDado tipo_condicao = obter_tipo_no(no->leftmostChild);
    if (tipo_condicao != TipoDado::INT) {
      throw std::runtime_error("Erro Semântico: A condição do comando de "
                               "controle precisa ser do tipo 'int'.");
    }

    EstadoInicializacao estado_antes = capturar_estado_inicializacao();

    if (no->type == NodeType::IF) {
      ASTNode *bloco_entao = no->leftmostChild ? no->leftmostChild->next : nullptr;
      ASTNode *bloco_senao = bloco_entao ? bloco_entao->next : nullptr;

      if (bloco_entao) {
        analisar_no_isolado(bloco_entao);
      }
      EstadoInicializacao estado_entao = capturar_estado_inicializacao();
      restaurar_estado_inicializacao(estado_antes);

      if (bloco_senao && bloco_senao->type != NodeType::EMPTY) {
        analisar_no_isolado(bloco_senao);
        EstadoInicializacao estado_senao = capturar_estado_inicializacao();
        intersectar_estado_inicializacao(estado_antes, estado_entao,
                                         estado_senao);
      } else {
        restaurar_estado_inicializacao(estado_antes);
      }
    } else {
      ASTNode *corpo = no->leftmostChild ? no->leftmostChild->next : nullptr;
      if (corpo) {
        analisar_no_isolado(corpo);
      }
      restaurar_estado_inicializacao(estado_antes);
    }

    if (no->next)
      analisar_no(no->next);
    return;
  }

  case NodeType::DO_WHILE: {
    ASTNode *corpo = no->leftmostChild;
    ASTNode *condicao = corpo ? corpo->next : nullptr;
    if (corpo)
      analisar_no_isolado(corpo);
    if (obter_tipo_no(condicao) != TipoDado::INT) {
      throw std::runtime_error("Erro Semântico: A condição do comando de "
                               "controle precisa ser do tipo 'int'.");
    }
    if (no->next)
      analisar_no(no->next);
    return;
  }

  case NodeType::FOR: {
    Escopo *escopo_pai = escopo_atual;
    escopo_atual = new Escopo(escopo_pai);

    ASTNode *inicializacao = no->leftmostChild;
    ASTNode *condicao = inicializacao ? inicializacao->next : nullptr;
    ASTNode *atualizacao = condicao ? condicao->next : nullptr;
    ASTNode *corpo = atualizacao ? atualizacao->next : nullptr;

    if (inicializacao) {
      ASTNode *proximo = inicializacao->next;
      inicializacao->next = nullptr;
      analisar_no(inicializacao);
      inicializacao->next = proximo;
    }
    if (obter_tipo_no(condicao) != TipoDado::INT) {
      throw std::runtime_error("Erro Semântico: A condição do comando de "
                               "controle precisa ser do tipo 'int'.");
    }
    EstadoInicializacao estado_apos_inicializacao =
        capturar_estado_inicializacao();
    if (corpo)
      analisar_no_isolado(corpo);
    if (atualizacao) {
      ASTNode *proximo = atualizacao->next;
      atualizacao->next = nullptr;
      analisar_no(atualizacao);
      atualizacao->next = proximo;
    }
    restaurar_estado_inicializacao(estado_apos_inicializacao);

    escopo_atual = escopo_pai;
    if (no->next)
      analisar_no(no->next);
    return;
  }

  case NodeType::BLOCK: {
    Escopo *escopo_pai = escopo_atual;
    escopo_atual = new Escopo(escopo_pai);
    if (no->leftmostChild)
      analisar_no(no->leftmostChild);

    escopo_atual = escopo_pai;
    if (no->next)
      analisar_no(no->next);
    return;
  }

  case NodeType::PRINT: {
    if (obter_tipo_no(no->leftmostChild) == TipoDado::VOID) {
      throw std::runtime_error(
          "Erro Semântico: Não é possível imprimir uma expressão 'void'.");
    }
    break;
  }

  case NodeType::CALL: {
    obter_tipo_no(no);
    break;
  }

  case NodeType::ID: {
    auto simbolo_resolvido = escopo_atual->resolver(no->value);
    if (!simbolo_resolvido) {
      throw std::runtime_error("Erro Semântico: Identificador '" + no->value +
                               "' nao declarado neste escopo.");
    }
    if (!simbolo_resolvido->e_funcao && !simbolo_resolvido->inicializado) {
      throw std::runtime_error("Erro Semântico: Variavel '" + no->value +
                               "' usada antes de ser inicializada.");
    }
    break;
  }

  default:
    break;
  }

  if (no->leftmostChild)
    analisar_no(no->leftmostChild);
  if (no->next)
    analisar_no(no->next);
}

TipoDado AnalisadorSematico::obter_tipo_no(ASTNode *no) {
  if (!no)
    return TipoDado::VOID;

  auto marcar_tipo = [no](TipoDado tipo) {
    switch (tipo) {
    case TipoDado::INT:
      no->inferredType = "int";
      break;
    case TipoDado::FLOAT:
      no->inferredType = "float";
      break;
    case TipoDado::CHAR:
      no->inferredType = "char";
      break;
    case TipoDado::STRING:
      no->inferredType = "string";
      break;
    case TipoDado::VOID:
      no->inferredType = "void";
      break;
    }
    return tipo;
  };

  switch (no->type) {
  case NodeType::NUMBER:
    if (no->value.find('.') != std::string::npos)
      return marcar_tipo(TipoDado::FLOAT);
    return marcar_tipo(TipoDado::INT);

  case NodeType::CHAR:
    return marcar_tipo(TipoDado::CHAR);
  case NodeType::STRING:
    return marcar_tipo(TipoDado::STRING);

  case NodeType::ID: {
    auto simbolo_resolvido = escopo_atual->resolver(no->value);
    if (simbolo_resolvido) {
      if (!simbolo_resolvido->e_funcao && !simbolo_resolvido->inicializado) {
        throw std::runtime_error("Erro Semântico: Variavel '" + no->value +
                                 "' usada antes de ser inicializada.");
      }
      return marcar_tipo(simbolo_resolvido->tipo);
    }
    throw std::runtime_error("Erro Semântico: Variavel '" + no->value +
                             "' nao declarada neste escopo.");
  }

  case NodeType::CALL: {
    auto simbolo_resolvido = escopo_atual->resolver(no->value);
    if (!simbolo_resolvido || !simbolo_resolvido->e_funcao) {
      throw std::runtime_error("Erro Semântico: Funcao '" + no->value +
                               "' nao declarada.");
    }

    std::vector<TipoDado> tipos_argumentos;
    ASTNode *argumento = no->leftmostChild;
    while (argumento) {
      tipos_argumentos.push_back(obter_tipo_no(argumento));
      argumento = argumento->next;
    }

    if (tipos_argumentos.size() != simbolo_resolvido->tipos_parametros.size()) {
      throw std::runtime_error(
          "Erro Semântico: Quantidade incorreta de argumentos para a funcao '" +
          no->value +
          "'. Esperado: " + std::to_string(simbolo_resolvido->tipos_parametros.size()) +
          ", mas recebeu: " + std::to_string(tipos_argumentos.size()));
    }

    for (size_t i = 0; i < tipos_argumentos.size(); ++i) {
      ASTNode *argumento_atual = no->leftmostChild;
      for (size_t j = 0; j < i; ++j)
        argumento_atual = argumento_atual->next;

      if (!tipo_compativel(argumento_atual, tipos_argumentos[i],
                           simbolo_resolvido->tipos_parametros[i])) {
        throw std::runtime_error(
            "Erro Semântico: Tipo de argumento incompativel na funcao '" +
            no->value + "' na posicao " + std::to_string(i + 1) +
            ". Esperado: " + converter_tipo_para_texto(simbolo_resolvido->tipos_parametros[i]) +
            ", obteve: " + converter_tipo_para_texto(tipos_argumentos[i]));
      }
    }
    return marcar_tipo(simbolo_resolvido->tipo);
  }

  case NodeType::BIN_OP: {
    TipoDado tipo_esquerda = obter_tipo_no(no->leftmostChild);
    TipoDado tipo_direita = obter_tipo_no(no->leftmostChild->next);

    if (no->value == "&&" || no->value == "||") {
      if (tipo_esquerda != TipoDado::INT || tipo_direita != TipoDado::INT) {
        throw std::runtime_error("Erro Semântico: Operador lógico '" +
                                 no->value +
                                 "' exige operandos do tipo 'int'.");
      }
      return marcar_tipo(TipoDado::INT);
    }

    if (no->value == "==" || no->value == "!=") {
      if (tipo_esquerda == TipoDado::STRING || tipo_direita == TipoDado::STRING) {
        throw std::runtime_error(
            "Erro Semântico: Comparação de strings ainda não é suportada.");
      }
      if (tipo_esquerda != tipo_direita && e_numerico(tipo_esquerda) &&
          e_numerico(tipo_direita)) {
        if (tipo_esquerda == TipoDado::INT)
          marcar_conversao(no->leftmostChild, tipo_esquerda, TipoDado::FLOAT);
        if (tipo_direita == TipoDado::INT)
          marcar_conversao(no->leftmostChild->next, tipo_direita, TipoDado::FLOAT);
      } else if (tipo_esquerda != tipo_direita) {
        throw std::runtime_error("Erro Semântico: Comparação '" + no->value +
                                 "' invalida entre tipos diferentes.");
      }
      return marcar_tipo(TipoDado::INT);
    }

    if (no->value == "<" || no->value == ">" || no->value == "<=" ||
        no->value == ">=") {
      bool esquerda_numerica =
          tipo_esquerda == TipoDado::INT || tipo_esquerda == TipoDado::FLOAT;
      bool direita_numerica =
          tipo_direita == TipoDado::INT || tipo_direita == TipoDado::FLOAT;
      if (!esquerda_numerica || !direita_numerica) {
        throw std::runtime_error(
            "Erro Semântico: Comparação relacional '" + no->value +
            "' exige operandos numéricos.");
      }
      if (tipo_esquerda == TipoDado::INT && tipo_direita == TipoDado::FLOAT)
        marcar_conversao(no->leftmostChild, tipo_esquerda, TipoDado::FLOAT);
      if (tipo_esquerda == TipoDado::FLOAT && tipo_direita == TipoDado::INT)
        marcar_conversao(no->leftmostChild->next, tipo_direita, TipoDado::FLOAT);
      return marcar_tipo(TipoDado::INT);
    }

    if (no->value == "%") {
      if (tipo_esquerda != TipoDado::INT || tipo_direita != TipoDado::INT) {
        throw std::runtime_error(
            "Erro Semântico: Operador '%' exige operandos do tipo 'int'.");
      }
      return marcar_tipo(TipoDado::INT);
    }

    bool esquerda_numerica =
        tipo_esquerda == TipoDado::INT || tipo_esquerda == TipoDado::FLOAT;
    bool direita_numerica =
        tipo_direita == TipoDado::INT || tipo_direita == TipoDado::FLOAT;
    if (!esquerda_numerica || !direita_numerica) {
      throw std::runtime_error("Erro Semântico: Operação '" + no->value +
                               "' exige operandos numéricos.");
    }

    TipoDado tipo_resultado =
        tipo_esquerda == TipoDado::FLOAT || tipo_direita == TipoDado::FLOAT
            ? TipoDado::FLOAT
            : TipoDado::INT;
    marcar_conversao(no->leftmostChild, tipo_esquerda, tipo_resultado);
    marcar_conversao(no->leftmostChild->next, tipo_direita, tipo_resultado);
    return marcar_tipo(tipo_resultado);
  }

  case NodeType::UN_OP: {
    TipoDado tipo_operando = obter_tipo_no(no->leftmostChild);
    if (no->value == "-" && tipo_operando != TipoDado::INT && tipo_operando != TipoDado::FLOAT) {
      throw std::runtime_error(
          "Erro Semântico: Operador unário '-' exige tipo numérico.");
    }
    if (no->value == "!" && tipo_operando != TipoDado::INT) {
      throw std::runtime_error(
          "Erro Semântico: Operador unário '!' exige tipo 'int'.");
    }
    return marcar_tipo(tipo_operando);
  }

  default:
    return marcar_tipo(TipoDado::VOID);
  }
}

TipoDado
AnalisadorSematico::converter_texto_para_tipo(const std::string &texto_tipo) {
  if (texto_tipo == "int")
    return TipoDado::INT;
  if (texto_tipo == "float")
    return TipoDado::FLOAT;
  if (texto_tipo == "char")
    return TipoDado::CHAR;
  if (texto_tipo == "string")
    return TipoDado::STRING;
  if (texto_tipo == "void")
    return TipoDado::VOID;
  throw std::runtime_error("Tipo desconhecido: " + texto_tipo);
}

std::string AnalisadorSematico::converter_tipo_para_texto(TipoDado tipo) {
  switch (tipo) {
  case TipoDado::INT:
    return "int";
  case TipoDado::FLOAT:
    return "float";
  case TipoDado::CHAR:
    return "char";
  case TipoDado::STRING:
    return "string";
  case TipoDado::VOID:
    return "void";
  }
  return "desconhecido";
}
