# Referência da linguagem Nir

## Visão geral

Nir é uma linguagem procedural, estaticamente tipada e sensível a
maiúsculas e minúsculas. O código é organizado em variáveis globais e funções.
A unidade de compilação deve possuir extensão `.nir`.

## Estrutura mínima

```nir
fun main(): void {
    print("Olá, mundo!");
}
```

A função `main`:

- é obrigatória;
- não recebe parâmetros;
- deve retornar `void`;
- deve existir uma única vez no programa.

## Identificadores e literais

Identificadores começam com letra ou `_` e podem continuar com letras,
algarismos ou `_`.

```nir
var contador: int = 10;
var _taxa2: float = 1.5;
```

Literais suportados:

| Categoria | Exemplos |
| --- | --- |
| Inteiro | `0`, `42`, `1000` |
| Ponto flutuante | `1.5`, `100.0` |
| Caractere | `'A'`, `'\n'` |
| String | `"texto"` |

## Tipos

| Tipo | Uso |
| --- | --- |
| `int` | inteiros e valores lógicos |
| `float` | ponto flutuante de precisão simples |
| `char` | um caractere |
| `string` | texto alocado no heap da SaM |
| `void` | ausência de retorno em funções |

`void` não pode ser usado como tipo de variável ou parâmetro.

### Conversões

`int` é promovido implicitamente para `float` em:

- inicializações e atribuições;
- operações e comparações numéricas;
- argumentos de funções;
- valores de retorno.

A conversão de `float` para `int` não é implícita.

```nir
var inteiro: int = 2;
var real: float = inteiro + 0.5;
```

## Variáveis

Declaração com inicialização:

```nir
var idade: int = 20;
```

Declaração sem inicialização:

```nir
var total: int;
total = 10;
```

Variáveis podem ser globais ou locais. Um identificador local pode sombrear um
identificador de um escopo externo. Duas declarações com o mesmo nome no mesmo
escopo são rejeitadas.

## Operadores

### Aritméticos

| Operador | Operação |
| --- | --- |
| `+` | adição |
| `-` | subtração ou negação unária |
| `*` | multiplicação |
| `/` | divisão |
| `%` | resto entre operandos `int` |

Precedência, da maior para a menor:

1. parênteses e negação unária;
2. `*`, `/`, `%`;
3. `+`, `-`;
4. relacionais;
5. `!`;
6. `&&`;
7. `||`.

### Relacionais

`==`, `!=`, `<`, `>`, `<=` e `>=` produzem `int`: zero representa falso e um
valor diferente de zero representa verdadeiro. Comparações entre `int` e
`float` promovem o inteiro. Strings não podem ser comparadas.

### Lógicos

`&&`, `||` e `!` exigem operandos `int`. `&&` e `||` usam curto-circuito.

```nir
if (idade >= 18 && idade < 60) {
    print("adulto");
}
```

## Decisão

```nir
if (condicao) {
    // comandos
} else if (outra_condicao) {
    // comandos
} else {
    // comandos
}
```

A condição deve possuir tipo `int`.

## Repetição

### While

```nir
while (contador < 10) {
    contador = contador + 1;
}
```

### Do while

```nir
do {
    contador = contador - 1;
} while (contador > 0);
```

### For

O `for` de Nir possui declaração de inicialização, condição e atualização:

```nir
for (var i: int = 0; i < 5; i = i + 1) {
    print(i);
}
```

`break` encerra o laço mais interno e `continue` segue para a próxima
iteração. Ambos são inválidos fora de laços.

## Funções

```nir
fun multiplicar(a: int, b: int): int {
    return a * b;
}
```

Chamadas podem aparecer em expressões ou como comandos:

```nir
var resultado: int = multiplicar(2, 4);
multiplicar(1, 2);
```

Funções podem chamar funções declaradas posteriormente e podem ser recursivas.
Uma função que não retorna `void` deve garantir um retorno. A quantidade e os
tipos dos argumentos são verificados semanticamente.

## Saída

`print` aceita `int`, `float`, `char` e `string`:

```nir
print(10);
print(2.5);
print('N');
print("Nir");
```

Cada chamada produz uma linha na saída da SaM.

## Strings

Strings são um recurso opcional implementado com ownership único. Uma variável
`string` pode receber um literal e ser reatribuída:

```nir
var mensagem: string = "início";
mensagem = "fim";
print(mensagem);
```

Não são permitidos:

- cópia entre variáveis de string;
- parâmetros do tipo `string`;
- retornos do tipo `string`;
- comparações de strings.

Essas restrições evitam cópia rasa, uso depois de liberação e dupla liberação.

## Comentários

```nir
// comentário de linha
-- comentário de linha
# comentário de linha
/* comentário
   de bloco */
```

## Palavras reservadas

```text
var fun return print
if else while do for
break continue
int float char string void
```

## Gramática

A gramática completa está em
[`gramatica/gramatica.txt`](../gramatica/gramatica.txt).
