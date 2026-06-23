#include "../include/analisador_lexico.hpp"
#include <cctype>
#include <fstream>
#include <iostream>
#include <regex>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

const unordered_map<string, string> AnalisadorLexico::TIPOS_TOKEN = {{"return", "RETURN"},
                                                     {"fun", "FUN"},
                                                     {"if", "IF"},
                                                     {"else", "ELSE"},
                                                     {"while", "WHILE"},
                                                     {"do", "DO"},
                                                     {"for", "FOR"},
                                                     {"break", "BREAK"},
                                                     {"continue", "CONTINUE"},
                                                     {"print", "PRINT"},
                                                     {"var", "VAR"},

                                                     {"int", "TYPE_INT"},
                                                     {"float", "TYPE_FLOAT"},
                                                     {"char", "TYPE_CHAR"},
                                                     {"void", "TYPE_VOID"},
                                                     {"string", "TYPE_STRING"},

                                                     {"+", "PLUS"},
                                                     {"-", "MINUS"},
                                                     {"*", "MULT"},
                                                     {"/", "DIV"},
                                                     {"%", "MOD"},

                                                     {"==", "EQ"},
                                                     {"!=", "NEQ"},
                                                     {"<", "LT"},
                                                     {">", "GT"},
                                                     {"<=", "LE"},
                                                     {">=", "GE"},

                                                     {"||", "OR"},
                                                     {"&&", "AND"},
                                                     {"!", "NOT"},

                                                     {"{", "LBRACE"},
                                                     {"}", "RBRACE"},
                                                     {"(", "LPAREN"},
                                                     {")", "RPAREN"},
                                                     {",", "COMMA"},
                                                     {":", "COLON"},
                                                     {";", "SEMICOLON"},
                                                     {"=", "ASSIGN"}};
std::string AnalisadorLexico::ler_e_limpar_arquivo(std::fstream &arquivo) {
  std::string codigo_fonte((std::istreambuf_iterator<char>(arquivo)),
                  std::istreambuf_iterator<char>());

  std::string resultado = codigo_fonte;
  bool dentro_string = false;
  char delimitador_string = 0;
  bool dentro_caractere = false;

  size_t i = 0;
  while (i < resultado.length()) {
    if (dentro_string) {
      if (resultado[i] == '\\' && i + 1 < resultado.length()) {
        i += 2;
        continue;
      }
      if (resultado[i] == delimitador_string) {
        dentro_string = false;
      }
      i++;
      continue;
    }
    if (dentro_caractere) {
      if (resultado[i] == '\\' && i + 1 < resultado.length()) {
        i += 2;
        continue;
      }
      if (resultado[i] == '\'') {
        dentro_caractere = false;
      }
      i++;
      continue;
    }

    if (resultado[i] == '"') {
      dentro_string = true;
      delimitador_string = '"';
      i++;
      continue;
    }
    if (resultado[i] == '\'') {
      dentro_caractere = true;
      i++;
      continue;
    }

    if ((resultado[i] == '/' && i + 1 < resultado.length() && resultado[i + 1] == '/') ||
        (resultado[i] == '-' && i + 1 < resultado.length() && resultado[i + 1] == '-') ||
        (resultado[i] == '#')) {
      while (i < resultado.length() && resultado[i] != '\n') {
        resultado[i] = ' ';
        i++;
      }
      continue;
    }

    if (resultado[i] == '/' && i + 1 < resultado.length() && resultado[i + 1] == '*') {
      bool fechado = false;
      resultado[i] = ' ';
      resultado[i + 1] = ' ';
      i += 2;
      while (i < resultado.length()) {
        if (resultado[i] == '*' && i + 1 < resultado.length() &&
            resultado[i + 1] == '/') {
          resultado[i] = ' ';
          resultado[i + 1] = ' ';
          i += 2;
          fechado = true;
          break;
        }
        if (resultado[i] != '\n') {
          resultado[i] = ' ';
        }
        i++;
      }
      if (!fechado)
        throw std::runtime_error("Erro léxico: comentário de bloco não fechado.");
      continue;
    }

    i++;
  }
  if (dentro_string)
    throw std::runtime_error("Erro léxico: string não fechada.");
  if (dentro_caractere)
    throw std::runtime_error("Erro léxico: caractere não fechado.");
  return resultado;
}

vector<Token> AnalisadorLexico::analisar(fstream &arquivo) {
  vector<Token> tokens;

  string codigo_fonte = ler_e_limpar_arquivo(arquivo);

  const regex re(
      R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)'|==|!=|<=|>=|\|\||&&|[A-Za-z_][A-Za-z0-9_]*|\d+\.\d+|\d+|[{}():;,=+\-*/%<>!])");
  const auto inicio =
      sregex_iterator(codigo_fonte.begin(), codigo_fonte.end(), re);
  const auto fim = sregex_iterator();
  size_t fim_anterior = 0;

  for (auto i = inicio; i != fim; ++i) {
    string lexema = i->str();
    size_t posicao = i->position();
    for (size_t p = fim_anterior; p < posicao; ++p) {
      if (!std::isspace(static_cast<unsigned char>(codigo_fonte[p]))) {
        int linha = 1;
        int coluna = 1;
        for (size_t j = 0; j < p; ++j) {
          if (codigo_fonte[j] == '\n') {
            linha++;
            coluna = 1;
          } else {
            coluna++;
          }
        }
        throw std::runtime_error(
            "Erro léxico na linha " + std::to_string(linha) + ", coluna " +
            std::to_string(coluna) + ": caractere inválido '" + codigo_fonte[p] + "'.");
      }
    }
    fim_anterior = posicao + lexema.size();
    int linha = 1;
    int coluna = 1;
    for (size_t p = 0; p < posicao; ++p) {
      if (codigo_fonte[p] == '\n') {
        linha++;
        coluna = 1;
      } else {
        coluna++;
      }
    }

    if (TIPOS_TOKEN.find(lexema) != TIPOS_TOKEN.end()) {
      tokens.push_back({TIPOS_TOKEN.at(lexema), lexema, linha, coluna});
    } else if (regex_match(lexema, regex(R"(^\d+(?:[\.]\d+)?$)"))) {
      tokens.push_back({"NUMBER", lexema, linha, coluna});
    } else if (lexema.front() == '\'' && lexema.back() == '\'') {
      tokens.push_back({"CHAR", lexema, linha, coluna});
    } else if (lexema.front() == '"' && lexema.back() == '"') {
      tokens.push_back({"STRING", lexema, linha, coluna});
    } else {
      tokens.push_back({"ID", lexema, linha, coluna});
    }
  }

  for (size_t p = fim_anterior; p < codigo_fonte.size(); ++p) {
    if (!std::isspace(static_cast<unsigned char>(codigo_fonte[p]))) {
      throw std::runtime_error("Erro léxico: caractere inválido no fim do arquivo.");
    }
  }

  int ultima_linha = 1;
  int ultima_coluna = 1;
  for (char c : codigo_fonte) {
    if (c == '\n') {
      ultima_linha++;
      ultima_coluna = 1;
    } else {
      ultima_coluna++;
    }
  }
  tokens.push_back({"EOF", "EOF", ultima_linha, ultima_coluna});
  return tokens;
}

void AnalisadorLexico::print_tokens(const vector<Token> &tokens) {
  for (const Token &token : tokens) {
    cout << token.tipo << " (" << token.valor << ")"
         << " linha " << token.linha << ", coluna " << token.coluna << endl;
  }
}
