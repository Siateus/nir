#ifndef TABELA_SIMBOLOS_HPP
#define TABELA_SIMBOLOS_HPP

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

enum class TipoDado { INT, FLOAT, CHAR, STRING, VOID };

struct Simbolo {
  std::string nome;
  TipoDado tipo;
  bool e_funcao;
  bool inicializado;
  std::vector<TipoDado> tipos_parametros;
};

class Escopo {
private:
  Escopo *pai;
  std::vector<Escopo *> filhos;
  std::unordered_map<std::string, std::shared_ptr<Simbolo>> simbolos;

public:
  explicit Escopo(Escopo *escopo_pai) : pai(escopo_pai) {
    if (pai)
      pai->filhos.push_back(this);
  }
  ~Escopo() {
    for (Escopo *filho : filhos)
      delete filho;
  }

  bool definir(std::shared_ptr<Simbolo> simbolo) {
    if (simbolos.find(simbolo->nome) != simbolos.end()) {
      return false;
    }
    simbolos[simbolo->nome] = simbolo;
    return true;
  }

  std::shared_ptr<Simbolo> resolver(const std::string &nome) {
    auto encontrado = simbolos.find(nome);
    if (encontrado != simbolos.end()) {
      return encontrado->second;
    }
    if (pai != nullptr) {
      return pai->resolver(nome);
    }
    return nullptr;
  }

  void coletar_estado_inicializacao(
      std::unordered_map<const Simbolo *, bool> &estado) const {
    for (const auto &entrada : simbolos)
      estado[entrada.second.get()] = entrada.second->inicializado;
    for (const Escopo *filho : filhos)
      filho->coletar_estado_inicializacao(estado);
  }

  void aplicar_estado_inicializacao(
      const std::unordered_map<const Simbolo *, bool> &estado) {
    for (const auto &entrada : simbolos) {
      auto encontrado = estado.find(entrada.second.get());
      if (encontrado != estado.end())
        entrada.second->inicializado = encontrado->second;
    }
    for (Escopo *filho : filhos)
      filho->aplicar_estado_inicializacao(estado);
  }

  Escopo *obter_pai() const { return pai; }
};

#endif
