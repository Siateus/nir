# Exemplos da linguagem Nir

Os exemplos estão divididos em:

- `validos/`: devem compilar e executar normalmente.
- `erros/`: devem ser rejeitados pelo compilador.

## Programas válidos

| Arquivo | Recursos exercitados |
| --- | --- |
| `01_tipos_e_print.nir` | `int`, `float`, `char`, `string`, inicialização e `print` |
| `02_aritmetica.nir` | precedência, `+`, `-`, `*`, `/`, `%` e menos unário |
| `03_relacionais.nir` | `==`, `!=`, `<`, `>`, `<=` e `>=` |
| `04_logicos.nir` | `&&`, `||`, `!` e parênteses em condições |
| `05_if_else.nir` | `if`, `else` e `else if` |
| `06_while.nir` | `while`, atribuição e acumulador |
| `07_do_while.nir` | `do while` e execução obrigatória da primeira iteração |
| `08_for.nir` | `for`, declaração no cabeçalho e incremento no corpo |
| `09_lacos_aninhados.nir` | `while`, `for`, escopos e condição composta |
| `10_funcoes.nir` | parâmetros, retorno, chamadas e tipos de retorno |
| `11_recursao.nir` | chamada recursiva e múltiplos retornos |
| `12_globais.nir` | variáveis globais acessadas por funções |
| `13_sombreamento.nir` | variáveis de mesmo nome em escopos diferentes |
| `14_comentarios.nir` | comentários `//`, `--`, `#` e `/* ... */` |
| `15_completo.nir` | combinação dos principais recursos |
| `16_import.nir` | imports reais, recursivos e sem duplicação |
| `17_declaracao_sem_inicializacao.nir` | declaração sem valor inicial e atribuição posterior |
| `18_conversao_numerica.nir` | promoção implicita de `int` para `float` |
| `19_break_continue.nir` | `break`, `continue` e laços aninhados |
| `20_regressoes_compilador.nir` | chamadas isoladas, curto-circuito, expressões e memória de strings |

## Erros esperados

| Arquivo | Erro exercitado |
| --- | --- |
| `01_variavel_redeclarada.nir` | duas variáveis com o mesmo nome no mesmo escopo |
| `02_variavel_nao_declarada.nir` | uso de identificador inexistente |
| `03_atribuicao_tipo_invalido.nir` | atribuição de `float` em `int` |
| `04_operacao_tipos_diferentes.nir` | operação entre tipos não numéricos |
| `05_modulo_float.nir` | operador `%` aplicado a `float` |
| `06_logico_float.nir` | operador lógico aplicado a `float` |
| `07_retorno_invalido.nir` | retorno diferente do tipo da função |
| `08_argumento_invalido.nir` | argumento com tipo incorreto |
| `09_quantidade_argumentos.nir` | quantidade incorreta de argumentos |
| `10_condicao_invalida.nir` | condição de controle do tipo `float` |
| `11_break_fora_de_laco.nir` | `break` usado fora de um laço |
| `12_continue_fora_de_laco.nir` | `continue` usado fora de um laço |
| `13_caractere_invalido.nir` | caractere desconhecido pelo lexer |
| `14_funcao_sem_retorno.nir` | função não-void sem retorno garantido |
| `15_parametro_repetido.nir` | parâmetros com o mesmo nome |
| `16_variavel_void.nir` | variável declarada como `void` |
| `17_copia_string.nir` | copia rasa de string |
| `18_sem_main.nir` | programa sem função principal |

Existe promoção implícita segura de `int` para `float` em inicializações,
atribuições, argumentos, retornos, operações e comparações. A conversão inversa,
de `float` para `int`, continua exigindo um recurso explícito que ainda não faz
parte da linguagem.

`import "caminho/arquivo.nir";` inclui exatamente o arquivo indicado. Caminhos
relativos são resolvidos a partir do arquivo importador. Imports repetidos são
carregados uma vez e ciclos geram erro.

Strings possuem ownership único: variáveis podem receber literais de string e
ser reatribuídas, mas cópias entre variáveis, parâmetros e retornos de string
ainda não são permitidos.

## Executando os testes

Os próprios exemplos formam a suíte de testes:

```bash
python3 scripts/test_examples.py
```

Para visualizar no terminal a saída real produzida pela SaM:

```bash
python3 scripts/test_examples.py --show-output
```

O script compila e executa todos os arquivos de `validos/`, compara a saída com
os comentários `// Saida esperada:`, verifica a pilha final da SaM e confirma
que todos os arquivos de `erros/` são rejeitados. O caminho do simulador pode
ser informado por `SAM_JAR` ou `--sam-jar`.

Depois de configurar o CMake, a mesma suíte pode ser executada com:

```bash
ctest --test-dir build --output-on-failure
```
