#ifndef ANALISADOR_SEMANTICO_HPP
#define ANALISADOR_SEMANTICO_HPP

#include "ast.hpp"
#include "tabela_simbolos.hpp"
#include <string>
#include <unordered_map>

class AnalisadorSematico {
private:
  using EstadoInicializacao = std::unordered_map<const Simbolo *, bool>;

  Escopo *escopo_global;
  Escopo *escopo_atual;

  void registrar_simbolos_globais(ASTNode *raiz);
  void analisar_no(ASTNode *no);
  void analisar_no_isolado(ASTNode *no);
  bool garante_retorno(ASTNode *no);
  EstadoInicializacao capturar_estado_inicializacao() const;
  void restaurar_estado_inicializacao(const EstadoInicializacao &estado);
  void intersectar_estado_inicializacao(const EstadoInicializacao &base,
                                        const EstadoInicializacao &primeiro,
                                        const EstadoInicializacao &segundo);
  TipoDado obter_tipo_no(ASTNode *no);
  TipoDado converter_texto_para_tipo(const std::string &texto_tipo);
  std::string converter_tipo_para_texto(TipoDado tipo);
  TipoDado tipo_retorno_funcao_atual;

public:
  AnalisadorSematico();
  ~AnalisadorSematico();

  void analisar(ASTNode *raiz);
};
#endif
