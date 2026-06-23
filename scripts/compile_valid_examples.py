#!/usr/bin/env python3

import argparse
import shutil
import subprocess
import sys
from pathlib import Path


RAIZ_PROJETO = Path(__file__).resolve().parents[1]
DIRETORIO_VALIDOS = RAIZ_PROJETO / "exemplos" / "validos"


def executar_comando(comando):
    return subprocess.run(
        [str(parte) for parte in comando],
        cwd=RAIZ_PROJETO,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        timeout=30,
    )


def main():
    analisador_argumentos = argparse.ArgumentParser(
        description="Compila todos os exemplos validos em arquivos .sam separados."
    )
    analisador_argumentos.add_argument(
        "--compiler",
        dest="compilador",
        default=str(RAIZ_PROJETO / "build" / "nir"),
        help="Caminho do executavel nir.",
    )
    analisador_argumentos.add_argument(
        "--output-dir",
        dest="diretorio_saida",
        default=str(RAIZ_PROJETO / "build" / "exemplos_sam"),
        help="Diretorio onde os arquivos .sam serao gravados.",
    )
    argumentos = analisador_argumentos.parse_args()

    compilador = Path(argumentos.compilador).resolve()
    if not compilador.is_file():
        print(f"Compilador nao encontrado: {compilador}", file=sys.stderr)
        return 2

    diretorio_saida = Path(argumentos.diretorio_saida).resolve()
    if diretorio_saida.exists():
        shutil.rmtree(diretorio_saida)
    diretorio_saida.mkdir(parents=True)

    falhas = []
    for fonte in sorted(DIRETORIO_VALIDOS.glob("*.nir")):
        arquivo_saida = diretorio_saida / f"{fonte.stem}.sam"
        resultado = executar_comando([compilador, fonte.resolve(), arquivo_saida])

        if resultado.returncode != 0:
            falhas.append(
                f"{fonte.name}: compilacao falhou\n"
                + resultado.stdout
                + resultado.stderr
            )
            print(f"FAIL sam     {fonte.name}")
            continue

        if not arquivo_saida.is_file():
            falhas.append(f"{fonte.name}: {arquivo_saida} nao foi gerado")
            print(f"FAIL sam     {fonte.name}")
            continue

        print(f"PASS sam     {fonte.name} -> {arquivo_saida.relative_to(RAIZ_PROJETO)}")

    print()
    quantidade_sam = len(list(diretorio_saida.glob("*.sam")))
    quantidade_fontes = len(list(DIRETORIO_VALIDOS.glob("*.nir")))
    print(f"Arquivos .sam: {quantidade_sam}/{quantidade_fontes}")
    print(f"Diretorio: {diretorio_saida}")

    if falhas:
        print("\nFalhas:")
        for falha in falhas:
            print(f"- {falha}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
