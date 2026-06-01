#include "../include/lexer.hpp"
#include <iostream>
#include <unordered_map>
#include <vector>
#include <string>
#include <regex>
#include <fstream>

using namespace std;

unordered_map<string, string> TOKENS = {
    {"return", "RETURN"},
    {"fun", "FUN"},
    {"if", "IF"},
    {"else", "ELSE"},
    {"while", "WHILE"},
    {"do", "DO"},
    {"for", "FOR"},
    {"print", "PRINT"},
    {"var", "VAR"},

    {"i32", "TYPE_I32"},
    {"i64", "TYPE_I64"},
    {"f32", "TYPE_F32"},
    {"f64", "TYPE_F64"},
    {"char", "TYPE_CHAR"},
    {"void", "TYPE_VOID"},

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
    {"=", "ASSIGN"}
};

vector<Token> lexer(fstream &arquivo) {
    vector<Token> tokens;

    string src;
    string linha;
    while(getline(arquivo,linha)){
        src += linha+"\n";
    }
    const regex re(R"([A-Za-z_][A-Za-z0-9_]*|-?\d+\.\d+|-?\d+|==|!=|<=|>=|\|\||&&|".*?"|'.*?'|[{}():;,=+\-*/%])");
    const auto begin = sregex_iterator(src.begin(),src.end(), re);
    const auto end = sregex_iterator();

    for(auto i = begin; i != end; ++i) {
        string lin = i->str();
        if(TOKENS.find(lin) != TOKENS.end()) {
            tokens.push_back({TOKENS[lin],lin});
        }
        else if(regex_match(lin,regex(R"(^-?\d+(?:[\.]\d+)?$)"))) {
            tokens.push_back({"NUMBER",lin});
        }else if (regex_match(lin,regex("'[A-Za-z0-9]'"))) {
            tokens.push_back({"CHAR",lin});
        }else if (regex_match(lin, regex(R"("[^"]*")"))) {
            tokens.push_back({"STRING",lin});
        }else {
            tokens.push_back({"NAME",lin});
        }
    }

    return tokens;
}
