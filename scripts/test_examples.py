#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys
import tempfile
from pathlib import Path


RAIZ_PROJETO = Path(__file__).resolve().parents[1]
DIRETORIO_VALIDOS = RAIZ_PROJETO / "examples" / "validos"
DIRETORIO_ERROS = RAIZ_PROJETO / "examples" / "erros"
MARCADOR_SAIDA_ESPERADA = "// Saida esperada:"
MARCADOR_ERRO_ESPERADO = "// Erro esperado:"
MARCADOR_PILHA = "__NIR_STACK_SIZE__="
MARCADOR_HEAP = "__NIR_HEAP_ALLOCATIONS__="


def executar_comando(comando, diretorio_trabalho=None):
    return subprocess.run(
        [str(parte) for parte in comando],
        cwd=diretorio_trabalho,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=30,
    )


def obter_saida_esperada(fonte):
    linhas = fonte.read_text(encoding="utf-8").splitlines()
    try:
        indice_marcador = linhas.index(MARCADOR_SAIDA_ESPERADA)
    except ValueError as erro:
        raise RuntimeError(
            f"{fonte.relative_to(RAIZ_PROJETO)} nao possui '{MARCADOR_SAIDA_ESPERADA}'"
        ) from erro

    esperado = []
    for linha in linhas[indice_marcador + 1 :]:
        linha_limpa = linha.strip()
        if not linha_limpa:
            continue
        if not linha_limpa.startswith("//"):
            break
        valor = linha_limpa[2:]
        if valor.startswith(" "):
            valor = valor[1:]
        esperado.append(valor)
    return esperado


def obter_erro_esperado(fonte):
    for linha in fonte.read_text(encoding="utf-8").splitlines():
        linha_limpa = linha.strip()
        if linha_limpa.startswith(MARCADOR_ERRO_ESPERADO):
            return linha_limpa[len(MARCADOR_ERRO_ESPERADO) :].strip()
    return None


def encontrar_sam_jar(caminho_explicito):
    candidatos = []
    if caminho_explicito:
        candidatos.append(Path(caminho_explicito))
    if os.environ.get("SAM_JAR"):
        candidatos.append(Path(os.environ["SAM_JAR"]))

    candidatos.extend(
        [
            RAIZ_PROJETO / "SaM-2.6.3.jar",
            RAIZ_PROJETO / "tools" / "SaM-2.6.3.jar",
            Path.home()
            / "Documentos"
            / "compiladores"
            / "material-sam"
            / "SaM-2.6.3.jar",
            Path.home()
            / "ifb"
            / "2026-1"
            / "Compiladores"
            / "material-sam"
            / "SaM-2.6.3.jar",
        ]
    )

    for candidato in candidatos:
        if candidato.is_file():
            return candidato.resolve()

    raise RuntimeError(
        "SaM-2.6.3.jar nao encontrado. Defina SAM_JAR ou use --sam-jar."
    )


def compilar_executor_sam(diretorio_temporario, sam_jar):
    fonte = diretorio_temporario / "NirSamRunner.java"
    fonte.write_text(
        """
import edu.cornell.cs.sam.core.Program;
import edu.cornell.cs.sam.core.SamAssembler;
import edu.cornell.cs.sam.core.Sys;
import edu.cornell.cs.sam.core.Video;
import java.io.FileReader;

public class NirSamRunner {
    private static class ConsoleVideo implements Video {
        public void writeInt(int valor) { System.out.println(valor); }
        public void writeString(String valor) { System.out.println(valor); }
        public void writeFloat(float valor) { System.out.println(valor); }
        public void writeChar(char valor) { System.out.println(valor); }
        public int readInt() { return 0; }
        public float readFloat() { return 0.0f; }
        public char readChar() { return '\\0'; }
        public String readString() { return ""; }
    }

    public static void main(String[] argumentos) throws Exception {
        Program programa = SamAssembler.assemble(new FileReader(argumentos[0]));
        Sys sistema = new Sys();
        sistema.setVideo(new ConsoleVideo());
        sistema.mem().init();
        sistema.cpu().init();
        sistema.cpu().load(programa);
        sistema.cpu().run();
        System.out.println("__NIR_STACK_SIZE__=" + sistema.mem().getStack().size());
        int alocacoes = 0;
        java.util.Iterator<?> iterador =
            sistema.mem().getHeapAllocator().getAllocations();
        while (iterador.hasNext()) {
            iterador.next();
            alocacoes++;
        }
        System.out.println("__NIR_HEAP_ALLOCATIONS__=" + alocacoes);
    }
}
""".strip()
        + "\n",
        encoding="utf-8",
    )

    resultado = executar_comando(["javac", "-cp", sam_jar, fonte], diretorio_trabalho=diretorio_temporario)
    if resultado.returncode != 0:
        raise RuntimeError("Falha ao compilar runner SaM:\n" + resultado.stderr)


