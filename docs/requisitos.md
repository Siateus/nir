# Atendimento aos requisitos do projeto

Este documento relaciona a implementação de Nir aos requisitos fornecidos para
o trabalho da disciplina de Compiladores.

| Requisito | Estado | Implementação |
| --- | --- | --- |
| Paradigma procedural | Atendido | programas organizados em funções e procedimentos `void` |
| Inteiros | Atendido | tipo `int` e instruções inteiras da SaM |
| Ponto flutuante simples | Atendido | tipo `float`, instruções `*F` e conversão `ITOF` |
| Caracteres | Atendido | tipo `char`, literais e `WRITECH` |
| Variáveis | Atendido | variáveis globais e locais com escopos |
| Constantes numéricas | Atendido | literais inteiros e de ponto flutuante |
| Atribuição | Atendido | atribuição local e global com verificação de tipo |
| Adição, subtração, multiplicação e divisão | Atendido | operadores `+`, `-`, `*` e `/` |
| Resto | Atendido | operador `%` para `int` |
| Operadores relacionais | Atendido | `==`, `!=`, `>`, `<`, `>=`, `<=` |
| Operadores lógicos | Atendido | `&&`, `||`, `!`, com curto-circuito |
| Estrutura de decisão | Atendido | `if`, `else` e `else if` |
| Repetição com teste no início | Atendido | `while` e `for` |
| Repetição com teste no final | Atendido | `do while` |
| Lexer implementado | Atendido | `AnalisadorLexico`, sem gerador externo |
| Parser implementado | Atendido | parser descendente recursivo |
| Projeto da gramática | Atendido | `gramatica/gramatica.txt` |
| Análise semântica | Atendido | tabela de símbolos, tipos, escopos e validações |
| Geração de código | Atendido | assembly SaM 2.6 em `saida.sam` |
| Testes | Atendido | exemplos válidos e inválidos executados automaticamente |
| Documentação | Atendido | README, referência, arquitetura, gramática e exemplos |

## Recursos opcionais

| Recurso | Estado |
| --- | --- |
| Funções e procedimentos | Implementado, incluindo parâmetros, retorno e recursão |
| Strings | Implementadas com ownership único e restrições documentadas |
| Constantes de string | Implementadas |
| Vetores | Não implementados |
| Tipo booleano | Não implementado; expressões lógicas usam `int` |
| Operadores bit a bit | Não implementados |

## Evidências

- A gramática formal está em `gramatica/gramatica.txt`.
- A implementação está dividida em analisadores léxico, sintático e semântico,
  carregador e gerador SaM.
- `exemplos/validos` cobre recursos da linguagem.
- `exemplos/erros` cobre erros léxicos, sintáticos e semânticos.
- `scripts/compile_valid_examples.py` compila os exemplos válidos em `.sam`.

## Comando de verificação

```bash
cmake -S . -B build
cmake --build build
cmake --build build --target exemplos
```
