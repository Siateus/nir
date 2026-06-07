#ifndef PARSER_HPP
#define PARSER_HPP
#include <iostream>
#include <vector>
#include "lexer.hpp"

class Parser {
private:
    std::vector<Token> tokens;
    size_t atual;
    Token peek();
    void match(const std::string& tipo);
    void advance();
    bool is_eof();

public:
};



#endif
