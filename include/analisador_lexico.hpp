#ifndef ANALISADOR_LEXICO_HPP
#define ANALISADOR_LEXICO_HPP

#include <vector>
#include <fstream>
#include <string>
#include <unordered_map>

struct Token {
  std::string tipo;
  std::string valor;
  int linha;
  int coluna;
};

class AnalisadorLexico {
private:
  static const std::unordered_map<std::string, std::string> TIPOS_TOKEN;

  static std::string ler_e_limpar_arquivo(std::fstream &arquivo);

public:
  AnalisadorLexico() = default;

  static std::vector<Token> analisar(std::fstream &arquivo);
  static void print_tokens(const std::vector<Token> &tokens);
};

#endif
