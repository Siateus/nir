# Arquitetura do compilador

## Fluxo de compilação

```text
arquivo .nir
    |
    v
carregador
    |
    v
análise léxica -> tokens
    |
    v
análise sintática -> AST
    |
    v
análise semântica -> AST anotada
    |
    v
geração de código -> saida.sam
```

O fluxo é coordenado por `main.cpp`. Em caso de erro, uma exceção
`std::runtime_error` interrompe as etapas seguintes e o compilador retorna
código diferente de zero.

## Carregamento

Arquivos:

- `include/carregador_programa.hpp`
- `src/carregador_programa.cpp`

Responsabilidades:

- validar a extensão `.nir`;
- abrir o arquivo fonte;
- analisar o arquivo informado.

## Análise léxica

Arquivos:

- `include/analisador_lexico.hpp`
- `src/analisador_lexico.cpp`

O analisador lê o arquivo, substitui comentários por espaços preservando as
quebras de linha e reconhece os tokens com expressões regulares da biblioteca
padrão. Nenhum gerador de lexer é utilizado.

Cada `Token` registra:

- tipo;
- valor textual;
- linha;
- coluna.

Caracteres desconhecidos e literais ou comentários não fechados geram erro
léxico.

## Análise sintática

Arquivos:

- `include/analisador_sintatico.hpp`
- `src/analisador_sintatico.cpp`

O parser é descendente recursivo e foi escrito a partir da
gramática em `gramatica/gramatica.txt`. Os métodos `peek`, `match` e `advance`
controlam o fluxo de tokens.

O resultado é uma árvore formada por `ASTNode`, definido em `include/ast.hpp`.
Durante a análise, o parser rastreia nós alocados para liberar árvores parciais
quando ocorre erro.

## AST

Arquivo:

- `include/ast.hpp`

Esse componente define:

- `NodeType`, com as categorias sintáticas;
- `ASTNode`, com valor, filhos, irmãos e pai;
- operações para adotar filhos e unir irmãos.

A análise semântica anota cada nó de expressão com o tipo inferido e, quando
necessário, com a conversão para `float`.

## Análise semântica

Arquivos:

- `include/analisador_semantico.hpp`
- `src/analisador_semantico.cpp`
- `include/tabela_simbolos.hpp`

A tabela de símbolos possui escopos encadeados. Cada símbolo registra nome,
tipo, natureza de função e tipos dos parâmetros.

A análise ocorre em duas etapas:

1. registro das variáveis globais e assinaturas de funções;
2. verificação dos corpos, comandos e expressões.

Principais verificações:

- existência e assinatura da função `main`;
- redeclarações e identificadores inexistentes;
- tipos de inicialização e atribuição;
- tipos e quantidade de argumentos;
- compatibilidade de retornos;
- retorno garantido em funções não `void`;
- condições do tipo `int`;
- uso de `break` e `continue` dentro de laços;
- restrições de `void`, `%`, operadores lógicos e strings;
- promoção de `int` para `float`.

## Geração de código

Arquivos:

- `include/gerador_sam.hpp`
- `src/gerador_sam.cpp`

O gerador percorre a AST e emite instruções SaM 2.6. Variáveis globais usam
endereços absolutos; parâmetros e variáveis locais usam deslocamentos relativos
ao frame da função.

O componente implementa:

- prólogo e epílogo de funções;
- slot de retorno;
- chamadas e recursão;
- operações inteiras e de ponto flutuante;
- conversão `ITOF`;
- rótulos para decisões e laços;
- curto-circuito lógico;
- pilhas de rótulos para `break` e `continue`;
- liberação de strings locais e globais.
