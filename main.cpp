#include "include/ast.hpp"
#include "include/codegen.hpp"
#include "include/lexer.hpp"
#include "include/parser.hpp"
#include "include/semantic.hpp"
#include <climits>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std;

static string diretorio(const string &caminho) {
  size_t separador = caminho.find_last_of("/\\");
  return separador == string::npos ? "." : caminho.substr(0, separador);
}

static bool arquivo_existe(const string &caminho) {
  ifstream arquivo(caminho);
  return arquivo.good();
}

static string caminho_canonico(const string &caminho) {
  char resolvido[PATH_MAX];
  if (realpath(caminho.c_str(), resolvido))
    return resolvido;
  return caminho;
}

static string resolver_import(const string &arquivoAtual,
                              const string &caminhoImportado) {
  string base = diretorio(arquivoAtual);
  string candidato = base + "/" + caminhoImportado;
  if (arquivo_existe(candidato))
    return caminho_canonico(candidato);

  throw runtime_error("Arquivo importado '" + caminhoImportado +
                      "' nao encontrado a partir de '" + arquivoAtual + "'.");
}

static ASTNode *analisar_arquivo(const string &caminho) {
  fstream arquivo(caminho);
  if (!arquivo.is_open())
    throw runtime_error("Erro ao abrir: " + caminho);

  const vector<Token> tokens = Lexer::lexer(arquivo);
  Parser parser(tokens);
  return parser.parser();
}

static void anexar_lista(ASTNode *&inicio, ASTNode *&fim, ASTNode *lista) {
  while (lista) {
    ASTNode *proximo = lista->next;
    lista->next = nullptr;
    if (!inicio)
      inicio = lista;
    else
      fim->next = lista;
    fim = lista;
    lista = proximo;
  }
}

static ASTNode *carregar_programa(const string &caminho,
                                  set<string> &carregados,
                                  set<string> &carregando) {
  string atual = caminho_canonico(caminho);
  if (carregando.count(atual))
    throw runtime_error("Ciclo de imports detectado envolvendo '" + atual +
                        "'.");
  if (carregados.count(atual))
    return nullptr;

  carregando.insert(atual);
  ASTNode *programa = analisar_arquivo(atual);
  ASTNode *inicio = nullptr;
  ASTNode *fim = nullptr;

  ASTNode *nivel = programa ? programa->leftmostChild : nullptr;
  while (nivel) {
    ASTNode *proximo = nivel->next;
    nivel->next = nullptr;

    if (nivel->type == NodeType::IMPORT) {
      string importado = resolver_import(atual, nivel->value);
      ASTNode *programaImportado =
          carregar_programa(importado, carregados, carregando);
      if (programaImportado)
        anexar_lista(inicio, fim, programaImportado->leftmostChild);
    } else {
      anexar_lista(inicio, fim, nivel);
    }
    nivel = proximo;
  }

  carregando.erase(atual);
  carregados.insert(atual);
  if (programa)
    programa->leftmostChild = inicio;
  return programa;
}

int main(int argc, char *argv[]) {

  if (argc < 2) {
    cerr << "Passe o arquivo do programa" << endl;
    return 1;
  }

  try {
    set<string> carregados;
    set<string> carregando;
    ASTNode *raiz_AST = carregar_programa(argv[1], carregados, carregando);

    AnalisadorSematico semantico;
    semantico.Analisar_sematica(raiz_AST);

    GeradorSAM codegen;
    codegen.gerar(raiz_AST);
    codegen.salvar("saida.sam");
  } catch (std::runtime_error &e) {
    std::cerr << "Erro de compilação\n" << e.what() << std::endl;
    return 1;
  }
  return 0;
}
