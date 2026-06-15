#include "include/ast.hpp"
#include "include/lexer.hpp"
#include "include/parser.hpp"
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

using namespace std;

int main(int argc, char *argv[]) {

  if (argc < 2) {
    cerr << "Passe o arquivo do programa" << endl;
    return 1;
  }

  fstream arquivo(argv[1]);

  if (!arquivo.is_open()) {
    cerr << "Erro ao abrir: " << argv[1] << endl;
    return 1;
  }

  cout << "Nome do Arquivo " << argv[1] << endl;
  const vector<Token> tokens = Lexer::lexer(arquivo);
  arquivo.close();

  /*for (auto &t : tokens) {
    cout << "{" << t.type << ", '" << t.value << "' }" << endl;
  }*/
  Parser parser(tokens);
  try {
    ASTNode *raiz_AST = parser.parser();

    if (raiz_AST != nullptr) {
      raiz_AST->print();
    }
  } catch (std::runtime_error &e) {
    std::cerr << "Erro durante compilação: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
