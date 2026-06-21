#include "../include/lexer.hpp"
#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

const unordered_map<string, string> Lexer::TOKENS = {{"return", "RETURN"},
                                                     {"fun", "FUN"},
                                                     {"import", "IMPORT"},
                                                     {"if", "IF"},
                                                     {"else", "ELSE"},
                                                     {"while", "WHILE"},
                                                     {"do", "DO"},
                                                     {"for", "FOR"},
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
std::string Lexer::ler_limpar_arquivo(std::fstream &arquivo) {
  std::string src((std::istreambuf_iterator<char>(arquivo)),
                  std::istreambuf_iterator<char>());

  std::string result = src;
  bool in_string = false;
  char string_char = 0;
  bool in_char = false;

  size_t i = 0;
  while (i < result.length()) {
    if (in_string) {
      if (result[i] == '\\' && i + 1 < result.length()) {
        i += 2;
        continue;
      }
      if (result[i] == string_char) {
        in_string = false;
      }
      i++;
      continue;
    }
    if (in_char) {
      if (result[i] == '\\' && i + 1 < result.length()) {
        i += 2;
        continue;
      }
      if (result[i] == '\'') {
        in_char = false;
      }
      i++;
      continue;
    }

    if (result[i] == '"') {
      in_string = true;
      string_char = '"';
      i++;
      continue;
    }
    if (result[i] == '\'') {
      in_char = true;
      i++;
      continue;
    }

    if ((result[i] == '/' && i + 1 < result.length() && result[i + 1] == '/') ||
        (result[i] == '-' && i + 1 < result.length() && result[i + 1] == '-') ||
        (result[i] == '#')) {
      while (i < result.length() && result[i] != '\n') {
        result[i] = ' ';
        i++;
      }
      continue;
    }

    if (result[i] == '/' && i + 1 < result.length() && result[i + 1] == '*') {
      result[i] = ' ';
      result[i + 1] = ' ';
      i += 2;
      while (i < result.length()) {
        if (result[i] == '*' && i + 1 < result.length() &&
            result[i + 1] == '/') {
          result[i] = ' ';
          result[i + 1] = ' ';
          i += 2;
          break;
        }
        if (result[i] != '\n') {
          result[i] = ' ';
        }
        i++;
      }
      continue;
    }

    i++;
  }
  return result;
}

vector<Token> Lexer::lexer(fstream &arquivo) {
  vector<Token> tokens;

  string src = ler_limpar_arquivo(arquivo);

  const regex re(
      R"("(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)'|==|!=|<=|>=|\|\||&&|[A-Za-z_][A-Za-z0-9_]*|\d+\.\d+|\d+|[{}():;,=+\-*/%<>!])");
  const auto begin = sregex_iterator(src.begin(), src.end(), re);
  const auto end = sregex_iterator();

  for (auto i = begin; i != end; ++i) {
    string lin = i->str();
    size_t pos = i->position();
    int line = 1;
    int col = 1;
    for (size_t p = 0; p < pos; ++p) {
      if (src[p] == '\n') {
        line++;
        col = 1;
      } else {
        col++;
      }
    }

    if (TOKENS.find(lin) != TOKENS.end()) {
      tokens.push_back({TOKENS.at(lin), lin, line, col});
    } else if (regex_match(lin, regex(R"(^\d+(?:[\.]\d+)?$)"))) {
      tokens.push_back({"NUMBER", lin, line, col});
    } else if (lin.front() == '\'' && lin.back() == '\'') {
      tokens.push_back({"CHAR", lin, line, col});
    } else if (lin.front() == '"' && lin.back() == '"') {
      tokens.push_back({"STRING", lin, line, col});
    } else {
      tokens.push_back({"ID", lin, line, col});
    }
  }

  int last_line = 1;
  int last_col = 1;
  for (char c : src) {
    if (c == '\n') {
      last_line++;
      last_col = 1;
    } else {
      last_col++;
    }
  }
  tokens.push_back({"EOF", "EOF", last_line, last_col});
  return tokens;
}
