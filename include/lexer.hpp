#ifndef LEXER_HPP
#define LEXER_HPP

#include <vector>
#include <fstream>
#include <string>

struct Token {
    std::string type;
    std::string value;
};

std::vector<Token> lexer(std::fstream &arquivo);

#endif // !LEXER_HPP