def executar_sam(diretorio_temporario, sam_jar, arquivo_sam):
    caminho_classes = os.pathsep.join([str(diretorio_temporario), str(sam_jar)])
    resultado = executar_comando(
        ["java", "-cp", caminho_classes, "NirSamRunner", arquivo_sam],
        diretorio_trabalho=diretorio_temporario,
    )
    if resultado.returncode != 0:
        raise RuntimeError("Falha ao executar SaM:\n" + resultado.stdout + resultado.stderr)

    saida = resultado.stdout.splitlines()
    linhas_pilha = [linha for linha in saida if linha.startswith(MARCADOR_PILHA)]
    if len(linhas_pilha) != 1:
        raise RuntimeError("Runner SaM nao informou o tamanho final da pilha.")

    tamanho_pilha = int(linhas_pilha[0][len(MARCADOR_PILHA) :])
    linhas_heap = [linha for linha in saida if linha.startswith(MARCADOR_HEAP)]
    if len(linhas_heap) != 1:
        raise RuntimeError("Runner SaM nao informou as alocacoes finais do heap.")
    alocacoes_heap = int(linhas_heap[0][len(MARCADOR_HEAP) :])
    saida_programa = [
        linha
        for linha in saida
        if not linha.startswith(MARCADOR_PILHA)
        and not linha.startswith(MARCADOR_HEAP)
    ]
    return saida_programa, tamanho_pilha, alocacoes_heap


def testar_exemplos_validos(compilador, diretorio_temporario, sam_jar, mostrar_saida):
    falhas = []
    arquivos = sorted(DIRETORIO_VALIDOS.glob("*.nir"))

    for fonte in arquivos:
        diretorio_caso = diretorio_temporario / fonte.stem
        diretorio_caso.mkdir()
        resultado_compilacao = executar_comando([compilador, fonte.resolve()], diretorio_trabalho=diretorio_caso)

        if resultado_compilacao.returncode != 0:
            falhas.append(
                f"{fonte.name}: compilacao falhou\n"
                + resultado_compilacao.stdout
                + resultado_compilacao.stderr
            )
            print(f"FAIL valido  {fonte.name}")
            continue

        arquivo_sam = diretorio_caso / "saida.sam"
        if not arquivo_sam.is_file():
            falhas.append(f"{fonte.name}: saida.sam nao foi gerado")
            print(f"FAIL valido  {fonte.name}")
            continue

        try:
            esperado = obter_saida_esperada(fonte)
            obtido, tamanho_pilha, alocacoes_heap = executar_sam(
                diretorio_temporario, sam_jar, arquivo_sam
            )
        except RuntimeError as erro:
            falhas.append(f"{fonte.name}: {erro}")
            print(f"FAIL valido  {fonte.name}")
            continue

        if obtido != esperado:
            falhas.append(
                f"{fonte.name}: saida diferente\n"
                f"  esperado: {esperado}\n"
                f"  obtido:   {obtido}"
            )
            print(f"FAIL valido  {fonte.name}")
            continue

        if tamanho_pilha != 1:
            falhas.append(
                f"{fonte.name}: pilha final deveria ter 1 item, obteve {tamanho_pilha}"
            )
            print(f"FAIL valido  {fonte.name}")
            continue

        if alocacoes_heap != 0:
            falhas.append(
                f"{fonte.name}: heap terminou com {alocacoes_heap} "
                "alocacao(oes)"
            )
            print(f"FAIL valido  {fonte.name}")
            continue

        print(f"PASS valido  {fonte.name}")
        if mostrar_saida:
            if obtido:
                for linha in obtido:
                    print(f"    SaM> {linha}")
            else:
                print("    SaM> <sem saida>")
            print(f"    Pilha final: {tamanho_pilha}")
            print(f"    Heap final: {alocacoes_heap}")

    return len(arquivos), falhas


