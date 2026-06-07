#ifndef LEXER_HPP
#define LEXER_HPP

#include <vector>
#include <fstream>
#include <string>
#include <unordered_map>

struct Token {
    std::string type;
    std::string value;
};
class Lexer {
private:
    static const std::unordered_map<std::string, std::string> TOKENS;

    static std::string ler_limpar_arquivo(std::fstream &arquivo);
public:
    Lexer() = default;

    static std::vector<Token> lexer(std::fstream &arquivo);
};
#endif // !LEXER_HPP
