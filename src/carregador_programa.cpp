#include "carregador_programa.hpp"
#include "ast.hpp"
#include "analisador_lexico.hpp"
#include "analisador_sintatico.hpp"
#include <climits>
#include <cstdlib>
#include <fstream>
#include <memory>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void validar_extensao_nir(const std::string &caminho) {
  constexpr char extensao[] = ".nir";
  if (caminho.size() < sizeof(extensao) - 1 ||
      caminho.compare(caminho.size() - (sizeof(extensao) - 1),
                      sizeof(extensao) - 1, extensao) != 0) {
    throw std::runtime_error("Formato de arquivo invalido: '" + caminho +
                             "'. Apenas arquivos .nir sao aceitos.");
  }
}

std::string diretorio(const std::string &caminho) {
  const std::size_t separador = caminho.find_last_of("/\\");
  return separador == std::string::npos ? "." : caminho.substr(0, separador);
}

bool arquivo_existe(const std::string &caminho) {
  std::ifstream arquivo(caminho);
  return arquivo.good();
}

std::string caminho_canonico(const std::string &caminho) {
  char resolvido[PATH_MAX];
  if (realpath(caminho.c_str(), resolvido))
    return resolvido;
  return caminho;
}

std::string resolver_import(const std::string &arquivo_atual,
                            const std::string &caminho_importado) {
  validar_extensao_nir(caminho_importado);

  const std::string base = diretorio(arquivo_atual);
  const std::string candidato =
      !caminho_importado.empty() && caminho_importado[0] == '/'
          ? caminho_importado
          : base + "/" + caminho_importado;
  if (arquivo_existe(candidato))
    return caminho_canonico(candidato);

  throw std::runtime_error("Arquivo importado '" + caminho_importado +
                           "' nao encontrado a partir de '" + arquivo_atual +
                           "'.");
}

ASTNode *analisar_arquivo(const std::string &caminho) {
  validar_extensao_nir(caminho);

  std::fstream arquivo(caminho);
  if (!arquivo.is_open())
    throw std::runtime_error("Erro ao abrir: " + caminho);

  const std::vector<Token> tokens = AnalisadorLexico::analisar(arquivo);
  AnalisadorSintatico analisador_sintatico(tokens);
  return analisador_sintatico.analisar();
}

ASTNode *carregar_programa_recursivo(const std::string &caminho,
                                     std::set<std::string> &carregados,
                                     std::set<std::string> &carregando) {
  const std::string atual = caminho_canonico(caminho);
  if (carregando.count(atual))
    throw std::runtime_error("Ciclo de imports detectado envolvendo '" + atual +
                             "'.");
  if (carregados.count(atual))
    return nullptr;

  carregando.insert(atual);
  std::unique_ptr<ASTNode> programa(analisar_arquivo(atual));
  std::unique_ptr<ASTNode> resultado;
  ASTNode *fim = nullptr;

  std::unique_ptr<ASTNode> restante(programa ? programa->leftmostChild
                                             : nullptr);
  if (programa)
    programa->leftmostChild = nullptr;

  auto anexar = [&resultado, &fim](ASTNode *lista) {
    if (!lista)
      return;
    if (!resultado)
      resultado.reset(lista);
    else
      fim->next = lista;
    while (lista->next)
      lista = lista->next;
    fim = lista;
  };

  while (restante) {
    std::unique_ptr<ASTNode> nivel(restante.release());
    restante.reset(nivel->next);
    nivel->next = nullptr;

    if (nivel->type == NodeType::IMPORT) {
      const std::string importado = resolver_import(atual, nivel->value);
      std::unique_ptr<ASTNode> programa_importado(
          carregar_programa_recursivo(importado, carregados, carregando));
      if (programa_importado) {
        anexar(programa_importado->leftmostChild);
        programa_importado->leftmostChild = nullptr;
      }
    } else {
      anexar(nivel.release());
    }
  }

  carregando.erase(atual);
  carregados.insert(atual);
  if (programa)
    programa->leftmostChild = resultado.release();

  ASTNode *inicio = programa ? programa->leftmostChild : nullptr;
  for (ASTNode *nivel = inicio; nivel; nivel = nivel->next) {
    nivel->parent = programa.get();
    nivel->leftmostSib = inicio;
  }
  return programa.release();
}

} // namespace

std::unique_ptr<ASTNode> carregar_programa(const std::string &caminho) {
  validar_extensao_nir(caminho);

  std::set<std::string> carregados;
  std::set<std::string> carregando;
  return std::unique_ptr<ASTNode>(
      carregar_programa_recursivo(caminho, carregados, carregando));
}
