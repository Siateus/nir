# Exemplos da linguagem Nir

Os exemplos estao divididos em:

- `validos/`: devem compilar e executar normalmente.
- `erros/`: devem ser rejeitados pelo compilador.

## Programas validos

| Arquivo | Recursos exercitados |
| --- | --- |
| `01_tipos_e_print.nir` | `int`, `float`, `char`, `string`, inicializacao e `print` |
| `02_aritmetica.nir` | precedencia, `+`, `-`, `*`, `/`, `%` e menos unario |
| `03_relacionais.nir` | `==`, `!=`, `<`, `>`, `<=` e `>=` |
| `04_logicos.nir` | `&&`, `||`, `!` e parenteses em condicoes |
| `05_if_else.nir` | `if`, `else` e `else if` |
| `06_while.nir` | `while`, atribuicao e acumulador |
| `07_do_while.nir` | `do while` e execucao obrigatoria da primeira iteracao |
| `08_for.nir` | `for`, declaracao no cabecalho e incremento no corpo |
| `09_lacos_aninhados.nir` | `while`, `for`, escopos e condicao composta |
| `10_funcoes.nir` | parametros, retorno, chamadas e tipos de retorno |
| `11_recursao.nir` | chamada recursiva e multiplos retornos |
| `12_globais.nir` | variaveis globais acessadas por funcoes |
| `13_sombreamento.nir` | variaveis de mesmo nome em escopos diferentes |
| `14_comentarios.nir` | comentarios `//`, `--`, `#` e `/* ... */` |
| `15_completo.nir` | combinacao dos principais recursos |
| `16_import.nir` | imports reais, recursivos e sem duplicacao |
| `17_declaracao_sem_inicializacao.nir` | declaracao sem valor inicial e atribuicao posterior |
| `18_conversao_numerica.nir` | promocao implicita de `int` para `float` |

## Erros esperados

| Arquivo | Erro exercitado |
| --- | --- |
| `01_variavel_redeclarada.nir` | duas variaveis com o mesmo nome no mesmo escopo |
| `02_variavel_nao_declarada.nir` | uso de identificador inexistente |
| `03_atribuicao_tipo_invalido.nir` | atribuicao de `float` em `int` |
| `04_operacao_tipos_diferentes.nir` | operacao entre tipos nao numericos |
| `05_modulo_float.nir` | operador `%` aplicado a `float` |
| `06_logico_float.nir` | operador logico aplicado a `float` |
| `07_retorno_invalido.nir` | retorno diferente do tipo da funcao |
| `08_argumento_invalido.nir` | argumento com tipo incorreto |
| `09_quantidade_argumentos.nir` | quantidade incorreta de argumentos |
| `10_condicao_invalida.nir` | condicao de controle do tipo `float` |

Existe promocao implicita segura de `int` para `float` em inicializacoes,
atribuicoes, argumentos, retornos, operacoes e comparacoes. A conversao inversa,
de `float` para `int`, continua exigindo um recurso explicito que ainda nao faz
parte da linguagem.

`import "caminho/arquivo.nir";` inclui exatamente o arquivo indicado. Caminhos
relativos sao resolvidos a partir do arquivo importador. Imports repetidos sao
carregados uma vez e ciclos geram erro.