def testar_exemplos_com_erro(compilador, diretorio_temporario):
    falhas = []
    arquivos = sorted(DIRETORIO_ERROS.glob("*.nir"))

    for fonte in arquivos:
        diretorio_caso = diretorio_temporario / ("erro-" + fonte.stem)
        diretorio_caso.mkdir()
        resultado = executar_comando([compilador, fonte.resolve()], diretorio_trabalho=diretorio_caso)

        if resultado.returncode == 0:
            falhas.append(f"{fonte.name}: deveria falhar, mas compilou")
            print(f"FAIL erro    {fonte.name}")
        else:
            trecho = obter_erro_esperado(fonte)
            saida_compilador = resultado.stdout + resultado.stderr
            if trecho and trecho not in saida_compilador:
                falhas.append(
                    f"{fonte.name}: erro diferente\n"
                    f"  esperado conter: {trecho!r}\n"
                    f"  obtido: {saida_compilador.strip()!r}"
                )
                print(f"FAIL erro    {fonte.name}")
            else:
                print(f"PASS erro    {fonte.name}")

    return len(arquivos), falhas


def main():
    analisador_argumentos = argparse.ArgumentParser(
        description="Testa o compilador usando os programas em examples/."
    )
    analisador_argumentos.add_argument(
        "--compiler",
        dest="compilador",
        default=str(RAIZ_PROJETO / "build" / "nir"),
        help="Caminho do executavel nir.",
    )
    analisador_argumentos.add_argument(
        "--sam-jar",
        dest="sam_jar",
        help="Caminho do SaM-2.6.3.jar.",
    )
    analisador_argumentos.add_argument(
        "--show-output",
        dest="mostrar_saida",
        action="store_true",
        help="Mostra no terminal a saida capturada da SaM.",
    )
    argumentos = analisador_argumentos.parse_args()

    compilador = Path(argumentos.compilador).resolve()
    if not compilador.is_file():
        print(f"Compilador nao encontrado: {compilador}", file=sys.stderr)
        return 2

    try:
        sam_jar = encontrar_sam_jar(argumentos.sam_jar)
        with tempfile.TemporaryDirectory(prefix="nir-tests-") as temporario:
            diretorio_temporario = Path(temporario)
            compilar_executor_sam(diretorio_temporario, sam_jar)
            quantidade_validos, falhas_validos = testar_exemplos_validos(
                compilador, diretorio_temporario, sam_jar, argumentos.mostrar_saida
            )
            quantidade_erros, falhas_erros = testar_exemplos_com_erro(compilador, diretorio_temporario)
    except (RuntimeError, subprocess.TimeoutExpired) as erro:
        print(f"Erro ao preparar testes: {erro}", file=sys.stderr)
        return 2

    falhas = falhas_validos + falhas_erros
    print()
    print(f"Validos: {quantidade_validos - len(falhas_validos)}/{quantidade_validos}")
    print(f"Erros:   {quantidade_erros - len(falhas_erros)}/{quantidade_erros}")

    if falhas:
        print("\nFalhas:")
        for falha in falhas:
            print(f"- {falha}")
        return 1

    print("Todos os exemplos passaram.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
