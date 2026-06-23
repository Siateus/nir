# Nir

Nir é uma linguagem procedural compilada para o assembly da máquina virtual
SaM 2.6. O compilador foi desenvolvido manualmente em C++ para a disciplina de
Compiladores, sem geradores de analisadores léxicos ou sintáticos.

O projeto implementa:

- tipos `int`, `float`, `char`, `string` e `void`;
- variáveis globais e locais;
- expressões aritméticas, relacionais e lógicas;
- `if`, `else`, `while`, `do while` e `for`;
- funções, parâmetros, retornos e recursão;
- `break` e `continue`;
- imports de outros arquivos `.nir`;
- promoção implícita de `int` para `float`;
- geração de código SaM 2.6.

## Exemplo

```nir
fun soma(a: int, b: int): int {
    return a + b;
}

fun main(): void {
    var resultado: int = soma(2, 3);

    if (resultado == 5) {
        print("resultado:");
        print(resultado);
    }
}
```

Todo programa executável deve declarar exatamente uma função
`fun main(): void`, sem parâmetros.

## Compilação do compilador

Requisitos:

- compilador C++ com suporte a C++14;
- CMake 3.16 ou superior.

```bash
cmake -S . -B build
cmake --build build
```

O executável será criado em `build/nir`.

## Compilação de um programa Nir

O compilador aceita apenas arquivos com extensão `.nir`:

```bash
./build/nir examples/validos/15_completo.nir
```

Quando a compilação termina com sucesso, o arquivo `saida.sam` é criado no
diretório atual:

```text
Código SaM v2.6 gerado em: saida.sam
```

Erros léxicos, sintáticos, semânticos, de import ou de acesso a arquivos
encerram o compilador com código de saída diferente de zero.

## Testes

A suíte usa os programas em `examples/validos` e `examples/erros`. Além da
compilação, ela executa o assembly, compara a saída esperada, verifica a pilha
final e procura alocações restantes no heap da SaM.

Requisitos adicionais:

- Python 3;
- Java/JDK;
- `SaM-2.6.3.jar`.

O JAR pode estar na raiz do projeto, em `tools/`, ser indicado por `SAM_JAR` ou
pela opção `--sam-jar`.

```bash
python3 scripts/test_examples.py
python3 scripts/test_examples.py --show-output
ctest --test-dir build --output-on-failure
```

## Documentação

- [Referência da linguagem](docs/linguagem.md)
- [Arquitetura do compilador](docs/arquitetura.md)
- [Atendimento aos requisitos](docs/requisitos.md)
- [Gramática formal](gramatica/gramatica.txt)
- [Catálogo de exemplos](examples/README.md)

## Estrutura

```text
include/       interfaces dos componentes
src/           implementação do compilador
gramatica/     gramática da linguagem
examples/      programas válidos, inválidos e módulos
scripts/       automação dos testes
docs/          documentação técnica
main.cpp       fluxo principal de compilação
```

## Limitações atuais

- não há vetores;
- não há tipo booleano: `int` representa valores lógicos;
- `float` não é convertido implicitamente para `int`;
- strings não podem ser copiadas entre variáveis, comparadas, recebidas como
  parâmetros ou retornadas por funções;
- não há entrada de dados em tempo de execução;
- o nome do arquivo de saída é fixo: `saida.sam`.
