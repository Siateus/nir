#include "../include/lexer.hpp"
#include <fstream>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std;

const unordered_map<string, string> Lexer::TOKENS = {
    {"return", "RETURN"}, {"fun", "FUN"},        {"if", "IF"},
    {"else", "ELSE"},     {"while", "WHILE"},    {"do", "DO"},
    {"for", "FOR"},       {"print", "PRINT"},    {"var", "VAR"},

    {"i32", "TYPE_I32"},  {"i64", "TYPE_I64"},   {"f32", "TYPE_F32"},
    {"f64", "TYPE_F64"},  {"char", "TYPE_CHAR"}, {"void", "TYPE_VOID"},

    {"+", "PLUS"},        {"-", "MINUS"},        {"*", "MULT"},
    {"/", "DIV"},         {"%", "MOD"},

    {"==", "EQ"},         {"!=", "NEQ"},         {"<", "LT"},
    {">", "GT"},          {"<=", "LE"},          {">=", "GE"},

    {"||", "OR"},         {"&&", "AND"},         {"!", "NOT"},

    {"{", "LBRACE"},      {"}", "RBRACE"},       {"(", "LPAREN"},
    {")", "RPAREN"},      {",", "COMMA"},        {":", "COLON"},
    {";", "SEMICOLON"},   {"=", "ASSIGN"}};
std::string Lexer::ler_limpar_arquivo(std::fstream &arquivo) {
  string src;
  string linha;
  bool em_comentario_bloco = false;
  while (getline(arquivo, linha)) {
    if (em_comentario_bloco) {
      size_t fim = linha.find("*/");
      if (fim != string::npos) {
        linha = linha.substr(fim + 2);
        em_comentario_bloco = false;
      } else {
        continue;
      }
    }

    size_t inicio_bloco = linha.find("/*");
    if (inicio_bloco != string::npos) {
      size_t fim_bloco = linha.find("*/", inicio_bloco + 2);
      if (fim_bloco != string::npos) {
        linha = linha.substr(0, inicio_bloco) + linha.substr(fim_bloco + 2);
      } else {
        linha = linha.substr(0, inicio_bloco);
        em_comentario_bloco = true;
      }
    }

    size_t pos_barra = linha.find("//");
    size_t pos_traco = linha.find("--");
    size_t pos_tralha = linha.find('#');

    size_t pos_comentario = string::npos;
    if (pos_barra != string::npos)
      pos_comentario = pos_barra;
    if (pos_traco != string::npos &&
        (pos_comentario == string::npos || pos_traco < pos_comentario))
      pos_comentario = pos_traco;
    if (pos_tralha != string::npos &&
        (pos_comentario == string::npos || pos_tralha < pos_comentario))
      pos_comentario = pos_tralha;

    if (pos_comentario != string::npos) {
      linha = linha.substr(0, pos_comentario);
    }

    src += linha + "\n";
  }
  return src;
}

vector<Token> Lexer::lexer(fstream &arquivo) {
  vector<Token> tokens;

  string src = ler_limpar_arquivo(arquivo);

  const regex re(
      R"([A-Za-z_][A-Za-z0-9_]*|-?\d+\.\d+|-?\d+|==|!=|<=|>=|\|\||&&|".*?"|'.*?'|[{}():;,=+\-*/%])");
  const auto begin = sregex_iterator(src.begin(), src.end(), re);
  const auto end = sregex_iterator();

  for (auto i = begin; i != end; ++i) {
    string lin = i->str();
    if (TOKENS.find(lin) != TOKENS.end()) {
      tokens.push_back({TOKENS.at(lin), lin});
    } else if (regex_match(lin, regex(R"(^-?\d+(?:[\.]\d+)?$)"))) {
      tokens.push_back({"NUMBER", lin});
    } else if (regex_match(lin, regex("'[A-Za-z0-9]'"))) {
      tokens.push_back({"CHAR", lin});
    } else if (regex_match(lin, regex(R"("[^"]*")"))) {
      tokens.push_back({"STRING", lin});
    } else {
      tokens.push_back({"ID", lin});
    }
  }
  tokens.push_back({"EOF", "EOF"});
  return tokens;
}
