#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include "include/lexer.hpp"

using namespace std;

int main (int argc, char *argv[]) {

    if (argc < 2) {
        cerr << "Passe o arquivo do programa" << endl;
        return 1;
    }

    fstream arquivo(argv[1]);

    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir: "<< argv[1] << endl;
        return 1;
    }

    cout << "Nome do Arquivo " << argv[1] << endl;

    vector<Token> tokens = lexer(arquivo);

    for(auto &t: tokens) {
        cout << "(" << t.type << ", " << t.value << " )" << endl;
    }
    return 0;
}
