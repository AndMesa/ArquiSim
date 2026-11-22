#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
trazas.py -- Genera (en Markdown) la traza registro por registro de cada función de
proyecto.asm para los 4 píxeles de la Tabla 4, ejecutándola en el simulador real.

Para cada función arma un programa mínimo:   li $a0, píxel / li $a1, botones / jal FUNCION
con las MISMAS funciones de proyecto.asm, lo corre con  harness --pasos  y anota el valor
que deja cada instrucción en su registro destino.

Uso:  python trazas.py > trazas.md
"""
import os
import subprocess
import sys
import tempfile

AQUI = os.path.dirname(os.path.abspath(__file__))
ENTREGA = os.path.dirname(AQUI)
SIM = os.path.dirname(ENTREGA)
sys.path.insert(0, AQUI)
from probar import compilar_harness  # noqa: E402

PIXELES = [("Fondo (0,0)", 0xFF), ("Rojo (5,1)", 0xE0), ("Letra (3,11)", 0x00), ("Sopa (9,10)", 0xB1)]
REG = ["$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4", "$t5",
       "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9", "$k0", "$k1",
       "$gp", "$sp", "$fp", "$ra"]

NEGATIVOS = [(n, p ^ 0xFF) for n, p in PIXELES]                   # columna "Negativo" de la Tabla 4
GRISES = [("Fondo (0,0)", 0xFF), ("Rojo (5,1)", 0x49), ("Letra (3,11)", 0x00), ("Sopa (9,10)", 0x92)]

# (función, botones, título, píxeles de entrada)
CASOS = [
    ("A_16COLORES", 0x00, "A_16COLORES con el píxel original", PIXELES),
    ("A_16COLORES", 0x00, "A_16COLORES con el píxel en negativo", NEGATIVOS),
    ("A_16COLORES", 0x00, "A_16COLORES con el píxel en grises", GRISES),
    ("A_RGB", 0x00, "A_RGB con el píxel original", PIXELES),
    ("BRILLO", 0x00, "BRILLO con el píxel original", PIXELES),
    ("APLICAR_FILTROS", 0x00, "APLICAR_FILTROS con botones = 0x00 (sin filtros)", PIXELES),
    ("APLICAR_FILTROS", 0x01, "APLICAR_FILTROS con botones = 0x01 (negativo)", PIXELES),
    ("APLICAR_FILTROS", 0x02, "APLICAR_FILTROS con botones = 0x02 (grises)", PIXELES),
    ("APLICAR_FILTROS", 0x04, "APLICAR_FILTROS con botones = 0x04 (mapa de color, paleta térmica)", PIXELES),
    ("APLICAR_FILTROS", 0x07, "APLICAR_FILTROS con botones = 0x07 (los tres filtros en orden)", PIXELES),
]


def binario(v):
    return f"{v >> 4:04b} {v & 15:04b}"


def valor(v, direccion=False):
    if direccion:
        return f"`0x{v:03X}`"
    if v <= 0xFF:
        return f"`0x{v:02X}` = {binario(v)}"
    return f"`0x{v:06X}`" if v > 0xFFF else f"`0x{v:03X}`"


def correr(exe, funcion, pixel, botones, fuente):
    cabeza, resto = fuente.split("\n.text\n")
    funciones = resto[resto.index("# ====", resto.index("j    FIN")):]      # desde el encabezado de APLICAR_FILTROS
    prog = (cabeza + "\n.text\n"
            + f"        li   $a0, {pixel}\n        li   $a1, {botones}\n        jal  {funcion}\n        j    FIN\n"
            + funciones)
    with tempfile.NamedTemporaryFile("w", suffix=".asm", delete=False, encoding="utf-8", newline="\n") as f:
        f.write(prog)
    try:
        r = subprocess.run([exe, os.path.join(SIM, "isa.txt"), f.name, "0", "--pasos"],
                           capture_output=True, text=True, encoding="utf-8")
    finally:
        os.unlink(f.name)
    pasos = []
    for linea in r.stderr.splitlines()[3:-1]:            # sin li, li, jal del inicio ni el j FIN del final
        partes = linea.split("|")
        texto, salto, regs = " ".join(partes[1].split()), partes[2] == "1", [int(v) for v in partes[3:35]]
        mn = texto.split()[0]
        ops = [o.strip() for o in texto[len(mn):].split(",")]
        if mn in ("beq", "bne"):
            efecto = ("", "**salta**" if salto else "no salta")
        elif mn == "jal":
            efecto = ("$ra", valor(regs[31], True))
        elif mn == "jr":
            efecto = ("PC", valor(regs[31], True) + " (vuelve)")
        elif mn == "sw":
            efecto = ("MEM[0x3FC]", valor(regs[REG.index(ops[0])], True))
        else:
            efecto = (ops[0], valor(regs[REG.index(ops[0])], ops[0] in ("$ra", "$sp")))
        pasos.append((texto, efecto))
    return pasos


def main():
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    exe = compilar_harness(SIM)
    with open(os.path.join(ENTREGA, "proyecto.asm"), encoding="utf-8") as f:
        fuente = f.read()
    for funcion, botones, titulo, pixeles in CASOS:
        columnas = [correr(exe, funcion, p, botones, fuente) for _, p in pixeles]
        assert all([t for t, _ in c] == [t for t, _ in columnas[0]] for c in columnas), "caminos distintos"
        print(f"#### {titulo}\n")
        print("| # | Instrucción | Destino | " + " | ".join(f"{n}<br>`$a0 = 0x{p:02X}`" for n, p in pixeles) + " |")
        print("|---|---|---|" + "---|" * len(pixeles))
        for i, (texto, (destino, _)) in enumerate(columnas[0]):
            celdas = " | ".join(c[i][1][1] for c in columnas)
            print(f"| {i + 1} | `{texto}` | {destino} | {celdas} |")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
